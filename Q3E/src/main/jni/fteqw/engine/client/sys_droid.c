#include <jni.h>
#include <errno.h>

#include <android/log.h>

#include "quakedef.h"
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "glquake.h"

#ifndef ANDROID
#error ANDROID wasnt defined
#endif

#include <android/keycodes.h>
#include <android/native_window_jni.h>

#ifndef isDedicated
#ifdef SERVERONLY
qboolean isDedicated = true;
#else
qboolean isDedicated = false;
#endif
#endif
extern int r_blockvidrestart;
float sys_dpi_x, sys_dpi_y;
static void *sys_memheap;
//static unsigned int vibrateduration;
static char sys_binarydir[MAX_OSPATH];
static char sys_basedir[MAX_OSPATH];
static char sys_basepak[MAX_OSPATH];
extern  jmp_buf 	host_abort;
extern qboolean r_forceheadless;
static qboolean r_forcevidrestart;
extern ANativeWindow *sys_nativewindow; //karin: dup symbol

//cvar_t sys_vibrate = CVARFD("sys_vibrate", "1", CVAR_ARCHIVE, "Enables the system vibrator for damage events and such things. The value provided is a duration scaler.");
static cvar_t sys_osk = CVAR("sys_osk", "0");	//to be toggled
static cvar_t sys_keepscreenon = CVARFD("sys_keepscreenon", "1", CVAR_ARCHIVE, "If set, the screen will never darken. This might cost some extra battery power, but then so will running a 3d engine.");	//to be toggled
cvar_t sys_orientation = CVARFD("sys_orientation", "landscape", CVAR_ARCHIVE, "Specifies what angle to render quake at.\nValid values are: sensor (autodetect), landscape, portrait, reverselandscape, reverseportrait");
extern cvar_t vid_conautoscale;
void VID_Register(void);

static qboolean sys_wantshutdown;
static JavaVM* sys_javavm;
static jobject *sys_activity;
static jobject *sys_cursurface;	//surface we're currently trying to draw to
static jobject *sys_cursholder; //silly android junk
static jobject *sys_newsurface;	//surface we're meant to be switching our gl context to
static jobject *sys_newsholder; //silly android junk
static void *sys_mainthread;
static void *sys_mainconditional;

#undef LOGI
#undef LOGW
#undef LOGE
#ifndef LOGI
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, DISTRIBUTION"Droid", __VA_ARGS__))
#endif
#ifndef LOGW
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, DISTRIBUTION"Droid", __VA_ARGS__))
#endif
#ifndef LOGE
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, DISTRIBUTION"Droid", __VA_ARGS__))
#endif

void INS_Move(void)
{
}
void INS_Commands(void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
}
void INS_Init(void)
{
}
void INS_ReInit(void)
{
}
void INS_Shutdown(void)
{
}
enum controllertype_e INS_GetControllerType(int id)
{
	return CONTROLLER_NONE;
}
void INS_Rumble(int joy, uint16_t amp_low, uint16_t amp_high, uint32_t duration)
{
}
void INS_RumbleTriggers(int joy, uint16_t left, uint16_t right, uint32_t duration)
{
}
void INS_SetLEDColor(int id, vec3_t color)
{
}
void INS_SetTriggerFX(int id, const void *data, size_t size)
{
}
void Sys_Vibrate(float count)
{
//	if (count < 0)
//		count = 0;
//	vibrateduration += count*10*sys_vibrate.value;
}

qboolean INS_KeyToLocalName(int qkey, char *buf, size_t bufsize)
{	//onscreen keyboard? erk.
	return false;
}
static int mapkey(int androidkey)
{
	switch(androidkey)
	{
	case AKEYCODE_SOFT_LEFT:	return K_LEFTARROW;
	case AKEYCODE_SOFT_RIGHT:	return K_RIGHTARROW;
	case AKEYCODE_HOME:			return K_HOME;	//not quite right, but w/e
	case AKEYCODE_BACK:			return K_ESCAPE;
//	case AKEYCODE_CALL:			return K_;
//	case AKEYCODE_ENDCALL:		return K_;
	case AKEYCODE_0:			return '0';
	case AKEYCODE_1:			return '1';
	case AKEYCODE_2:			return '2';
	case AKEYCODE_3:			return '3';
	case AKEYCODE_4:			return '4';
	case AKEYCODE_5:			return '5';
	case AKEYCODE_6:			return '6';
	case AKEYCODE_7:			return '7';
	case AKEYCODE_8:			return '8';
	case AKEYCODE_9:			return '9';
	case AKEYCODE_STAR:			return '*';
	case AKEYCODE_POUND:		return '#';	//americans don't know what a pound symbol looks like.
	case AKEYCODE_DPAD_UP:		return K_GP_DPAD_UP;
	case AKEYCODE_DPAD_DOWN:	return K_GP_DPAD_DOWN;
	case AKEYCODE_DPAD_LEFT:	return K_GP_DPAD_LEFT;
	case AKEYCODE_DPAD_RIGHT:	return K_GP_DPAD_RIGHT;
	case AKEYCODE_DPAD_CENTER:	return K_ENTER;
	case AKEYCODE_VOLUME_UP:	return K_VOLUP;
	case AKEYCODE_VOLUME_DOWN:	return K_VOLDOWN;
	case AKEYCODE_POWER:		return K_POWER;
//	case AKEYCODE_CAMERA:		return K_CAMERA;
//	case AKEYCODE_CLEAR:		return K_;
	case AKEYCODE_A:			return 'a';
	case AKEYCODE_B:			return 'b';
	case AKEYCODE_C:			return 'c';
	case AKEYCODE_D:			return 'd';
	case AKEYCODE_E:			return 'e';
	case AKEYCODE_F:			return 'f';
	case AKEYCODE_G:			return 'g';
	case AKEYCODE_H:			return 'h';
	case AKEYCODE_I:			return 'i';
	case AKEYCODE_J:			return 'j';
	case AKEYCODE_K:			return 'k';
	case AKEYCODE_L:			return 'l';
	case AKEYCODE_M:			return 'm';
	case AKEYCODE_N:			return 'n';
	case AKEYCODE_O:			return 'o';
	case AKEYCODE_P:			return 'p';
	case AKEYCODE_Q:			return 'q';
	case AKEYCODE_R:			return 'r';
	case AKEYCODE_S:			return 's';
	case AKEYCODE_T:			return 't';
	case AKEYCODE_U:			return 'u';
	case AKEYCODE_V:			return 'v';
	case AKEYCODE_W:			return 'w';
	case AKEYCODE_X:			return 'x';
	case AKEYCODE_Y:			return 'y';
	case AKEYCODE_Z:			return 'z';
	case AKEYCODE_COMMA:		return ',';
	case AKEYCODE_PERIOD:		return '.';
	case AKEYCODE_ALT_LEFT:		return K_LALT;
	case AKEYCODE_ALT_RIGHT:	return K_RALT;
	case AKEYCODE_SHIFT_LEFT:	return K_LSHIFT;
	case AKEYCODE_SHIFT_RIGHT:	return K_RSHIFT;
	case AKEYCODE_TAB:			return K_TAB;
	case AKEYCODE_SPACE:		return K_SPACE;
//	case AKEYCODE_SYM:			return K_IMEMODE_SYMBOL;
//	case AKEYCODE_EXPLORER:		return K_MM_APP_FILES;
//	case AKEYCODE_ENVELOPE:		return K_MM_APP_EMAIL;
	case AKEYCODE_ENTER:		return K_ENTER;
	case AKEYCODE_DEL:			return K_BACKSPACE;
	case AKEYCODE_GRAVE:		return '`';
	case AKEYCODE_MINUS:		return '-';
	case AKEYCODE_EQUALS:		return '=';
	case AKEYCODE_LEFT_BRACKET:	return '[';
	case AKEYCODE_RIGHT_BRACKET:return ']';
	case AKEYCODE_BACKSLASH:	return '#';	//this kinda sums up keymaps like this.
	case AKEYCODE_SEMICOLON:	return ';';
	case AKEYCODE_APOSTROPHE:	return '\'';
	case AKEYCODE_SLASH:		return '/';
	case AKEYCODE_AT:			return '@';
//	case AKEYCODE_NUM:			return K_;
//	case AKEYCODE_HEADSETHOOK:	return K_;
//	case AKEYCODE_FOCUS:		return K_CAMERAFOCUS;
	case AKEYCODE_PLUS:			return '+';
	case AKEYCODE_MENU:			return K_APP;
//	case AKEYCODE_NOTIFICATION:	return K_;
	case AKEYCODE_SEARCH:		return K_SEARCH;
	case AKEYCODE_MEDIA_PLAY_PAUSE:		return K_MM_TRACK_PLAYPAUSE;
	case AKEYCODE_MEDIA_STOP:			return K_MM_TRACK_STOP;
	case AKEYCODE_MEDIA_NEXT:			return K_MM_TRACK_NEXT;
	case AKEYCODE_MEDIA_PREVIOUS:		return K_MM_TRACK_PREV;
//	case AKEYCODE_MEDIA_REWIND:			return K_MM_TRACK_REWIND;
//	case AKEYCODE_MEDIA_FAST_FORWARD:	return K_MM_TRACK_FASTFWD;
	case AKEYCODE_MUTE:					return K_MM_VOLUME_MUTE;
	case AKEYCODE_PAGE_UP:				return K_PGUP;
	case AKEYCODE_PAGE_DOWN:			return K_PGDN;
//	case AKEYCODE_PICTSYMBOLS:			return K_IMEMODE_EMOJI;
//	case AKEYCODE_SWITCH_CHARSET:		return K_IMEMODE_CHARSET;
	case AKEYCODE_BUTTON_A:				return K_GP_A;
	case AKEYCODE_BUTTON_B:				return K_GP_B;
//	case AKEYCODE_BUTTON_C:				return K_GP_C;
	case AKEYCODE_BUTTON_X:				return K_GP_X;
	case AKEYCODE_BUTTON_Y:				return K_GP_Y;
//	case AKEYCODE_BUTTON_Z:				return K_GP_Z;
	case AKEYCODE_BUTTON_L1:			return K_GP_LEFT_SHOULDER;
	case AKEYCODE_BUTTON_R1:			return K_GP_RIGHT_SHOULDER;
	case AKEYCODE_BUTTON_L2:			return K_GP_LEFT_TRIGGER;
	case AKEYCODE_BUTTON_R2:			return K_GP_RIGHT_TRIGGER;
	case AKEYCODE_BUTTON_THUMBL:		return K_GP_LEFT_STICK;
	case AKEYCODE_BUTTON_THUMBR:		return K_GP_RIGHT_STICK;
	case AKEYCODE_BUTTON_START:			return K_GP_START;
	case AKEYCODE_BUTTON_SELECT:		return K_GP_BACK;
	case AKEYCODE_BUTTON_MODE:			return K_GP_GUIDE;

//And this is the part where you start to see quite why I hate android so much
	case 111/*AKEYCODE_ESCAPE*/:		return K_ESCAPE;
	case 112/*AKEYCODE_FORWARD_DEL*/:	return K_DEL;
	case 113/*AKEYCODE_CTRL_LEFT*/:		return K_LCTRL;
	case 114/*AKEYCODE_CTRL_RIGHT*/:	return K_RCTRL;
	case 115/*AKEYCODE_CAPS_LOCK*/:		return K_CAPSLOCK;
	case 116/*AKEYCODE_SCROLL_LOCK*/:	return K_SCRLCK;
	case 117/*AKEYCODE_META_LEFT*/:		return K_LWIN;
	case 118/*AKEYCODE_META_RIGHT*/:	return K_RWIN;
//	case 119/*AKEYCODE_FUNCTION*/:		return K_FUNCTION;
//	case 120/*AKEYCODE_SYSRQ*/:			return K_SYSRQ;
	case 121/*AKEYCODE_BREAK*/:			return K_PAUSE;
	case 122/*AKEYCODE_MOVE_HOME*/:		return K_HOME;
	case 123/*AKEYCODE_MOVE_END*/:		return K_END;
	case 124/*AKEYCODE_INSERT*/:		return K_INS;
//	case 125/*AKEYCODE_FORWARD*/:		return K_FORWARD;
//	case 126/*AKEYCODE_MEDIA_PLAY*/:	return K_MEDIA_PLAY;
//	case 127/*AKEYCODE_MEDIA_PAUSE*/:	return K_MEDIA_PAUSE;
//	case 128/*AKEYCODE_MEDIA_CLOSE*/:	return K_MEDIA_CLOSE;
//	case 129/*AKEYCODE_MEDIA_EJECT*/:	return K_MEDIA_EJECT;
//	case 130/*AKEYCODE_MEDIA_RECORD*/:	return K_MEDIA_RECORD;
	case 131/*AKEYCODE_F1*/:			return K_F1;
	case 132/*AKEYCODE_F2*/:			return K_F2;
	case 133/*AKEYCODE_F3*/:			return K_F3;
	case 134/*AKEYCODE_F4*/:			return K_F4;
	case 135/*AKEYCODE_F5*/:			return K_F5;
	case 136/*AKEYCODE_F6*/:			return K_F6;
	case 137/*AKEYCODE_F7*/:			return K_F7;
	case 138/*AKEYCODE_F8*/:			return K_F8;
	case 139/*AKEYCODE_F9*/:			return K_F9;
	case 140/*AKEYCODE_F10*/:			return K_F10;
	case 141/*AKEYCODE_F11*/:			return K_F11;
	case 142/*AKEYCODE_F12*/:			return K_F12;
	case 143/*AKEYCODE_NUM_LOCK*/:			return K_KP_NUMLOCK;
	case 144/*AKEYCODE_NUMPAD_0*/:			return K_KP_INS;
	case 145/*AKEYCODE_NUMPAD_1*/:			return K_KP_END;
	case 146/*AKEYCODE_NUMPAD_2*/:			return K_KP_DOWNARROW;
	case 147/*AKEYCODE_NUMPAD_3*/:			return K_KP_PGDN;
	case 148/*AKEYCODE_NUMPAD_4*/:			return K_KP_LEFTARROW;
	case 149/*AKEYCODE_NUMPAD_5*/:			return K_KP_5;
	case 150/*AKEYCODE_NUMPAD_6*/:			return K_KP_RIGHTARROW;
	case 151/*AKEYCODE_NUMPAD_7*/:			return K_KP_HOME;
	case 152/*AKEYCODE_NUMPAD_8*/:			return K_KP_UPARROW;
	case 153/*AKEYCODE_NUMPAD_9*/:			return K_KP_PGUP;
	case 154/*AKEYCODE_NUMPAD_DIVIDE*/:		return K_KP_SLASH;
	case 155/*AKEYCODE_NUMPAD_MULTIPLY*/:	return K_KP_STAR;
	case 156/*AKEYCODE_NUMPAD_SUBTRACT*/:	return K_KP_MINUS;
	case 157/*AKEYCODE_NUMPAD_ADD*/:		return K_KP_PLUS;
	case 158/*AKEYCODE_NUMPAD_DOT*/:		return K_KP_DEL;
//	case 159/*AKEYCODE_NUMPAD_COMMA*/:		return K_KP_COMMA;
	case 160/*AKEYCODE_NUMPAD_ENTER*/:		return K_KP_ENTER;
	case 161/*AKEYCODE_NUMPAD_EQUALS*/:		return K_KP_EQUALS;
//	case 162/*AKEYCODE_NUMPAD_LEFT_PAREN*/:	return K_KP_;
//	case 163/*AKEYCODE_NUMPAD_RIGHT_PAREN*/:return K_KP_;

//	case 164/*AKEYCODE_VOLUME_MUTE*/:		return K_;
//	case 165/*AKEYCODE_INFO*/:				return K_;
//	case 166/*AKEYCODE_CHANNEL_UP*/:		return K_;
//	case 167/*AKEYCODE_CHANNEL_DOWN*/:		return K_;
//	case 168/*AKEYCODE_ZOOM_IN*/:			return K_;
//	case 169/*AKEYCODE_ZOOM_OUT*/:			return K_;
//	case 170/*AKEYCODE_TV*/:				return K_;
//	case 171/*AKEYCODE_WINDOW*/:			return K_;
//	case 172/*AKEYCODE_GUIDE*/:				return K_;
//	case 173/*AKEYCODE_DVR*/:				return K_;
//	case 174/*AKEYCODE_BOOKMARK*/:			return K_;
//	case 175/*AKEYCODE_CAPTIONS*/:			return K_;
//	case 176/*AKEYCODE_SETTINGS*/:			return K_;
//	case 177/*AKEYCODE_TV_POWER*/:			return K_;
//	case 178/*AKEYCODE_TV_INPUT*/:			return K_;
//	case 179/*AKEYCODE_STB_POWER*/:			return K_;
//	case 180/*AKEYCODE_STB_INPUT*/:			return K_;
//	case 181/*AKEYCODE_AVR_POWER*/:			return K_;
//	case 182/*AKEYCODE_AVR_INPUT*/:			return K_;
//	case 183/*AKEYCODE_PROG_RED*/:			return K_;
//	case 184/*AKEYCODE_PROG_GREEN*/:		return K_;
//	case 185/*AKEYCODE_PROG_YELLOW*/:		return K_;
//	case 186/*AKEYCODE_PROG_BLUE*/:			return K_;
//	case 187/*AKEYCODE_APP_SWITCH*/:		return K_;
	case 188/*AKEYCODE_BUTTON_1*/:			return K_AUX1;
	case 189/*AKEYCODE_BUTTON_2*/:			return K_AUX2;
	case 190/*AKEYCODE_BUTTON_3*/:			return K_AUX3;
	case 191/*AKEYCODE_BUTTON_4*/:			return K_AUX4;
	case 192/*AKEYCODE_BUTTON_5*/:			return K_AUX5;
	case 193/*AKEYCODE_BUTTON_6*/:			return K_AUX6;
	case 194/*AKEYCODE_BUTTON_7*/:			return K_AUX7;
	case 195/*AKEYCODE_BUTTON_8*/:			return K_AUX8;
	case 196/*AKEYCODE_BUTTON_9*/:			return K_AUX9;
	case 197/*AKEYCODE_BUTTON_10*/:			return K_AUX10;
	case 198/*AKEYCODE_BUTTON_11*/:			return K_AUX11;
	case 199/*AKEYCODE_BUTTON_12*/:			return K_AUX12;
	case 200/*AKEYCODE_BUTTON_13*/:			return K_AUX13;
	case 201/*AKEYCODE_BUTTON_14*/:			return K_AUX14;
	case 202/*AKEYCODE_BUTTON_15*/:			return K_AUX15;
	case 203/*AKEYCODE_BUTTON_16*/:			return K_AUX16;
//	case 204/*AKEYCODE_LANGUAGE_SWITCH*/:			return K_;	//like shift+space
//	case 205/*AKEYCODE_MANNER_MODE*/:				return K_;	//toggles silent-mode
//	case 206/*AKEYCODE_3D_MODE*/:					return K_;
//	case 207/*AKEYCODE_CONTACTS*/:					return K_MM_APP_CONTACTS;
//	case 208/*AKEYCODE_CALENDAR*/:					return K_MM_APP_CALENDAR;
//	case 209/*AKEYCODE_MUSIC*/:						return K_MM_APP_MUSIC;
//	case 210/*AKEYCODE_CALCULATOR*/:				return K_MM_APP_CALCULATOR;
//	case 211/*AKEYCODE_ZENKAKU_HANKAKU*/:			return K_IME_;
//	case 212/*AKEYCODE_EISU*/:						return K_IME_;
//	case 213/*AKEYCODE_MUHENKAN*/:					return K_IME_;
//	case 214/*AKEYCODE_HENKAN*/:					return K_IME_;
//	case 215/*AKEYCODE_KATAKANA_HIRAGANA*/:			return K_IME_;
//	case 216/*AKEYCODE_YEN*/:						return K_;
//	case 217/*AKEYCODE_RO*/:						return K_;
//	case 218/*AKEYCODE_KANA*/:						return K_;
//	case 219/*AKEYCODE_ASSIST*/:					return K_MM_APP_ASSIST;
//	case 220/*AKEYCODE_BRIGHTNESS_DOWN*/:			return K_;
//	case 221/*AKEYCODE_BRIGHTNESS_UP*/:				return K_;
//	case 222/*AKEYCODE_MEDIA_AUDIO_TRACK*/:			return K_;
//	case 223/*AKEYCODE_SLEEP*/:						return K_;
//	case 224/*AKEYCODE_WAKEUP*/:					return K_;
//	case 225/*AKEYCODE_PAIRING*/:					return K_;
//	case 226/*AKEYCODE_MEDIA_TOP_MENU*/:			return K_;
//	case 227/*AKEYCODE_11*/:						return K_;
//	case 228/*AKEYCODE_12*/:						return K_;
//	case 229/*AKEYCODE_LAST_CHANNEL*/:				return K_;
//	case 230/*AKEYCODE_TV_DATA_SERVICE*/:			return K_;
//	case 231/*AKEYCODE_VOICE_ASSIST*/:				return K_MM_APP_VOICE;
//	case 232/*AKEYCODE_TV_RADIO_SERVICE*/:			return K_;
//	case 233/*AKEYCODE_TV_TELETEXT*/:				return K_;
//	case 234/*AKEYCODE_TV_NUMBER_ENTRY*/:			return K_;
//	case 235/*AKEYCODE_TV_TERRESTRIAL_ANALOG*/:		return K_;
//	case 236/*AKEYCODE_TV_TERRESTRIAL_DIGITAL*/:	return K_;
//	case 237/*AKEYCODE_TV_SATELLITE*/:				return K_;
//	case 238/*AKEYCODE_TV_SATELLITE_BS*/:			return K_;
//	case 239/*AKEYCODE_TV_SATELLITE_CS*/:			return K_;
//	case 240/*AKEYCODE_TV_SATELLITE_SERVICE*/:		return K_;
//	case 241/*AKEYCODE_TV_NETWORK*/:				return K_;
//	case 242/*AKEYCODE_TV_ANTENNA_CABLE*/:			return K_;
//	case 243/*AKEYCODE_TV_INPUT_HDMI_1*/:			return K_;
//	case 244/*AKEYCODE_TV_INPUT_HDMI_2*/:			return K_;
//	case 245/*AKEYCODE_TV_INPUT_HDMI_3*/:			return K_;
//	case 246/*AKEYCODE_TV_INPUT_HDMI_4*/:			return K_;
//	case 247/*AKEYCODE_TV_INPUT_COMPOSITE_1*/:		return K_;
//	case 248/*AKEYCODE_TV_INPUT_COMPOSITE_2*/:		return K_;
//	case 249/*AKEYCODE_TV_INPUT_COMPONENT_1*/:		return K_;
//	case 250/*AKEYCODE_TV_INPUT_COMPONENT_2*/:		return K_;
//	case 251/*AKEYCODE_TV_INPUT_VGA_1*/:			return K_;
//	case 252/*AKEYCODE_TV_AUDIO_DESCRIPTION*/:		return K_;
//	case 253/*AKEYCODE_TV_AUDIO_DESCRIPTION_MIX_UP*/:	return K_;
//	case 254/*AKEYCODE_TV_AUDIO_DESCRIPTION_MIX_DOWN*/:	return K_;
//	case 255/*AKEYCODE_TV_ZOOM_MODE*/:				return K_;
//	case 256/*AKEYCODE_TV_CONTENTS_MENU*/:			return K_;
//	case 257/*AKEYCODE_TV_MEDIA_CONTEXT_MENU*/:		return K_;
//	case 258/*AKEYCODE_TV_TIMER_PROGRAMMING*/:		return K_;
//	case 259/*AKEYCODE_HELP*/:						return K_;
//	case 260/*AKEYCODE_NAVIGATE_PREVIOUS*/:			return K_;
//	case 261/*AKEYCODE_NAVIGATE_NEXT*/:				return K_;
//	case 262/*AKEYCODE_NAVIGATE_IN*/:				return K_;
//	case 263/*AKEYCODE_NAVIGATE_OUT*/:				return K_;
//	case 264/*AKEYCODE_STEM_PRIMARY*/:				return K_;
//	case 265/*AKEYCODE_STEM_1*/:					return K_;
//	case 266/*AKEYCODE_STEM_2*/:					return K_;
//	case 267/*AKEYCODE_STEM_3*/:					return K_;
//	case 268/*AKEYCODE_DPAD_UP_LEFT*/:				return K_UPLEFTARROW;
//	case 269/*AKEYCODE_DPAD_DOWN_LEFT*/:			return K_DOWNLEFTARROW;
//	case 270/*AKEYCODE_DPAD_UP_RIGHT*/:				return K_UPRIGHTARROW;
//	case 271/*AKEYCODE_DPAD_DOWN_RIGHT*/:			return K_DOWNRIGHTARROW;
//	case 272/*AKEYCODE_MEDIA_SKIP_FORWARD*/:		return K_;
//	case 273/*AKEYCODE_MEDIA_SKIP_BACKWARD*/:		return K_;
//	case 274/*AKEYCODE_MEDIA_STEP_FORWARD*/:		return K_;
//	case 275/*AKEYCODE_MEDIA_STEP_BACKWARD*/:		return K_;
//	case 276/*AKEYCODE_SOFT_SLEEP*/:				return K_;
//	case 277/*AKEYCODE_CUT*/:						return K_;
//	case 278/*AKEYCODE_COPY*/:						return K_;
//	case 279/*AKEYCODE_PASTE*/:						return K_;
	case 280/*KEYCODE_SYSTEM_NAVIGATION_UP*/:		return K_UPARROW;
	case 281/*KEYCODE_SYSTEM_NAVIGATION_DOWN*/:		return K_DOWNARROW;
	case 282/*KEYCODE_SYSTEM_NAVIGATION_LEFT*/:		return K_LEFTARROW;
	case 283/*KEYCODE_SYSTEM_NAVIGATION_RIGHT*/:	return K_RIGHTARROW;
//	case 284/*AKEYCODE_ALL_APPS */:					return K_;
//	case 285/*AKEYCODE_REFRESH */:					return K_;
	default:
		Con_DPrintf("Android keycode %i is not supported\n", androidkey);
	}
	return 0;
}

#if 0
static void run_intent_url(void)
{
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jobject intent = (*jni)->CallObjectMethod(jni, act, (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "getIntent", "()Landroid/content/Intent;"));
		if (intent)
		{
			jstring data = (*jni)->CallObjectMethod(jni, intent, (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, intent), "getDataString", "()Ljava/lang/String;"));
			if (data)
			{
				const char *url = (*jni)->GetStringUTFChars(jni, data, NULL);
				if (url)
				{
					if (!strncmp(url, "content:", 8))
					{
						Con_Printf(CON_ERROR"Content uris are not supported\n");
						/*Java:
						Cursor cursor = this.getContentResolver().query(data, null, null, null, null);
						cursor.moveToFirst();   
						String myloc = cursor.getString(0);
						cursor.close();
						*/
					}
					else
						Host_RunFile(url, strlen(url), NULL);
					(*jni)->ReleaseStringUTFChars(jni, data, url);
				}
			}
		}
		//FIXME: do we need to release methodids/objects?
		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}
}
#endif

static qboolean read_apk_path(char *out, size_t outsize)
{
	qboolean res = false;
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jstring result = (*jni)->CallObjectMethod(jni, act, (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "getPackageCodePath", "()Ljava/lang/String;"));
		const char *tmp = (*jni)->GetStringUTFChars(jni, result, NULL);
		if (tmp)
		{
			res = true;
			Q_strncpyz(out, tmp, outsize);
			(*jni)->ReleaseStringUTFChars(jni, result, tmp);
		}

		//FIXME: do we need to release methodids/objects?
		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}

	return res;
}

static void setsoftkeyboard(int flags)
{	//the NDK is unusably buggy when it comes to keyboards, so call into java.
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jmethodID func = (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "showKeyboard", "(I)V" );
		if (func)
			(*jni)->CallVoidMethod(jni, act, func, flags);

		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}
}
static void showMessageAndQuit(const char *errormsg)
{	//no nice way to do this from native.
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jmethodID func = (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "showMessageAndQuit", "(Ljava/lang/String;)V" );
		if (func)
			(*jni)->CallVoidMethod(jni, act, func, (*jni)->NewStringUTF(jni, errormsg));
		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}
}
static void updateOrientation(const char *neworientation)
{	//no nice way to do this from native.
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jmethodID func = (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "updateOrientation", "(Ljava/lang/String;)V" );
		if (func)
			(*jni)->CallVoidMethod(jni, act, func, (*jni)->NewStringUTF(jni, neworientation));
		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}
}
static void updateScreenKeepOn(jboolean keepon)
{	//the NDK is unusably buggy when it comes to keyboards, so call into java.
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jmethodID func = (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "updateScreenKeepOn", "(Z)V" );
		if (func)
			(*jni)->CallVoidMethod(jni, act, func, keepon);

		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}
}

static void setCursorVisibility(jboolean visible)
{	//this is meant to use the nvidia-added setCursorVisibility function
	//but its fatal if it doesn't exist, and it doesn't seem to exist.
#if 0
	jobject act = sys_activity;
	JNIEnv *jni;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &jni, NULL))
	{
		jobject inputManager = NULL;
		jmethodID setvis = NULL;
		jmethodID func = (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, act), "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;" );
		if (func)
			inputManager = (*jni)->CallObjectMethod(jni, act, func, (*jni)->NewStringUTF(jni, "input"));
		if (inputManager)
			setvis = (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, inputManager), "setCursorVisibility", "(Z)V" );
		if (setvis)
			(*jni)->CallVoidMethod(jni, inputManager, setvis, visible);

		(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}
#endif
}

static void FTENativeActivity_keypress(JNIEnv *env, jobject this, jint devid, jboolean down, jint keycode, jint unicode)
{
	int qkeycode = mapkey(keycode);
//	Sys_Printf("FTENativeActivity_keypress: d=%i s=%i a=%i,q=%i u=%i\n", devid, down, keycode, qkeycode, unicode);
	if (devid < 0)
		devid = 0;
	IN_KeyEvent(devid, down, qkeycode, unicode);
}
static jboolean FTENativeActivity_wantrelative(JNIEnv *env, jobject this)
{
	if (!in_windowed_mouse.ival)	//emulators etc have no grabs so we're effectively always windowed in such situations.
		return false;
	return !Key_MouseShouldBeFree();
}
static void FTENativeActivity_mousepress(JNIEnv *env, jobject this, jint devid, jint buttonbits)
{
	static int heldbuttons;
	jint changed = buttonbits^heldbuttons;
//	Sys_Printf("FTENativeActivity_mousepress: d=%i bits=%x (changed=%x)\n", devid, buttonbits, changed);
	static int qbutton[] = {
		K_MOUSE1,	//primary
		K_MOUSE2,	//secondary
		K_MOUSE3,	//tertiary
		K_MOUSE4,	//back
		K_MOUSE5,	//forward
		K_MOUSE1,	//stylus_primary
		K_MOUSE2,	//stylus_secondary
	};
	size_t i;
	if (devid < 0)
		devid = 0;
	heldbuttons = buttonbits;
	if (changed)
	for (i = 0; i < countof(qbutton); i++)
	{
		if (changed&(1<<i))
			IN_KeyEvent(devid, buttonbits&(1<<i), qbutton[i], 0);
	}
}
static void FTENativeActivity_motion(JNIEnv *env, jobject this, jint ptrid, jint act, jfloat x, jfloat y, jfloat z, jfloat size)
{
	if (ptrid < 0)
		ptrid = 0;
//	Sys_Printf("FTENativeActivity_motion: d=%i a=%i x=%f y=%f z=%f s=%f\n", ptrid, act, x, y, z, size);
	switch(act)
	{
	case 2:	//mouse down
	case 3:	//mouse up
		IN_KeyEvent(ptrid, act==2, K_TOUCH, 0);
		break;
	case 1:	//relative motion
	case 0: //absolute motion (android sucks)
		IN_MouseMove(ptrid, act==0, x, y, z, size);
		break;
	};
}
static void FTENativeActivity_axis(JNIEnv *env, jobject this, jint devid, jint axis, jfloat value)
{
	if (devid < 0)
		devid = 0;
	IN_JoystickAxisEvent(devid, axis, value);
}
//static void FTENativeActivity_accelerometer(JNIEnv *env, jobject obj, jint devid, jfloat x, jfloat y, jfloat z)
//{
//	IN_Accelerometer(devid, x, y, z);
//}
//static void FTENativeActivity_gryoscope(JNIEnv *env, jobject obj, jint devid, jfloat pitch, jfloat yaw, jfloat roll)
//{
//	IN_Gyroscope(devid, pitch, yaw, roll);
//}

static int FTEDroid_MainThread(void *arg)
{
	int osk = 0, wantgrabs = 0, t;
	double newtime,oldtime=Sys_DoubleTime(), time, sleeptime;
	r_forceheadless = true;
	sys_nativewindow = NULL;
	vid.activeapp = true;

	if (!host_initialized)
	{
		static const char *args [] =
		{
			"ftedroid",	/*binary name, not really meaningful*/
			"-basepack",
			sys_basepak,	/*filled in later*/
			"",
			""
		};
		static quakeparms_t parms;
		if (sys_memheap)
			free(sys_memheap);
		memset(&parms, 0, sizeof(parms));
		parms.binarydir = sys_binarydir;
		parms.basedir = sys_basedir;	/*filled in later*/
		parms.argc = read_apk_path(sys_basepak, sizeof(sys_basepak))?3:1;
		parms.argv = args;
#ifdef CONFIG_MANIFEST_TEXT
		parms.manifest = CONFIG_MANIFEST_TEXT;
#endif
		sys_dpi_x = 72;	//no idea
		sys_dpi_y = 72;	//no idea

		Sys_Printf("Starting up (apk=%s, usr=%s)\n", sys_basepak, parms.basedir);

		VID_Register();
		COM_InitArgv(parms.argc, parms.argv);
		TL_InitLanguages(sys_basedir);

		Host_Init(&parms);
		Sys_Printf("Host Inited\n");
	}
	else
		Sys_Printf("Restarting up!\n");

	sys_orientation.modified = false;
	updateOrientation(sys_orientation.string);
	sys_keepscreenon.modified = false;
	updateScreenKeepOn(sys_keepscreenon.ival);

//	run_intent_url();
	/*if (state->savedState != NULL)
	{	//oh look, we're pretending to already be running...
		//oh.
	}*/


	//we're sufficiently done loading. let the ui thread resume.
	Sys_LockConditional(sys_mainconditional);
	Sys_ConditionSignal(sys_mainconditional);
	Sys_UnlockConditional(sys_mainconditional);

	while (!sys_wantshutdown)
	{
		//handle things if the UI thread is blocking for us (video restarts)
		Sys_LockConditional(sys_mainconditional);
		if (r_forcevidrestart)
		{
			ANativeWindow *oldwnd = NULL;
			jobject oldholder = NULL;
			jobject oldsurf = NULL;
			JNIEnv *env = NULL;
			if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &env, NULL))
			{
				oldholder = sys_cursholder;
				sys_cursholder = (*env)->NewGlobalRef(env, sys_newsholder);
				oldsurf = sys_cursurface;
				sys_cursurface = (*env)->NewGlobalRef(env, sys_newsurface);

				oldwnd = sys_nativewindow;
				if (sys_cursurface)
					sys_nativewindow = ANativeWindow_fromSurface(env, sys_cursurface);
				else
					sys_nativewindow = NULL;
				ANativeWindow_acquire(sys_nativewindow);
			}

			r_forceheadless = r_forcevidrestart&1;
			r_forcevidrestart = 0;
			Sys_ConditionSignal(sys_mainconditional);	//let the java ui thread thread wake up now that we've got a handle to the new+old surfaces
			Sys_UnlockConditional(sys_mainconditional);
			LOGI("Video Restart...\n");
			R_RestartRenderer_f();
			LOGI("Video Restarted...\n");
			//main thread can wake up now.

			if (oldwnd)
				ANativeWindow_release(oldwnd);
			if (oldsurf)
				(*env)->DeleteGlobalRef(env, oldsurf);
			if (oldholder)
				(*env)->DeleteGlobalRef(env, oldholder);
			if (env)
				(*sys_javavm)->DetachCurrentThread(sys_javavm);

			continue;
		}

		if (sys_nativewindow && vid.activeapp && !r_forcevidrestart)
		{
			// find time spent rendering last frame
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;

			sleeptime = Host_Frame(time);
			oldtime = newtime;

			if (sleeptime)
				Sys_Sleep(sleeptime);
		}
		else
			sleeptime = 0.25;
		Sys_UnlockConditional(sys_mainconditional);

		t = 0;
		if (sys_osk.ival >= 0)
		{
			if (Key_Dest_Has(kdm_console|kdm_message))
				t |= 1;
			if (!Key_Dest_Has(~kdm_game) && cls.state == ca_disconnected)
				t |= 1;
			if (sys_osk.ival)
				t |= 2;
		}
		if (osk != t)
		{
			setsoftkeyboard(t);
			osk = t;
		}
	
		if (sys_orientation.modified)
		{
			sys_orientation.modified = false;
			updateOrientation(sys_orientation.string);
		}
	
		if (sys_keepscreenon.modified)
		{
			sys_keepscreenon.modified = false;
			updateScreenKeepOn(sys_keepscreenon.ival);
		}

		t = FTENativeActivity_wantrelative(NULL,NULL);
		if (wantgrabs != t)
		{
			wantgrabs = t;
			setCursorVisibility(wantgrabs);
		}


		if (sleeptime)
			Sys_Sleep(sleeptime);
	}

	//don't permanently hold these when there's no active activity.
	//(hopefully there's no gl context active right now...)
	JNIEnv *env = NULL;
	if (JNI_OK == (*sys_javavm)->AttachCurrentThread(sys_javavm, &env, NULL))
	{
		if (sys_nativewindow)
			ANativeWindow_release(sys_nativewindow);
		sys_nativewindow = NULL;
		if (sys_cursurface)
			(*env)->DeleteGlobalRef(env, sys_cursurface);
		sys_cursurface = NULL;
		if (sys_cursholder)
			(*env)->DeleteGlobalRef(env, sys_cursholder);
		sys_cursholder = NULL;
		if (env)
			(*sys_javavm)->DetachCurrentThread(sys_javavm);
	}

	return 0;
}

static int secbase;

#ifdef _POSIX_TIMERS
double Sys_DoubleTime(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec/1000000000.0;
	}
	return (ts.tv_sec - secbase) + ts.tv_nsec/1000000000.0;
}
unsigned int Sys_Milliseconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec/1000000;
	}
	return (ts.tv_sec - secbase)*1000 + ts.tv_nsec/1000000;
}
#else
double Sys_DoubleTime(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
			secbase = tp.tv_sec;
			return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}
unsigned int Sys_Milliseconds(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	return (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
}
#endif

void Sys_Shutdown(void)
{
	free(sys_memheap);
}
void Sys_Quit(void)
{
#ifndef SERVERONLY
	Host_Shutdown ();
#else
	SV_Shutdown();
#endif

	LOGI("%s", "quitting");
	showMessageAndQuit("");

	longjmp(host_abort, 1);
	exit(0);
}
void Sys_Error (const char *error, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);
	COM_WorkerAbort(string);
	if (!*string)
		strcpy(string, "no error");

	LOGE("e: %s", string);
	showMessageAndQuit(string);

	host_initialized = false;	//don't keep calling Host_Frame, because it'll screw stuff up more. Can't trust Host_Shutdown either. :(
	vid.activeapp = false;		//make sure we don't busyloop.
	longjmp(host_abort, 1);
	exit(1);
}
void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;
	char *e;

	//android doesn't do \ns properly *sigh*
	//this means we have to buffer+split it ourselves.
	//and because of lots of threads, we have to mutex it too.
	static char linebuf[2048];
	static char *endbuf = linebuf;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&lock);

	//append the new data
	va_start (argptr, fmt);
	vsnprintf (endbuf,sizeof(linebuf)-(endbuf-linebuf)-1, fmt,argptr);
	va_end (argptr);
	endbuf += strlen(endbuf);

	//split it on linebreaks
	while ((e = strchr(linebuf, '\n')))
	{
		*e = 0;
		LOGI("%s", linebuf);
		memmove(linebuf, e+1, endbuf-(e+1));
		linebuf[endbuf-(e+1)] = 0;
		endbuf -= (e+1)-linebuf;
	}

	pthread_mutex_unlock(&lock);
}
void Sys_Warn (char *fmt, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	LOGW("w: %s", string);
}

void Sys_CloseLibrary(dllhandle_t *lib)
{
	if (lib)
		dlclose(lib);
}
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	return dlsym(module, exportname);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	size_t i;
	dllhandle_t *h;
	h = dlopen(va("%s.so", name), RTLD_LAZY|RTLD_LOCAL);
	if (!h)
		h = dlopen(name, RTLD_LAZY|RTLD_LOCAL);
	if (!h)
	{
		Con_DLPrintf(2,"%s\n", dlerror());
		return NULL;
	}

	if (h && funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = dlsym(h, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Con_DPrintf("Unable to find symbol \"%s\" in \"%s\"\n", funcs[i].name, name);
			Sys_CloseLibrary(h);
			h = NULL;
		}
	}
	return h;
}
char *Sys_ConsoleInput (void)
{
	return NULL;
}
void Sys_mkdir (const char *path)    //not all pre-unix systems have directories (including dos 1)
{
	mkdir(path, 0755);
}
qboolean Sys_rmdir (const char *path)
{
	if (rmdir (path) == 0)
		return true;
	if (errno == ENOENT)
		return true;
	return false;
}
qboolean Sys_remove (const char *path)
{
	return !unlink(path);
}
qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return !rename(oldfname, newfname);
}

#if _POSIX_C_SOURCE >= 200112L
	#include <sys/statvfs.h>
#endif
qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace)
{
#if _POSIX_C_SOURCE >= 200112L
	//posix 2001
	struct statvfs inf;
	if(0==statvfs(path, &inf))
	{
		*freespace = inf.f_bsize*(quint64_t)inf.f_bavail;
		return true;
	}
#endif
	return false;
}

void Sys_SendKeyEvents(void)
{
}
char *Sys_URIScheme_NeedsRegistering(void)
{	//android does its mime/etc registrations via android xml junk. dynamically registering stuff isn't supported, so pretend that its already registered to avoid annoying prompts.
	return NULL;
}
void Sys_Init(void)
{
	Cvar_Register(&sys_keepscreenon, "android stuff");
	Cvar_Register(&sys_orientation, "android stuff");
	Cvar_Register(&sys_osk, "android stuff");
}

qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	*width = 320;
	*height = 240;
	*bpp = 16;
	*refreshrate = 60;
	return false;
}
qboolean Sys_RandomBytes(qbyte *string, int len)
{
	qboolean res = false;
	int fd = open("/dev/urandom", 0);
	if (fd >= 0)
	{
		res = (read(fd, string, len) == len);
		close(fd);
	}

	return res;
}

void Sys_ServerActivity(void)
{
	/*FIXME: flash window*/
}

#ifdef WEBCLIENT
qboolean Sys_RunInstaller(void)
{       //not implemented
	return false;
}
#endif

#ifndef MULTITHREAD
void Sys_Sleep (double seconds)
{
	struct timespec ts;

	ts.tv_sec = (time_t)seconds;
	seconds -= ts.tv_sec;
	ts.tv_nsec = seconds * 1000000000.0;

	nanosleep(&ts, NULL);
}
#endif
qboolean Sys_InitTerminal(void)
{
	/*switching to dedicated mode, show text window*/
	return false;
}
void Sys_CloseTerminal(void)
{
}

#define SYS_CLIPBOARD_SIZE  256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, const char *utf8), void *ctx)
{
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, const char *text)
{
 	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
}

int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	DIR *dir;
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH];
	char truepath[MAX_OSPATH];
	char *s;
	struct dirent *ent;
	struct stat st;

	//printf("path = %s\n", gpath);
	//printf("match = %s\n", match);

	if (!gpath)
		gpath = "";
	*apath = '\0';

	Q_strncpyz(apath, match, sizeof(apath));
	for (s = apath+strlen(apath)-1; s >= apath; s--)
	{
		if (*s == '/')
		{
			s[1] = '\0';
			match += s - apath+1;
			break;
		}
	}
	if (s < apath)  //didn't find a '/' 
		*apath = '\0'; 

	Q_snprintfz(truepath, sizeof(truepath), "%s/%s", gpath, apath); 


	//printf("truepath = %s\n", truepath); 
	//printf("gamepath = %s\n", gpath); 
	//printf("apppath = %s\n", apath); 
	//printf("match = %s\n", match); 
	dir = opendir(truepath); 
	if (!dir) 
	{ 
		Con_DPrintf("Failed to open dir %s\n", truepath); 
		return true; 
	} 
	do 
	{ 
		ent = readdir(dir); 
		if (!ent) 
			break; 
		if (*ent->d_name != '.') 
		{ 
			if (wildcmp(match, ent->d_name)) 
			{ 
				Q_snprintfz(file, sizeof(file), "%s/%s", truepath, ent->d_name); 

				if (stat(file, &st) == 0) 
				{ 
					Q_snprintfz(file, sizeof(file), "%s%s%s", apath, ent->d_name, S_ISDIR(st.st_mode)?"/":""); 

					if (!func(file, st.st_size, st.st_mtime, parm, spath)) 
					{ 
						closedir(dir); 
						return false; 
					} 
				} 
				else 
					printf("Stat failed for \"%s\"\n", file); 
			} 
		} 
	} while(1); 
	closedir(dir); 

	return true; 
}

static jboolean FTENativeActivity_startup(JNIEnv *jni, jobject this, jstring externalDataPath, jstring libraryPath)
{
	const char *tmp;
	if (sys_mainthread)
		return false;
	if (!sys_activity)
	{
		sys_activity = (*jni)->NewGlobalRef(jni, this);

		tmp = (*jni)->GetStringUTFChars(jni, externalDataPath, NULL);
		if (tmp)
		{
			Q_strncpyz(sys_basedir, tmp, sizeof(sys_basedir));
			if (*sys_basedir && sys_basedir[strlen(sys_basedir)-1] != '/')
				Q_strncatz(sys_basedir, "/", sizeof(sys_basedir));
			(*jni)->ReleaseStringUTFChars(jni, externalDataPath, tmp);
		}
		else
			*sys_basedir = 0;
		tmp = (*jni)->GetStringUTFChars(jni, libraryPath, NULL);
		if (tmp)
		{
			Q_strncpyz(sys_binarydir, tmp, sizeof(sys_binarydir));
			if (*sys_binarydir && sys_binarydir[strlen(sys_binarydir)-1] != '/')
				Q_strncatz(sys_binarydir, "/", sizeof(sys_binarydir));
			(*jni)->ReleaseStringUTFChars(jni, libraryPath, tmp);
		}
		else
			*sys_binarydir = 0;

		LOGI("FTENativeActivity_startup: basedir=%s binarydir=%s\n", sys_basedir, sys_binarydir);

		sys_wantshutdown = false;
		sys_mainconditional = Sys_CreateConditional();
		Sys_LockConditional(sys_mainconditional);
		sys_mainthread = Sys_CreateThread("ftedroid", FTEDroid_MainThread, NULL, THREADP_NORMAL, -1);
		if (sys_mainthread)
			Sys_ConditionWait(sys_mainconditional);
		Sys_UnlockConditional(sys_mainconditional);
		return !!sys_mainthread;
	}
	LOGI("conflicting FTENativeActivity_startup. ignoring.\n");
	return false;
}

static void FTENativeActivity_surfacechange(JNIEnv *env, jobject this, jboolean teardown, jboolean recreate, jobject holder, jobject surface)
{
	if (!(*env)->IsSameObject(env, this, sys_activity))
	{
		LOGI("FTENativeActivity_surfacechange: inactive %p, active %p\n", this, sys_activity);
		return;	//wasn't me...
	}
	LOGI("FTENativeActivity_surfacechange: %i %i %p\n", teardown, recreate, surface);

//FIXME: if teardown&&recreate then this is a window RESIZE.
//there shouldn't be a need to destroy the entire context but anbox crashes when simply moving the window if we early out here.
//	if (teardown && recreate && (*env)->IsSameObject(env, surface, sys_newsurface))
//		return;
//	LOGI("FTENativeActivity_surfacechange: %i %i (%p==%p)==%i\n", teardown, recreate, surface, sys_newsurface, (*env)->IsSameObject(env, surface, sys_newsurface));

	Sys_LockConditional(sys_mainconditional);
	//get the main thread to let us know when its done...
	if (qrenderer || (r_forceheadless && recreate))
		r_forcevidrestart = recreate?2:1;
	if (sys_newsurface)
		(*env)->DeleteGlobalRef(env, sys_newsurface);
	sys_newsurface = surface?(*env)->NewGlobalRef(env, surface):NULL;
	if (sys_newsholder)
		(*env)->DeleteGlobalRef(env, sys_newsholder);
	sys_newsholder = holder?(*env)->NewGlobalRef(env, holder):NULL;
	//and wake up then	
	Sys_ConditionWait(sys_mainconditional);
	//and we're done...
	Sys_UnlockConditional(sys_mainconditional);
}
static void FTENativeActivity_shutdown(JNIEnv *env, jobject this)
{
	if (!(*env)->IsSameObject(env, this, sys_activity))
	{
		LOGI("FTENativeActivity_shutdown: inactive %p, active %p\n", this, sys_activity);
		return;	//wasn't me...
	}
	LOGI("FTENativeActivity_shutdown\n");

	sys_wantshutdown = true;
	if (sys_mainthread)
		Sys_WaitOnThread(sys_mainthread);
	sys_mainthread = NULL;
	if (sys_mainconditional)
		Sys_DestroyConditional(sys_mainconditional);
	sys_mainconditional = NULL;

	(*env)->DeleteGlobalRef(env, sys_newsurface);
	sys_newsurface = NULL;
	
	(*env)->DeleteGlobalRef(env, sys_newsholder);
	sys_newsholder = NULL;

	(*env)->DeleteGlobalRef(env, sys_activity);
	sys_activity = NULL;
}

//FIXME: we need a version of this that takes a byte array instead of a filename, for android's content gibberish.
static void FTENativeActivity_openfile(JNIEnv *env, jobject this, jstring filename)
{
	const char *tmp = (*env)->GetStringUTFChars(env, filename, NULL);
	if (tmp)
	{
		Sys_Printf("FTENativeActivity_openfile: %s\n", tmp);
		Host_RunFile(tmp, strlen(tmp), NULL);
		(*env)->ReleaseStringUTFChars(env, filename, tmp);
	}
}

static JNINativeMethod methods[] = {
	//
	{"startup",			"(Ljava/lang/String;Ljava/lang/String;)Z",	FTENativeActivity_startup},			//creates our 'main' thread too
	{"surfacechange",	"(ZZLandroid/view/SurfaceHolder;Landroid/view/Surface;)V",				FTENativeActivity_surfacechange},	//syncs
	{"shutdown",		"()V",										FTENativeActivity_shutdown},		//joins 'main' thread.
	{"openfile",		"(Ljava/lang/String;)V",					FTENativeActivity_openfile},

	//inputs. these use our in_generic.c ringbuffer so don't need to sync at all
	{"keypress",		"(IZII)V", 		FTENativeActivity_keypress},
	{"mousepress",		"(II)V", 		FTENativeActivity_mousepress},
	{"motion",			"(IIFFFF)V", 	FTENativeActivity_motion},
	{"wantrelative",	"()Z", 			FTENativeActivity_wantrelative},	//so the java code knows if it should use (often buggy) relative mouse movement or (limited) abs cursor coords.
	{"axis",			"(IIF)V", 		FTENativeActivity_axis},
//	{"accelerometer",	"(IFFF)V",		FTENativeActivity_accelerometer},
//	{"gyroscope",		"(IFFF)V",		FTENativeActivity_gyroscope},
};
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *jni;
	sys_javavm = vm;
	if (JNI_OK == (*vm)->GetEnv(vm, (void**)&jni, JNI_VERSION_1_2))
	{
		jclass naclass = (*jni)->FindClass(jni, "com/fteqw/FTENativeActivity");
		if (naclass)
		{
			(*jni)->RegisterNatives(jni, naclass, methods, countof(methods));
			return JNI_VERSION_1_2;
		}
	}
	return -1;
}
