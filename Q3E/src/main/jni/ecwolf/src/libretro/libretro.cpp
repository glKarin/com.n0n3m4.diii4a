/*
 *  Copyright (C) 2020 Google
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "wl_def.h"
#include "c_cvars.h"
#include "streams/file_stream.h" // Must be before id_sd.h
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "config.h"
#include "wl_play.h"
#include "wl_net.h"
#include "libretro.h"
#include "retro_dirent.h"
#include "state_machine.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_in.h"
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
#include "am_map.h"
#include "wl_loadsave.h"
#include "compat/msvc.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "farchive.h"
#include "thinker.h"
#include "thingdef/thingdef.h"

static void fallback_log(enum retro_log_level level, const char *fmt, ...);

static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
extern struct retro_vfs_interface *vfs_interface;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_video_refresh_t video_cb;
static retro_log_printf_t log_cb = fallback_log;
static bool libretro_supports_bitmasks = false;
// fp10s is 10 times the FPS
static int screen_width = 640, screen_height = 400, fp10s = 350;
static bool dynamic_fps = false;
static int analog_deadzone;
static int preferred_bpp;
static int bpp;

const int TIC_TIME_US = 1000000 / TICRATE;
static const int SAMPLERATE = 44100;

extern AutoMap AM_Main;
extern AutoMap AM_Overlay;

IVideo *Video = NULL;

wl_state_t g_state;

Aspect r_ratio = ASPECT_4_3, vid_aspect = ASPECT_NONE;
fixed movebob = FRACUNIT;

bool alwaysrun;
bool preload_digital_sounds;
float localDesiredFOV = 90.0f;

class LibretroFBBase : public DFrameBuffer
{
public:
	LibretroFBBase(int width, int height) : DFrameBuffer (width, height) {}
	virtual void ShowFrame() = 0;
};

template <typename color_t> class LibretroFB : public LibretroFBBase
{
public:
	LibretroFB(int width, int height) : LibretroFBBase (width, height) {
		width_ = width;
		height_ = height;
		lr_pitch_ = width_ * sizeof(color_t);
		lr_buffer_ = (color_t *) malloc (height_ * lr_pitch_);
		CHECKMALLOCRESULT(lr_buffer_);
		Buffer = (BYTE *) malloc (height_ * Pitch);
		CHECKMALLOCRESULT(Buffer);
		memcpy (SourcePalette, GPalette.BaseColors, sizeof(PalEntry)*256);
		memset (Buffer, 0, height_ * Pitch);
		PaletteNeedsUpdate = true;
		FlashAmount = 0;
		Flash.r = 0;
		Flash.g = 0;
		Flash.b = 0;
	}
	~LibretroFB () {
		free(lr_buffer_);
		free(Buffer);
	}

	bool Lock (bool buffer) { return true; }
	void Unlock () {}
	bool Relock () { return true; }
	void ForceBuffering (bool force) {}
	bool IsValid () { return true; }
	void Update () {
		ComputePalette();
		for (int y = 0; y < height_; y++)
			for (int x = 0; x < width_; x++)
				lr_buffer_[y * (lr_pitch_ / sizeof(lr_buffer_[0])) + x]
					= effective_palette_[Buffer[y * (Pitch / sizeof (Buffer[0])) + x]];
		ShowFrame();
	}
	void ShowFrame() {
		if (video_cb)
			video_cb(lr_buffer_, width_, height_, lr_pitch_);
		g_state.frame_counter++;
	}
	PalEntry *GetPalette () { return SourcePalette; }
	void GetFlashedPalette (PalEntry pal[256]) {
		ComputePalette();
		memcpy(pal, FlashedPalette, 256 * sizeof (PalEntry));
	}
	void UpdatePalette () {
		PaletteNeedsUpdate = true;
	}
	bool SetGamma (float gamma) { return true; }
	bool SetFlash (PalEntry rgb, int amount) {
		Flash = rgb;
		FlashAmount = amount;
		PaletteNeedsUpdate = true;
		return true;
	}
	void GetFlash (PalEntry &rgb, int &amount) {
		rgb = Flash;
		amount = FlashAmount;
	}
	void SetFullscreen (bool fullscreen) {}
	int GetPageCount () { return 0; }
	bool IsFullscreen () { return false; }

	void PaletteChanged () {
		PaletteNeedsUpdate = true;
	}
	int QueryNewPalette () { return 0; }
	bool Is8BitMode() { return true; }

	void SetVSync (bool vsync) {}
	void ScaleCoordsFromWindow(SWORD &x, SWORD &y) {}
protected:
	PalEntry FlashedPalette[256];
	color_t effective_palette_[256];
	virtual void ComputeEffectivePalette() = 0;
private:
	void ComputePalette() {
		if (!PaletteNeedsUpdate)
			return;
		PaletteNeedsUpdate = false;
		memcpy (FlashedPalette, SourcePalette, 256*sizeof(PalEntry));
		if (FlashAmount)
		{
			DoBlending (FlashedPalette, FlashedPalette, 256, Flash.r, Flash.g, Flash.b, FlashAmount);
		}
		ComputeEffectivePalette();
	}
	color_t *lr_buffer_;
	int lr_pitch_, width_, height_;
	int FlashAmount;
	bool PaletteNeedsUpdate;
	PalEntry Flash;
	PalEntry SourcePalette[256];
};

class LibretroFB32 : public LibretroFB<uint32_t> {
public:
	LibretroFB32 (int width, int height) : LibretroFB<uint32_t> (width, height) {}

protected:
	void ComputeEffectivePalette() {
		for (int i = 0; i < 256; i++) {
#ifdef PS2
			effective_palette_[i] =
				(FlashedPalette[i].b<<16)
				| (FlashedPalette[i].g<<8)
				| (FlashedPalette[i].r);
#else
			effective_palette_[i] =
				(FlashedPalette[i].r<<16)
				| (FlashedPalette[i].g<<8)
				| (FlashedPalette[i].b);
#endif
		}
	}
};

class LibretroFB16 : public LibretroFB<uint16_t> {
public:
	LibretroFB16 (int width, int height) : LibretroFB<uint16_t> (width, height)  {}
protected:
	void ComputeEffectivePalette() {
		for (int i = 0; i < 256; i++) {
#ifdef PS2
			effective_palette_[i] =
				(FlashedPalette[i].b >> 3 << 10)
				| (FlashedPalette[i].g >> 3 << 5)
				| (FlashedPalette[i].r >> 3);
#else
			effective_palette_[i] =
				(FlashedPalette[i].r >> 3 << 11)
				| (FlashedPalette[i].g >> 2 << 5)
				| (FlashedPalette[i].b >> 3);
#endif
		}
	}
};

class LibretroVideo : public IVideo
{
public:
	LibretroVideo () {}
	~LibretroVideo () {}

	EDisplayType GetDisplayType () { return DISPLAY_Both; }
	void SetWindowedScale (float scale) {};

	DFrameBuffer *CreateFrameBuffer (int width, int height, bool fs, DFrameBuffer *old) {
		if (bpp == 32)
			return new LibretroFB32(width, height);
		return new LibretroFB16(width, height);		
	}

	void StartModeIterator (int bits, bool fs) {
		modeCounter = 0;
	}
	bool NextMode (int *width, int *height, bool *letterbox) {
		if (modeCounter >= 1)
			return false;
		*width = screen_width;
		*height = screen_height;
		*letterbox = false;
		modeCounter++;
		return false;
	}
private:
	int modeCounter;
};

bool IVideo::SetResolution (int width, int height, int bits)
{
	int cx1, cx2;
	PalEntry palette[256];


	// Load fonts now so they can be packed into textures straight away,
	// if D3DFB is being used for the display.
	//FFont::StaticPreloadFonts();

	V_CalcCleanFacs(320, 200, width, height, &CleanXfac, &CleanYfac, &cx1, &cx2);

	CleanWidth = width / CleanXfac;
	CleanHeight = height / CleanYfac;
	assert(CleanWidth >= 320);
	assert(CleanHeight >= 200);

	if (screen) {
		memcpy (palette, screen->GetPalette(), sizeof(PalEntry)*256);
	} else {
		memcpy (palette, GPalette.BaseColors, sizeof(PalEntry)*256);
	}

	if (screen) {
		screen->ObjectFlags |= OF_YesReallyDelete;
		delete screen;
	}

	screen = Video->CreateFrameBuffer(width, height, true, NULL);
	GC::WriteBarrier(screen);
	memcpy (screen->GetPalette(), palette, sizeof(PalEntry)*256);
	screen->UpdatePalette();
	
	return true;
}

Net::NetInit Net::InitVars = {
	Net::MODE_SinglePlayer,
	5029, //NET_DEFAULT_PORT,
	1
};


void I_InitGraphics () {
	Video = new LibretroVideo();
}

void I_ShutdownGraphics () {
	if (screen) {
		screen->ObjectFlags |= OF_YesReallyDelete;
		delete screen;
		screen = NULL;
	}
}

static struct retro_frame_time_callback frame_cb;   

static void frame_time_cb(retro_usec_t usec)
{
	if (!dynamic_fps)
		usec = frame_cb.reference;
	g_state.usec += usec;
	g_state.frame_tic = g_state.usec / TIC_TIME_US;
	tics = (usec + g_state.tic_rest) / TIC_TIME_US;
	g_state.tic_rest = (usec + g_state.tic_rest) % TIC_TIME_US;
}

int32_t GetTimeCount()
{
	return g_state.frame_tic;
}


static int set_color_format(void)
{
	enum retro_pixel_format fmt = bpp == 32 ? RETRO_PIXEL_FORMAT_XRGB8888 : RETRO_PIXEL_FORMAT_RGB565;
	return environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
}

int game_init_pixelformat(void)
{
	bpp = preferred_bpp;
	if (set_color_format())
		return 1;
	
	bpp = bpp == 32 ? 16 : 32;

	if (set_color_format())
		return 1;
	
	libretro_log("no color format is not supported.\n");
	return 0;
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
	va_list va;

	(void)level;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

void libretro_log(const char *format, ...)
{
	va_list va;
	char formatted[1024];

	va_start(va, format);
	vsnprintf(formatted, sizeof(formatted) - 1, format, va);
	log_cb(RETRO_LOG_INFO, "%s\n", formatted);
	va_end(va);
}

void Quit ()
{
	struct retro_message msg;

	libretro_log("Fatal error");
	msg.msg    = "Fatal error";
	msg.frames = fp10s;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
	environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
	throw CFatalError("Fatal error");
}

void retro_unload_game()
{
	SoundInfo.Clear();
	CallTerminateFunctions();
}

static char g_wad_dir[1024];
static char g_basename[1024];

static void extract_basename(char *buf, const char *path, size_t size)
{
	const char *base = strrchr(path, '/');
	if (!base)
		base = strrchr(path, '\\');
	if (!base)
		base = path;

	if (*base == '\\' || *base == '/')
		base++;

	strncpy(buf, base, size - 1);
	buf[size - 1] = '\0';
}

static void extract_directory(char *buf, const char *path, size_t size)
{
	char *base;
	strncpy(buf, path, size - 1);
	buf[size - 1] = '\0';

	base = strrchr(buf, '/');
	if (!base)
		base = strrchr(buf, '\\');

	if (base)
		*base = '\0';
	else
	{
		buf[0] = '.';
		buf[1] = '\0';
	}
}

static char* remove_extension(char *buf, const char *path, size_t size)
{
	char *base;
	memcpy(buf, path, size - 1);
	buf[size - 1] = '\0';

	base = strrchr(buf, '.');

	if (base)
		*base = '\0';

	return base+1;
}

int MusicVolume = 20;
int SpeakerVolume = 20;
int AdlibVolume = 20;
int DigiVolume = 20;
int AnalogMoveSensitivity = 20;
int AnalogTurnSensitivity = 20;

static void announce_frame_callback()
{
	frame_cb.callback  = frame_time_cb;
	switch (fp10s) {
	case 700: // 70.0 fps, 1 tic/frame
		frame_cb.reference = TIC_TIME_US;
		break;
	case 350: // 35.0 fps, 2 tics/frame
		frame_cb.reference = 2 * TIC_TIME_US;
		break;
	case 175: // 17.5 fps, 4 tics/frame
		frame_cb.reference = 4 * TIC_TIME_US;
		break;
	case 140: // 14.0 fps, 5 tics/frame
		frame_cb.reference = 5 * TIC_TIME_US;
		break;
	case 100: // 10.0 fps, 7 tics/frame
		frame_cb.reference = 7 * TIC_TIME_US;
		break;
	case 88: // 8.75 fps, 8 tics/frame
		frame_cb.reference = 8 * TIC_TIME_US;
		break;
	case 70: // 7.0 fps, 8 tics/frame
		frame_cb.reference = 10 * TIC_TIME_US;
		break;
	default:
		frame_cb.reference = 10000000 / fp10s;
		break;
	}
	environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_cb);
}

static const char *get_string_variable(const char *name)
{
	struct retro_variable var;
	var.key = name;
	var.value = NULL;

	if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
		return NULL;

	return var.value;
}

static const char *get_string_variable_def(const char *name, const char *def)
{
	const char *ret = get_string_variable(name);
	if (ret)
		return ret;
	return def;
}

static bool get_bool_option(const char *name)
{
	const char *str = get_string_variable (name);

	return (str != NULL && strcmp(str, "enabled") == 0);
}

static unsigned get_unsigned_variable(const char *name, unsigned def)
{
	const char *str = get_string_variable (name);

	if (str == NULL)
		return def;

	return strtoul(str, NULL, 0);
}

static int get_slider_option(const char *name, int def = 20)
{
	const char *str = get_string_variable (name);

	if (str == NULL)
		return 20;
	int ret = strtoul(str, NULL, 0);
	if (ret < 0 || ret > 20)
		return def;
	return ret;
}

static int get_multiple_choice_option(const char *name, const char **values, int def = 0)
{
	int i;
	const char *str = get_string_variable (name);

	if (str == NULL)
		return def;
	for (i = 0; values[i]; i++)
		if (strcmp(str, values[i]) == 0)
			return i;
	return def;
}

static void am_bool_option (const char *name, bool &var, bool &is_updated)
{
	bool newval = get_bool_option(name);
	if (newval != var) {
		var = newval;
		is_updated = true;
	}
}

static void am_multiple_choice (const char *name, unsigned &var, bool &is_updated, const char **values)
{
	int newval = get_multiple_choice_option(name, values);
	if (newval != var) {
		var = newval;
		is_updated = true;
	}
}

static void update_variables(bool startup)
{
#if defined(RS90)
	store_files_in_memory = false;
#elif defined(_3DS) || defined(GEKKO)
	store_files_in_memory = true;
#else
	store_files_in_memory = get_bool_option("ecwolf-memstore");
#endif
#ifdef RS90
	preload_digital_sounds = false;
#else
	preload_digital_sounds = get_bool_option("ecwolf-preload-digisounds");
#endif
	
	int oldw = screen_width;
	int oldh = screen_height;
	int oldfp10s = fp10s;

	const char *resolution = get_string_variable("ecwolf-resolution");

	if (resolution)
	{
		char *pch;
		char str[100];
		memset(str, 0, sizeof(str));
		strncpy(str, resolution, sizeof(str)-2);

		pch = strtok(str, "x");
		if (pch)
			screen_width = strtoul(pch, NULL, 0);
		pch = strtok(NULL, "x");
		if (pch)
			screen_height = strtoul(pch, NULL, 0);

		if (log_cb)
			log_cb(RETRO_LOG_INFO, "Got size: %u x %u.\n", screen_width, screen_height);
	}
	else
	{
		screen_width = 320;
		screen_height = 200;
	}

	const char *fpsstr = get_string_variable_def("ecwolf-fps", "35");
	if (strcmp (fpsstr, "17.5") == 0)
		fp10s = 175;
	else
		fp10s = strtoul(fpsstr, NULL, 0) * 10;
	if (log_cb)
		log_cb(RETRO_LOG_INFO, "Got FPS: %u.%u.\n", fp10s / 10, fp10s % 10);

	if (startup) {
		const char *palette = get_string_variable("ecwolf-palette");

		if (palette != NULL && strcmp(palette, "xrgb8888") == 0)
			preferred_bpp = 32;
		else
			preferred_bpp = 16;
	}

	if (oldw != screen_width || oldh != screen_height || oldfp10s != fp10s)
	{
		struct retro_system_av_info avinfo;
		retro_get_system_av_info(&avinfo);
		environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &avinfo);
		screenWidth = screen_width;
		screenHeight = screen_height;
		if (screen) {
			VL_SetVGAPlaneMode();
			if(playstate)
			{
				DrawPlayScreen();
			}
			IN_AdjustMouse();
		}
	}
	if (oldfp10s != fp10s)
	{
		announce_frame_callback();
	}

	alwaysrun = get_bool_option("ecwolf-alwaysrun");

	analog_deadzone = get_unsigned_variable("ecwolf-analog-deadzone", 15);

	int newviewsize = get_unsigned_variable("ecwolf-viewsize", 20);
	if (newviewsize < 4 || newviewsize > 21)
		newviewsize = 20;
	if (newviewsize != viewsize) {
		if (g_state.stage == PLAY_STEP_A ||
		    g_state.stage == PLAY_STEP_B) {
			NewViewSize (newviewsize);
			DrawPlayScreen (false);
		} else {
			viewsize = newviewsize;
		}
	}

	bool am_updated = false;

	static const char *overlay_options[] = {"off", "on", "both", NULL};
	static const char *rotate_options[] = {"off", "on", "overlay_only", NULL};
	am_multiple_choice ("ecwolf-am-overlay", am_overlay, am_updated, overlay_options);
	am_multiple_choice ("ecwolf-am-rotate", am_rotate, am_updated, rotate_options);
	am_bool_option ("ecwolf-am-drawtexturedwalls", am_drawtexturedwalls, am_updated);
	am_bool_option ("ecwolf-am-drawtexturedfloors", am_drawfloors, am_updated);
	am_bool_option ("ecwolf-am-texturedoverlay", am_overlaytextured, am_updated);
	am_bool_option ("ecwolf-am-showratios", am_showratios, am_updated);
	am_bool_option ("ecwolf-am-pause", am_pause, am_updated);
	if (am_updated)
		AM_UpdateFlags();

#ifndef DISABLE_ADLIB
	MusicVolume = get_slider_option("ecwolf-music-volume");
	AdlibVolume = get_slider_option("ecwolf-adlib-volume");
#else
	MusicVolume = 0;
	AdlibVolume = 0;
#endif
	SpeakerVolume = get_slider_option("ecwolf-speaker-volume");
	DigiVolume = get_slider_option("ecwolf-digi-volume");
	AnalogMoveSensitivity = get_slider_option ("ecwolf-analog-move-sensitivity");
	AnalogTurnSensitivity = get_slider_option ("ecwolf-analog-turn-sensitivity");

	SetSoundPriorities(get_string_variable_def("ecwolf-effects-priority", "digi-adlib-speaker"));

	godmode = get_bool_option("ecwolf-invulnerability");
	dynamic_fps = get_bool_option("ecwolf-dynamic-fps");

	const char *aspect = get_string_variable ("ecwolf-aspect");

	if (aspect == NULL) {
		vid_aspect = ASPECT_NONE;
	} else {
		vid_aspect = ASPECT_NONE;
		if (strcmp(aspect, "16:9") == 0)
			vid_aspect = ASPECT_16_9;
		if (strcmp(aspect, "16:10") == 0)
			vid_aspect = ASPECT_16_10;
		if (strcmp(aspect, "17:10") == 0)
			vid_aspect = ASPECT_17_10;
		if (strcmp(aspect, "4:3") == 0)
			vid_aspect = ASPECT_4_3;
		if (strcmp(aspect, "5:4") == 0)
			vid_aspect = ASPECT_5_4;
		if (strcmp(aspect, "21:9") == 0)
			vid_aspect = ASPECT_64_27;
	}

	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));

	panxadjustment = get_slider_option("ecwolf-panx-adjustment", 5);
	panyadjustment = get_slider_option("ecwolf-pany-adjustment", 5);
}

void ScannerMessageHandler(Scanner::MessageLevel level, const char *error, va_list list)
{
	FString errorMessage;
	errorMessage.VFormat(error, list);

	if(level == Scanner::ERROR)
		throw CRecoverableError(errorMessage);
	else
		printf("%s", errorMessage.GetChars());
}

extern const void *PullerADeathCam;
extern const void *PullerAQuizItem;
extern const void *PullerAWeaponGiver;
extern const void *PullerAScoreItem;
extern const void *PullerAExtraLifeItem;
extern const void *PullerAMapRevealer;
extern const void *PullerAInventory;
extern const void *PullerAAmmo;
extern const void *PullerABackpackItem;
extern const void *PullerACustomInventory;
extern const void *PullerAHealth;
extern const void *PullerAWeapon;
extern const void *PullerAKeyGiver;
extern const void *PullerAKey;
extern const void *PullerAPatrolPoint;
extern const void *PullerAPlayerPawn;
extern const void *PullerARandomSpawner;
extern const void *PullerASpearOfDestiny;
bool __AF_A_InitSmartAnim(AActor *, AActor *, const Frame * const, const CallArguments &, struct ActionResult *);

const void * pullers[] = {
	PullerADeathCam,
	PullerAQuizItem,
	PullerAWeaponGiver,
	PullerAScoreItem,
	PullerAExtraLifeItem,
	PullerAMapRevealer,
	PullerAInventory,
	PullerAAmmo,
	PullerABackpackItem,
	PullerACustomInventory,
	PullerAHealth,
	PullerAWeapon,
	PullerAKeyGiver,
	PullerAKey,
	PullerAPatrolPoint,
	PullerAPlayerPawn,
	PullerARandomSpawner,
	PullerASpearOfDestiny,
	(void*) __AF_A_InitSmartAnim
 };

bool try_retro_load_game(const struct retro_game_info *info, size_t num_info)
{
	Scanner::SetMessageHandler(ScannerMessageHandler);
	update_variables(true);

	if (!game_init_pixelformat())
		return false;

	announce_frame_callback();

	ReadConfig();

	{
		TArray<FString> wadfiles, files;
     
		Printf("IWad: Selecting base game data.\n");
		char name_without_ext[1023];

		extract_directory(g_wad_dir, info->path, sizeof(g_wad_dir));
		extract_basename(g_basename, info->path, sizeof(g_basename));
		const char *extension = remove_extension(name_without_ext, g_basename, sizeof(name_without_ext));
		config.CreateSetting("BaseDataPaths", g_wad_dir);
		const char* sysDir = 0;
		environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &sysDir);
		IWad::SelectGame(files, extension, MAIN_PK3, sysDir);

		for (size_t i = 0; i < num_info; i++)
			if (info[i].path)
				files.Push(info[i].path);

		printf("W_Init: Init WADfiles.\n");
		Wads.InitMultipleFiles(files);
		LumpRemapper::RemapAll();
		language.SetupStrings();
	}
   
	thinkerList.DestroyAll(static_cast<ThinkerList::Priority>(0));

	R_InitRenderer();

	log_cb (RETRO_LOG_INFO, "InitGame: Setting up the game...\n");
	rngseed = I_MakeRNGSeed(); // May change after initializing a net game
	InitGame();

	log_cb (RETRO_LOG_INFO, "Clearing random...\n");

	FRandom::StaticClearRandom();

	g_state.clear();

	log_cb (RETRO_LOG_INFO, "Game setup finished...\n");

	return true;
}

bool retro_load_game(const struct retro_game_info *info)
{
	try
	{
		return try_retro_load_game(info, 1);
	}
	catch (CDoomError &err) {
		log_cb (RETRO_LOG_ERROR, "Error loading game: %s\n", err.GetMessage());
		return false;
	} catch (...) {
		log_cb (RETRO_LOG_ERROR, "Unknown exception while loading game\n");
		return false;	  
	}
}

void IVideo::DumpAdapters ()
{
	Printf("Multi-monitor support unavailable.\n");
}

static int transform_axis(int val, int run, int sensitivity)
{
	int sign = val >= 0 ? +1 : -1;
	int absval = val >= 0 ? val : -val;
	int deadzone_val = analog_deadzone * 0x7fff / 100;
	if (absval < deadzone_val)
		return 0;
	return sign * ((absval - deadzone_val) * 5 * (1 + run) * sensitivity)
		/ (0x7fff - deadzone_val);
}

static void TransformAutomapInputs(const wl_input_state_t *input)
{
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_L))
	{
		AM_Overlay.SetScale(FRACUNIT*135/128, true);
		AM_Main.SetScale(FRACUNIT*135/128, true);
	}
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_R))
	{
		AM_Overlay.SetScale(FRACUNIT*122/128, true);
		AM_Main.SetScale(FRACUNIT*122/128, true);
	}

	const fixed PAN_AMOUNT = FixedDiv(FRACUNIT*10, AM_Main.GetScreenScale());
	const fixed PAN_ANALOG_MULTIPLIER = PAN_AMOUNT/100;
	fixed panx = 0, pany = 0;

	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_LEFT))
		panx += PAN_AMOUNT;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_RIGHT))
		panx -= PAN_AMOUNT;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_UP))
		pany += PAN_AMOUNT;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_DOWN))
		pany -= PAN_AMOUNT;
	panx -= transform_axis(input->rsx, false, AnalogMoveSensitivity) * PAN_ANALOG_MULTIPLIER;
	pany -= transform_axis(input->lsy, false, AnalogMoveSensitivity) * PAN_ANALOG_MULTIPLIER;
	panx -= transform_axis(input->lsx, false, AnalogMoveSensitivity) * PAN_ANALOG_MULTIPLIER;
	AM_Main.SetPanning(panx, pany, true);
}

void TransformPlayInputs(const wl_input_state_t *input, int newly_pressed)
{
	TicCmd_t &cmd = control[ConsolePlayer];
	int runbutton = !!(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_X));
	int delta = (runbutton == !alwaysrun) ? RUNMOVE : BASEMOVE;
	cmd.controlx = transform_axis(input->rsx, runbutton, AnalogTurnSensitivity);
	cmd.controly = transform_axis(input->lsy, runbutton, AnalogMoveSensitivity);
	cmd.controlstrafe = transform_axis(input->lsx, runbutton, AnalogMoveSensitivity);
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_UP))
		cmd.controly = -delta;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_DOWN))
		cmd.controly = +delta;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_LEFT))
		cmd.controlx = -delta;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_RIGHT))
		cmd.controlx = +delta;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_L))
		cmd.controlstrafe = -delta;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_R))
		cmd.controlstrafe = +delta;

	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_A))
		cmd.buttonstate[bt_attack] = 1;
	if(newly_pressed & (1<<RETRO_DEVICE_ID_JOYPAD_B))
		cmd.buttonstate[bt_use] = 1;
	if(input->button_mask & (1<<RETRO_DEVICE_ID_JOYPAD_Y))
		cmd.buttonstate[bt_showstatusbar] = 1;
	if(newly_pressed & (1<<RETRO_DEVICE_ID_JOYPAD_L2))
		cmd.buttonstate[bt_prevweapon] = 1;
	if(newly_pressed & (1<<RETRO_DEVICE_ID_JOYPAD_R2))
		cmd.buttonstate[bt_nextweapon] = 1;

	/*
	  bt_strafe,
	  bt_slot0,
	  bt_slot1,
	  bt_slot2,
	  bt_slot3,
	  bt_slot4,
	  bt_slot5,
	  bt_slot6,
	  bt_slot7,
	  bt_slot8,
	  bt_slot9,
	  bt_esc,

	  bt_altattack,
	  bt_reload,
	  bt_zoom,
	  bt_showstatusbar,
	  NUMBUTTONS,


	*/
}

// TODO: mouse, keyboard
static void TransformInputs(wl_input_state_t *input)
{
	TicCmd_t &cmd = control[ConsolePlayer];
	static int old_button_mask = 0;

	cmd.controlx = 0;
	cmd.controly = 0;
	cmd.controlpanx = 0;
	cmd.controlpany = 0;
	cmd.controlstrafe = 0;
	memcpy (cmd.buttonheld, cmd.buttonstate, sizeof (cmd.buttonstate));
	memset (cmd.buttonstate, 0, sizeof (cmd.buttonstate));

	int newly_pressed = ~old_button_mask & input->button_mask;
	old_button_mask = input->button_mask;
	input->screenAcked = !!newly_pressed;

	input->menuEnter = !!(newly_pressed & (1<<RETRO_DEVICE_ID_JOYPAD_A));
	input->menuBack = !!(newly_pressed & (1<<RETRO_DEVICE_ID_JOYPAD_B));
	input->pauseToggled = !!(newly_pressed & (1 << RETRO_DEVICE_ID_JOYPAD_START));

	switch (newly_pressed & 0xf0) {
	case (1<<RETRO_DEVICE_ID_JOYPAD_LEFT):
		input->menuDir = dir_West;
		break;
	case (1<<RETRO_DEVICE_ID_JOYPAD_RIGHT):
		input->menuDir = dir_East;
		break;	  
	case (1<<RETRO_DEVICE_ID_JOYPAD_UP):
		input->menuDir = dir_North;
		break;
	case (1<<RETRO_DEVICE_ID_JOYPAD_DOWN):
		input->menuDir = dir_South;
		break;
	case (1<<RETRO_DEVICE_ID_JOYPAD_UP) | (1<<RETRO_DEVICE_ID_JOYPAD_LEFT):
		input->menuDir = dir_NorthEast;
		break;
	case (1<<RETRO_DEVICE_ID_JOYPAD_UP) | (1<<RETRO_DEVICE_ID_JOYPAD_RIGHT):
		input->menuDir = dir_NorthWest;
		break;
	case (1<<RETRO_DEVICE_ID_JOYPAD_DOWN) | (1<<RETRO_DEVICE_ID_JOYPAD_LEFT):
		input->menuDir = dir_SouthEast;
		break;
	case (1<<RETRO_DEVICE_ID_JOYPAD_DOWN) | (1<<RETRO_DEVICE_ID_JOYPAD_RIGHT):
		input->menuDir = dir_SouthWest;
		break;
	default:
		input->menuDir = dir_None;
		break;
	}

	if((newly_pressed & (1<<RETRO_DEVICE_ID_JOYPAD_SELECT))) {
		AM_Toggle();
		return;
	}

	if (automap == AMA_Normal && am_pause)
		TransformAutomapInputs(input);
	else
		TransformPlayInputs(input, newly_pressed);
}

static void poll_inputs(wl_input_state_t *input)
{
	input_poll_cb();
	
	input->button_mask = 0;

	if (libretro_supports_bitmasks)
		input->button_mask = input_state_cb(0, RETRO_DEVICE_JOYPAD,
						    0, RETRO_DEVICE_ID_JOYPAD_MASK);
	else
	{
		unsigned i;
		for (i = 0; i < 16; i++)
		{
			if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
				input->button_mask |= (1 << i);
		}
	}

	input->lsx = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	input->lsy = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	input->rsx = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
}

static int16_t soundbuf[SAMPLERATE / TICRATE * 2];

#define TO_SDL_POSITION(pos) (((64 - ((pos) * (pos))) * 3) + 63)

static void mixChannel(long long tic, SoundChannelState *channel)
{
	if (!channel->isPlaying(tic))
		return;
	int volume = 20;
	if (channel->isMusic)
		volume = MusicVolume;
	else
		switch (channel->type) {
		case SoundData::DIGITAL:
			volume = DigiVolume;
			break;
		case SoundData::ADLIB:
			volume = AdlibVolume;
			break;
		case SoundData::PCSPEAKER:
			volume = SpeakerVolume;
			break;			
		}
	if (volume == 0)
		return;
	int leftpos = channel->leftPos;
	int rightpos = channel->rightPos;
	if ((leftpos < 0) || (leftpos > 15) || (rightpos < 0) || (rightpos > 15)
	    || ((leftpos == 15) && (rightpos == 15))) {
		leftpos = 0;
		rightpos = 0;
	}
	int sdlleft = TO_SDL_POSITION(leftpos);
	int sdlright = TO_SDL_POSITION(rightpos);
	if (sdlleft < 0)
		sdlleft = 0;
	if (sdlright < 0)
		sdlright = 0;	
	fixed leftmul = (FRACUNIT * volume * sdlleft) / (20 * 255);
	fixed rightmul = (FRACUNIT * volume * sdlright) / (20 * 255);
	if (leftmul == 0 && rightmul == 0)
		return;
	int start_tic = tic - channel->startTick + channel->skipTicks;

	channel->sample->MixInto (soundbuf, SAMPLERATE, SAMPLERATE / TICRATE, start_tic, leftmul, rightmul);
}

#define MB(x) ((x) << 20)

size_t limit_sound_cache_size =
#ifdef RS90
	MB(5)
#else
	MB(15)
#endif
	;

void generate_audio(long long tic)
{
	memset (soundbuf, 0, sizeof(soundbuf));
	touched_sound_size = 0;
	for (int channelno = 0; channelno < MIX_CHANNELS; channelno++) {
		mixChannel(tic, &g_state.channels[channelno]);
	}
	if (MusicVolume != 0)
		mixChannel(tic, &g_state.musicChannel);
	audio_batch_cb(soundbuf, SAMPLERATE / TICRATE);
	if (!preload_digital_sounds) {
		// We don't want to keep dropping and reloading the same files every frame
		if (limit_sound_cache_size <= (touched_sound_size * 3) / 2) {
			limit_sound_cache_size = (touched_sound_size * 3) / 2;
#ifdef RS90
			if (limit_sound_cache_size >= MB(7))
				limit_sound_cache_size = MB(7);
#endif		
		}

		if (loaded_sound_size > limit_sound_cache_size) {
			decreaseSoundCache(limit_sound_cache_size);
		}
		
	}
}

void generate_silent_audio(void)
{
	memset (soundbuf, 0, sizeof(soundbuf));
	audio_batch_cb(soundbuf, SAMPLERATE / TICRATE);
}


void retro_run(void)
{
	int expectframes = 0;
	wl_input_state_t input;
	long long framestarttic = GetTimeCount();

	// When we load something we end up having very slow
	// retro_run. Then next invocation to retro_run
	// tries to cover the same length of time (tics) with a
	// single frame and then we're stuck in slow fps until user
	// pauses. Limit this by decreasing tics
	// Over 20 or in init stage is definitely after load
	if (tics > 20 || (g_state.stage == BEFORE_NON_SHAREWARE && tics > 3))
		tics = 3;
	// Cap at 5 (14 fps)
	if (tics > 5)
		tics = 5;

	unsigned frametics = tics;

	if (tics == 0) {
		((LibretroFBBase *)screen)->ShowFrame();
		return;
	}

	bool updated = false;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
		update_variables(false);

	poll_inputs(&input);
	TransformInputs(&input);
	if (input.pauseToggled) {
		Paused ^= 1;
	}
	if (Paused & 1) {
		VWB_DrawGraphic(TexMan("PAUSED"), (20 - 4)*8, 80 - 2*8);
		VH_UpdateScreen();
		generate_silent_audio();
		return;
	}
	do
	{
		int oldfc = g_state.frame_counter;
		wl_stage_t oldstage = g_state.stage;
		bool oldQuiz = g_state.isInQuiz;
		expectframes = !!TopLoopStep(&g_state, &input);
		if (expectframes != (int) (g_state.frame_counter - oldfc)) {
			fprintf(stderr, "State %d[%d] produces %d frames but reports %d\n", oldstage, oldQuiz,
				(int) (g_state.frame_counter - oldfc), expectframes);
		}
	}
	while (expectframes == 0);

	assert(frametics > 0);
	for (unsigned i = 0; i < frametics; i++)
		generate_audio(framestarttic + i);
}


void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name     = "ecwolf";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
	info->library_version  = "v0.01" GIT_VERSION;
	info->need_fullpath    = true;
	info->valid_extensions = "wl6|n3d|sod|sdm|wl1|pk3";
	info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	memset(info, 0, sizeof(*info));
	info->timing.fps            = (double) fp10s / 10.0;
	info->timing.sample_rate    = (double) SAMPLERATE;

	info->geometry.base_width   = screen_width;
	info->geometry.base_height  = screen_height;
	info->geometry.max_width    = screen_width;
	info->geometry.max_height   = screen_height;
	info->geometry.aspect_ratio = 0.0;
}

char *my_asnprintf(size_t maxlen, const char *fmt, ...)
{
	va_list va;

	char * ret = (char *) malloc (maxlen + 1);
	CHECKMALLOCRESULT(ret);
	va_start(va, fmt);
	vsnprintf(ret, maxlen, fmt, va);
	va_end(va);

	return ret;
}

#define MAXNROMS 5

#include "libretro_core_options.h"

void retro_set_environment(retro_environment_t cb)
{
	struct retro_log_callback logging;
	bool no_rom = false;
	static struct retro_subsystem_info subsys[MAXNROMS];
	char *descs[MAXNROMS];
      
	memset (subsys, 0, sizeof(subsys));
	for (int i = 0; i < MAXNROMS; i++) {
		descs[i] = my_asnprintf (45, "Additional pack %d", i+1);
	}

	for (int i = 0; i < MAXNROMS - 1; i++)
	{
		int nroms = i + 1;
		subsys[i].desc = my_asnprintf (45, "Load with %d packs", nroms);
		subsys[i].ident = my_asnprintf (45, "multipack-%d", nroms);;
		subsys[i].num_roms = nroms;
		subsys[i].id = nroms;
		struct retro_subsystem_rom_info *rom_info = (struct retro_subsystem_rom_info *) malloc(sizeof(rom_info[0]) * nroms);
		CHECKMALLOCRESULT(rom_info);
		subsys[i].roms = rom_info;
		memset (rom_info, 0, sizeof(rom_info[0]) * nroms);
		rom_info[0].desc = "Main pack";
		rom_info[0].valid_extensions = "wl6|n3d|sod|sdm|wl1|pk3";
		rom_info[0].need_fullpath = true;
		rom_info[0].required = true;

		for (int j = 1; j < nroms; j++) {
			rom_info[j].desc = descs[j-1];
			rom_info[j].valid_extensions = "pk3|bin";
			rom_info[j].need_fullpath = true;
			rom_info[j].required = false;
		}
	}

	cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, subsys);

	environ_cb = cb;

	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

	if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
		log_cb = logging.log;
	else
		log_cb = fallback_log;

	libretro_set_core_options(cb);

	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Forward" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Backwards" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,  "Strafe Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Strafe Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,  "Fire" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Use" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Run" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Show Status" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Map" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,  "Previous weapon" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Next weapon" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Strafe" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Move" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Turn" },
		{ 0 },
	};

	cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

	struct retro_vfs_interface_info vfs_interface_info;
	vfs_interface_info.required_interface_version = 3;
	vfs_interface_info.iface = NULL;
	if (cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_interface_info)) {
		vfs_interface = vfs_interface_info.iface;
		filestream_vfs_init(&vfs_interface_info);
		dirent_vfs_init(&vfs_interface_info);
	}
}

void ControlMenuItem::draw()
{
}

void ControlMenuItem::activate()
{
}

void retro_init()
{
	if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
		libretro_supports_bitmasks = true;
}

void retro_deinit()
{
}

unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	(void)port;
	(void)device;
}

void retro_reset(void)
{
}

size_t retro_serialize_size(void)
{
	return 0x80000;
}

unsigned long long GetSaveVersion()
{
	return 1582131136ull; // TODO use git time like standalone
}

extern unsigned automap;

void SerializeExtra(FArchive &arc, bool &isGameless, DWORD &version)
{
	DWORD serialize_version = 12;
	arc << serialize_version;
	version = serialize_version;
	arc << (QWORD &) GameSave::SaveVersion;
	arc << GameSave::SaveProdVersion;

	arc << (DWORD &) g_state.stage;
	arc << (DWORD &) g_state.stageAfterIntermission;
	arc << g_state.transitionSlideshow;
	arc << g_state.isInQuiz;
	arc << g_state.isFading;
	arc << g_state.isInWait;
	arc << g_state.waitCanBeAcked;
	arc << g_state.waitCanTimeout;
	arc << g_state.wasAcked;
	arc << g_state.ackTimeout;
	arc << g_state.fadeStep;
	arc << g_state.fadeStart;
	arc << g_state.fadeEnd;
	arc << g_state.fadeCur;
	arc << g_state.fadeRed;
	arc << g_state.fadeGreen;
	arc << g_state.fadeBlue;
	arc << g_state.died;
	arc << g_state.dointermission;
	arc << g_state.playing_title_music;

	arc << g_state.episode_num;
	arc << g_state.skill_num;
	arc << g_state.menuLevel;
	for (int i = 0; i < MAX_MENU_STACK; i++)
		arc << (DWORD &) g_state.menuStack[i];

	arc << g_state.frame_counter;
	arc << g_state.frame_tic;
	arc << g_state.tic_rest;
	arc << g_state.usec;

	arc << g_state.intermission.step;
	arc << g_state.intermission.demoMode;
	arc << g_state.intermission.intermission_name;
	arc << g_state.intermission.gototitle;
	arc << g_state.intermission.finishing;
	arc << g_state.intermission.finished;
	arc << g_state.intermission.ret_value;
	arc << g_state.intermission.image_ready;
	arc << g_state.intermission.fade_in;
	arc << g_state.intermission.fade_out;
	arc << g_state.intermission.fade_steps;
	arc << g_state.intermission.wait;
	arc << g_state.nextMap;
	arc << g_state.isCounting;
	arc << g_state.isCountingRatio;
	arc << g_state.prevCount;
	arc << g_state.countCurrent;
	arc << g_state.countEnd;
	arc << g_state.countStep;
	arc << g_state.countX;
	arc << g_state.countY;	
	arc << g_state.countFrame;
	arc << g_state.bonusFont;
	arc << g_state.intermissionSndFreq;
	arc << g_state.intermissionSound;
	PalEntry flash = PalEntry(0,0,0);
	int amount = 0;
	if (arc.IsStoring() && screen) {
		screen->GetFlash(flash, amount);
	}
	arc << flash;
	arc << (int32_t &) amount;
	if (serialize_version >= 10) {
		if (screen) {
			PalEntry *palette = screen->GetPalette();
			for (int i = 0; i < 256; i++)
				arc << palette[i];
		} else {
			PalEntry dummy;
			for (int i = 0; i < 256; i++)
				arc << dummy;
		}
	}

	if (!arc.IsStoring() && screen) {
		screen->SetFlash(flash, amount);
	}

	if (serialize_version >= 5) {
		int32_t num_channels = MIX_CHANNELS;
		arc << num_channels;
		for (int i = 0; i < num_channels; i++)
			g_state.channels[i].Serialize(arc);
		g_state.musicChannel.Serialize(arc);
	}

	if (serialize_version >= 6) {
		arc << g_state.iangle;
		arc << g_state.rndval;
	}

	if (serialize_version >= 7) {
		arc << damagecount;
		arc << bonuscount;
		arc << palshifted;
	}

	if (serialize_version >= 8) {
		arc << g_state.zoomer;
	}

	if (serialize_version >= 9) {
		arc << g_state.firstpage;
		arc << g_state.newpage;
		arc << g_state.article;
		arc << g_state.textposition;
		arc << g_state.pagenum;
		arc << g_state.numpages;
		arc << g_state.rowon;
		arc << g_state.fontcolor;
		arc << g_state.picx;
		arc << g_state.picy;
		arc << g_state.picdelay;
		arc << g_state.layoutdone;
	}

	if (serialize_version >= 11) {
		arc << automap;
		arc << Paused;
	}
	arc << (DWORD &) playstate;
	isGameless = map == NULL;
	arc << isGameless;
}

bool retro_serialize(void *data_, size_t size)
{
	bool isGameless;
	GameSave::SaveVersion = GetSaveVersion();
	GameSave::SaveProdVersion = SAVEPRODVER;

	// If we get hubs this will need to be moved so that we can have multiple of them
	FCompressedMemFile snapshot;
	snapshot.Open();
	{
		DWORD serialize_version;
		FArchive arc(snapshot);
		SerializeExtra(arc, isGameless, serialize_version);

		FString mapname = gamestate.mapname;
		arc << mapname;
		if (!isGameless) {
			GameSave::Serialize(arc);
		}
		if (!isGameless && serialize_version >= 12) {
			Dialog::quizSerialize(&g_state, arc);
		}
		DWORD rngcount = FRandom::GetRNGCount();
		arc << rngcount;
		FRandom::StaticWriteRNGState(arc);
	}
	
	unsigned int outsize = snapshot.GetSerializedSize();
	if (outsize > size)
		return false;
	snapshot.SerializeToBuffer(data_);

	return true;
}

extern byte *vbuf;
extern unsigned vbufPitch;
void R_RenderView();

void
redrawPlay(wl_state_t *state) {
	if (map == NULL)
		return;
	if (!(state->stage == PLAY_STEP_A ||
	      state->stage == PLAY_STEP_B ||
	      (state->stage >= DIED1 && state->stage <= DEATH_FIZZLE)))
		return;
	DrawPlayScreen (false);

	// Play states will redraw it themselves
	if (state->stage == PLAY_STEP_A ||
	    state->stage == PLAY_STEP_B)
		return;
	// Ensure we have a valid camera
	if(players[ConsolePlayer].camera == NULL)
		players[ConsolePlayer].camera = players[ConsolePlayer].mo;

	//
	// clear out the traced array
	//
	map->ClearVisibility();

	vbuf = VL_LockSurface();
	if(vbuf != NULL) {
		vbuf += screenofs;
		vbufPitch = SCREENPITCH;
			
		R_RenderView();
			
		VL_UnlockSurface();
		vbuf = NULL;
	}
}

bool retro_unserialize(const void *data_, size_t size)
{
	bool isGameless;
	// If we get hubs this will need to be moved so that we can have multiple of them
	FCompressedMemFile snapshot;
	snapshot.Open((void *)data_);
	FArchive arc(snapshot);
	DWORD serialize_version;
	SerializeExtra(arc, isGameless, serialize_version);
	FString mapname;
	arc << mapname;
	strcpy(gamestate.mapname, mapname);
	thinkerList.DestroyAll(ThinkerList::TRAVEL);
	if (!isGameless) {
		loadedgame = true;
		SetupGameLevel();
		GameSave::Serialize(arc);
		loadedgame = false;
	}

	if (!isGameless && serialize_version >= 12) {
		Dialog::quizSerialize(&g_state, arc);
	}

	DWORD rngcount;
	arc << rngcount;
	FRandom::StaticReadRNGState(arc, rngcount);

	players[0].PendingWeapon = WP_NOCHANGE;

	redrawPlay(&g_state);

	return true;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	(void)type;
	try
	{
		return try_retro_load_game(info, num);
	}
	catch (CDoomError &err) {
		log_cb (RETRO_LOG_ERROR, "Error loading game: %s\n", err.GetMessage());
		return false;
	}
}

void *retro_get_memory_data(unsigned id)
{
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	return 0;
}

void VL_Fade (int start, int end, int red, int green, int blue, int steps)
{
	State_Fade(&g_state, start, end, red, green, blue, steps);
	printf ("Invalid fade\n");
}

int I_PickIWad (WadStuff *wads, int numwads, bool showwin, int defaultiwad)
{
	return defaultiwad;
}

/*
====================
=
= ReadConfig
=
====================
*/

void ReadConfig(void)
{
	config.CreateSetting("DesiredFOV", localDesiredFOV);
	config.CreateSetting("MoveBob", FRACUNIT);

	localDesiredFOV = clamp<float>(static_cast<float>(config.GetSetting("DesiredFOV")->GetFloat()), 45.0f, 180.0f);
	movebob = config.GetSetting("MoveBob")->GetInteger();

	char hsName[50];
	char hsScore[50];
	char hsCompleted[50];
	char hsGraphic[50];
	for(unsigned int i = 0;i < MaxScores;i++)
	{
		sprintf(hsName, "HighScore%u_Name", i);
		sprintf(hsScore, "HighScore%u_Score", i);
		sprintf(hsCompleted, "HighScore%u_Completed", i);
		sprintf(hsGraphic, "HighScore%u_Graphic", i);

		config.CreateSetting(hsName, Scores[i].name);
		config.CreateSetting(hsScore, (int)Scores[i].score);
		config.CreateSetting(hsCompleted, Scores[i].completed);
		config.CreateSetting(hsGraphic, Scores[i].graphic);

		strcpy(Scores[i].name, config.GetSetting(hsName)->GetString());
		Scores[i].score = config.GetSetting(hsScore)->GetInteger();
		if(config.GetSetting(hsCompleted)->GetType() == SettingsData::ST_STR)
			Scores[i].completed = config.GetSetting(hsCompleted)->GetString();
		else
			Scores[i].completed.Format("%d", config.GetSetting(hsCompleted)->GetInteger());
		strncpy(Scores[i].graphic, config.GetSetting(hsGraphic)->GetString(), 8);
		Scores[i].graphic[8] = 0;
	}

	// Propogate localDesiredFOV to players[0]
	players[0].SetFOV(localDesiredFOV);
}

void I_FatalError (const char *format, ...)
{
        struct retro_message msg;
	va_list vlist;
	va_start(vlist, format);
	FString error;
	error.VFormat(format, vlist);
	va_end(vlist);

	msg.msg    = error.GetChars();
	msg.frames = 360;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, (void*)&msg);

	environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, 0);
	throw CFatalError(error);
}
