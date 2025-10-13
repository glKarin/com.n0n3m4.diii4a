//read menu.h

#include "quakedef.h"
#include "winquake.h"
#include "fs.h"


static const char *res4x3[] =
{
	"640x480",
	"800x600",
	"960x720",
	"1024x768",
	"1152x864",
	"1280x960",
	"1440x1080",
	"1600x1200",
//	"1792x1344",
//	"1856x1392",
	"1920x1440",
	"2048x1536",
	NULL
};
static const char *res5x4[] =
{
	"1280x1024",
	"1800x1440",
	"2560x2048",
	NULL
};
static const char *res16x9[] =
{
	"856x480",
	"1024x576",
	"1280x720",
	"1366x768",
	"1600x900",
	"1920x1080",
	"2048x1152",
	"2560x1440",
	"3840x2160",
	"4096x2304",
	NULL
};
static const char *res16x10[] =
{
	"1024x640",
	"1152x720",
	"1280x800",
	"1440x900",
	"1680x1050",
	"1920x1200",
	"2304x1440",
	"2560x1600",
	NULL
};
#define ASPECT_RATIOS 4
static const char **resaspects[ASPECT_RATIOS] =
{
	res4x3,
	res5x4,
	res16x9,
	res16x10
};
#define ASPECT_LIST "4:3", "5:4", "16:9", "16:10",
enum
{
	ASPECT3D_DESKTOP = ASPECT_RATIOS,
	ASPECT3D_CUSTOM,
};
enum
{
	ASPECT2D_AUTO = ASPECT_RATIOS,
	ASPECT2D_HEIGHT,
	ASPECT2D_SCALED,
	ASPECT2D_CUSTOM,
};


typedef struct {
	unsigned int w, h;
} ftevidmode_t;
static ftevidmode_t *vidmodes;
static size_t nummodes;
void VidMode_Clear(void)
{
	BZ_Free(vidmodes);
	vidmodes = NULL;
	nummodes = 0;
}
void VidMode_Add(int w, int h)
{
	extern cvar_t vid_minsize;
	size_t m;

	if (w < vid_minsize.vec4[0] || h < vid_minsize.vec4[1])
		return;	//would be too small. ignore it.
	for (m = 0; m < nummodes; m++)
	{
		if (vidmodes[m].w == w && vidmodes[m].h == h)
			return;
	}
	Z_ReallocElements((void**)&vidmodes, &nummodes, nummodes+1, sizeof(*vidmodes));
	vidmodes[m].w = w;
	vidmodes[m].h = h;
}
static int QDECL VidMode_Sort(const void *v1, const void *v2)
{
	const ftevidmode_t *m1 = v1, *m2 = v2;
	int n1 = m1->w, n2=m2->w;	//sort by width
	if (n1 == n2)
		n1 = m1->h, n2=m2->h;	//then by height
	if (n1 == n2)
		return 0;				//then give up and consider equal... which shouldn't happen.
	if (n1 > n2)
		return 1;
	return -1;
}
qboolean M_Vid_GetMode(qboolean forfullscreen, int num, int *w, int *h)
{
	int a;
	extern cvar_t vid_devicename;
	if (!nummodes)
	{
		VidMode_Clear();

		if (rf->VID_EnumerateVideoModes)
			rf->VID_EnumerateVideoModes("", vid_devicename.string, VidMode_Add);

		if (!nummodes)
		{	//failed to query, use internal fallbacks (may be too big).
			for (a = 0; a < countof(resaspects); a++)
			{
				const char **v = resaspects[a];
				for (; *v; v++)
				{
					const char *c = *v;
					const char *s = strchr(c, 'x');
					if (s)
						VidMode_Add(atoi(c), atoi(s+1));
				}
			}
		}
		qsort(vidmodes, nummodes, sizeof(*vidmodes), VidMode_Sort);
	}

	if (num < 0 || num >= nummodes)
		return false;
	*w = vidmodes[num].w;
	*h = vidmodes[num].h;
	return true;
}



#ifndef NOBUILTINMENUS

extern qboolean forcesaveprompt;
extern cvar_t pr_debugger;

emenu_t *M_Options_Title(int *y, int infosize)
{
	struct emenu_s *menu;
	*y = 32;

	menu = M_CreateMenu(infosize);

	switch(M_GameType())
	{
	case MGT_QUAKE2:	//q2...
		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_options");
		break;
#ifdef HEXEN2
	case MGT_HEXEN2://h2
		MC_AddPicture(menu, 16, 0, 35, 176, "gfx/menu/hplaque.lmp");
		MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title3.lmp");
		*y += 32;
		break;
#endif
	default: //q1
		MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_option.lmp");
		break;
	}
	return menu;
}

//these are awkward/strange
static qboolean M_Options_AlwaysRun (menucheck_t *option, struct emenu_s *menu, chk_set_t set)
{
	extern cvar_t cl_run;
	switch (M_GameType())
	{
	case MGT_HEXEN2:
		//hexen2 uses forwardspeed's magnitude as an 'isrunning' boolean check, with all else hardcoded.
		if (set != CHK_CHECKED)
		{
			if (cl_forwardspeed.value > 200 || cl_run.ival)
				Cvar_SetValue(&cl_forwardspeed, 200);
			else
				Cvar_SetValue(&cl_forwardspeed, 400);
			Cvar_Set(&cl_backspeed, "");
			Cvar_SetValue(&cl_run, 0);
		}
		return cl_forwardspeed.value > 200;
	default:
	case MGT_QUAKE2:
		//quake2 mods have a nasty tendancy to hack at the various cvars, which breaks everything
		if (set != CHK_CHECKED)
			Cvar_SetValue(&cl_run, !cl_run.ival);
		return cl_run.ival;
	case MGT_QUAKE1:
		//for better compat with other quake engines, we just ignore the cl_run cvar, at least for the menu.
		if (set == CHK_CHECKED)
			return cl_forwardspeed.value > 200;
		else if (cl_forwardspeed.value > 200)
		{
			Cvar_SetValue (&cl_forwardspeed, 200);
			if (*cl_backspeed.string)
				Cvar_SetValue (&cl_backspeed, 200);
			return false;
		}
		else
		{
			Cvar_SetValue (&cl_forwardspeed, 400);
			if (*cl_backspeed.string)
				Cvar_SetValue (&cl_backspeed, 400);
			return true;
		}
	}
}
static qboolean M_Options_InvertMouse (menucheck_t *option, struct emenu_s *menu, chk_set_t set)
{
	if (set == CHK_CHECKED)
		return m_pitch.value < 0;
	else
	{
		Cvar_SetValue (&m_pitch, -m_pitch.value);
		return m_pitch.value < 0;
	}
}

static void M_Options_Predraw(emenu_t *menu)
{
	extern cvar_t m_preset_chosen;
	menubutton_t *b;
	b = M_FindButton(menu, "fps_preset\n");
	b->text = (char*)(b+1) + (m_preset_chosen.ival?2:0);

#ifdef PACKAGEMANAGER
	b = M_FindButton(menu, "menu_download\n");
	b->text = (char*)(b+1) + (PM_AreSourcesNew(false)?0:2);
#endif
}

//options menu.
void M_Menu_Options_f (void)
{
	extern cvar_t m_preset_chosen;
	extern cvar_t crosshair, r_projection;
	int y;

	static const char *projections[] = {
		"Regular",
		"Stereographic",
		"Fisheye",
		"Panoramic",
		"Lambert Azimuthal Equal-Area",
		"Equirectangular",
		NULL
	};
	static const char *projectionvalues[] = {
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		NULL
	};

#if !defined(CLIENTONLY) && defined(SAVEDGAMES)
	extern cvar_t sv_autosave;
	static const char *autosaveopts[] = {
		"Off",
		"30 secs",
		"60 secs",
		"90 secs",
		"120 secs",
		"5 mins",
		NULL
	};
	static const char *autosavevals[] = {
		"0",
		"0.5",
		"1",
		"1.5",
		"2",
		"5",
		NULL
	};
#endif
#if !defined(CLIENTONLY) && defined(MVD_RECORDING)
	extern cvar_t sv_demoAutoRecord;
	static const char *autorecordopts[] = {
		"Off",
		"Clientside Only",
		"Prefer Serverside",
		NULL
	};
	static const char *autorecordvals[] = {
		"0",
		"-1",
		"1",
		NULL
	};
	extern cvar_t cl_loopbackprotocol;
	static const char *lprotopts[] = {
		"Vanilla QW",
		"FTE QW (recommended)",
#ifdef NQPROT
		"FTE NQ",
		"666",
		"BJP3",
//		"DP6",
//		"DP7",
		"Automatic (FTE NQ/QW)",
		"Vanilla NQ",
#endif
		NULL
	};
	static const char *lprotvals[] = {
		"qwid",
		"qw",
#ifdef NQPROT
		"nq",
		"fitz",
		"bjp3",
//		"dp6",
//		"dp7",
		"auto",
		"nqid",
#endif
		NULL
	};
#endif

	extern cvar_t scr_fov_mode;
		static const char *fovmodes[] = {
		"Major-",
		"Minor+",
		"Horizontal",
		"Vertical",
		"4:3 stretched",
		NULL
	};
	static const char *fovmodevalues[] = {
		"0",
		"1",
		"2",
		"3",
		"4",
		NULL
	};

	extern cvar_t cfg_save_auto;
	menubulk_t bulk[] = {
		MB_CONSOLECMD("Save all settings", "cfg_save\n", "Writes changed settings out to a config file."),
		MB_CHECKBOXCVARTIP("Auto-save Settings", cfg_save_auto, 1, "If this is disabled, you will need to explicitly save your settings."),
		MB_CONSOLECMD("Reset to defaults", "cvarreset *\nexec default.cfg\nplay misc/menu2.wav\n", "Reloads the default configuration."),
		MB_SPACING(4),
		MB_CONSOLECMD("Controls", "menu_keys\n", "Modify keyboard and mouse inputs."),
#ifdef PACKAGEMANAGER
		MB_CONSOLECMD("^bUpdates and Packages", "menu_download\n", "Configure additional content and plugins."),
#endif
		MB_CONSOLECMD("Go to console", "toggleconsole\nplay misc/menu2.wav\n", "Open up the engine console."),
		MB_COMBOCVAR("View Projection", r_projection, projections, projectionvalues, NULL),
		MB_COMBOCVAR("FOV Mode", scr_fov_mode, fovmodes, fovmodevalues, NULL),
		MB_SLIDER("Field of View", scr_fov, 70, 360, 5, NULL),
		MB_SLIDER("Mouse Speed", sensitivity, 1, 10, 0.2, NULL),
		MB_SLIDER("Crosshair", crosshair, 0, 22, 1, NULL), // move this to hud setup?
		MB_CHECKBOXFUNC("Always Run", M_Options_AlwaysRun, 0, "Set movement to run at fastest speed by default."),
		MB_CHECKBOXFUNC("Invert Mouse", M_Options_InvertMouse, 0, "Invert vertical mouse movement."),
		MB_CHECKBOXCVAR("Lookspring", lookspring, 0),
		MB_CHECKBOXCVAR("Lookstrafe", lookstrafe, 0),
		MB_CHECKBOXCVAR("Windowed Mouse", in_windowed_mouse, 0),
#if !defined(CLIENTONLY) && defined(SAVEDGAMES)
		MB_COMBOCVAR("Auto Save", sv_autosave, autosaveopts, autosavevals, NULL),
#endif
#if !defined(CLIENTONLY) && defined(MVD_RECORDING)
		MB_COMBOCVAR("Auto Record", sv_demoAutoRecord, autorecordopts, autorecordvals, NULL),
		MB_COMBOCVAR("Force Protocol", cl_loopbackprotocol, lprotopts, lprotvals, "Some protocols may impose additional limitations/breakages, and are listed only for potential demo-recording compat."),
#endif
		MB_SPACING(4),
		// removed hud options (cl_sbar, cl_hudswap, old-style chat, old-style msg)
		MB_CONSOLECMD("Audio Options", "menu_audio\n", "Set audio quality and speaker setup options."),
		MB_CONSOLECMD("^bGraphics Presets", "fps_preset\n", "Choose a different graphical preset to use."),
		MB_CONSOLECMD("Video Options", "menu_video\n", "Set video resolution, color depth, refresh rate, and anti-aliasing options."),
#ifdef TEXTEDITOR
		//this option is a bit strange in q2.
//		MB_CHECKBOXCVAR("QC Debugger", pr_debugger, 0),
#endif
		// removed downloads (is this still appropriate?)
		// removed teamplay
		// removed singleplayer cheats (move this to single player menu)
		MB_END()
	};
	emenu_t *menu = M_Options_Title(&y, 0);
	static menuresel_t resel;
	int framey = y;
	menubutton_t *o;

	MC_AddFrameStart(menu, framey);
	y = MC_AddBulk(menu, &resel, bulk, 16, 216, y);

#ifdef PLUGINS
	if (Cmd_Exists("ezhud_nquake"))
	{
		extern cvar_t plug_sbar;
		static const char *hudplugopts[] = {
			"Never",
			"Deathmatch",
			"Single Player/Coop",
			"Always",
			NULL
		};
		static const char *hudplugvalues[] = {
			"0",
			"1",
			"2",
			"3",
			NULL
		};
		MC_AddCvarCombo(menu, 16, 216, y, localtext("Use Hud Plugin"), &plug_sbar, hudplugopts, hudplugvalues);			y += 8;
	}
#endif
	MC_AddFrameEnd(menu, framey);

	menu->predraw = M_Options_Predraw;
	if (!resel.x)
	{
		o = NULL;
		if (!o && !m_preset_chosen.ival)
			o = M_FindButton(menu, "fps_preset\n");
#ifdef PACKAGEMANAGER
		if (!o && PM_AreSourcesNew(false))
			o = M_FindButton(menu, "menu_download\n");
#endif
		if (o)
		{
			menu->selecteditem = (menuoption_t*)o;
			menu->cursoritem->common.posy = o->common.posy + (o->common.height-menu->cursoritem->common.height)/2;
		}
	}
}

#ifndef __CYGWIN__
typedef struct {
	int cursorpos;
	menuoption_t *cursoritem;

	menutext_t *speaker[MAXSOUNDCHANNELS];
	menutext_t *testsoundsource;

	soundcardinfo_t *card;
} audiomenuinfo_t;

qboolean M_Audio_Key (struct emenu_s *menu, int key, unsigned int unicode)
{
	int i;
	audiomenuinfo_t *info = menu->data;
	soundcardinfo_t *sc;
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		if (sc == info->card)
			break;
	}
	if (!sc)
	{
		M_RemoveMenu(menu);
		return true;
	}


	if (key == K_DOWNARROW || key == K_KP_DOWNARROW || key == K_GP_DPAD_DOWN)
	{
		info->testsoundsource->common.posy+=10;
	}
	if (key == K_UPARROW || key == K_KP_UPARROW || key == K_GP_DPAD_UP)
	{
		info->testsoundsource->common.posy-=10;
	}
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_GP_DPAD_RIGHT)
	{
		info->testsoundsource->common.posx+=10;
	}
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_GP_DPAD_LEFT)
	{
		info->testsoundsource->common.posx-=10;
	}
	if (key >= '0' && key <= '5')
	{
		i = key - '0';

		sc->speakerdir[i][0] = (info->testsoundsource->common.posy-200/2)/-50.0;
		sc->speakerdir[i][1] = (info->testsoundsource->common.posx-320/2)/-50.0;
		sc->speakerdir[i][2] = 0;

		sc->dist[i] = VectorLength(sc->speakerdir[i]);
	}

	menu->selecteditem = NULL;

	return false;
}

void M_Audio_StartSound (struct emenu_s *menu)
{
	int i;
	vec3_t org;
	audiomenuinfo_t *info = menu->data;
	soundcardinfo_t *sc;
	vec3_t mat[4];

	static float lasttime;

	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		if (sc == info->card)
			break;
	}
	if (!sc)
	{
		M_RemoveMenu(menu);
		return;
	}

	for (i = 0; i < sc->sn.numchannels; i++)
	{
		info->speaker[i]->common.posx = 320/2 + sc->speakerdir[i][1] * 50;
		info->speaker[i]->common.posy = 200/2 - sc->speakerdir[i][0] * 50;
	}
	for (; i < 6; i++)
		info->speaker[i]->common.posy = -100;

	if (lasttime+0.5 < Sys_DoubleTime())
	{
		S_GetListenerInfo(0, mat[0], mat[1], mat[2], mat[3]);

		lasttime = Sys_DoubleTime();
		org[0] = mat[0][0] + 2*(mat[2][0]*(info->testsoundsource->common.posx-320/2) - mat[1][0]*(info->testsoundsource->common.posy-200/2));
		org[1] = mat[0][1] + 2*(mat[2][1]*(info->testsoundsource->common.posx-320/2) - mat[1][1]*(info->testsoundsource->common.posy-200/2));
		org[2] = mat[0][2] + 2*(mat[2][2]*(info->testsoundsource->common.posx-320/2) - mat[1][2]*(info->testsoundsource->common.posy-200/2));
		S_StartSound(0, 0, S_PrecacheSound("player/pain3.wav"), org, NULL, 1, 4, 0, 0, 0);
	}
}

void M_Menu_Audio_Speakers_f (void)
{
	int i;
	audiomenuinfo_t *info;
	emenu_t *menu;

	menu = M_CreateMenu(sizeof(audiomenuinfo_t));
	info = menu->data;
	menu->key = M_Audio_Key;
	menu->predraw = M_Audio_StartSound;

	for (i = 0; i < 6; i++)
		info->speaker[i] = MC_AddBufferedText(menu, 0, 0, 0, va("%i", i), false, true);

	info->testsoundsource = MC_AddBufferedText(menu, 320/2, 320/2, 200/2, "X", false, true);

	info->card = sndcardinfo;

	menu->selecteditem = NULL;
}

struct audiomenuinfo
{
	char **outdevnames;
	char **outdevdescs;
#ifdef VOICECHAT
	char **capdevnames;
	char **capdevdescs;
#endif
};
void M_Menu_Audio_Remove(emenu_t *menu)
{
	int i;
	struct audiomenuinfo *info = menu->data;
	for (i = 0; info->outdevnames[i]; i++)
		Z_Free(info->outdevnames[i]);
	Z_Free(info->outdevnames);
	for (i = 0; info->outdevdescs[i]; i++)
		Z_Free(info->outdevdescs[i]);
	Z_Free(info->outdevdescs);
#ifdef VOICECHAT
	for (i = 0; info->capdevnames[i]; i++)
		Z_Free(info->capdevnames[i]);
	Z_Free(info->capdevnames);
	for (i = 0; info->capdevdescs[i]; i++)
		Z_Free(info->capdevdescs[i]);
	Z_Free(info->capdevdescs);
#endif
}
struct audiomenuinfo *M_Menu_Audio_Setup(emenu_t *menu)
{
#ifdef VOICECHAT
	extern cvar_t snd_voip_capturedevice_opts;
#endif
	extern cvar_t snd_device_opts;
	int pairs, i;
	struct audiomenuinfo *info = menu->data;
	char buf[8192];
	menu->remove = M_Menu_Audio_Remove;

	Cmd_TokenizeString(snd_device_opts.string?snd_device_opts.string:"", false, false);
	pairs = Cmd_Argc()/2;
	info->outdevnames = BZ_Malloc((pairs+1)*sizeof(char*));
	info->outdevdescs = BZ_Malloc((pairs+1)*sizeof(char*));
	for (i = 0; i < pairs; i++)
	{
		info->outdevnames[i] = Z_StrDup(COM_QuotedString(Cmd_Argv(i*2+0), buf, sizeof(buf), false));
		info->outdevdescs[i] = Z_StrDup(Cmd_Argv(i*2+1));
	}
	info->outdevnames[i] = NULL;
	info->outdevdescs[i] = NULL;
#ifdef VOICECHAT
	Cmd_TokenizeString(snd_voip_capturedevice_opts.string?snd_voip_capturedevice_opts.string:"", false, false);
	pairs = Cmd_Argc()/2;
	info->capdevnames = BZ_Malloc((pairs+1)*sizeof(char*));
	info->capdevdescs = BZ_Malloc((pairs+1)*sizeof(char*));
	for (i = 0; i < pairs; i++)
	{
		info->capdevnames[i] = Z_StrDup(COM_QuotedString(Cmd_Argv(i*2+0), buf, sizeof(buf), false));
		info->capdevdescs[i] = Z_StrDup(Cmd_Argv(i*2+1));
	}
	info->capdevnames[i] = NULL;
	info->capdevdescs[i] = NULL;
#endif
	return info;
}

void M_Menu_Audio_f (void)
{
	int y;
	emenu_t *menu = M_Options_Title(&y, sizeof(struct audiomenuinfo));
	struct audiomenuinfo *info = M_Menu_Audio_Setup(menu);
	extern cvar_t nosound, snd_leftisright, snd_device, snd_khz, snd_speakers, ambient_level, bgmvolume, snd_playersoundvolume, ambient_fade, cl_staticsounds, snd_inactive, _snd_mixahead, snd_doppler;
//	extern cvar_t snd_noextraupdate, snd_eax, precache;
#ifdef VOICECHAT
	extern cvar_t snd_voip_capturedevice, snd_voip_play, snd_voip_send, snd_voip_test, snd_voip_micamp, snd_voip_vad_threshhold, snd_voip_ducking, snd_voip_codec;
#ifdef HAVE_SPEEX
	extern cvar_t snd_voip_noisefilter;
#endif
#endif

	static const char *soundqualityoptions[] = {
		"11025 Hz",
		"22050 Hz",
		"44100 Hz",
		"48000 Hz",
		NULL
	};

	static const char *soundqualityvalues[] = {
		"11",
		"22",
		"44",
		"48",
		NULL
	};

	static const char *speakeroptions[] = {
		"Mono",
		"Stereo",
		"Quad",
		"5.1",
		NULL
	};

	static const char *speakervalues[] = {
		"1",
		"2",
		"4",
		"6",
		NULL
	};
#ifdef VOICECHAT
	static const char *voipcodecoptions[] = {
#ifdef HAVE_OPUS
		"Opus",
#endif
#ifdef HAVE_SPEEX
#ifdef HAVE_LEGACY
		"Speex (ez-compat)",
#endif
		"Speex (Narrow)",
		"Speex (Wide)",
//		"Speex (UltraWide)",
#endif
//		"Raw16 (11025)",
//		"PCM A-Law",
//		"PCM U-Law",
		NULL
	};
	static const char *voipcodecvalue[] = {
#ifdef HAVE_OPUS
		"2",	//opus
#endif
#ifdef HAVE_SPEEX
#ifdef HAVE_LEGACY
		"0",	//speex non-standard (outdated)
#endif
		"3",	//speex narrow
		"4",	//speex wide
//		"5",	//speex UW
#endif
//		"1",	//pcm16 sucks
//		"6",	//pcma
//		"7",	//pcmu
		NULL
	};

	static const char *voipsendoptions[] = {
		"Push To Talk",
		"Voice Activation",
		"Continuous",
		NULL
	};
	static const char *voipsendvalue[] = {
		"0",
		"1",
		"2",
		NULL
	};
#endif
	menubulk_t bulk[] = {
		MB_REDTEXT("Sound Options", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_CONSOLECMD("Restart Sound", "snd_restart\n", "Restart audio systems and apply set options."),
		MB_SPACING(4),
		MB_COMBOCVAR("Output Device", snd_device, (const char**)info->outdevdescs, (const char**)info->outdevnames, "Choose which audio driver and device to use."),
		MB_SLIDER("Volume", volume, 0, 1, 0.1, NULL),
		MB_COMBOCVAR("Speaker Setup", snd_speakers, speakeroptions, speakervalues, NULL),
		MB_COMBOCVAR("Frequency", snd_khz, soundqualityoptions, soundqualityvalues, NULL),
		MB_CHECKBOXCVAR("Low Quality (8-bit)", snd_loadas8bit, 0),
		MB_CHECKBOXCVAR("Flip Speakers", snd_leftisright, 0),
		MB_SLIDER("Mixahead", _snd_mixahead, 0, 1, 0.05, NULL),
		MB_CHECKBOXCVAR("Disable All Sounds", nosound, 0),
		MB_SPACING(4),

		MB_SLIDER("Player Sound Volume", snd_playersoundvolume, 0, 1, 0.1, NULL),
		MB_SLIDER("Ambient Volume", ambient_level, 0, 1, 0.1, NULL),
		MB_SLIDER("Ambient Fade", ambient_fade, 0, 1000, 1, NULL),
		MB_CHECKBOXCVAR("Static Sounds", cl_staticsounds, 0),
		MB_SLIDER("Music Volume", bgmvolume, 0, 1, 0.1, NULL),
		MB_SLIDER("Doppler Factor", snd_doppler, 0, 10, 0.1, NULL),
		// removed music buffer
		// removed precache
		// removed eax2
		// remove no extra update
		MB_CHECKBOXCVAR("Sound While Inactive", snd_inactive, 0),
		MB_SPACING(4),

#ifdef VOICECHAT
		MB_REDTEXT("Voice Options", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_COMBOCVAR("Microphone Device", snd_voip_capturedevice, (const char**)info->capdevdescs, (const char**)info->capdevnames, NULL),
		MB_SLIDER("Voice Volume", snd_voip_play, 0, 2, 0.1, NULL),
		MB_CHECKBOXCVAR("Microphone Test", snd_voip_test, 0),
		MB_SLIDER("Microphone Volume", snd_voip_micamp, 0, 2, 0.1, NULL),
		MB_COMBOCVAR("Activation Mode", snd_voip_send, voipsendoptions, voipsendvalue, NULL),
		MB_SLIDER("Act. Threshhold", snd_voip_vad_threshhold, 0, 30, 1, NULL),
		MB_CHECKBOXCVAR("Audio Ducking", snd_voip_ducking, 0),
#ifdef HAVE_SPEEX
		MB_CHECKBOXCVAR("Noise Cancelation", snd_voip_noisefilter, 0),
#endif
		MB_COMBOCVAR("Codec", snd_voip_codec, voipcodecoptions, voipcodecvalue, NULL),
#endif

		//MB_CONSOLECMD("Speaker Test", "menu_speakers\n", "Test speaker setup output."),
		MB_END()
	};
	static menuresel_t resel;
	MC_AddFrameStart(menu, y);
	MC_AddBulk(menu, &resel, bulk, 16, 216, y);
	MC_AddFrameEnd(menu, y);
}

#else
void M_Menu_Audio_f (void)
{
	Con_Printf("No sound in cygwin\n");
}
#endif



void M_Menu_Particles_f (void)
{
	emenu_t *menu;
	extern cvar_t r_bouncysparks, r_part_rain, gl_part_flame, r_grenadetrail, r_rockettrail, r_part_rain_quantity, r_particledesc, r_particle_tracelimit, r_part_contentswitch, r_bloodstains;
//	extern cvar_t r_part_sparks_trifan, r_part_sparks_textured, r_particlesystem;

/*	static const char *psystemopts[] =
	{
		"Classic",
		"Script",
		"None",
		NULL
	};
	static const char *psystemvals[] =
	{
		"classic",
		"script",
		"null",
		NULL
	};
*/
	static const char *pdescopts[] =
	{
		"Classic",
		"Faithful",
		"High FPS",
		"Fancy",
		"Fancy+LG",
		"Snazzy",
		"Bare bones",
		NULL
	};
	static const char *pdescvals[] =
	{
		"classic",
		"faithful",
		"highfps",
		"spikeset",
		"spikeset tsshaft",
		"high tsshaft",
		"minimal",
		NULL
	};

	static const char *trailopts[] =
	{
		"Disable",
		"Default",
		"Swap",
		"Alternate",
		"Blood",
		"Zombie",
		"Scrag",
		"Knight",
		"Vore",
		"Rail",
		NULL
	};
	static const char *trailvals[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", NULL };

	int y;
	menubulk_t bulk[] = {
		MB_REDTEXT("Particle Options", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
//		MB_COMBOCVAR("Particle System", r_particlesystem, psystemopts, psystemvals, "Selects particle system to use. Classic is standard Quake particles, script is FTE style scripted particles, and none disables particles entirely."),
		MB_COMBOCVAR("Particle Set", r_particledesc, pdescopts, pdescvals, "Selects particle set to use with the scripted particle system."),
		MB_SPACING(4),
		MB_COMBOCVAR("Rocket Trail", r_rockettrail, trailopts, trailvals, "Chooses effect to replace rocket trails."),
		MB_COMBOCVAR("Grenade Trail", r_grenadetrail, trailopts, trailvals, "Chooses effect to replace grenade trails."),
		MB_SPACING(4),
		// removed texture sparks
		// removed trifan sparks
		MB_CHECKBOXCVAR("Particle Physics", r_bouncysparks, 0),
		MB_CHECKBOXCVAR("Particle Stains", r_bloodstains, 0),
		MB_CHECKBOXCVAR("Content Switching", r_part_contentswitch, 0),
		MB_CHECKBOXCVAR("Surface Emitting", r_part_rain, 0),
		MB_SLIDER("Surface Quantity", r_part_rain_quantity, 0, 10, 1, NULL),
		MB_CHECKBOXCVAR("Model Emitting", gl_part_flame, 0),
		MB_SLIDER("Trace Limit", r_particle_tracelimit, 0, 2000, 100, NULL),
		// removed particle beams
		MB_END()
	};
	static menuresel_t resel;

	menu = M_Options_Title(&y, 0);
	MC_AddFrameStart(menu, y);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
	MC_AddFrameEnd(menu, y);
}

const char *presetname[] =
{
	"286",		//everything turned off to make it as fast as possible, even if you're crippled without it
	"Fast",		//typical deathmatch settings.
	"Spasm",	//faithful NQ aesthetics (fairly strict, but not all out).
	"Vanilla",	//vanilla effects enabled, no content replacement.
	"Normal",	//content replacement enabled
	"Nice",		//potentially expensive, but not painful
	"Realtime",	//everything on
	NULL
};
#define PRESET_NUM (countof(presetname)-1)

// this is structured like this for a possible future feature
// also don't include cvars that need a restart here
const char *presetexec[] =
{
	// 286 options (also the first commands to be execed in the chain)
	"seta m_preset_chosen 1;"
	"seta gl_texturemode nn;"
	"seta gl_texturemode2d n;"
	"seta gl_blendsprites 0;"
	"seta r_particlesystem null;"
	"seta r_particledesc \"\";"
	"seta r_part_classic_square 0;"
	"seta r_part_classic_expgrav 10;"
	"seta r_part_classic_opaque 0;"
	"seta cl_expsprite 1;"
	"seta r_stains 0;"
	"seta r_drawflat 1;"
	"seta r_lightmap 0;"
	"seta r_nolerp 1;"
	"seta r_noframegrouplerp 1;"
	"seta r_nolightdir 1;"
	"seta r_dynamic 0;"
	"seta r_bloom 0;"
	"seta r_softwarebanding 0;"
	"seta d_mipcap 0 1000;"
	"seta gl_affinemodels 0;"
	"seta gl_polyblend 0;"
	"seta gl_flashblend 0;"
	"seta gl_specular 0;"
	"seta r_deluxemapping 0;"
	"seta r_loadlit 0;"
	"seta r_fastsky 1;"
	"seta r_drawflame 0;"
	"seta r_waterstyle 1;"
	"seta r_lavastyle 1;"		//defer to water
	"seta r_fog_cullentities 2;"
//	"seta r_slimestyle \"\";"	//defer to water
	"seta r_coronas 0;"
	"seta r_shadow_realtime_dlight 0;"
	"seta r_shadow_realtime_world 0;"
	"seta r_shadow_realtime_dlight_shadows 1;"
	"seta r_glsl_offsetmapping 0;"
	"seta vid_hardwaregamma 3;"	//people benchmarking against other engines with fte using glsl gamma and the other not is annoying as fuck.
//	"seta gl_detail 0;"
	"seta gl_load24bit 0;"
	"seta r_replacemodels \"\";"
	"seta r_waterwarp 0;"
	"seta r_lightstylesmooth 0;"
	"seta r_lightstylespeed 0;"
	"seta r_part_density 0.25;"
	"seta cl_nolerp 1;"
	"seta r_lerpmuzzlehack 0;"
	"seta v_gunkick 0;"
	"seta r_shadows 0;"
	"seta v_viewmodel_quake 0;"
	"seta cl_rollangle 0;"
	"seta cl_bob 0;"
	"seta cl_sbar 0;"
//	"cvarreset sv_nqplayerphysics;"	//server settings in a preset might be bad.
	"seta cl_demoreel 0;"
	"seta cl_gibfilter 1;"
	"if cl_deadbodyfilter == 0 then seta cl_deadbodyfilter 1;"		//as useful as 2 is, some mods use death frames for crouching etc.
	"seta gl_simpleitems 1;"
	"seta cl_fullpitch 1;seta maxpitch \"\";seta minpitch \"\";"	//mimic quakespasm where possible.
	"seta r_graphics 1;"
	"seta r_renderscale 1;"
	"seta gl_texture_anisotropic_filtering 0;"
	// end '286'

	, // fast options (for deathmatch)
	"gl_texturemode ln;"
	"gl_texturemode2d n;"
#ifdef MINIMAL
	"r_particlesystem classic;"
#else
	"r_particlesystem script;"
#endif
	"r_particledesc classic;"
	"r_drawflat 0;"
	"r_nolerp 0;"
	"gl_flashblend 1;"
	"r_loadlit 1;"
	"r_fastsky 0;"
	"r_waterstyle 1;"
	"r_lavastyle 1;"
	"r_nolightdir 0;"
	"seta gl_simpleitems 0;"
	// end fast

	, //quakespasm-esque options (for singleplayer faithful).
	"gl_texturemode2d l.l;"
	"r_part_density 1;"
	"gl_polyblend 1;"
	"r_dynamic 2;"
	"gl_flashblend 0;"
	"cl_nolerp 0;"	//projectiles lerped at least.
	"r_noframegrouplerp 1;" //flames won't lerp
	"r_waterwarp 1;"
	"r_drawflame 1;"
	"v_gunkick 1;"
	"cl_rollangle 2.0;"
	"cl_bob 0.02;"
	"r_fog_cullentities 1;"
	"r_lightstylespeed 10;"
	"vid_hardwaregamma 1;"		//auto hardware gamma, for fast fullscreen and usable windowed.
	"r_part_classic_expgrav 1;"	//vanillaery
	"r_part_classic_opaque 1;"
//	"r_particlesystem script;"	//q2 or hexen2 particle effects need to be loadable
	"cl_sbar 2;"				//its a style thing
//	"sv_nqplayerphysics 1;"		//gb wanted this, should give nq physics to people who want nq settings. note that this disables prediction. disabled again because 'auto' matches the mod, which generally works out better.
	"cl_demoreel 1;"			//yup, arcadey
	//"d_mipcap \"0 3\";"		//logically correct, but will fuck up on ATI drivers if increased mid-map, because ATI will just ignore any levels that are not currently enabled.
	"cl_gibfilter 0;"
	"seta cl_deadbodyfilter 0;"
	"gl_texture_anisotropic_filtering 4;"
	"cl_fullpitch 1;maxpitch 90;seta minpitch -90;"	//QS has cheaty viewpitch range. some maps require it.
	// end spasm

	, //vanilla-esque options (for purists).
	"cl_fullpitch 0;maxpitch \"\";seta minpitch \"\";"	//quakespasm is not vanilla
	"gl_texturemode nll;"		//yup, we went there.
	"gl_texturemode2d n.l;"		//yeah, 2d too.
	"r_nolerp 1;"
	"cl_sbar 1;"
	"d_mipcap 0 2;"				//gl without anisotropic filtering favours too-distant mips too often, so lets just pretend it doesn't exist. should probably mess with lod instead or something
	"v_viewmodel_quake 1;"
	"r_loadlit 0;"
	"gl_affinemodels 1;"
	"r_softwarebanding 1;"		//ugly software banding.
	"r_part_classic_square 1;"	//blocky baby!
	// end vanilla

	, // normal (faithful) options, but with content replacement thrown in
//#ifdef MINIMAL
//	"r_particlesystem classic;"
//#else
//	"r_particlesystem script;"
//	"r_particledesc classic;"
//#endif
	"r_part_classic_square 0;"
	"r_part_classic_expgrav 10;"	//gives a slightly more dynamic feel to them
	"r_part_classic_opaque 0;"
	"gl_load24bit 1;"
	"r_replacemodels \"md3 md2 md5mesh\";"
	"r_coronas 1;"
	"r_dynamic 1;"
	"r_softwarebanding 0;"
	"d_mipcap 0 1000;"
	"gl_affinemodels 0;"
	"r_lerpmuzzlehack 1;"
	"gl_texturemode ln;"
	"gl_texturemode2d l;"
	"cl_sbar 0;"
	"v_viewmodel_quake 0;"	//don't move the gun around weirdly.
//	"cvarreset sv_nqplayerphysics;"
	"cl_demoreel 0;"
	"r_loadlit 1;"
	"r_nolerp 0;"
	"r_noframegrouplerp 0;"
	"cl_fullpitch 1;maxpitch 90;seta minpitch -90;"
	//end normal

	, // nice options
//	"r_stains 0.75;"
	"gl_texturemode lll;"
#ifndef MINIMAL
//	"r_particlesystem script;"
	"r_particledesc \"high tsshaft\";"
#endif
	"gl_specular 1;"
//	"r_loadlit 3;"
	"r_waterstyle 2;"
	"gl_blendsprites 1;"
//	"r_fastsky -1;"
	"r_dynamic 0;"
	"r_shadow_realtime_dlight 1;"
//	"gl_detail 1;"
	"r_lightstylesmooth 1;"
	"r_deluxemapping 2;"
	//end 'nice'

	, // realtime options
	"r_bloom 1;"
	"r_deluxemapping 1;"	//won't be seen anyway
	"r_particledesc \"high tsshaft\";"
//	"r_waterstyle 3;"	//too expensive.
	"r_glsl_offsetmapping 1;"
	"r_shadow_realtime_world 1;"
	"gl_texture_anisotropic_filtering 16;"
	"vid_hardwaregamma 4;"	//scene gamma
	//end 'realtime'
};

struct
{
	const char *name;
	qboolean dorestart;
	const char *desc;
	const char *settings;
} builtinpresets[] =
{
	{	"hdr", true,
		"Don't let colour depth stop you!",

		"set vid_srgb 2\n"
		"set r_hdr_irisadaptation 1\n"
	},
	{	"shib", true,
		"Performance optimisations for large/detailed maps.",

		"set r_temporalscenecache 1\n"	//the main speedup.
		"set r_lightstylespeed 0\n"		//FIXME: we shouldn't need this, but its too stuttery without.
		"set sv_autooffload 1\n"		//Needs polish still.
		"set gl_pbolightmaps 1\n"		//FIXME: this needs to be the default eventually.
	},
	{	"dm", false,
		"Various settings to make you more competitive."

		"set cl_yieldcpu 0\n"
		"set v_kickroll 0\n"		//roll change when taking damage
		"set v_kickpitch 0\n"		//pitch change when taking damage
		"set v_damagecshift 0\n"	//colour change when taking damage
		"set v_gunkick 0\n"			//recoil when firing
		"set cl_rollangle 0\n"		//rolling when strafing
		"set cl_bob 0\n"			//view bobbing when moving.
#ifdef _WIN32
		"set sys_clockprecision 1\n"	//windows kinda sucks otherwise
#endif
	},
	{	"qw", false,
		"Enable QuakeWorld physics, for better gameplay.",

		"set sv_nqplayerphysics 0\n"
		"set sv_gameplayfix_multiplethinks 1\n"
		"cvarreset pm_bunnyfriction\n"
		"cvarreset pm_edgefriction\n"
		"cvarreset pm_slidefix\n"
		"cvarreset pm_slidyslopes\n"
	},
	{	"hybridphysics", false,
		"Tweak QuakeWorld player physics to feel like nq physics, while still supporting prediction.",

		"set sv_nqplayerphysics 0\n"
		"set sv_gameplayfix_multiplethinks 1\n"
		"set pm_bunnyfriction 1\n"	//don't need bunnyspeedcap with this.
		"set pm_edgefriction 2\n"	//forces traceline instead of tracebox, to match nq (applies earlier, making it more aggressive)
		"set pm_slidefix 1\n"		//smoother running down slopes
		"set pm_slidyslopes 1\n"	//*sigh*
		"set pm_noround 1\n"		//lame
		"set sv_maxtic 0\n"			//fixed tick rates.
	},
	{	"nq", false,
		"Disable QuakeWorld physics, for nq mod compat.",

		"set sv_nqplayerphysics 1\n"
		"set sv_gameplayfix_multiplethinks 0\n"
		//*also* set these, in case they use nqplayerphysics 2 after, which should give better hints.
		"set pm_bunnyfriction 1\n"	//don't need bunnyspeedcap with this.
		"set pm_edgefriction 2\n"	//forces traceline instead of tracebox, to match nq (applies earlier, making it more aggressive)
		"set pm_slidefix 1\n"		//smoother running down slopes
		"set pm_slidyslopes 1\n"	//*sigh*
		"set pm_noround 1\n"		//lame
	},

	{	"dp", false,
		"Reconfigures FTE to mimic DP for compat reasons.",

		"if $server then echo Be sure to restart your server\n"

		"fps_preset nq\n"
		//these are for smc+derived mods
		"sv_listen_dp 1\n"					//awkward, but forces the server to load the effectinfo.txt in advance.
		"sv_bigcoords 1\n"					//for viewmodel lep precision (would be better to use csqc)
		"r_particledesc \"effectinfo high\"\n" //blurgh.
		"dpcompat_noretouchground 1\n"		//don't call touch functions on entities that already appear onground. this also changes the order that the onground flag is set relative to touch functions.
		"cl_nopred 1\n"						//DP doesn't predict by default, and DP mods have a nasty habit of clearing .solid values during prethinks, which screws up prediction. so play safe.
		"r_dynamic 0\nr_shadow_realtime_dlight 1\n" //fte has separate cvars for everything. which kinda surprises people and makes stuff twice as bright as it should be.
		"r_coronas_intensity 0.25\n"
		"con_logcenterprint 0\n"			//kinda annoying....
		"scr_fov_mode 4\n"					//for fairer framerate comparisons

		//general compat stuff
		"dpcompat_console 1\n"				//
		"dpcompat_findradiusarealinks 1\n"	//faster findradiuses (but that require things are setorigined properly)
		"dpcompat_makeshitup 2\n"			//flatten shaders to a single pass, then add new specular etc passes.
		//"dpcompat_nopremulpics 1\n"			//don't use premultiplied alpha (solving issues with compressed image formats)
		"dpcompat_psa_ungroup 1\n"			//don't use framegroups with psk models at all.
		"dpcompat_set 1\n"					//handle 3-arg sets differently
		"dpcompat_stats 1\n"				//truncate float stats
		"dpcompat_strcat_limit 16383\n"		//xonotic compat. maximum length of strcat strings.

//		"sv_listen_dp 1\nsv_listen_nq 0\nsv_listen_qw 0\ncl_loopbackprotocol dpp7\ndpcompat_nopreparse 1\n"
	},

	{	"tenebrae", true,
		"Reconfigures FTE to mimic Tenebrae for compat/style reasons.",
		//for the luls. combine with the tenebrae mod for maximum effect.
		"fps_preset nq\n"
		"set r_shadow_realtime_world 1\n"
		"set r_shadow_realtime_dlight 1\n"
		"set r_shadow_bumpscale_basetexture 4\n"
		"set r_shadow_shadowmapping 0\n"
		"set gl_specular 1\n"
		"set gl_specular_power 16\n"
		"set gl_specular_fallback 1\n"
		"set mod_litsprites_force 1\n"
		"set gl_blendsprites 2\n"
		"set r_nolerp 1\n"	//well, that matches tenebrae. for the luls, right?
	},

	{	"timedemo", false,
		"Reconfigure some stuff to get through timedemos really fast. Some people might consider this cheating.",
		//some extra things to pwn timedemos.
		"fps_preset fast\n"
		"set r_renderscale 1\n"
		"set contrast 1\n"
		"set gamma 1\n"
		"set brightness 0\n"
		"set scr_autoid 0\n"
		"set scr_autoid_team 0\n"
		"set r_dynamic 0\n"
		"set sbar_teamstatus 2\n"
		"set gl_polyblend 0\n"
#if 1
		//these are cheaty settings.
		"set gl_flashblend 0\n"
		"set cl_predict_players 0\n"	//very cheaty. you won't realise its off, but noone would disable it for actual play.
#else
		//to make things fair
		"set gl_flashblend 1\n"
		"set r_part_density 1\n"
#endif
	},
};

static void ApplyPreset (int presetnum, qboolean doreload)
{
	int i;
	//this function is written backwards, to ensure things work properly in configs etc.

	// TODO: work backwards and only set cvars once
	if (doreload)
	{
		forcesaveprompt = true;
		Cbuf_InsertText("\nfs_restart\nvid_reload\ncl_warncmd 1\n", RESTRICT_LOCAL, true);
	}
	for (i = presetnum; i >= 0; i--)
	{
		Cbuf_InsertText(presetexec[i], RESTRICT_LOCAL, true);
	}
	if (doreload)
	{
		Cbuf_InsertText("\ncl_warncmd 0\n", RESTRICT_LOCAL, true);
	}
}

static int M_Menu_Preset_GetActive(void)
{
	extern cvar_t r_drawflat;

	//bottoms up!
#ifdef RTLIGHTS
	if (r_shadow_realtime_world.ival)
		return 6;	//realtime
	else
#endif
		if (r_deluxemapping_cvar.ival)
		return 5;	//nice
	else if (gl_load24bit.ival)
		return 4;	//normal
	else if (r_softwarebanding_cvar.ival)
		return 3;	//vanilla
	else if (cl_sbar.ival == 2)
		return 2;	//spasm
	else if (!r_drawflat.ival)
		return 1;	//fast
	else
		return 0;	//simple
}

static int M_Menu_ApplyGravity(menuoption_t *op)
{	//menu is bottom-up, so we return the y pos of its parent...
	if (!op)
		return 0;

	if (op->common.ishidden)
		return M_Menu_ApplyGravity(op->common.next);

	if (op->common.grav_y)
		op->common.posy = M_Menu_ApplyGravity(op->common.next)+op->common.grav_y;
	else	//not moving this one, but make sure others move properly.
		M_Menu_ApplyGravity(op->common.next);

	return op->common.posy;
}
static void M_Menu_Preset_Predraw(emenu_t *menu)
{
	extern cvar_t m_preset_chosen;
	menuoption_t *op;
	int preset = M_Menu_Preset_GetActive();
	int i;
	qboolean forcereload = false;
	qboolean filtering = false;
	extern cvar_t cfg_save_auto;

#ifdef RTLIGHTS
	preset = 6-preset;
#else
	preset = 5-preset;
#endif

	for (op = menu->options; op; op = op->common.next)
	{
		if (op->common.type == mt_button)
		{
			if (!strcmp(op->button.command, "menupop\n"))
			{
				if (m_preset_chosen.ival)
					op->button.text = localtext("^sAccept");
			}
			else if (!strncmp(op->button.command, "fps_preset ", 11))
			{
				if (((char*)op->button.text)[0] == '^' && ((char*)op->button.text)[1] == ((preset!=0)?'m':'7'))
					((char*)op->button.text)[1] = (preset==0)?'m':'7';
				preset--;
			}
#if defined(WEBCLIENT) && defined(PACKAGEMANAGER)
			else if (!strcmp(op->button.command, "menu_download\n"))
			{
				op->button.text = PM_AreSourcesNew(false)?localtext("^bPackages (New!)"):localtext("Packages");
				op->common.posx = op->common.next->common.posx;
				op->common.width = 216-op->common.posx;
			}
#endif
		}
		else if (!filtering)
		{
			if (op->common.type == mt_checkbox && op->check.var == &cfg_save_auto)
				filtering = true;
		}
		else if(op->common.type == mt_checkbox||
				op->common.type == mt_slider||
				op->common.type == mt_combo) op->common.ishidden = preset!=0;

		if (op->common.type == mt_combo && op->combo.cvar)
		{	//make sure combo items are updated properly if their cvar value is changed via some other mechanism...
			const char *curval = op->combo.cvar->latched_string?op->combo.cvar->latched_string:op->combo.cvar->string;
			if (strcmp(curval, op->combo.values[op->combo.selectedoption]))
			{
				for (i = 0; i < op->combo.numoptions; i++)
				{
					if (!strcmp(curval, op->combo.values[i]))
					{
						op->combo.selectedoption = i;
						break;
					}
				}
			}
			if (op->combo.cvar->latched_string && (op->combo.cvar->flags&CVAR_LATCHMASK) == CVAR_RENDERERLATCH)
				forcereload = true;
		}
	}
	M_Menu_ApplyGravity(menu->options);
	menu->cursoritem->common.posy = menu->selecteditem->common.posy + (menu->selecteditem->common.height-menu->cursoritem->common.height)/2;

	if (forcereload)
		Cbuf_InsertText("\nfs_restart\nvid_reload\n", RESTRICT_LOCAL, true);
}

void M_Menu_Preset_f (void)
{
	extern cvar_t cfg_save_auto;
	emenu_t *menu;
	int y;
	menuoption_t *presetoption[7];
	extern cvar_t r_nolerp, r_loadlits;
#ifdef HAVE_SERVER
	extern cvar_t sv_nqplayerphysics;
#endif
#if defined(RTLIGHTS) && (defined(GLQUAKE) || defined(VKQUAKE))
	extern cvar_t r_bloom, r_shadow_realtime_world_importlightentitiesfrommap;
#endif

	static const char *deluxeopts[] = {
		"Off",
		"Auto",
		"Force",
		NULL
	};
	static const char *litopts[] = {
		"Off",
		"Auto",
		NULL
	};
	static const char *litvals[] = {
		"1",
		"3",
		NULL
	};
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Please Choose Preset", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_CONSOLECMDRETURN("^7simple  (untextured)",	"fps_preset 286\n",			"Lacks textures, particles, pretty much everything.", presetoption[0]),
			MB_CHECKBOXCVAR("anim snapping", r_nolerp, 0),
		MB_CONSOLECMDRETURN("^7fast (qw deathmatch)",	"fps_preset fast\n",		"Fullscreen effects off to give consistant framerates", presetoption[1]),
		MB_CONSOLECMDRETURN("^7spasm    (nq compat)",	"fps_preset spasm\n",		"Aims for visual compatibility with common NQ engines. Also affects mods slightly.", presetoption[2]),
#ifdef HAVE_SERVER
			MB_CHECKBOXCVAR("nq physics", sv_nqplayerphysics, 0),
#endif
		MB_CONSOLECMDRETURN("^7vanilla  (softwarey)",	"fps_preset vanilla\n",		"This is for purists! Party like its 1995! No sanity spared!", presetoption[3]),
			MB_CHECKBOXCVAR("anim snapping", r_nolerp, 1),
#if defined(GLQUAKE)
			MB_CHECKBOXCVAR("model swimming", gl_affinemodels, 0),
#endif
		MB_CONSOLECMDRETURN("^7normal    (faithful)",	"fps_preset normal\n",		"An updated but still faithful appearance, using content replacements where applicable", presetoption[4]),
		MB_CONSOLECMDRETURN("^7nice       (dynamic)",	"fps_preset nice\n",		"For people who like nice things, but still want to actually play", presetoption[5]),
			MB_COMBOCVAR("rgb lighting", r_loadlits, litopts, litvals, NULL),
			MB_COMBOCVAR("deluxemaps", r_deluxemapping_cvar, deluxeopts, NULL, NULL),
#if defined(RTLIGHTS) && (defined(GLQUAKE) || defined(VKQUAKE))
		MB_CONSOLECMDRETURN("^7realtime    (all on)",	"fps_preset realtime\n",	"For people who value pretty over fast/smooth. Not viable for deathmatch.", presetoption[6]),
			MB_CHECKBOXCVAR("bloom", r_bloom, 1),
			MB_CHECKBOXCVAR("force rtlights", r_shadow_realtime_world_importlightentitiesfrommap, 1),
#endif
		MB_SPACING(16),
		MB_CHECKBOXCVARTIP("Auto-save Settings", cfg_save_auto, 1, "If this is disabled, you will need to explicitly save your settings."),
#if defined(WEBCLIENT) && defined(PACKAGEMANAGER)
		MB_CONSOLECMD("Packages",				"menu_download\n",	"Configure sources and packages."),
#endif
		MB_SPACING(16),
		MB_CONSOLECMD("Accept",					"menupop\n",	"Continue with selected settings."),
		MB_END()
	};
	static menuresel_t resel;
	int item;
	extern cvar_t r_drawflat;
	menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 216, y);
	menu->selecteditem = menu->options;
	//bottoms up!
#ifdef RTLIGHTS
	if (r_shadow_realtime_world.ival)
		item = 6;	//realtime
	else
#endif
		if (r_deluxemapping_cvar.ival)
		item = 5;	//nice
	else if (gl_load24bit.ival)
		item = 4;	//normal
	else if (r_softwarebanding_cvar.ival)
		item = 3;	//vanilla
	else if (cl_sbar.ival == 2)
		item = 2;	//spasm
	else if (!r_drawflat.ival)
		item = 1;	//fast
	else
		item = 0;	//simple
	if (presetoption[item])
	{
		menu->selecteditem = presetoption[item];
		menu->cursoritem->common.posy = menu->selecteditem->common.posy + (menu->selecteditem->common.height-menu->cursoritem->common.height)/2;
	}

	//so they can actually see the preset they're picking.
	menu->nobacktint = true;
	menu->predraw = M_Menu_Preset_Predraw;

	Cbuf_InsertText("\ndemos idle\n", RESTRICT_LOCAL, false);
}

void FPS_Preset_f (void)
{
	char *presetfname;
	char *arg = Cmd_Argv(1);
	int i;
	qboolean doreload = true;

	if (!*arg)
	{
		M_Menu_Preset_f();
		return;
	}

	if (!strncmp(arg, "builtin_", 8))
	{
		arg += 8;
		doreload = false;
	}
	else
	{
		presetfname = va("configs/preset_%s.cfg", arg);
		if (COM_FCheckExists(presetfname))
		{
			char buffer[MAX_OSPATH];
			COM_QuotedString(presetfname, buffer, sizeof(buffer), false);
			Cbuf_InsertText(va("\nexec %s\nfs_restart\n", buffer), RESTRICT_LOCAL, false);
			return;
		}
	}

	for (i = 0; i < PRESET_NUM; i++)
	{
		if (!stricmp(presetname[i], arg))
		{
			ApplyPreset(i, doreload);
			return;
		}
	}

	for (i = 0; i < countof(builtinpresets); i++)
	{
		if (!stricmp(builtinpresets[i].name, arg))
		{
			if (doreload && builtinpresets[i].dorestart)
				Cbuf_InsertText("\nfs_restart\nvid_reload\n", RESTRICT_LOCAL, false);
			Cbuf_InsertText(builtinpresets[i].settings, RESTRICT_LOCAL, false);
			return;
		}
	}

	Con_Printf("Preset %s not recognised\n", arg);
	Con_Printf("Valid presests:\n");
	for (i = 0; i < PRESET_NUM; i++)
		Con_Printf("%s\n", presetname[i]);
	for (i = 0; i < countof(builtinpresets); i++)
		Con_DPrintf("%s\n", builtinpresets[i].name);
}

static int QDECL CompletePresetList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	if (!Q_strncasecmp(name, "configs/preset_", 15))
	{
		char preset[MAX_QPATH];
		COM_StripExtension(name+15, preset, sizeof(preset));
		ctx->cb(preset, NULL, NULL, ctx);
	}
	return true;
}
void FPS_Preset_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	if (argn == 1)
	{
		int i;
		size_t partiallen = strlen(partial);

		COM_EnumerateFiles(va("configs/preset_%s*.cfg", partial), CompletePresetList, ctx);

		for (i = 0; i < PRESET_NUM; i++)
			if (!Q_strncasecmp(partial, presetname[i], partiallen))
				ctx->cb(presetname[i], NULL, NULL, ctx);

		for (i = 0; i < countof(builtinpresets); i++)
			if (!Q_strncasecmp(partial, builtinpresets[i].name, partiallen))
				ctx->cb(builtinpresets[i].name, builtinpresets[i].desc, NULL, ctx);
	}
}

/*typedef struct fpsmenuinfo_s
{
	menucombo_t *preset;
} fpsmenuinfo_t;
qboolean M_PresetApply (union menuoption_s *op, struct emenu_s *menu, int key)
{
	fpsmenuinfo_t *info = (fpsmenuinfo_t*)menu->data;

	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

	Cbuf_AddText("fps_preset ", RESTRICT_LOCAL);
	Cbuf_AddText(info->preset->options[info->preset->selectedoption], RESTRICT_LOCAL);
	Cbuf_AddText("\n", RESTRICT_LOCAL);

	return true;
}*/

void M_Menu_FPS_f (void)
{
	static const char *fpsopts[] =
	{
		"Disabled",
		"Average FPS",
		"Timing Graph",
		NULL
	};
	static const char *fpsvalues[] = {"0", "1", "2", NULL};
	static const char *entlerpopts[] =
	{
		"Enabled (always)",
		"Disabled",
		"Enabled (SP only)",
		NULL
	};
	static const char *playerlerpopts[] =
	{
		"Disabled",
		"Enabled",
		NULL
	};
	static const char *bodyopts[] =
	{
		"Disabled",
		"Ground",
		"All",
		NULL
	};
	static const char *values_0_1_2[] = {"0", "1", "2", NULL};
	static const char *values_0_1[] = {"0", "1", NULL};

	emenu_t *menu;
//	fpsmenuinfo_t *info;

	extern cvar_t v_contentblend, show_fps, cl_r2g, cl_gibfilter, cl_expsprite, cl_deadbodyfilter, cl_lerp_players, cl_nolerp, cl_maxfps, cl_yieldcpu, r_halfrate;
	static menuresel_t resel;
	int y;
	menu = M_Options_Title(&y, 0);
//	menu = M_Options_Title(&y, sizeof(fpsmenuinfo_t));
//	info = (fpsmenuinfo_t *)menu->data;

	/*lerping is here because I'm not sure where else to put it. if they care about framerate that much then they'll want to disable interpolation to get as up-to-date stuff as possible*/

	{
		menubulk_t bulk[] =
		{
			MB_REDTEXT("FPS Options", true),
			MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
//			MB_COMBORETURN("Preset", presetname, 2, info->preset, "Select a builtin configuration of graphical settings."),
//			MB_CMD("Apply", M_PresetApply, "Applies selected preset."),
//			MB_SPACING(4),
			MB_COMBOCVAR("Show FPS", show_fps, fpsopts, fpsvalues, "Display FPS or frame millisecond values on screen. Settings except immediate are for values across 1 second."),
			MB_EDITCVARSLIM("Framerate Limiter", cl_maxfps.name, "Limits the maximum framerate. Set to 0 for none."),
			MB_CHECKBOXCVARTIP("Yield CPU", cl_yieldcpu, 1, "Reduce CPU usage between frames.\nShould probably be off when using vsync."),
			MB_CHECKBOXCVARTIP("Half-Rate Shading", r_halfrate, 0, "Reduce the number of shader invocations to save gpu time (doesn't harm edges)."),
			MB_COMBOCVAR("Player lerping", cl_lerp_players, playerlerpopts, values_0_1, "Smooth movement of other players, but will increase effective latency. Does not affect all network protocols."),
			MB_COMBOCVAR("Entity lerping", cl_nolerp, entlerpopts, values_0_1_2, "Smooth movement of entities, but will increase effective latency."),
			MB_CHECKBOXCVAR("Content Blend", v_contentblend, 0),
			MB_CHECKBOXCVAR("Gib Filter", cl_gibfilter, 0),
			MB_COMBOCVAR("Dead Body Filter", cl_deadbodyfilter, bodyopts, values_0_1_2, "Selects which dead player frames to filter out in rendering. Ground frames are those of the player lying on the ground, and all frames include all used in the player dying animation."),
			MB_CHECKBOXCVAR("Explosion Sprite", cl_expsprite, 0),
			MB_CHECKBOXCVAR("Rockets to Grenades", cl_r2g, 0),
			MB_EDITCVAR("Skybox", "r_skybox"),
			MB_END()
		};
		MC_AddFrameStart(menu, y);
		MC_AddBulk(menu, &resel, bulk, 16, 216, y);
		MC_AddFrameEnd(menu, y);
	}
}

void M_Menu_Render_f (void)
{
	static const char *warpopts[] =
	{
		"Disabled",
		"FOV Warp",
		"Shader",
		NULL
	};
	static const char *warpvalues[] =
	{
		"0",
		"-1",
		"1",
		NULL
	};

	static const char *logcenteropts[] = {"Off", "Singleplayer", "Always", NULL};
	static const char *logcentervalues[] = {"0", "1", "2", NULL};

	static const char *cshiftopts[] = {"Off", "Fullscreen", "Edges", NULL};
	static const char *cshiftvalues[] = {"0", "1", "2", NULL};

	static const char *scenecacheopts[] = {"Auto", "Force Off", "Force On", NULL};
	static const char *scenecachevalues[] = {"", "0", "1", NULL};

	emenu_t *menu;
	extern cvar_t r_novis, cl_item_bobbing, r_waterwarp, r_nolerp, r_noframegrouplerp, r_fastsky, gl_nocolors, gl_lerpimages, r_wateralpha, r_drawviewmodel, gl_cshiftenabled, r_hdr_irisadaptation, scr_logcenterprint, r_fxaa, r_graphics;
#if defined(GLQUAKE) || defined(VKQUAKE)
	extern cvar_t r_bloom;
#endif
	static menuresel_t resel;

	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Rendering Options", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_CHECKBOXCVARTIP("Graphics", r_graphics, 0, "The only option console games have. Yes, this is a joke cvar. Try toggling it!"),	//graphics on / off. Its a general dig at modern games not having any real options.
		MB_CHECKBOXCVARTIP("Disable VIS", r_novis, 0, "You shouldn't normally set this. Its better to use vispatches for your maps in order to support transparent water properly."),
		MB_CHECKBOXCVARTIP("Fast Sky", r_fastsky, 0, "Use black skies. On modern machines this is more of a stylistic choice that a perfomance helper."),
		MB_CHECKBOXCVAR("Lerp Images", gl_lerpimages, 0),
		MB_CHECKBOXCVAR("Disable Model Lerp", r_nolerp, 0),
		MB_CHECKBOXCVAR("Disable Framegroup Lerp", r_noframegrouplerp, 0),
		MB_CHECKBOXCVAR("Model Bobbing", cl_item_bobbing, 0),
		MB_COMBOCVAR("Scene Cache", r_temporalscenecache, scenecacheopts,scenecachevalues, "Cache scene data to significantly optimise highly complex scenes or unvised maps.\nThis may result in offscreen surfaces getting rendered."),
		MB_COMBOCVAR("Water Warp", r_waterwarp, warpopts, warpvalues, NULL),
		MB_SLIDER("Water Alpha", r_wateralpha, 0, 1, 0.1, NULL),
		MB_SLIDER("Viewmodel Alpha", r_drawviewmodel, 0, 1, 0.1, NULL),
		MB_COMBOCVAR("Screen Tints", gl_cshiftenabled, cshiftopts, cshiftvalues, "Changes how screen flashes should be displayed (otherwise known as polyblends)."),
#ifdef QWSKINS
		MB_CHECKBOXCVAR("Ignore Player Colors", gl_nocolors, 0),
#endif
		MB_COMBOCVAR("Log Centerprints", scr_logcenterprint, logcenteropts, logcentervalues, "Display centerprints in the console also."),
		MB_CHECKBOXCVAR("FXAA", r_fxaa, 0),
#if defined(GLQUAKE) || defined(VKQUAKE)
		MB_CHECKBOXCVAR("Bloom", r_bloom, 0),
#endif
		MB_CHECKBOXCVARTIP("HDR", r_hdr_irisadaptation, 0, "Adjust scene brightness to compensate for lighting levels."),
		MB_END()
	};
	menu = M_Options_Title(&y, 0);
	MC_AddFrameStart(menu, y);
	MC_AddBulk(menu, &resel, bulk, 16, 216, y);
	MC_AddFrameEnd(menu, y);
}

void M_Menu_Textures_f (void)
{
	static const char *texturefilternames[] =
	{
		"Nearest (noise)",
		"Nearest (harsh)",
		"Nearest (soft)",
		"Bilinear",
		"Trilinear",
		NULL
	};
	static const char *texturefiltervalues[] =
	{
		"GL_NEAREST",
		"GL_NEAREST_MIPMAP_NEAREST",
		"nll",
		"GL_LINEAR_MIPMAP_NEAREST",
		"GL_LINEAR_MIPMAP_LINEAR",
		NULL
	};

	static const char *texture2dfilternames[] =
	{
		"Nearest",
		"Linear",
		NULL
	};
	static const char *texture2dfiltervalues[] =
	{
		"GL_NEAREST",
		"GL_LINEAR",
		NULL
	};


	static const char *anisotropylevels[] =
	{
		"Off",
		"2x",
		"4x",
		"8x",
		"16x",
		NULL
	};
	static const char *anisotropyvalues[] =
	{
		"1",
		"2",
		"4",
		"8",
		"16",
		NULL
	};

	static const char *texturesizeoptions[] =
	{
		"128",
//		"192",
		"256",
//		"384",
		"512",
//		"768",
		"1024",
		"2048",
		"4096",
		"8192",
		NULL
	};

	extern cvar_t gl_load24bit, gl_specular, gl_compress, gl_picmip, gl_picmip2d, gl_max_size, r_drawflat, r_glsl_offsetmapping;
	extern cvar_t gl_texture_anisotropic_filtering, gl_texturemode, gl_texturemode2d, gl_mipcap;
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Texture Options", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_CHECKBOXCVAR("Load Replacements", gl_load24bit, 0),
		MB_CHECKBOXCVAR("Simple Texturing", r_drawflat, 0),
		MB_COMBOCVAR("3D Filter Mode", gl_texturemode, texturefilternames, texturefiltervalues, "Chooses the texture filtering method used for 3D objects."),
		MB_COMBOCVAR("2D Filter Mode", gl_texturemode2d, texture2dfilternames, texture2dfiltervalues, "Chooses the texture filtering method used for HUD, menus, and other 2D assets."),
		MB_COMBOCVAR("Anisotropy", gl_texture_anisotropic_filtering, anisotropylevels, anisotropyvalues, NULL),
		MB_SPACING(4),
#ifdef GLQUAKE
		MB_CHECKBOXCVAR("Software-style Rendering", r_softwarebanding_cvar, 0),
#endif
		MB_CHECKBOXCVAR("Specular Highlights", gl_specular, 0),
//		MB_CHECKBOXCVAR("Detail Textures", gl_detail, 0),
		MB_CHECKBOXCVAR("offsetmapping", r_glsl_offsetmapping, 0),
		MB_SPACING(4),
		MB_CHECKBOXCVAR("Texture Compression", gl_compress, 0), // merge the save compressed tex options into here?
		MB_SLIDER("3D Picmip", gl_picmip, 0, 16, 1, NULL),
		MB_SLIDER("2D Picmip", gl_picmip2d, 0, 16, 1, NULL),
		MB_SLIDER("World Mipcap", gl_mipcap, 0, 3, 1, NULL),
		MB_COMBOCVAR("Max Texture Size", gl_max_size, texturesizeoptions, texturesizeoptions, NULL),
		MB_END()
	};
	emenu_t *menu = M_Options_Title(&y, 0);
	static menuresel_t resel;
	MC_AddFrameStart(menu, y);
	MC_AddBulk(menu, &resel, bulk, 16, 216, y);
	MC_AddFrameEnd(menu, y);
}

typedef struct {
	menucombo_t *lightcombo;
	menucombo_t *dlightcombo;
} lightingmenuinfo_t;

qboolean M_VideoApplyShadowLighting (union menuoption_s *op,struct emenu_s *menu,int key)
{
	lightingmenuinfo_t *info = (lightingmenuinfo_t*)menu->data;

	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

#ifdef RTLIGHTS
	{
		char *cvarsrw = "0";
		char *cvarsrws = "0";
		char *cvarv = "0";
		switch (info->lightcombo->selectedoption)
		{
		case 1:
			cvarsrw = "1";
			break;
		case 2:
			cvarsrw = "1";
			cvarsrws = "1";
			break;
		case 3:
			cvarv = "1";
			break;
		}
#ifdef MINIMAL
		(void)cvarv;
		Cbuf_AddText(va("r_shadow_realtime_world %s;r_shadow_realtime_world_shadows %s\n", cvarsrw, cvarsrws), RESTRICT_LOCAL);
#else
		Cbuf_AddText(va("r_vertexlight %s;r_shadow_realtime_world %s;r_shadow_realtime_world_shadows %s\n", cvarv, cvarsrw, cvarsrws), RESTRICT_LOCAL);
#endif
	}
#endif

	{
		char *cvard = "0";
		char *cvarvd = "0";
		char *cvarsrd = "0";
		char *cvarsrds = "0";
		switch (info->dlightcombo->selectedoption)
		{
		case 1:
			cvard = "1";
			break;
		case 2:
			cvarsrd = "1";
			break;
		case 3:
			cvarsrd = "1";
			cvarsrds = "1";
			break;
		case 4:
			cvard = "1";
			cvarvd = "1";
			break;
		}
#ifdef RTLIGHTS
#ifdef MINIMAL
		Cbuf_AddText(va("r_shadow_realtime_dlight %s;r_shadow_realtime_dlight_shadows %s;r_dynamic %s\n", cvarsrd, cvarsrds, cvard), RESTRICT_LOCAL);
#else
		Cbuf_AddText(va("r_shadow_realtime_dlight %s;r_shadow_realtime_dlight_shadows %s;r_dynamic %s;r_vertexdlight %s\n", cvarsrd, cvarsrds, cvard, cvarvd), RESTRICT_LOCAL);
#endif
#else
#ifdef MINIMAL
		Cbuf_AddText(va("r_dynamic %s\n", cvard), RESTRICT_LOCAL);
#else
		Cbuf_AddText(va("r_dynamic %s;r_vertexdlight %s\n", cvard, cvarvd), RESTRICT_LOCAL);
#endif
#endif
		(void)cvarsrd, (void)cvarsrds, (void)cvard, (void)cvarvd;
	}

	Cbuf_AddText("vid_restart\n", RESTRICT_LOCAL);

	M_RemoveMenu(menu);
	Cbuf_AddText("menu_lighting\n", RESTRICT_LOCAL);
	return true;
}

void M_Menu_Lighting_f (void)
{
#ifndef MINIMAL
	extern cvar_t r_vertexlight;
#endif
	extern cvar_t r_stains, r_shadows, r_loadlits, r_lightmap_format;
	extern cvar_t r_lightstylesmooth, r_nolightdir;
#ifdef RTLIGHTS
	extern cvar_t r_dynamic, r_shadow_realtime_world, r_shadow_realtime_dlight, r_shadow_realtime_dlight_shadows;
#ifndef MINIMAL
	extern cvar_t r_vertexdlights;
#endif
#endif
	extern cvar_t r_fb_models, r_rocketlight, r_powerupglow;
	extern cvar_t v_powerupshell, r_explosionlight;
	//extern cvar_t r_fb_bmodels, r_shadow_realtime_world_lightmaps, r_lightstylespeed;

	static const char *lightingopts[] =
	{
		"Standard",
#ifdef RTLIGHTS
		"Realtime",
		"RT+Shadows",
#ifndef MINIMAL
		"Vertex",
#endif
#endif
		NULL
	};
	static const char *dlightopts[] =
	{
		"None",
		"Standard",
#ifdef RTLIGHTS
		"Realtime",
		"RT+Shadows",
#ifndef MINIMAL
		"Vertex",
#endif
#endif
		NULL
	};

	static const char *loaddeluxeopts[] =
	{
		"Disabled",
		"Enabled",
		"Generate",
		NULL
	};

	static const char *loadlitopts[] =
	{
		"Disabled",
		"Enabled",
		"Generate LDR",
		"Generate HDR",
		NULL
	};

	static const char *fbopts[] =
	{
		"Disabled",
		"Enabled",
		"Traced",
		NULL
	};
	static const char *fbvalues[] =
	{
		"0",
		"1",
		"2",
		NULL
	};

	static const char *powerupopts[] =
	{
		"Disabled",
		"Enabled",
		"Non-Self",
		NULL
	};
	static const char *powerupvalues[] =
	{
		"0",
		"1",
		"2",
		NULL
	};

	static const char *fb_models_opts[] =
	{
		"Disabled",
		"Entire model",
		"If textured",
		NULL
	};
	static const char *fb_models_values[] =
	{
		"0",
		"1",
		"2",
		NULL
	};

	static const char *lightmapformatopts[] =
	{
		"Automatic",
		"4bit",
		"5bit (5551)",
		"5bit (565)",
		"8bit (Greyscale)",
		"8bit (Misaligned)",
		"8bit (Aligned)",
		"10bit",
		"9bit (HDR)",
		"Half-Float (HDR)",
		"Float (HDR)",
		NULL
	};
	static const char *lightmapformatvalues[] =
	{
		"",
		"rgba4",
		"rgb5a1",
		"rgb565",
		"l8",
		"rgb8",
		"bgrx8",
		"rgb10",
		"rgb9e5",
		"rgba16f",
		"rgba32f",
		NULL
	};

	int y;
	emenu_t *menu = M_Options_Title(&y, sizeof(lightingmenuinfo_t));

	int lightselect, dlightselect;

#ifdef RTLIGHTS
	if (r_shadow_realtime_world.ival)
	{
		if (r_shadow_realtime_world_shadows.ival)
			lightselect = 2;
		else
			lightselect = 1;
	}
	else
#endif
#ifndef MINIMAL
	if (r_vertexlight.ival)
		lightselect = 3;
	else
#endif
		lightselect = 0;

#ifdef RTLIGHTS
	if (r_shadow_realtime_dlight.ival)
	{
		if (r_shadow_realtime_dlight_shadows.ival)
			dlightselect = 3;
		else
			dlightselect = 2;
	}
#ifndef MINIMAL
	else if (r_vertexdlights.ival)
		dlightselect = 4;
#endif
	else
#endif
	if (r_dynamic.ival > 0)
		dlightselect = 1;
	else
		dlightselect = 0;

	{
		lightingmenuinfo_t *info = menu->data;
		menubulk_t bulk[] =
		{
			MB_REDTEXT("Lighting Options", true),
			MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
			MB_COMBORETURN("Lighting Mode", lightingopts, lightselect, info->lightcombo, "Selects method used for world lighting. Realtime lighting requires appropriate realtime lighting files for maps."),
			MB_COMBORETURN("Dynamic Lighting Mode", dlightopts, dlightselect, info->dlightcombo, "Selects method used for dynamic lighting such as explosion lights and muzzle flashes."),
#ifdef RTLIGHTS
#ifdef VKQUAKE
			MB_CHECKBOXCVARTIP("Raytrace Shadows", r_shadow_raytrace, 0, "Enables raytraced shadows when supported by hardware+drivers. Consider combining with half-rate shading."),
#endif
			MB_CHECKBOXCVARTIP("Soft Shadows", r_shadow_shadowmapping, 0, "Enables softer shadows instead of course-edged pixelated shadows."),
			MB_CMD("Apply Lighting", M_VideoApplyShadowLighting, "Applies set lighting modes and restarts video."),
			MB_SPACING(4),
#endif
			MB_COMBOCVAR("Lightmap Format", r_lightmap_format, lightmapformatopts, lightmapformatvalues, "Selects which format to use for lightmaps."),
			MB_COMBOCVAR("LIT Loading", r_loadlits, loadlitopts, NULL, "Determines if the engine should use external colored lighting for maps. The generated setting will cause the engine to generate colored lighting for maps that don't have the associated data."),
			MB_COMBOCVAR("Deluxemapping", r_deluxemapping_cvar, loaddeluxeopts, NULL, "Controls whether static lighting should respond to lighting directions."),
			MB_CHECKBOXCVAR("Lightstyle Lerp", r_lightstylesmooth, 0),
			MB_SPACING(4),
			MB_COMBOCVAR("Flash Blend", r_flashblend, fbopts, fbvalues, "Disables or enables the spherical light effect for dynamic lights. Traced means the sphere effect will be line of sight checked before displaying the effect."),
			MB_SLIDER("Explosion Light", r_explosionlight, 0, 1, 0.1, NULL),
			MB_SLIDER("Rocket Light", r_rocketlight, 0, 1, 0.1, NULL),
			MB_COMBOCVAR("Powerup Glow", r_powerupglow, powerupopts, powerupvalues, "Disables or enables the dynamic light effect for powerups. Non-self will disable the light only for the current player."),
			MB_CHECKBOXCVAR("Powerup Shell", v_powerupshell, 0),
			MB_SPACING(4),
			MB_SLIDER("Blob Shadows", r_shadows, 0, 1, 0.05, "Small blobs underneath monsters and players, to add depth to the scene without excessive rendering."),
			MB_SLIDER("Stains", r_stains, 0, 1, 0.05, "Allows discolouration of world surfaces, commonly used for blood trails."),
			MB_CHECKBOXCVARTIP("No Light Direction", r_nolightdir, 0, "Disables shading calculations for uniform light levels on models from all directions."),
			MB_COMBOCVAR("Model Fullbrights", r_fb_models, fb_models_opts, fb_models_values, "Affects loading of fullbrights on models/polymeshes."),
			MB_END()
		};
		static menuresel_t resel;
		MC_AddFrameStart(menu, y);
		MC_AddBulk(menu, &resel, bulk, 16, 216, y);
		MC_AddFrameEnd(menu, y);
	}
}


typedef struct {
menucombo_t *skillcombo;
menucombo_t *mapcombo;
} singleplayerinfo_t;

#ifdef HAVE_SERVER
static const char *maplist_q1[] =
{
	"start",
	"e1m1",
	"e1m2",
	"e1m3",
	"e1m4",
	"e1m5",
	"e1m6",
	"e1m7",
	"e1m8",
	"e2m1",
	"e2m2",
	"e2m3",
	"e2m4",
	"e2m5",
	"e2m6",
	"e2m7",
	"e3m1",
	"e3m2",
	"e3m3",
	"e3m4",
	"e3m5",
	"e3m6",
	"e3m7",
	"e4m1",
	"e4m2",
	"e4m3",
	"e4m4",
	"e4m5",
	"e4m6",
	"e4m7",
	"e4m8",
	"end"
};
static const char *mapoptions_q1[] =
{
	"Start (Introduction)",
	"E1M1 (The Slipgate Complex)",
	"E1M2 (Castle Of The Damned)",
	"E1M3 (The Necropolis)",
	"E1M4 (The Grisly Grotto)",
	"E1M5 (Gloom Keep)",
	"E1M6 (The Door To Chthon)",
	"E1M7 (The House Of Chthon)",
	"E1M8 (Ziggarat Vertigo)",
	"E2M1 (The Installation)",
	"E2M2 (The Ogre Citadel)",
	"E2M3 (The Crypt Of Decay)",
	"E2M4 (The Ebon Fortress)",
	"E2M5 (The Wizard's Manse)",
	"E2M6 (The Dismal Oubliette",
	"E2M7 (The Underearth)",
	"E3M1 (Termination Central)",
	"E3M2 (The Vaults Of Zin)",
	"E3M3 (The Tomb Of Terror)",
	"E3M4 (Satan's Dark Delight)",
	"E3M5 (The Wind Tunnels)",
	"E3M6 (Chambers Of Torment)",
	"E3M7 (Tha Haunted Halls)",
	"E4M1 (The Sewage System)",
	"E4M2 (The Tower Of Despair)",
	"E4M3 (The Elder God Shrine)",
	"E4M4 (The Palace Of Hate)",
	"E4M5 (Hell's Atrium)",
	"E4M6 (The Pain Maze)",
	"E4M7 (Azure Agony)",
	"E4M8 (The Nameless City)",
	"End (Shub-Niggurath's Pit)",
	NULL
};

#if defined(Q2CLIENT) && defined(Q2SERVER)
static const char *maplist_q2[] =
{
	"base1",
	"base2",
	"base3",
	"train",
	"bunk1",
	"ware1",
	"ware2",
	"jail1",
	"jail2",
	"jail3",
	"jail4",
	"jail5",
	"security",
	"mintro",
	"mine1",
	"mine2",
	"mine3",
	"mine4",
	"fact1",
	"fact3",
	"fact2",
	"power1",
	"power2",
	"cool1",
	"waste1",
	"waste2",
	"waste3",
	"biggun",
	"hangar1",
	"space",
	"lab",
	"hangar2",
	"command",
	"strike",
	"city1",
	"city2",
	"city3",
	"boss1",
	"boss2",
};
static const char *mapoptions_q2[] =
{
	"base1 (Unit 1 Base Unit: Outer Base)",
	"base2 (Unit 1 Base Unit: Installation)",
	"base3 (Unit 1 Base Unit: Comm Center)",
	"train (Unit 1 Base Unit: Lost Station)",
	"bunk1 (Unit 2 Warehouse Unit: Ammo Depot)",
	"ware1 (Unit 2 Warehouse Unit: Supply Station)",
	"ware2 (Unit 2 Warehouse Unit: Warehouse)",
	"jail1 (Unit 3 Jail Unit: Main Gate)",
	"jail2 (Unit 3 Jail Unit: Destination Center)",
	"jail3 (Unit 3 Jail Unit: Security Compex)",
	"jail4 (Unit 3 Jail Unit: Torture Chambers)",
	"jail5 (Unit 3 Jail Unit: Guard House)",
	"security (Unit 3 Jail Unit: Grid Control)",
	"mintro (Unit 4 Mine Unit: Mine Entrance)",
	"mine1 (Unit 4 Mine Unit: Upper Mines)",
	"mine2 (Unit 4 Mine Unit: Borehole)",
	"mine3 (Unit 4 Mine Unit: Drilling Area)",
	"mine4 (Unit 4 Mine Unit: Lower Mines)",
	"fact1 (Unit 5 Factory Unit: Receiving Center)",
	"fact3 (Unit 5 Factory Unit: Sudden Death)",
	"fact2 (Unit 5 Factory Unit: Processing Plant)",
	"power1 (Unit 6 Power Unit/Big Gun: Power Plant)",
	"power2 (Unit 6 Power Unit/Big Gun: The Reactor)",
	"cool1 (Unit 6 Power Unit/Big Gun: Cooling Facility)",
	"waste1 (Unit 6 Power Unit/Big Gun: Toxic Waste Dump)",
	"waste2 (Unit 6 Power Unit/Big Gun: Pumping Station 1)",
	"waste3 (Unit 6 Power Unit/Big Gun: Pumping Station 2)",
	"biggun (Unit 6 Power Unit/Big Gun: Big Gun)",
	"hangar1 (Unit 7 Hangar Unit: Outer Hangar)",
	"space (Unit 7 Hangar Unit: Comm Satelite)",
	"lab (Unit 7 Hangar Unit: Research Lab)",
	"hangar2 (Unit 7 Hangar Unit: Inner Hangar)",
	"command (Unit 7 Hangar Unit: Launch Command)",
	"strike (Unit 7 Hangar Unit: Outlands)",
	"city1 (Unit 8 City Unit: Outer Courts)",
	"city2 (Unit 8 City Unit: Lower Palace)",
	"city3 (Unit 8 City Unit: Upper Palace)",
	"boss1 (Unit 9 Boss Levels: Inner Chamber)",
	"boss2 (Unit 9 Boss Levels: Final Showdown)",
	NULL
};
#endif
#endif

qboolean M_Apply_SP_Cheats (union menuoption_s *op,struct emenu_s *menu,int key)
{
	singleplayerinfo_t *info = menu->data;

	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

	switch(info->skillcombo->selectedoption)
	{
	case 0:
		Cbuf_AddText("skill 0\n", RESTRICT_LOCAL);
		break;
	case 1:
		Cbuf_AddText("skill 1\n", RESTRICT_LOCAL);
		break;
	case 2:
		Cbuf_AddText("skill 2\n", RESTRICT_LOCAL);
		break;
	case 3:
		Cbuf_AddText("skill 3\n", RESTRICT_LOCAL);
		break;
	}

#ifdef HAVE_SERVER
	if ((unsigned int)info->mapcombo->selectedoption < countof(maplist_q1)-1)
		Cbuf_AddText(va("map %s\n", maplist_q1[info->mapcombo->selectedoption]), RESTRICT_LOCAL);
#endif

	M_RemoveMenu(menu);
	Cbuf_AddText("menu_spcheats\n", RESTRICT_LOCAL);
	return true;
}


void M_Menu_Singleplayer_Cheats_Quake (void)
{
	#ifdef HAVE_SERVER
	static const char *skilloptions[] =
	{
		"Easy",
		"Normal",
		"Hard",
		"Nightmare",
		"None Set",
		NULL
	};
	int currentskill;
	int currentmap;
	extern cvar_t sv_gravity, sv_cheats, sv_maxspeed, skill;
	#endif
	singleplayerinfo_t *info;
	int cursorpositionY;
	int y;
	emenu_t *menu = M_Options_Title(&y, sizeof(*info));
	info = menu->data;

	cursorpositionY = (y + 24);

	#ifdef HAVE_SERVER
	if ( !*skill.string )
		currentskill = 4; // no skill selected
	else
		currentskill = skill.value;

	for (currentmap = countof(maplist_q1); currentmap --> 0; )
		if (!strcmp(host_mapname.string, maplist_q1[currentmap]))
			break;
	/*anything that doesn't match will end up with 0*/
	#endif

	MC_AddRedText(menu, 16, 170, y, 			"     Quake Singleplayer Cheats", false); y+=8;
	MC_AddWhiteText(menu, 16, 170, y,		"     ^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082 ", false); y+=8;
	y+=8;
	#ifdef HAVE_SERVER
	info->skillcombo = MC_AddCombo(menu,16,170, y,	"Difficulty", skilloptions, currentskill);	y+=8;
	info->mapcombo = MC_AddCombo(menu,16,170, y,	"Map", mapoptions_q1, currentmap);	y+=8;
	MC_AddCheckBox(menu,	16, 170, y,		"Cheats", &sv_cheats,0);	y+=8;
	#endif
	#ifdef TEXTEDITOR
	MC_AddCheckBox(menu,	16, 170, y,		"Debugger", &pr_debugger, 0); y+=8;
	#endif
	MC_AddConsoleCommand(menu, 16, 170, y,	"     Toggle Godmode", "god\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"     Toggle Flymode", "fly\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"      Toggle Noclip", "noclip\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"        Quad Damage", "impulse 255\n"); y+=8;
	#ifdef HAVE_SERVER
	MC_AddSlider(menu,	16, 170, y,			"Gravity", &sv_gravity,0,800,25);	y+=8;
	#endif
	MC_AddSlider(menu,	16, 170, y,			"Forward Speed", &cl_forwardspeed,0,1000,50);	y+=8;
	MC_AddSlider(menu,	16, 170, y,			"Side Speed", &cl_sidespeed,0,1000,50);	y+=8;
	MC_AddSlider(menu,	16, 170, y,			"Back Speed", &cl_backspeed,0,1000,50);	y+=8;
	#ifdef HAVE_SERVER
	MC_AddSlider(menu,	16, 170, y,			"Max Movement Speed", &sv_maxspeed,0,1000,50);	y+=8;
	#endif
	MC_AddConsoleCommand(menu, 16, 170, y,	" Silver & Gold Keys", "impulse 13\nimpulse 14\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"All Weapons & Items", "impulse 9\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"No Enemy Targetting", "notarget\n"); y+=8;
	#ifdef HAVE_SERVER
	MC_AddConsoleCommand(menu, 16, 170, y,   "Restart Map", "restart\n"); y+=8;
	#else
	MC_AddConsoleCommand(menu, 16, 170, y,   "Suicide", "kill\n"); y+=8;
	#endif

	y+=8;
	MC_AddCommand(menu,	16, 170, y,			"Apply Changes", M_Apply_SP_Cheats);	y+=8;

	menu->selecteditem = (union menuoption_s *)info->skillcombo;
	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 170, 0, cursorpositionY, NULL, false);
}

#ifdef Q2SERVER
// Quake 2

typedef struct {
menucombo_t *skillcombo;
menucombo_t *mapcombo;
} singleplayerq2info_t;

qboolean M_Apply_SP_Cheats_Q2 (union menuoption_s *op,struct emenu_s *menu,int key)
{
	singleplayerq2info_t *info = menu->data;

	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

	switch(info->skillcombo->selectedoption)
	{
	case 0:
		Cbuf_AddText("skill 0\n", RESTRICT_LOCAL);
		break;
	case 1:
		Cbuf_AddText("skill 1\n", RESTRICT_LOCAL);
		break;
	case 2:
		Cbuf_AddText("skill 2\n", RESTRICT_LOCAL);
		break;
	}

	if ((unsigned int)info->mapcombo->selectedoption < countof(maplist_q2)-1)
		Cbuf_AddText(va("map %s\n", maplist_q2[info->mapcombo->selectedoption]), RESTRICT_LOCAL);

	M_RemoveMenu(menu);
	Cbuf_AddText("menu_spcheats\n", RESTRICT_LOCAL);
	return true;
}

void M_Menu_Singleplayer_Cheats_Quake2 (void)
{

	static const char *skilloptions[] =
	{
		"Easy",
		"Normal",
		"Hard",
		"None Set",
		NULL
	};

	singleplayerq2info_t *info;
	int cursorpositionY;
	#ifdef HAVE_SERVER
	int currentskill;
	int currentmap;
	extern cvar_t sv_gravity, sv_cheats, sv_maxspeed, skill;
	#endif
	int y;
	emenu_t *menu = M_Options_Title(&y, sizeof(*info));
	info = menu->data;

	cursorpositionY = (y + 24);

	#ifdef HAVE_SERVER
	if ( !*skill.string )
		currentskill = 3; // no skill selected
	else
		currentskill = skill.value;

	for (currentmap = countof(maplist_q2); currentmap --> 0; )
		if (!Q_strcasecmp(host_mapname.string, maplist_q2[currentmap]))
			break;
	/*anything that doesn't match will end up with 0*/
	#endif

	MC_AddRedText(menu, 16, 170, y, 		"Quake2 Singleplayer Cheats", false); y+=8;
	MC_AddWhiteText(menu, 16, 170, y,		"^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", false); y+=8;
	y+=8;
	#ifdef HAVE_SERVER
	info->skillcombo = MC_AddCombo(menu,16,170, y,	"Difficulty", skilloptions, currentskill);	y+=8;
	info->mapcombo = MC_AddCombo(menu,16,170, y,	"Map", mapoptions_q2, currentmap);	y+=8;
	MC_AddCheckBox(menu,	16, 170, y,		"Cheats", &sv_cheats,0);	y+=8;
	#endif
	MC_AddConsoleCommand(menu, 16, 170, y,	"Toggle Godmode", "god\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Toggle Noclip", "noclip\n"); y+=8;
	#ifdef HAVE_SERVER
	MC_AddSlider(menu,	16, 170, y,			"Gravity", &sv_gravity,0,850,25);	y+=8;
	#endif
	MC_AddSlider(menu,	16, 170, y,			"Forward Speed", &cl_forwardspeed,0,1000,50);	y+=8;
	MC_AddSlider(menu,	16, 170, y,			"Side Speed", &cl_sidespeed,0,1000,50);	y+=8;
	MC_AddSlider(menu,	16, 170, y,			"Back Speed", &cl_backspeed,0,1000,50);	y+=8;
	#ifdef HAVE_SERVER
	MC_AddSlider(menu,	16, 170, y,			"Max Movement Speed", &sv_maxspeed,0,1000,50);	y+=8;
	#endif
	MC_AddConsoleCommand(menu, 16, 170, y,	"Unlimited Ammo", "dmflags 8192\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Quad Damage", "give quad damage\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Blue & Red Key", "give blue key\ngive red key\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Pyramid Key", "give pyramid key\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"All Weapons & Items", "give all\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Data Spinner", "give data spinner\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Power Cube", "give power cube\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Data CD", "give data cd\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Ammo Pack", "give ammo pack\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Bandolier", "give bandolier\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Adrenaline", "give adrenaline\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Ancient Head", "give ancient head\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Environment Suit", "give environment suit\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Rebreather", "give rebreather\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Invulnerability", "give invulnerability\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Silencer", "give silencer\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Power Shield", "give power shield\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Commander's Head", "give commander's head\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Security Pass", "give security pass\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Airstrike Marker", "give airstrike marker\n"); y+=8;
	#ifdef HAVE_SERVER
	MC_AddConsoleCommand(menu, 16, 170, y,   "Restart Map", va("restart\n")); y+=8;
	#endif

	y+=8;
	MC_AddCommand(menu,	16, 170, y,			"Apply Changes", M_Apply_SP_Cheats_Q2);	y+=8;

	menu->selecteditem = (union menuoption_s *)info->skillcombo;
	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 170, 0, cursorpositionY, NULL, false);
}
#endif	// Quake 2

#ifdef HEXEN2 // Hexen 2
typedef struct {
menucombo_t *skillcombo;
menucombo_t *mapcombo;
} singleplayerh2info_t;

#ifdef HAVE_SERVER
static const char *maplist_h2[] =
{
	"demo1",
	"demo2",
	"demo3",
	"village1",
	"village3",
	"village2",
	"village4",
	"village5",
	"rider1a",
	"meso1",
	"meso2",
	"meso3",
	"meso4",
	"meso5",
	"meso6",
	"meso8",
	"meso9",
	"egypt1",
	"egypt2",
	"egypt3",
	"egypt4",
	"egypt5",
	"egypt6",
	"egypt7",
	"rider2c",
	"romeric1",
	"romeric2",
	"romeric3",
	"romeric4",
	"romeric5",
	"romeric6",
	"romeric7",
	"castle4",
	"castle5",
	"cath",
	"tower",
	"eidolon",
};
static const char *mapoptions_h2[] =
{
	"demo1 (Blackmarsh: Hub 1 Blackmarsh)",
	"demo2 (Barbican: Hub 1 Blackmarsh)",
	"demo3 (The Mill: Hub 1 Blackmarsh)",
	"village1 (King's Court: Hub 1 Blackmarsh)",
	"village3 (Stables: Hub 1 Blackmarsh)",
	"village2 (Inner Courtyard: Hub 1 Blackmarsh)",
	"village4 (Palance Entrance: Hub 1 Blackmarsh)",
	"village5 (The Forgotten Chapel: Hub 1 Blackmarsh)",
	"rider1a (Famine's Domain: Hub 1 Blackmarsh)",
	"meso1 (Palance of Columns: Hub 2 Mazaera)",
	"meso2 (Plaza of the Sun: Hub 2 Mazaera)",
	"meso3 (Square of the Stream: Hub 2 Mazaera)",
	"meso4 (Tomb of the High Priest: Hub 2 Mazaera)",
	"meso5 (Obelisk of the Moon: Hub 2 Mazaera)",
	"meso6 (Court of 1000 Warriors: Hub 2 Mazaera)",
	"meso8 (Bridge of Stars: Hub 2 Mazaera)",
	"meso9 (Well of Souls: Hub 2 Mazaera)",
	"egypt1 (Temple of Horus: Hub 3 Thysis)",
	"egypt2 (Ancient Tempor of Nefertum: Hub 3 Thysis)",
	"egypt3 (Tempor of Nefertum: Hub 3 Thysis)",
	"egypt4 (Palace of the Pharaoh: Hub 3 Thysis",
	"egypt5 (Pyramid of Anubus: Hub 3 Thysis)",
	"egypt6 (Temple of Light: Hub 3 Thysis)",
	"egypt7 (Shrine of Naos: Hub 3 Thysis)",
	"rider2c (Pestilence's Lair: Hub 3 Thysis)",
	"romeric1 (The Hall of Heroes: Hub 4 Septimus)",
	"romeric2 (Gardens of Athena: Hub 4 Septimus)",
	"romeric3 (Forum of Zeus: Hub 4 Septimus)",
	"romeric4 (Baths of Demetrius: Hub 4 Septimus)",
	"romeric5 (Temple of Mars: Hub 4 Septimus)",
	"romeric6 (Coliseum of War: Hub 4 Septimus)",
	"romeric7 (Reflecting Pool: Hub 4 Septimus)",
	"castle4 (The Underhalls: Hub 5 Return to Blackmarsh)",
	"castle5 (Eidolon's Ordeal: Hub 5 Return to Blackmarsh)",
	"cath (Cathedral: Hub 5 Return to Blackmarsh)",
	"tower (Tower of the Dark Mage: Hub 5 Return to Blackmarsh)",
	"eidolon (Eidolon's Lair: Hub 5 Return to Blackmarsh)",
	NULL
};
#endif

qboolean M_Apply_SP_Cheats_H2 (union menuoption_s *op,struct emenu_s *menu,int key)
{
#ifdef HAVE_SERVER
	singleplayerh2info_t *info = menu->data;
#endif

	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

#ifdef HAVE_SERVER
	switch(info->skillcombo->selectedoption)
	{
	case 0:
		Cbuf_AddText("skill 0\n", RESTRICT_LOCAL);
		break;
	case 1:
		Cbuf_AddText("skill 1\n", RESTRICT_LOCAL);
		break;
	case 2:
		Cbuf_AddText("skill 2\n", RESTRICT_LOCAL);
		break;
	case 3:
		Cbuf_AddText("skill 3\n", RESTRICT_LOCAL);
		break;
	}

	if ((unsigned)info->mapcombo->selectedoption < countof(maplist_h2))
		Cbuf_AddText(va("map %s\n", maplist_h2[info->mapcombo->selectedoption]), RESTRICT_LOCAL);
#endif

	M_RemoveMenu(menu);
	Cbuf_AddText("menu_spcheats\n", RESTRICT_LOCAL);
	return true;
}


void M_Menu_Singleplayer_Cheats_Hexen2 (void)
{
	singleplayerh2info_t *info;
	int cursorpositionY;
	#ifdef HAVE_SERVER
		int currentmap;
		int currentskill;
		static const char *skilloptions[] =
		{
			"Easy",
			"Normal",
			"Hard",
			"Nightmare",
			"None Set",
			NULL
		};
		extern cvar_t sv_gravity, sv_cheats, sv_maxspeed, skill;
	#endif
	int y;
	emenu_t *menu = M_Options_Title(&y, sizeof(*info));
	info = menu->data;

	cursorpositionY = (y + 24);

	#ifdef HAVE_SERVER
	if ( !*skill.string )
		currentskill = 4; // no skill selected
	else
		currentskill = skill.value;

	for (currentmap = countof(maplist_h2); currentmap --> 0; )
		if (!Q_strcasecmp(host_mapname.string, maplist_h2[currentmap]))
			break;
	#endif

	MC_AddRedText(menu, 16, 170, y, 		"Hexen2 Singleplayer Cheats", false); y+=8;
	MC_AddWhiteText(menu, 16, 170, y,		"^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082 ", false); y+=8;
	y+=8;
	#ifdef HAVE_SERVER
	info->skillcombo = MC_AddCombo(menu,16,170, y,	"Difficulty", skilloptions, currentskill);	y+=8;
	info->mapcombo = MC_AddCombo(menu,16,170, y,	"Map", mapoptions_h2, currentmap);	y+=8;
	#endif
	#ifdef HAVE_SERVER
	MC_AddCheckBox(menu,	16, 170, y,		"Cheats", &sv_cheats,0);	y+=8;
	#endif
	MC_AddConsoleCommand(menu, 16, 170, y,	"Toggle Godmode", "god\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Toggle Flymode", "fly\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Toggle Noclip", "noclip\n"); y+=8;
	#ifdef HAVE_SERVER
	MC_AddSlider(menu,	16, 170, y,			"Gravity", &sv_gravity,0,800,25);	y+=8;
	#endif
	MC_AddSlider(menu,	16, 170, y,			"Forward Speed", &cl_forwardspeed,0,1000,50);	y+=8;
	MC_AddSlider(menu,	16, 170, y,			"Side Speed", &cl_sidespeed,0,1000,50);	y+=8;
	MC_AddSlider(menu,	16, 170, y,			"Back Speed", &cl_backspeed,0,1000,50);	y+=8;
	#ifdef HAVE_SERVER
	MC_AddSlider(menu,	16, 170, y,			"Max Movement Speed", &sv_maxspeed,0,1000,50);	y+=8;
	#endif
	MC_AddConsoleCommand(menu, 16, 170, y,	"Sheep Transformation", "impulse 14\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Change To Paladin (lvl3+)", "impulse 171\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Change To Crusader (lvl3+)", "impulse 172\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Change to Necromancer (lvl3+)", "impulse 173\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Change to Assassin (lvl3+)", "impulse 174\n"); y+=8;
	//demoness?
	MC_AddConsoleCommand(menu, 16, 170, y,	"Remove Monsters", "impulse 35\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Freeze Monsters", "impulse 36\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Unfreeze Monsters", "impulse 37\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Increase Level By 1", "impulse 40\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Increase Experience", "impulse 41\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Display Co-ordinates", "impulse 42\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"All Weapons & Mana", "impulse 9\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"All Weapons & Mana & Items", "impulse 43\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"No Enemy Targetting", "notarget\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"Enable Crosshair", "crosshair 1\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,	"20 Of Each Artifact", "impulse 299\n"); y+=8;
	MC_AddConsoleCommand(menu, 16, 170, y,  "Restart Map", "impulse 99\n"); y+=8;

	y+=8;
	MC_AddCommand(menu,	16, 170, y,			"Apply Changes", M_Apply_SP_Cheats_H2);	y+=8;

	menu->selecteditem = (union menuoption_s *)info->skillcombo;
	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 250, 0, cursorpositionY, NULL, false);
}
#endif

void M_Menu_Singleplayer_Cheats_f (void)
{
	switch(M_GameType())
	{
	case MGT_QUAKE1:
		M_Menu_Singleplayer_Cheats_Quake();
		break;
#ifdef Q2SERVER
	case MGT_QUAKE2:
		M_Menu_Singleplayer_Cheats_Quake2();
		break;
#endif
#ifdef HEXEN2
	case MGT_HEXEN2:
		M_Menu_Singleplayer_Cheats_Hexen2();
		break;
#endif
	}
}

// video mode options
#if defined(D3DQUAKE) && defined(GLQUAKE)
#define MULTIRENDERER // allow options for selecting renderer
#endif

typedef struct {
	menucombo_t *dispmode;
	menucombo_t *resmode;
	menuedit_t *width;
	menuedit_t *height;
	menuedit_t *bpp;
	menuedit_t *hz;
	menucombo_t *bppfixed;
	menucombo_t *hzfixed;
	menucombo_t *res2dmode;
	menucombo_t *scale;
	menuedit_t *width2d;
	menuedit_t *height2d;
	menucombo_t *ressize[ASPECT_RATIOS];
	menucombo_t *res2dsize[ASPECT_RATIOS];
} videomenuinfo_t;

void CheckCustomMode(struct emenu_s *menu)
{
	int i, sel;
	videomenuinfo_t *info = (videomenuinfo_t*)menu->data;

	// hide all display controls
	info->resmode->common.ishidden = true;
	info->width->common.ishidden = true;
	info->height->common.ishidden = true;
	info->bpp->common.ishidden = true;
	info->hz->common.ishidden = true;
	info->bppfixed->common.ishidden = true;
	info->hzfixed->common.ishidden = true;
	for (i = 0; i < ASPECT_RATIOS; i++)
		info->ressize[i]->common.ishidden = true;
	if (!info->dispmode || info->dispmode->selectedoption != 2)
	{
		info->resmode->common.ishidden = false;
		sel = info->resmode->selectedoption;
		if (sel == ASPECT3D_CUSTOM)
		{ // unhide custom entries for custom option
			info->width->common.ishidden = false;
			info->height->common.ishidden = false;
			info->bpp->common.ishidden = false;
			info->hz->common.ishidden = false;
		}
		else if (sel != ASPECT3D_DESKTOP)
		{
			// unhide appropriate aspect ratio combo and restricted bpp/hz combos
			info->bppfixed->common.ishidden = false;
			info->hzfixed->common.ishidden = false;
			info->ressize[sel]->common.ishidden = false;
		}
	}
	// hide all 2d display controls
	info->width2d->common.ishidden = true;
	info->height2d->common.ishidden = true;
	info->scale->common.ishidden = true;
	for (i = 0; i < ASPECT_RATIOS; i++)
		info->res2dsize[i]->common.ishidden = true;
	sel = info->res2dmode->selectedoption;
	if (sel == ASPECT2D_SCALED) // unhide scale option
		info->scale->common.ishidden = false;
	else if (sel == ASPECT2D_HEIGHT) // unhide custom entries for height-only option
		info->height2d->common.ishidden = false;
	else if (sel == ASPECT2D_CUSTOM) // unhide custom entries for custom option
	{
		info->width2d->common.ishidden = false;
		info->height2d->common.ishidden = false;
	}
	else if (sel != ASPECT2D_AUTO) // unhide appropriate aspect ratio combo
		info->res2dsize[sel]->common.ishidden = false;
}

//return value is aspect group, *outres is the mode index inside that aspect.
static int M_MatchModes(int width, int height, int *outres)
{
	int i;
	int ratio = -1;

	// find closest resolution for each ratio
	for (i = 0; i < ASPECT_RATIOS; i++)
	{
		const char **v = resaspects[i];
		outres[i] = 0;
		// search through each string in ratio array
		while (*v)
		{
			const char *c = *v;
			int w = atoi(c);
			if (width <= w)
			{
				if (width == w)
				{
					// if we match height as well we have a direct resolution match
					// so record ratio index
					const char *s = strchr(c, 'x');
					if (s)
					{
						int h = atoi(s + 1);
						if (height == h)
							ratio = i;
					}
				}
				break;
			}
			outres[i]++;
			v++;
		}
	}

	return ratio;
}

qboolean M_VideoApply (union menuoption_s *op, struct emenu_s *menu, int key)
{
	extern cvar_t vid_desktopsettings;
	videomenuinfo_t *info = (videomenuinfo_t*)menu->data;

	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

	// force update display options
	{
		int w = 0, h = 0;
		const char *wc = NULL;
		const char *hc = NULL;
		const char *bc = "32";
		const char *fc = "0";
		const char *dc = "0";

		switch (info->resmode->selectedoption)
		{
		case ASPECT3D_DESKTOP: // Desktop
			dc = "1";
			break;
		case ASPECT3D_CUSTOM: // Custom
			wc = info->width->text;
			hc = info->height->text;
			bc = info->bpp->text;
			fc = info->hz->text;
			break;
		default: // Aspects
			{
				menucombo_t *c = info->ressize[info->resmode->selectedoption];
				const char *res = c->options[c->selectedoption];
				const char *x = strchr(res, 'x');

				w = atoi(res);
				h = atoi(x + 1);

				bc = info->bppfixed->values[info->bppfixed->selectedoption];
				fc = info->hzfixed->values[info->hzfixed->selectedoption];
			}
		}

		if (!wc)
			Cvar_SetValue(info->width->cvar, w);
		else
			Cvar_Set(info->width->cvar, wc);
		if (!hc)
			Cvar_SetValue(info->height->cvar, h);
		else
			Cvar_Set(info->height->cvar, hc);
		Cvar_Set(info->bpp->cvar, bc);
		Cvar_Set(info->hz->cvar, fc);
		Cvar_Set(&vid_desktopsettings, dc);
	}

	// force update 2d options
	{
		int w = 0, h = 0;
		const char *wc = NULL;
		const char *hc = NULL;
		const char *sc = "0";

		switch (info->res2dmode->selectedoption)
		{
		case ASPECT_RATIOS: // Default
			break;
		case ASPECT2D_SCALED: // Scale
			sc = info->scale->values[info->scale->selectedoption];
			break;
		case ASPECT2D_HEIGHT:
			wc = "0";
			hc = info->height2d->text;
			break;
		case ASPECT2D_CUSTOM: // Custom
			wc = info->width2d->text;
			hc = info->height2d->text;
			break;
		default: // Aspects
			{
				menucombo_t *c = info->res2dsize[info->res2dmode->selectedoption];
				const char *res = c->options[c->selectedoption];
				const char *x = strchr(res, 'x');

				w = atoi(res);
				h = atoi(x + 1);
			}
		}

		if (!wc)
			Cvar_SetValue(info->width2d->cvar, w);
		else
			Cvar_Set(info->width2d->cvar, wc);
		if (!hc)
			Cvar_SetValue(info->height2d->cvar, h);
		else
			Cvar_Set(info->height2d->cvar, hc);
		Cvar_Set(info->scale->cvar, sc);
	}

	// restart video to apply latched cvars
	M_RemoveMenu(menu);
	Cbuf_AddText("vid_restart\nmenu_video\n", RESTRICT_LOCAL);
	return true;
}

void M_Menu_Video_f (void)
{
	extern cvar_t v_contrast, vid_conwidth, vid_conheight;
//	extern cvar_t vid_width, vid_height, vid_preservegamma, vid_hardwaregamma, vid_desktopgamma;
	extern cvar_t vid_desktopsettings, vid_conautoscale;
	extern cvar_t vid_bpp, vid_refreshrate, vid_multisample, vid_srgb;

	static const char *gammamodeopts[] = {
		"Off",
		"Auto",
		"GLSL",
		"Hardware",
		"Scene-Only",
		NULL
	};
	static const char *gammamodevalues[] = {
		"0",
		"1",
		"2",
		"3",
		"4",
		NULL
	};

	static const char *srgbopts[] = {
		"Non-Linear",	//0 (legacy buggy linear->srgb non-transforms)
		"sRGB-Aware",	//1 (linear->srgb transforms)
		"Linear (HDR)",	//2 (try to use a float framebuffer, otherwise fall back on srgb framebuffer)
		"Linearised",	//-1
		NULL
	};
	static const char *srgbvalues[] = { "0", "1", "2", "-1", NULL};


#if defined(ANDROID) && !defined(FTE_SDL) && !defined(_DIII4A) //karin: not support portrait
	extern cvar_t sys_orientation;
	static const char *orientationopts[] = {
		"Auto",
		"Landscape",
		"Portrait",
		"Reverse Landscape",
		"Reverse Portrait",
		NULL
	};
	static const char *orientationvalues[] = {
		"",
		"landscape",
		"portrait",
		"reverselandscape",
		"reverseportrait",
		NULL
	};
#else
	extern cvar_t vid_fullscreen;
	static const char *fullscreenopts[] = {
		"Windowed",
		"Fullscreen",
		"Borderless Windowed",
		NULL
	};
	static const char *fullscreenvalues[] = {"0", "1", "2", NULL};
#endif
	extern cvar_t vid_renderer;
	static const char *rendererops[] =
	{
#ifdef GLQUAKE
		"OpenGL",
#endif
#ifdef VKQUAKE
		"Vulkan",
#endif
#ifdef D3D8QUAKE
		"Direct3D 8 (limited)",
#endif
#ifdef D3D9QUAKE
		"Direct3D 9 (limited)",
#endif
#ifdef D3D11QUAKE
		"Direct3D 11 Hardware (Experimental)",
		"Direct3D 11 Software (Experimental)",
#endif
#ifdef SWQUAKE
		"Software Rendering (unusable)",
#endif
#if defined(_WIN32) && !defined(CLIENTONLY)	//win32 because other systems probably won't display a console if started via some shortcut
		"Dedicated Server",
#endif
#if 0
		"Headless",
#endif
		NULL
	};
	static const char *renderervalues[] =
	{
#ifdef GLQUAKE
		"gl",
#endif
#ifdef VKQUAKE
		"vk",
#endif
#ifdef D3D8QUAKE
		"d3d8",
#endif
#ifdef D3D9QUAKE
		"d3d9",
#endif
#ifdef D3D11QUAKE
		"d3d11",
		"d3d11 warp",
#endif
#ifdef SWQUAKE
		"sw",
#endif
#if defined(_WIN32) && !defined(CLIENTONLY)
		"sv",
#endif
#if 0
		"headless",
#endif
		NULL
	};

	static const char *aaopts[] = {
		"1x",
		"2x",
		"4x",
		"6x",
		"8x",
		NULL
	};
	static const char *aavalues[] = {"0", "2", "4", "6", "8", NULL};

	static const char *resmodeopts[] = {
		ASPECT_LIST
		"Desktop",
		"Custom",
		NULL
	};

	static const char *bppopts[] =
	{
		"16-bit",
		"24-bit",
		NULL
	};
	static const char *bppvalues[] = {"16", "24", NULL};
#if defined(_WIN32) && !defined(FTE_SDL)
	extern int qwinvermaj, qwinvermin;
	//on win8+, hide the 16bpp option - windows would just reject it.
	int bppbias = ((qwinvermaj == 6 && qwinvermin >= 2) || qwinvermaj>6)?1:0;
#else
	const int bppbias = 0;
#endif

	static const char *refreshopts[] =
	{
		"Default",
		"59Hz",
		"60Hz",
		"70Hz",
		"72Hz",
		"75Hz",
		"85Hz",
		"100Hz",
		"120Hz",
		"144Hz",
		NULL
	};
	static const char *refreshvalues[] = {"", "59", "60", "70", "72", "75", "85", "100", "120", "144", NULL};

	static const char *res2dmodeopts[] = {
		ASPECT_LIST
		"Default",
		"Square (by height)",
		"Scale",
		"Custom",
		NULL
	};

	static const char *scaleopts[] = {
		"1x",
		"1.5x",
		"2x",
		"2.5x",
		"3x",
		"4x",
		"5x",
		"6x",
		NULL
	};
	static const char *scalevalues[] = { "1", "1.5", "2", "2.5", "3", "4", "5", "6", NULL};

	static const char *vsyncopts[] =
	{
		"Off",
		"Strict",
		"Lax",
		"Alternate Frames",
		NULL
	};
	static const char *vsyncvalues[] = { "0", "1", "-1", "2", NULL};
	extern cvar_t vid_vsync;
	extern cvar_t cl_maxfps;
	extern cvar_t cl_yieldcpu;

/*
	static const char *vsyncoptions[] =
	{
		"Off",
		"Wait for Vertical Sync",
		"Wait for Display Enable",
		NULL
	};
	extern cvar_t vid_vsync;
*/
	videomenuinfo_t *info;
	static char current3dres[32]; // enough to fit 1920x1200

	static menuresel_t resel;
	int y;
	int resmodechoice, res2dmodechoice;
	int reschoices[ASPECT_RATIOS], res2dchoices[ASPECT_RATIOS];
	emenu_t *menu;

	//not calling M_Options_Title because of quake2's different banner.
	y = 32;
	menu = M_CreateMenu(sizeof(videomenuinfo_t));
	switch(M_GameType())
	{
	case MGT_QUAKE2:	//q2...
		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_video");
		break;
#ifdef HEXEN2
	case MGT_HEXEN2://h2
		MC_AddPicture(menu, 16, 0, 35, 176, "gfx/menu/hplaque.lmp");
		MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title3.lmp");
		y += 32;
		break;
#endif
	default: //q1
		MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_option.lmp");
		break;
	}

	info = (videomenuinfo_t*)menu->data;

	snprintf(current3dres, sizeof(current3dres), "Current: %ix%i", vid.pixelwidth, vid.pixelheight);
	resmodechoice = M_MatchModes(vid.pixelwidth, vid.pixelheight, reschoices);
	if (vid_desktopsettings.ival)
		resmodechoice = ASPECT3D_DESKTOP;
	else if (resmodechoice < 0)
		resmodechoice = ASPECT3D_CUSTOM;
	res2dmodechoice = M_MatchModes(vid.pixelwidth, vid.pixelheight, res2dchoices);
	if (vid_conautoscale.value > 0)
		res2dmodechoice = ASPECT2D_SCALED;
	else if (!vid_conwidth.ival && !vid_conheight.ival)
		res2dmodechoice = ASPECT2D_AUTO;
	else if (!vid_conwidth.ival)
		res2dmodechoice = ASPECT2D_HEIGHT;
	else if (res2dmodechoice < 0)
		res2dmodechoice = ASPECT2D_CUSTOM;

	{
		menubulk_t bulk[] =
		{
			MB_REDTEXT("Video Options", true),
			MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
			MB_CMD("Apply Settings", M_VideoApply, "Restart video and apply renderer, display, and 2D resolution options."),
			MB_SPACING(4),
			MB_COMBOCVAR("Renderer", vid_renderer, rendererops, renderervalues, NULL),
#if defined(ANDROID) && !defined(FTE_SDL) && !defined(_DIII4A) //karin: not support orientation
			MB_COMBOCVAR("Orientation", sys_orientation, orientationopts, orientationvalues, NULL),
#else
			MB_COMBOCVARRETURN("Display Mode", vid_fullscreen, fullscreenopts, fullscreenvalues, info->dispmode, vid_fullscreen.description),
#endif
			MB_COMBOCVAR("MSAA", vid_multisample, aaopts, aavalues, NULL),
			MB_REDTEXT(current3dres, true),
			MB_COMBORETURN("Aspect", resmodeopts, resmodechoice, info->resmode, "Select method for determining or configuring display options. The desktop option will attempt to use the width, height, color depth, and refresh from your operating system's desktop environment."),
			// aspect entries
			MB_COMBORETURN("Size", resaspects[0], reschoices[0], info->ressize[0], "Select resolution for display."),
			MB_SPACING(-8),
			MB_COMBORETURN("Size", resaspects[1], reschoices[1], info->ressize[1], "Select resolution for display."),
			MB_SPACING(-8),
			MB_COMBORETURN("Size", resaspects[2], reschoices[2], info->ressize[2], "Select resolution for display."),
			MB_SPACING(-8),
			MB_COMBORETURN("Size", resaspects[3], reschoices[3], info->ressize[3], "Select resolution for display."),
			MB_COMBOCVARRETURN("Color Depth", vid_bpp, bppopts+bppbias, bppvalues+bppbias, info->bppfixed, vid_bpp.description),
			MB_COMBOCVARRETURN("Refresh Rate", vid_refreshrate, refreshopts, refreshvalues, info->hzfixed, vid_refreshrate.description),
			MB_SPACING(-24), // really hacky...
			// custom entries
			MB_EDITCVARSLIMRETURN("Width", "vid_width", info->width),
			MB_EDITCVARSLIMRETURN("Height", "vid_height", info->height),
			MB_EDITCVARSLIMRETURN("Color Depth", "vid_bpp", info->bpp),
			MB_EDITCVARSLIMRETURN("Refresh Rate", "vid_displayfrequency", info->hz),

//			MB_SPACING(4),
			MB_COMBORETURN("2D Mode", res2dmodeopts, res2dmodechoice, info->res2dmode, "Select method for determining or configuring 2D resolution and scaling. The default option matches the current display resolution, and the scale option scales by a factor of the display resolution."),
			// scale entry
			MB_COMBOCVARRETURN("Amount", vid_conautoscale, scaleopts, scalevalues, info->scale, NULL),
			MB_SPACING(-8),
			// 2d aspect entries
			MB_COMBORETURN("Size", resaspects[0], res2dchoices[0], info->res2dsize[0], "Select resolution for 2D rendering."),
			MB_SPACING(-8),
			MB_COMBORETURN("Size", resaspects[1], res2dchoices[1], info->res2dsize[1], "Select resolution for 2D rendering."),
			MB_SPACING(-8),
			MB_COMBORETURN("Size", resaspects[2], res2dchoices[2], info->res2dsize[2], "Select resolution for 2D rendering."),
			MB_SPACING(-8),
			MB_COMBORETURN("Size", resaspects[3], res2dchoices[3], info->res2dsize[3], "Select resolution for 2D rendering."),
			MB_SPACING(-8),
			// 2d custom entries
			MB_EDITCVARSLIMRETURN("Width", "vid_conwidth", info->width2d),
			MB_EDITCVARSLIMRETURN("Height", "vid_conheight", info->height2d),
			MB_SPACING(4),
			MB_COMBOCVAR("sRGB", vid_srgb, srgbopts, srgbvalues, "Controls the colour space to try to use."),
			MB_COMBOCVAR("Gamma Mode", vid_hardwaregamma, gammamodeopts, gammamodevalues, "Controls how gamma is applied"),
			MB_SLIDER("Gamma", v_gamma, 1.5, 0.25, -0.05, NULL),
			MB_SLIDER("Contrast", v_contrast, 0.8, 3, 0.05, NULL),
			MB_SLIDER("Brightness", v_brightness, 0.0, 0.5, 0.05, NULL),
			MB_SPACING(4),
			MB_SLIDER("View Size", scr_viewsize, 30, 120, 10, NULL),
			MB_COMBOCVAR("VSync", vid_vsync, vsyncopts, vsyncvalues, "Controls whether to wait for rendering to finish."),
//			MB_EDITCVARSLIM("Framerate Limiter", cl_maxfps.name, "Limits the maximum framerate. Set to 0 for none."),
//			MB_CHECKBOXCVARTIP("Yield CPU", cl_yieldcpu, 1, "Reduce CPU usage between frames.\nShould probably be off when using vsync."),

			MB_SPACING(4),
			MB_CONSOLECMD("FPS Options", "menu_fps\n", "Set model filtering and graphical profile options."),
			MB_CONSOLECMD("Rendering Options", "menu_render\n", "Set rendering options such as water warp and tinting effects."),
			MB_CONSOLECMD("Lighting Options", "menu_lighting\n", "Set options for level lighting and dynamic lights."),
			MB_CONSOLECMD("Texture Options", "menu_textures\n", "Set options for texture detail and effects."),
#ifndef MINIMAL
			MB_CONSOLECMD("Particle Options", "menu_particles\n", "Set particle effect options."),
#endif
			
			MB_END()
		};
		MC_AddFrameStart(menu, y);
		MC_AddBulk(menu, &resel, bulk, 16, 200, y);
		MC_AddFrameEnd(menu, y);
	}

	/*
	y += 8;
	MC_AddRedText(menu, 200, y, current3dres, false); y+=8;

 	y+=8;
	MC_AddRedText(menu, 0, y,								"      ^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082 ", false); y+=8;
	y+=8;
	info->renderer = MC_AddCombo(menu,	16, y,				"         Renderer", rendererops, i);	y+=8;
	info->bppcombo = MC_AddCombo(menu,	16, y,				"      Color Depth", bppnames, currentbpp); y+=8;
	info->refreshratecombo = MC_AddCombo(menu,	16, y,		"     Refresh Rate", refreshrates, currentrefreshrate); y+=8;
	info->modecombo = MC_AddCombo(menu,	16, y,				"       Video Size", modenames, prefabmode+1);	y+=8;
	MC_AddWhiteText(menu, 16, y, 							"  3D Aspect Ratio", false); y+=8;
	info->conscalecombo = MC_AddCombo(menu,	16, y,			"          2D Size", modenames, prefab2dmode+1);	y+=8;
	MC_AddWhiteText(menu, 16, y, 							"  2D Aspect Ratio", false); y+=8;
	MC_AddCheckBox(menu,	16, y,							"       Fullscreen", &vid_fullscreen,0);	y+=8;
	y+=4;info->customwidth = MC_AddEdit(menu, 16, y,		"     Custom width", vid_width.string);	y+=8;
	y+=4;info->customheight = MC_AddEdit(menu, 16, y,		"    Custom height", vid_height.string);	y+=12;
	info->vsynccombo = MC_AddCombo(menu,	16, y,			"            VSync", vsyncoptions, currentvsync); y+=8;
	//MC_AddCheckBox(menu,	16, y,							"   Override VSync", &vid_vsync,0);	y+=8;
	MC_AddCheckBox(menu,	16, y,							" Desktop Settings", &vid_desktopsettings,0);	y+=8;
	y+=8;
	MC_AddCommand(menu,	16, y,								"= Apply Changes =", M_VideoApply);	y+=8;
	y+=8;
	MC_AddSlider(menu,	16, y,								"      Screen size", &scr_viewsize,	30,		120, 1);y+=8;
	MC_AddSlider(menu,	16, y,								"Console Autoscale",&vid_conautoscale, 0, 6, 0.25);	y+=8;
	MC_AddSlider(menu,	16, y,								"            Gamma", &v_gamma, 0.3, 1, 0.05);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"    Desktop Gamma", &vid_desktopgamma,0);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"   Hardware Gamma", &vid_hardwaregamma,0);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"   Preserve Gamma", &vid_preservegamma,0);	y+=8;
	MC_AddSlider(menu,	16, y,								"         Contrast", &v_contrast, 1, 3, 0.05);	y+=8;
	y+=8;
	MC_AddCheckBox(menu,	16, y,							"   Windowed Mouse", &in_windowed_mouse,0);	y+=8;

	menu->selecteditem = (union menuoption_s *)info->renderer;
	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 152, menu->selecteditem->common.posy, NULL, false);
	*/
	menu->predraw = CheckCustomMode;
}

#ifndef MINIMAL

#ifdef RAGDOLL
#include "pr_common.h"
#endif

typedef struct
{
	enum {
		MV_NONE,
		MV_BONES,
		MV_SHADER,
		MV_TEXTURE,
		MV_COLLISION,
		MV_EVENTS,
		MV_NORMALS,
	} mode;
	int surfaceidx;
	int skingroup;
	int framegroup;
	int boneidx;
	int bonebias;	//shift the bones menu down to ensure the boneidx stays visible
	int textype;
	double frametime;
	double skintime;

	float pitch;
	float yaw;
	vec3_t cameraorg;
	vec2_t mousepos;
	qboolean mousedown;
	float dist;

	char modelname[MAX_QPATH];
	char skinname[MAX_QPATH];
	char animname[MAX_QPATH];

	char shaderfile[MAX_QPATH];
	char *shadertext;

	qboolean paused;
#ifdef RAGDOLL
	lerpents_t ragent;
	world_t ragworld;
	wedict_t ragworldedict;
	comentvars_t ragworldvars;
#ifdef VM_Q1
	comextentvars_t ragworldextvars;
#endif
	pubprogfuncs_t ragfuncs;
	qboolean flop;	//ragdoll flopping enabled.
	float fixedrate;
#endif
} modelview_t;

static unsigned int genhsv(float h_, float s, float v)
{
	float r=0, g=1, b=0;

	float h = h_ - floor(h_);

	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	switch(i)
	{
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}

	return 0xff000000 |
		((int)(r*255)<<16) |
		((int)(g*255)<<8) |
		((int)(b*255)<<0);
}

#include "com_mesh.h"
#ifdef SKELETALMODELS
static int M_BoneDisplayLame(modelview_t *mods, entity_t *e, int topy, int y, int depth, int parent, int first, int last)
{
	int i;
	for (i = first; i < last;  i++)
	{
		int p = Mod_GetBoneParent(e->model, i+1);
		if (p == parent)
		{
			const char *bname = Mod_GetBoneName(e->model, i+1);
			float result[12];
			if (!bname)
				bname = "NULL";
			memset(result, 0, sizeof(result));
			if (i == mods->boneidx)
			{
				if (y < 0)
					mods->bonebias+=8;
				else if (topy+y+8 >= vid.height)
					mods->bonebias-=8;
			}
			if (y >= 0 && y+8 < vid.height)
			{
				if (Mod_GetTag(e->model, i+1, &e->framestate, result))
				{
					if (developer.ival)
					{
						float scale = sqrt(result[0]*result[0] + result[1]*result[1] + result[2]*result[2]);
						float scale2 = sqrt(result[0]*result[0] + result[4]*result[4] + result[8]*result[8]);
						Draw_FunString(depth*16, topy+y, va("%s%i: %s (%g %g %g, %g %g)", (i==mods->boneidx)?"^1":"", i+1, bname, result[3], result[7], result[11], scale, scale2));
					}
					else
						Draw_FunString(depth*16, topy+y, va("%s%i: %s", (i==mods->boneidx)?"^1":"", i+1, bname));
				}
				else
					Draw_FunString(depth*16, topy+y, va("%s%i: %s (err)", (i==mods->boneidx)?"^1":"", i, bname));
			}
			y += 8;
			y += M_BoneDisplayLame(mods, e, topy, y, depth+1, i+1, i+1, last)-y;
		}
	}
	return y;
}
#endif

static unsigned int tobit(unsigned int bitmask)
{
	unsigned int b;
	for (b = 0; b < 32; b++)
	{
		if (bitmask & (1<<b))
			return b;
	}
	return 0;
}
static void M_ModelViewerDraw(int x, int y, struct menucustom_s *c, struct emenu_s *m)
{
	static playerview_t pv;
	entity_t ent;
	vec3_t fwd, rgt, up;
	const char *fname;
	shader_t *shader;
	vec2_t fs = {8,8};
//	float bones[12*MAX_BONES];
	vec3_t lightpos = {0, 1, 0};

	modelview_t *mods = c->dptr;
	skinfile_t *skin;
	texnums_t *texnums;
#ifdef SKELETALMODELS
	qboolean boneanimsonly;
#endif
	model_t *animmodel = NULL;

	if (R2D_Flush)
		R2D_Flush();

	memset(&pv, 0, sizeof(pv));

	Alias_FlushCache();	//doesn't like us using stack...
	CL_DecayLights ();
	CL_ClearEntityLists();
	V_ClearRefdef(&pv);
	r_refdef.drawsbar = false;
	V_CalcRefdef(&pv);
	r_refdef.grect.width = vid.width;
	r_refdef.grect.height = vid.height;
	r_refdef.grect.x = 0;
	r_refdef.grect.y = 0;
	r_refdef.time = mods->skintime;

	r_refdef.flags = RDF_NOWORLDMODEL;

	r_refdef.afov = 60;
	r_refdef.fov_x = 0;
	r_refdef.fov_y = 0;
	r_refdef.dirty |= RDFD_FOV;

	VectorClear(r_refdef.viewangles);
	r_refdef.viewangles[0] = mods->pitch;
	r_refdef.viewangles[1] = mods->yaw;
	AngleVectors(r_refdef.viewangles, fwd, rgt, up);
	VectorScale(fwd, -mods->dist, r_refdef.vieworg);

	if (keydown[K_MOUSE1] && mods->mousedown&1)
	{
		mods->pitch += (mousecursor_y-mods->mousepos[1]) * m_pitch.value * sensitivity.value;
		mods->yaw -= (mousecursor_x-mods->mousepos[0]) * m_yaw.value * sensitivity.value;

		if (keydown['w'] || keydown['s'] || keydown['a'] || keydown['d'])
		{
			VectorAdd(mods->cameraorg, r_refdef.vieworg, mods->cameraorg);
			mods->dist = 0;

			if (keydown['w'])
				VectorMA(mods->cameraorg, host_frametime*cl_forwardspeed.value, fwd, mods->cameraorg);
			if (keydown['s'])
				VectorMA(mods->cameraorg, host_frametime*-(cl_backspeed.value?cl_backspeed.value:cl_forwardspeed.value), fwd, mods->cameraorg);
			if (keydown['a'])
				VectorMA(mods->cameraorg, host_frametime*-cl_sidespeed.value, rgt, mods->cameraorg);
			if (keydown['d'])
				VectorMA(mods->cameraorg, host_frametime*cl_sidespeed.value, rgt, mods->cameraorg);
		}
	}
	else if (keydown[K_MOUSE3] && (mods->mousedown&2))
	{
		float r = (mousecursor_x-mods->mousepos[0]);
		float u = (mousecursor_y-mods->mousepos[1]);
		VectorMA(mods->cameraorg, r*cl_sidespeed.value/4000, rgt, mods->cameraorg);
		VectorMA(mods->cameraorg, u*cl_upspeed.value/4000, up, mods->cameraorg);
	}
	mods->mousedown = (!!keydown[K_MOUSE1])<<0;
	mods->mousedown|= (!!keydown[K_MOUSE3])<<1;
	mods->mousepos[0] = mousecursor_x;
	mods->mousepos[1] = mousecursor_y;

	VectorAdd(r_refdef.vieworg, mods->cameraorg, r_refdef.vieworg);

	memset(&ent, 0, sizeof(ent));
//	ent.angles[1] = realtime*45;//mods->yaw;
//	ent.angles[0] = realtime*23.4;//mods->pitch;

	ent.model = Mod_ForName(mods->modelname, MLV_WARN);
	if (!ent.model)
		return;	//panic!

	if (ent.model->type == mod_alias)	//should we even bother with this here?
	{
		if (*mods->animname)
			animmodel = Mod_ForName(mods->animname, MLV_WARN);

		AngleVectorsMesh(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
	}
	else
		AngleVectors(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
	VectorInverse(ent.axis[1]);

	ent.scale = max(max(fabs(ent.model->maxs[0]-ent.model->mins[0]), fabs(ent.model->maxs[1]-ent.model->mins[1])), fabs(ent.model->maxs[2]-ent.model->mins[2]));
	ent.scale = ent.scale?64.0/ent.scale:1;
//	ent.scale = 1;
	ent.origin[2] -= ent.model->mins[2] + (ent.model->maxs[2]-ent.model->mins[2]) * 0.5;
	ent.origin[2] *= ent.scale;
	Vector4Set(ent.shaderRGBAf, 1, 1, 1, 1);
	VectorSet(ent.glowmod, 1, 1, 1);

//	VectorScale(ent.axis[0], ent.scale, ent.axis[0]);
//	VectorScale(ent.axis[1], ent.scale, ent.axis[1]);
//	VectorScale(ent.axis[2], ent.scale, ent.axis[2]);
//	ent.scale = 1;

	if (strstr(mods->modelname, "player"))
	{
		ent.bottomcolour	= genhsv(realtime*0.1 + 0, 1, 1);
		ent.topcolour		= genhsv(realtime*0.1 + 0.5, 1, 1);
	}
	else
	{
		ent.topcolour = TOP_DEFAULT;
		ent.bottomcolour = BOTTOM_DEFAULT;
	}
//	ent.fatness = sin(realtime)*5;
	ent.playerindex = -1;
	ent.skinnum = mods->skingroup;
	ent.shaderTime = 0;//realtime;
	ent.framestate.g[FS_REG].lerpweight[0] = 1;
	ent.framestate.g[FS_REG].frame[0] = mods->framegroup;
	ent.framestate.g[FS_REG].frametime[0] = ent.framestate.g[FS_REG].frametime[1] = mods->frametime;
	ent.framestate.g[FS_REG].endbone = 0x7fffffff;
	if (*mods->skinname)
	{
		ent.customskin = Mod_RegisterSkinFile(mods->skinname);	//explicit .skin file to use
		if (!ent.customskin)
		{
			Con_Printf(CON_WARNING"Named skinfile not loaded\n");
			*mods->skinname = 0;	//don't spam.
		}
	}
	else
	{
		ent.customskin = Mod_RegisterSkinFile(va("%s_%i.skin", mods->modelname, ent.skinnum));
		if (ent.customskin == 0)
		{
			char haxxor[MAX_QPATH];
			COM_StripExtension(mods->modelname, haxxor, sizeof(haxxor));
			ent.customskin = Mod_RegisterSkinFile(va("%s_default.skin", haxxor));	//fall back to some default
		}
	}
	skin = Mod_LookupSkin(ent.customskin);

	ent.light_avg[0] = ent.light_avg[1] = ent.light_avg[2] = 0.66;
	ent.light_range[0] = ent.light_range[1] = ent.light_range[2] = 0.33;

#ifdef HEXEN2
	ent.drawflags = SCALE_ORIGIN_ORIGIN;
#endif

	V_ApplyRefdef();

	if (!mods->paused)
	{
		mods->frametime += host_frametime;
		mods->skintime += host_frametime;
	}
/*
	{
		trace_t tr;
		vec3_t worldmouse;
		vec3_t mouse = {mousecursor_x/vid.width, 1-mousecursor_y/vid.height, 0.5};
		float d;
		Matrix4x4_CM_UnProject(mouse, worldmouse, r_refdef.viewangles, r_refdef.vieworg, r_refdef.fov_x, r_refdef.fov_y);

		d = DotProduct(worldmouse, fwd);
		VectorMA(worldmouse, -d, fwd, worldmouse);

		if (ent.model->funcs.NativeTrace && ent.model->funcs.NativeTrace(ent.model, 0, &ent.framestate, ent.axis, r_refdef.vieworg, worldmouse, vec3_origin, vec3_origin, false, ~0, &tr))
			;
		else
			VectorCopy(worldmouse, tr.endpos);

		VectorCopy(tr.endpos, lightpos);
	}
*/
	lightpos[0] = sin(realtime*0.1);
	lightpos[1] = cos(realtime*0.1);
	lightpos[2] = 0;

	VectorNormalize(lightpos);
	ent.light_dir[0] = DotProduct(lightpos, ent.axis[0]);
	ent.light_dir[1] = DotProduct(lightpos, ent.axis[1]);
	ent.light_dir[2] = DotProduct(lightpos, ent.axis[2]);

	ent.light_known = 2;

	if (ent.model->type == mod_dummy)
	{
		Draw_FunString(0, 0, va("model \"%s\" not loaded", ent.model->name));
		return;
	}
	switch(ent.model->loadstate)
	{
	case MLS_LOADED:
		break;
	case MLS_NOTLOADED:
		Draw_FunString(0, 0, va("\"%s\" not loaded", ent.model->name));
		return;
	case MLS_LOADING:
		Draw_FunString(0, 0, va("\"%s\" still loading", ent.model->name));
		return;
	case MLS_FAILED:
		Draw_FunString(0, 0, va("Unable to load \"%s\"", ent.model->name));
		return;
	}


#ifdef RAGDOLL
	if (mods->flop)
		ent.framestate.g[FS_REG].frame[0] |= 0x8000;
	if (ent.model->dollinfo && mods->ragworld.rbe)
	{
		float rate = 1.0/60;	//try a fixed tick rate...
		rag_doallanimations(&mods->ragworld);
		if (mods->fixedrate > 1)
			mods->fixedrate = 1;
		while (mods->fixedrate >= rate)
		{
			mods->ragworld.rbe->RunFrame(&mods->ragworld, rate, 800);
			mods->fixedrate -= rate;
		}
		if (!mods->paused)
			mods->fixedrate += host_frametime;

		rag_updatedeltaent(&mods->ragworld, &ent, &mods->ragent);
	}
#endif

#ifdef SKELETALMODELS
	if (animmodel)// && Mod_GetNumBones(ent.model, false)==Mod_GetNumBones(animmodel, false))
	{
		int numbones = Mod_GetNumBones(ent.model, false);
		galiasbone_t *boneinfo = Mod_GetBoneInfo(ent.model, &numbones);
		float *bonematrix = alloca(numbones*sizeof(*bonematrix)*12);
		ent.framestate.bonecount = Mod_GetBoneRelations(animmodel, 0, numbones, boneinfo, &ent.framestate, bonematrix);
		ent.framestate.bonestate = bonematrix;
		ent.framestate.skeltype = SKEL_RELATIVE;
	}
	else
#endif
		animmodel = ent.model;	//not using it. sorry. warn?


	if (mods->mode == MV_NORMALS)
	{
		shader_t *s;
		if (1)
		{
			s = R_RegisterShader("hitbox_nodepth", SUF_NONE,
				"{\n"
					"polygonoffset\n"
					"{\n"
						"map $whiteimage\n"
						"blendfunc gl_src_alpha gl_one\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepthtest\n"
					"}\n"
				"}\n");
			Mod_AddSingleSurface(&ent, mods->surfaceidx, s, true);
		}
	}
	if (mods->mode == MV_COLLISION)
	{
		shader_t *s;

#ifdef HALFLIFEMODELS
		if (ent.model->type == mod_halflife)
			HLMDL_DrawHitBoxes(&ent);
		else
#endif
		if (1)
		{
			s = R_RegisterShader("hitbox_nodepth", SUF_NONE,
				"{\n"
					"polygonoffset\n"
					"{\n"
						"map $whiteimage\n"
						"blendfunc gl_src_alpha gl_one\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepthtest\n"
					"}\n"
				"}\n");
			Mod_AddSingleSurface(&ent, mods->surfaceidx, s, false);
		}
		else
		{
			vec3_t mins, maxs;
			VectorAdd(ent.model->mins, ent.origin, mins);
			VectorAdd(ent.model->maxs, ent.origin, maxs);

			s = R_RegisterShader("bboxshader_nodepth", SUF_NONE,
				"{\n"
					"polygonoffset\n"
					"{\n"
						"map $whiteimage\n"
						"blendfunc gl_src_alpha gl_one\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepthtest\n"
					"}\n"
				"}\n");
			CLQ1_AddOrientedCube(s, mins, maxs, NULL, 1, 1, 1, 0.2);
		}

#ifdef _DEBUG
		{
			trace_t tr;
			vec3_t v1, v2;
			VectorSet(v1, 1000*sin(realtime*2*M_PI/180), 1000*cos(realtime*2*M_PI/180), -ent.origin[2]);
			VectorScale(v1, -1, v2);
			v2[2] = v1[2];
			s = R_RegisterShader("bboxshader", SUF_NONE,
				"{\n"
					"polygonoffset\n"
					"{\n"
						"map $whiteimage\n"
						"blendfunc gl_src_alpha gl_one\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
					"}\n"
				"}\n");

			if (ent.model->funcs.NativeTrace && ent.model->funcs.NativeTrace(ent.model, 0, &ent.framestate, ent.axis, v1, v2, vec3_origin, vec3_origin, false, ~0, &tr))
			{
				vec3_t dir;
				float f;

				VectorMA(ent.origin, ent.scale, v1, v1);
				VectorMA(ent.origin, ent.scale, v2, v2);
				VectorMA(ent.origin, ent.scale, tr.endpos, tr.endpos);

				VectorSubtract(tr.endpos, v1, dir);
				f = DotProduct(dir, tr.plane.normal) * -2;
				VectorMA(dir, f, tr.plane.normal, v2);
				VectorAdd(v2, tr.endpos, v2);

				CLQ1_DrawLine(s, v1, tr.endpos, 0, 1, 0, 1);
				CLQ1_DrawLine(s, tr.endpos, v2, 1, 0, 0, 1);
			}
			else
			{
				VectorAdd(v1, ent.origin, v1);
				VectorAdd(v2, ent.origin, v2);
				CLQ1_DrawLine(s, v1, v2, 0, 1, 0, 1);
			}
		}
#endif
	}
#ifdef SKELETALMODELS
	boneanimsonly = false;
	if (ent.model && ent.model->loadstate == MLS_LOADED && ent.model->type == mod_alias)
	{	//some models don't actually contain any mesh data, but exist as containers for skeletal animations that can be skel_built into a different model's anims.
		//always show their bones, so users don't think its an engine bug.
		galiasinfo_t *inf = Mod_Extradata(ent.model);
		if (inf && !inf->nextsurf && !inf->numindexes && inf->numbones)
			boneanimsonly = true;
	}
	if (mods->mode == MV_BONES || boneanimsonly)
	{
		shader_t *lineshader;
		int tags = Mod_GetNumBones(ent.model, true);
		int b;
		//if (ragdoll)
		//	ent->frame[] |= 0x8000;
		//rag_updatedeltaent(&ent, le);

		lineshader = R_RegisterShader("lineshader_nodepth", SUF_NONE,
					"{\n"
						"polygonoffset\n"
						"{\n"
							"map $whiteimage\n"
							"blendfunc add\n"
							"rgbgen vertex\n"
							"alphagen vertex\n"
							"nodepthtest\n"
						"}\n"
					"}\n");
		for (b = 1; b <= tags; b++)
		{
			int p = Mod_GetBoneParent(ent.model, b);
			vec3_t start, end;
			float boneinfo[12];

			Mod_GetTag(ent.model, b, &ent.framestate, boneinfo);
			//fixme: no axis transform
			VectorSet(start, boneinfo[3], boneinfo[7], boneinfo[11]);
			VectorMA(ent.origin, ent.scale, start, start);

			if (p)
			{
				Mod_GetTag(ent.model, p, &ent.framestate, boneinfo);
				//fixme: no axis transform
				VectorSet(end, boneinfo[3], boneinfo[7], boneinfo[11]);
				VectorMA(ent.origin, ent.scale, end, end);
				CLQ1_DrawLine(lineshader, start, end, 1, (b-1 == mods->boneidx)?0:1, 1, 1);
			}
			if (b-1 == mods->boneidx)
			{
				VectorSet(end, start[0]+boneinfo[0], start[1]+boneinfo[4], start[2]+boneinfo[8]);
				CLQ1_DrawLine(lineshader, start, end, 1, 0, 0, 1);
				VectorSet(end, start[0]+boneinfo[1], start[1]+boneinfo[5], start[2]+boneinfo[9]);
				CLQ1_DrawLine(lineshader, start, end, 0, 1, 0, 1);
				VectorSet(end, start[0]+boneinfo[2], start[1]+boneinfo[6], start[2]+boneinfo[10]);
				CLQ1_DrawLine(lineshader, start, end, 0, 0, 1, 1);
			}
		}
	}
#endif

	V_AddAxisEntity(&ent);

	R_RenderView();

	y = 0;
	{
		fname = Mod_SurfaceNameForNum(ent.model, mods->surfaceidx);
		if (!fname)
			fname = "Unknown Surface";
		Draw_FunString(0, y, va("Surf %i: %s", mods->surfaceidx, fname));
		y+=8;
	}
	{
		fname = Mod_SkinNameForNum(ent.model, mods->surfaceidx, mods->skingroup);
		if (!fname)
		{
			Draw_FunString(0, y, va("Skin %i: <invalid skin>", mods->skingroup));
		}
		else
		{
			shader = Mod_ShaderForSkin(ent.model, mods->surfaceidx, mods->skingroup, r_refdef.time, &texnums);
			Draw_FunString(0, y, va("Skin %i: \"%s\", material \"%s\"", mods->skingroup, fname, shader?shader->name:"NO SHADER"));
		}
		y+=8;
	}
	{
		char *fname;
		int numframes = 0;
		float duration = 0;
		qboolean loop = false;
		int act = -1;
		if (!Mod_FrameInfoForNum(animmodel, mods->surfaceidx, mods->framegroup, &fname, &numframes, &duration, &loop, &act))
			fname = "Unknown Sequence";
		if (animmodel != ent.model)
			fname = va("^[%s^] %s", animmodel->name, fname);	//tag it properly if its from our animmodel
		if (act != -1)
			Draw_FunString(0, y, va("Frame%i[%i]: %s (%i poses, %f of %f secs, %s)", mods->framegroup, act, fname, numframes, ent.framestate.g[FS_REG].frametime[0], duration, loop?"looped":"unlooped"));
		else
			Draw_FunString(0, y, va("Frame%i: %s (%i poses, %f of %f secs, %s)", mods->framegroup, fname, numframes, ent.framestate.g[FS_REG].frametime[0], duration, loop?"looped":"unlooped"));
		y+=8;
	}

	switch(mods->mode)
	{
	case MV_NONE:
		R_DrawTextField(r_refdef.grect.x, r_refdef.grect.y+y, r_refdef.grect.width, r_refdef.grect.height-y, 
			va("Help:\narrows: pitch/rotate\n"
			"w: zoom in\n"
			"s: zoom out\n"
			"m: mode\n"
			"r: reset times\n"
			"home: skin+=1\n"
			"end: skin-=1\n"
			"pgup: frame+=1\n"
			"pgdn: frame-=1\n"
			"mins: %g %g %g, maxs: %g %g %g\n"
			"flags: %#x %#x\n", 
				ent.model->mins[0], ent.model->mins[1], ent.model->mins[2], ent.model->maxs[0], ent.model->maxs[1], ent.model->maxs[2],
				ent.model->flags, ent.model->engineflags),
			CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, fs);
		break;
	case MV_COLLISION:
		if (!ent.model)
			;
		else if (ent.model->type == mod_alias)
		{
			galiasinfo_t *inf = Mod_Extradata(ent.model);
			int surfnum = mods->surfaceidx;
			while(inf && surfnum-->0)
				inf = inf->nextsurf;
			if (inf)
			{
				char contents[512];
				unsigned int i;
				char *contentnames[32] = {NULL};
				contentnames[tobit(FTECONTENTS_SOLID)] = "solid";
				contentnames[tobit(FTECONTENTS_LAVA)] = "lava";
				contentnames[tobit(FTECONTENTS_SLIME)] = "slime";
				contentnames[tobit(FTECONTENTS_WATER)] = "water";
				contentnames[tobit(FTECONTENTS_LADDER)] = "ladder";
				contentnames[tobit(FTECONTENTS_PLAYERCLIP)] = "playerclip";
				contentnames[tobit(FTECONTENTS_MONSTERCLIP)] = "monsterclip";
				contentnames[tobit(FTECONTENTS_BODY)] = "body";
				contentnames[tobit(FTECONTENTS_CORPSE)] = "corpse";
				contentnames[tobit(FTECONTENTS_SKY)] = "sky";
				for (*contents = 0, i = 0; i < 32; i++)
				{
					if (inf->contents & (1<<i))
					{
						if (*contents)
							Q_strncatz(contents, "|", sizeof(contents));
						if (contentnames[i])
							Q_strncatz(contents, contentnames[i], sizeof(contents));
						else
							Q_strncatz(contents, va("%#x", 1<<i), sizeof(contents));
					}
				}
				if (!*contents)
					Q_strncatz(contents, "non-solid", sizeof(contents));
				R_DrawTextField(r_refdef.grect.x, r_refdef.grect.y+y, r_refdef.grect.width, r_refdef.grect.height-y, 
					va(	"Collision:\n"
						"mins: %g %g %g, maxs: %g %g %g\n"
						"contents: %s\n"
						"surfflags: %#x\n"
						"body: %i\n"
						"geomset: %i %i%s\n"
						"numverts: %i\nnumtris: %i\n"
#ifdef SKELETALMODELS
						"numbones: %i\n"
#endif
						"minlod: %g\n"
						"maxlod: %g%s\n"
						, ent.model->mins[0], ent.model->mins[1], ent.model->mins[2], ent.model->maxs[0], ent.model->maxs[1], ent.model->maxs[2],
						contents,
						inf->csurface.flags,
						inf->surfaceid,
						inf->geomset>=MAX_GEOMSETS?-1:inf->geomset, inf->geomid,
								inf->geomset>=MAX_GEOMSETS?" (always)":
								((skin?skin->geomset[inf->geomset]:0)!=inf->geomid)?" (hidden)":
								"",
						inf->numverts, inf->numindexes/3
#ifdef SKELETALMODELS
						,inf->numbones
#endif
						,inf->mindist,inf->maxdist,inf->maxdist?"":" (infinite)"
						)
					, CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, fs);
			}
		}
		else
		{
			R_DrawTextField(r_refdef.grect.x, r_refdef.grect.y+y, r_refdef.grect.width, r_refdef.grect.height-y, 
				va(	"Collision info not available\n"
					"mins: %g %g %g, maxs: %g %g %g\n", ent.model->mins[0], ent.model->mins[1], ent.model->mins[2], ent.model->maxs[0], ent.model->maxs[1], ent.model->maxs[2])
				, CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, fs);
		}
		break;
	case MV_NORMALS:
		Draw_FunString(0, y, va("Normals"));
		break;
	case MV_BONES:
#ifdef SKELETALMODELS
		{
			int bonecount = Mod_GetNumBones(ent.model, true);
			if (bonecount)
			{
				if (mods->boneidx >= bonecount)
					mods->boneidx = bonecount-1;
				if (mods->boneidx < 0)
					mods->boneidx = 0;
				Draw_FunString(0, y, va("Bones: "));
				y+=8;
				M_BoneDisplayLame(mods, &ent, y, mods->bonebias, 0, 0, 0, bonecount);
			}
			else
				R_DrawTextField(r_refdef.grect.x, r_refdef.grect.y+y, r_refdef.grect.width, r_refdef.grect.height-y, "No bones in model", CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, fs);
		}
#endif
		break;
	case MV_EVENTS:
		{
			int i;
			float timestamp = 0;
			int code = 0;
			char *data = NULL;
			Draw_FunString(0, y, va("Events: "));
			y+=8;
			for (i = 0; Mod_GetModelEvent(animmodel, mods->framegroup, i, &timestamp, &code, &data); y+=8, i++)
			{
				Draw_FunString(0, y, va("%i %f: %i %s", i, timestamp, code, data));
			}
			Draw_FunString(0, y, va("%f: <end of animation>", Mod_GetFrameDuration(animmodel, 0, mods->framegroup)));
		}
		break;
	case MV_SHADER:
		{
			if (!mods->shadertext)
			{
				char *cr;
				char *body = Shader_GetShaderBody(Mod_ShaderForSkin(ent.model, mods->surfaceidx, mods->skingroup, r_refdef.time, &texnums), mods->shaderfile, sizeof(mods->shaderfile));
				if (!body)
				{
					Draw_FunString(0, y, "Shader info not available");
					break;
				}
				if (*mods->shaderfile)
					mods->shadertext = Z_StrDupf("\n\nPress space to view+edit the shader\n\n%s", body);
				else
					mods->shadertext = Z_StrDupf("{ %s",body);

				while ((cr = strchr(mods->shadertext, '\r')))
					*cr = ' ';
			}
			R_DrawTextField(r_refdef.grect.x, r_refdef.grect.y+24, r_refdef.grect.width, r_refdef.grect.height-16, mods->shadertext, CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, fs);

			//fixme: draw the shader's textures.
		}
		break;
	case MV_TEXTURE:
		{
			shader_t *shader = Mod_ShaderForSkin(ent.model, mods->surfaceidx, mods->skingroup, r_refdef.time, &texnums);
			if (shader)
			{
				char *t;
				texnums_t *skin = (texnums&&TEXVALID(texnums->base))?texnums:shader->defaulttextures;
				shader = R_RegisterShader("modelviewertexturepreview", SUF_2D, "{\nprogram default2d\n{\nmap $diffuse\n}\n}\n");

				switch(mods->textype)
				{
				case 1:
					t = "Normalmap";
					shader->defaulttextures->base = skin->bump;
					break;
				case 2:
					t = "SpecularMap";
					shader->defaulttextures->base = skin->specular;		//specular lighting values.
					break;
				case 3:
					t = "UpperMap";
					shader->defaulttextures->base = skin->upperoverlay;	//diffuse texture for the upper body(shirt colour). no alpha channel. added to base.rgb
					break;
				case 4:
					t = "LowerMap";
					shader->defaulttextures->base = skin->loweroverlay;	//diffuse texture for the lower body(trouser colour). no alpha channel. added to base.rgb
					break;
				case 5:
					t = "PalettedMap";
					shader->defaulttextures->base = skin->paletted;		//8bit paletted data, just because.
					break;
				case 6:
					t = "FullbrightMap";
					shader->defaulttextures->base = skin->fullbright;
					break;
				case 7:
					t = "ReflectCube";
					shader->defaulttextures->base = skin->reflectcube;
					break;
				case 8:
					t = "ReflectMask";
					shader->defaulttextures->base = skin->reflectmask;
					break;
				case 9:
					t = "DisplacementMap";
					shader->defaulttextures->base = skin->displacement;
					break;
				default:
					mods->textype = 0;
					t = "Diffusemap";
					shader->defaulttextures->base = skin->base;
					break;
				}
				if (shader->defaulttextures->base)
				{
					float w, h;
					Draw_FunString(0, y, va("%s: %s  (%s), %s %u*%u*%u",
							t, shader->defaulttextures->base->ident, shader->defaulttextures->base->subpath?shader->defaulttextures->base->subpath:"",
							Image_FormatName(shader->defaulttextures->base->format), shader->defaulttextures->base->width, shader->defaulttextures->base->height, shader->defaulttextures->base->depth));
					y+=8;

					w = (float)vid.width / shader->defaulttextures->base->width;
					h = (float)(vid.height-y) / shader->defaulttextures->base->height;
					h = min(w,h);
					w = h*shader->defaulttextures->base->width;
					h = h*shader->defaulttextures->base->height;
					R2D_Image(0, y, w, h, 0, 0, 1, 1, shader);


					{
						shader_t *s = R_RegisterShader("hitbox_nodepth", SUF_NONE,
							"{\n"
								"polygonoffset\n"
								"{\n"
									"map $whiteimage\n"
									"blendfunc gl_src_alpha gl_one\n"
									"rgbgen vertex\n"
									"alphagen vertex\n"
									"nodepthtest\n"
								"}\n"
							"}\n");
						int oldtris = cl_numstris;
						int oldidx = cl_numstrisidx;
						int oldvert = cl_numstrisvert;
						mesh_t mesh;
						memset(&mesh, 0, sizeof(mesh));

						AngleVectors(vec3_origin, ent.axis[0], ent.axis[1], ent.axis[2]);
						VectorInverse(ent.axis[1]);
						VectorScale(ent.axis[0], w, ent.axis[0]);
						VectorScale(ent.axis[1], h, ent.axis[1]);
						VectorSet(ent.origin, 0, y, 0);
						ent.scale = 1;

						Mod_AddSingleSurface(&ent, mods->surfaceidx, s, 2);

						mesh.xyz_array = cl_strisvertv + oldvert;
						mesh.st_array = cl_strisvertt + oldvert;
						mesh.colors4f_array[0] = cl_strisvertc + oldvert;
						mesh.indexes = cl_strisidx + oldidx;
						mesh.numindexes = cl_numstrisidx - oldidx;
						mesh.numvertexes = cl_numstrisvert-oldvert;

						cl_numstris = oldtris;
						cl_numstrisidx = oldidx;
						cl_numstrisvert = oldvert;

						if (R2D_Flush)
							R2D_Flush();
						BE_DrawMesh_Single(s, &mesh, NULL, BEF_LINES);
					}
				}
				else
					Draw_FunString(0, y, va("%s: <NO TEXTURE>", t));
			}
			else
				Draw_FunString(0, y, "Texture info not available");
		}
		break;
	default:
		Draw_FunString(0, y, "Unknown display mode");
		break;
	}
}
static qboolean M_ModelViewerKey(struct menucustom_s *c, struct emenu_s *m, int key, unsigned int unicode)
{
	modelview_t *mods = c->dptr;

	if ((key == 'w' && !keydown[K_MOUSE1]) || key == K_MWHEELUP)
	{
		mods->dist *= 0.9;
		if (mods->dist < 1)
			mods->dist = 1;
	}
	else if ((key == 's' && !keydown[K_MOUSE1]) || key == K_MWHEELDOWN)
		mods->dist /= 0.9;
	else if (key == 'm')
	{
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
		switch (mods->mode)
		{
		case MV_NONE:	mods->mode = MV_BONES;		break; 
		case MV_BONES:	mods->mode = MV_SHADER;		break;
		case MV_SHADER:	mods->mode = MV_TEXTURE;	break;
		case MV_TEXTURE: mods->mode = MV_COLLISION;	break;
		case MV_COLLISION: mods->mode = MV_EVENTS;	break;
		case MV_EVENTS: mods->mode = MV_NORMALS;	break;
		case MV_NORMALS: mods->mode = MV_NONE;		break;
		}
	}
	else if (key >= '1' && key <= '7')
	{
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
		mods->mode = MV_NONE + (key - '1');
	}
	else if (key == 'p')
		mods->paused = !mods->paused;
	else if (key == 'r')
	{
		mods->frametime = 0;
		mods->skintime = 0;
	}
#ifdef RAGDOLL
	else if (key == 'f')
	{
		mods->flop ^= 1;
		if (!mods->flop)
			rag_removedeltaent(&mods->ragent);
	}
#endif
	else if (key == '[')
	{
		mods->boneidx--;
		if (mods->boneidx < 0)
			mods->boneidx = 0;
	}
	else if (key == ']')
		mods->boneidx++;
	else if (key == K_UPARROW || key == K_KP_UPARROW || key == K_GP_DPAD_UP)
		mods->pitch += 5;
	else if (key == K_DOWNARROW || key == K_KP_DOWNARROW || key == K_GP_DPAD_DOWN)
		mods->pitch -= 5;
	else if (key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_GP_DPAD_LEFT)
		mods->yaw -= 5;
	else if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_GP_DPAD_RIGHT)
		mods->yaw += 5;
	else if (key == 't')
	{
		if (mods->mode == MV_TEXTURE)
			mods->textype += 1;
		else
			mods->textype = 0;
		mods->mode = MV_TEXTURE;
	}
	else if (key == K_END)
	{
		mods->skingroup = max(0, mods->skingroup-1);
		mods->skintime = 0;
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
	}
	else if (key == K_HOME)
	{
		mods->skingroup += 1;
		mods->skintime = 0;
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
	}
	else if (key == K_PGDN)
	{
		mods->framegroup = max(0, mods->framegroup-1);
		mods->frametime = 0;
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
	}
	else if (key == K_PGUP)
	{
		mods->framegroup += 1;
		mods->frametime = 0;
	}
	else if (key == K_DEL)
	{
		mods->surfaceidx = max(0, mods->surfaceidx-1);
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
	}
	else if (key == K_INS)
	{
		mods->surfaceidx += 1;
		Z_Free(mods->shadertext);
		mods->shadertext = NULL;
	}
	else if (key == K_SPACE)
	{
		if (*mods->shaderfile)
			Cbuf_AddText(va("\nedit %s\n", mods->shaderfile), RESTRICT_LOCAL);
		return true;
	}
	else
		return false;
	return true;
}

#ifdef RAGDOLL
void M_Modelviewer_Reset(struct menu_s *cmenu)
{
	emenu_t *menu = (emenu_t*)cmenu;
	modelview_t *mv = menu->data;
	mv->ragworld.worldmodel = NULL;	//already went away
	rag_removedeltaent(&mv->ragent);
	skel_reset(&mv->ragworld);
	World_RBE_Shutdown(&mv->ragworld);
	//we still want it.
	mv->ragworld.worldmodel = NULL;//Mod_ForName("", MLV_SILENT);
//	World_RBE_Start(&mv->ragworld);
}
static void M_Modelviewer_Shutdown(struct emenu_s *menu)
{
	modelview_t *mv = menu->data;
	rag_removedeltaent(&mv->ragent);
	skel_reset(&mv->ragworld);
	World_RBE_Shutdown(&mv->ragworld);
}
//haxors, for skeletal objects+RBE
static char	*PDECL M_Modelviewer_AddString(pubprogfuncs_t *prinst, const char *val, int minlength, pbool demarkup)
{
	return Z_Malloc(minlength);
}
static struct edict_s	*PDECL M_Modelviewer_ProgsToEdict(pubprogfuncs_t *prinst, int num)
{
	return (struct edict_s*)prinst->edicttable[num];
}
#endif

void M_Menu_ModelViewer_f(void)
{
	modelview_t *mv;
	menucustom_t *c;
	emenu_t *menu;

	if (!*Cmd_Argv(1))
	{
		Con_Printf("modelviewer <MODELNAME> [SKINFILE] [ANIMATIONFILE]\n");
		return;
	}

	menu = M_CreateMenu(sizeof(*mv));
	menu->menu.persist = true;
	mv = menu->data;
	c = MC_AddCustom(menu, 64, 32, mv, 0, NULL);
	menu->selecteditem = menu->cursoritem = (menuoption_t*)c;
	c->draw = M_ModelViewerDraw;
	c->key = M_ModelViewerKey;

	mv->yaw = 180;// + crandom()*45;
	mv->dist = 150;
	Q_strncpyz(mv->modelname, Cmd_Argv(1), sizeof(mv->modelname));
	Q_strncpyz(mv->skinname, Cmd_Argv(2), sizeof(mv->skinname));
	Q_strncpyz(mv->animname, Cmd_Argv(3), sizeof(mv->animname));

	mv->frametime = 0;
	mv->skintime = 0;
#ifdef RAGDOLL
	menu->menu.videoreset = M_Modelviewer_Reset;
	menu->remove = M_Modelviewer_Shutdown;
	mv->ragworld.progs = &mv->ragfuncs;
	mv->ragfuncs.AddString = M_Modelviewer_AddString;
	mv->ragfuncs.ProgsToEdict = M_Modelviewer_ProgsToEdict;
	mv->ragfuncs.edicttable = (edict_t**)&mv->ragworld.edicts;
	mv->ragworld.edicts = &mv->ragworldedict;
	mv->ragworld.edicts->v = &mv->ragworldvars;
#ifdef VM_Q1
	mv->ragworld.edicts->xv = &mv->ragworldextvars;
#endif
	mv->ragworld.num_edicts = 1;
	mv->ragworld.edicts->v->solid = SOLID_BBOX;
	VectorSet(mv->ragworld.edicts->v->mins, -1000, -1000, -1000);
	VectorSet(mv->ragworld.edicts->v->maxs, 1000, 1000, -100);
	mv->ragworld.edicts->xv->dimension_hit = mv->ragworld.edicts->xv->dimension_solid = 0xff;

	mv->ragworld.worldmodel = Mod_ForName("", MLV_SILENT);
	World_RBE_Start(&mv->ragworld);
#endif
}
static int QDECL CompleteModelViewerList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	const char *ext = COM_GetFileExtension(name, NULL);
	if (!strcmp(ext, ".mdl") || !strcmp(ext, ".md2") || !strcmp(ext, ".md3")
		|| !strcmp(ext, ".iqm") || !strcmp(ext, ".dpm") || !strcmp(ext, ".zym")
		|| !strcmp(ext, ".psk") || !strcmp(ext, ".md5mesh") || !strcmp(ext, ".md5anim")
		|| !strcmp(ext, ".bsp") || !strcmp(ext, ".map") || !strcmp(ext, ".hmp")
		|| !strcmp(ext, ".spr") || !strcmp(ext, ".sp2") || !strcmp(ext, ".spr32")
		|| !strcmp(ext, ".gltf") || !strcmp(ext, ".glb") || !strcmp(ext, ".ase") || !strcmp(ext, ".lwo") || !strcmp(ext, ".obj")
		|| !strncmp(name, "xmodel/", 7) || !strncmp(name, "xanim/", 6))	//urgh!
	{
		ctx->cb(name, NULL, NULL, ctx);
	}
	return true;
}
void M_Menu_ModelViewer_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	if (argn == 1)
	{
		COM_EnumerateFiles(va("%s*", partial), CompleteModelViewerList, ctx);
		COM_EnumerateFiles(va("%s*/*", partial), CompleteModelViewerList, ctx);
		COM_EnumerateFiles(va("%s*/*", partial), CompleteModelViewerList, ctx);
		COM_EnumerateFiles(va("%s*/*/*", partial), CompleteModelViewerList, ctx);
	}
}
#else
void M_Menu_ModelViewer_f(void)
{
	Con_Printf("modelviewer: not in this build\n");
}
#endif

static void Mods_Draw(int x, int y, struct menucustom_s *c, struct emenu_s *m)
{
	int i = c->dint;
	struct modlist_s *mod = Mods_GetMod(i);

	if (!mod && !i)
	{
		float scale[] = {8,8};

		m->height = vid.height;

		if (y==0)
		{	//just take the full screen.
			y = m->ypos;
			c->common.posy = 0;
			c->common.height = m->height;

			m->dontexpand = true;
			m->xpos = x = 0;
			m->width = vid.width;
		}
		else
		{	//at least expand it.
			c->common.height = m->height - c->common.posy;
		}

		//take the full width of the menu
		x = m->xpos;
		c->common.posx = 0;
		c->common.width = m->width;

		R_DrawTextField(x, y, c->common.width, c->common.height,
					va(
					"No games or mods known.\n"
#ifdef FTE_TARGET_WEB
					"Try providing packages/gamedirs via drag+drop.\n"
					"%s", Cmd_Exists("sys_openfile")?"Or click to add a package\n":""
#else
	#ifndef ANDROID
					"You may need to use -basedir $PATHTOGAME on the commandline.\n"
	#endif
					"\nExpected data path:\n^a%s", com_gamepath
#endif
					), CON_WHITEMASK, 0, font_default, scale);
		return;
	}
	c->common.height = 8;
	c->common.width = vid.width - x - 16;

	if (!mod)
		return;
	if (mod->manifest)
		Draw_FunStringWidth(x, y, mod->manifest->formalname, c->common.width, 0, m->selecteditem == (menuoption_t*)c);
	else
		Draw_FunStringWidth(x, y, mod->gamedir, c->common.width, 0, m->selecteditem == (menuoption_t*)c);
}
static qboolean Mods_Key(struct menucustom_s *c, struct emenu_s *m, int key, unsigned int unicode)
{
	int gameidx = c->dint;
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		qboolean wasgameless = !*FS_GetGamedir(false);
		if (!Mods_GetMod(c->dint))
		{
			if (Cmd_Exists("sys_openfile"))
				Cbuf_AddText("sys_openfile\n", RESTRICT_LOCAL);
			return false;
		}
		M_RemoveMenu(m);

		Cbuf_AddText(va("\nfs_changegame %u\n", gameidx+1), RESTRICT_LOCAL);
		if (wasgameless && !!*FS_GetGamedir(false))
		{
			//starting to a blank state generally means that the current(engine-default) config settings are utterly useless and windowed by default.
			//so generally when switching to a *real* game, we want to restart video just so things like fullscreen etc are saved+used properly.
			Cbuf_AddText("\nvid_restart\n", RESTRICT_LOCAL);
		}
		return true;
	}

	return false;
}

void M_Menu_Mods_f (void)
{
	menucustom_t *c;
	emenu_t *menu;
	size_t i;
	int y;

	//FIXME: sort by mtime?

	menu = M_CreateMenu(0);
	if (COM_FCheckExists("gfx/p_option.lmp"))
	{
		MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
		MC_AddCenterPicture(menu, 0, 24, "gfx/p_option.lmp");
		y = 32;
	}
	else
		y = 0;

	MC_AddFrameStart(menu, y);
	for (i = 0; i<1 || Mods_GetMod(i); i++)
	{
		struct modlist_s *mod = Mods_GetMod(i);
		c = MC_AddCustom(menu, 64, y+i*8, menu->data, i, (mod&&mod->manifest)?mod->manifest->basedir:NULL);
//		if (!menu->selecteditem)
//			menu->selecteditem = (menuoption_t*)c;
		c->common.height = 8;
		c->draw = Mods_Draw;
		c->key = Mods_Key;
	}
	MC_AddFrameEnd(menu, y);
}

#if 0
extern ftemanifest_t	*fs_manifest;	//currently active manifest.
struct installermenudata
{
	menuedit_t *syspath;
};
static void Installer_Remove	(struct menu_s *m)
{
	Cbuf_AddText("quit force\n", RESTRICT_LOCAL);
}


void FS_CreateBasedir(const char *path);
#include <process.h>
static qboolean Installer_Go(menuoption_t *opt, menu_t *menu, int key)
{
	struct installermenudata *md = menu->data;
	
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		extern int startuppending;
		vfsfile_t *f;
		char path[MAX_OSPATH];
		char exepath[MAX_OSPATH];
		char newexepath[MAX_OSPATH];

		Q_snprintfz(path, sizeof(path), "%s/", md->syspath->text);

		Con_Printf("path %s\n", path);

		menu->remove = NULL;
		M_RemoveMenu(menu);

		FS_CreateBasedir(path);

#ifdef _WIN32
		GetModuleFileNameW(NULL, exepath, sizeof(exepath));
		FS_SystemPath(va("%s.exe", fs_manifest->installation), FS_ROOT, newexepath, sizeof(newexepath));
		CopyFileW(exepath, newexepath, FALSE);

//		SetHookState(false);
		Host_Shutdown ();
//		CloseHandle (qwclsemaphore);
//		SetHookState(false);
//		_execv(newexepath, host_parms.argv);
		{
			PROCESS_INFORMATION childinfo;
			STARTUPINFOW startinfo = {sizeof(startinfo)};
			memset(&childinfo, 0, sizeof(childinfo));
			if (CreateProcessW(newexepath, va("\"%s\" +sys_register_file_associations %s", newexepath, COM_Parse(GetCommandLineW())), NULL, NULL, FALSE, 0, NULL, path, &startinfo, &childinfo))
			{
				CloseHandle(childinfo.hProcess);
				CloseHandle(childinfo.hThread);
			}
		}
		exit(1);
#elif 0
#ifdef __linux__
		if (readlink("/proc/self/exe", exepath, sizeof(exepath)-1) <= 0)
#endif
			Q_strncpyz(exepath, host_parms.argv[0], sizeof(exepath));

		int fd = creat(newexepath, S_IRWXU | S_IRGRP|S_IXGRP);
		write(fd);
		close(fd);
#endif

		startuppending = 2;
		return TRUE;
	}

	return FALSE;
}

void M_Menu_Installer(void)
{
	menu_t *menu;
	struct installermenudata *md;

	Key_Dest_Add(kdm_menu);

	menu = M_CreateMenu(sizeof(*md));
	md = menu->data = (menu+1);

	md->syspath = MC_AddEdit(menu, 0, 160, 64, "Path", "C:/Games/AfterQuake/testinstall/base");//va("c:/%s", fs_manifest->installation));

	//FIXME: add path check display

	MC_AddCommand		(menu, 0, 160, 128, "Install", Installer_Go);
	MC_AddConsoleCommand(menu, 0, 160, 136,	"Cancel", "menu_quit\n");

	menu->selecteditem = (menuoption_t*)md->syspath;
	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 250, 0, menu->selecteditem->common.posy, NULL, false);
	menu->remove = Installer_Remove;
}
#endif
#endif
