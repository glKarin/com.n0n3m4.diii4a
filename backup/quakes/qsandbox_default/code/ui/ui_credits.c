// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

CREDITS

=======================================================================
*/

#include "ui_local.h"

/*
	The 3-d volume is unitary in size, stretching from
	x:(-0.5, 0.5), y:(-0.5, 0.5), z:(-0.5, 0.5)

	The z co-ordinate -0.5 is on the "backplane" where the image settles.

	Transforms are applied to these coordinates after they're copied
	into a rendering list

	Reminder: Q3 uses this array convention for x,y,z co-ordinates:
	vec3_t[0] -> z, vec3_t[1] -> x, vec3_t[2] -> y

*/

#define ARRAY_SIZEOF(x) ( sizeof(x)/sizeof(x[0]) )

#define MAX_TARGA_FILESIZE (128*1024)
#define TARGA_BUFSIZE 32768
#define MAX_TARGA_WIDTH 64
#define MAX_TARGA_HEIGHT 64

#define MAX_RENDEREDPIXELS 4090

#define STATICVIEW_FOV 80
#define MINVIEW_FOV 40
#define MAXVIEW_FOV 100

#define MAXROTATE_DELTA 50

#define BLANK_TIME			1000
#define FADEIN_TIME			3000
#define SWIRLBLEND_TIME 	2000
#define SWIRL_TIME 			4000
#define TOIMAGEBLEND_TIME	1000
#define TOIMAGE_TIME		4000
#define EXPLODE_TIME 		2000
#define HOLD_TIME			1500

#define CREDIT_BORDER		10
#define CREDIT_BAR			3
#define CREDIT_DURATION		4000
#define CREDIT_SHUFFLE_TIME (CREDIT_DURATION/4)

#define MAX_EFFECTS 2

#define MAX_RECENT_MEMORY	10

#define MAX_CREDIT_TEXT		64
#define MAX_CREDITS_ONSCREEN 2
#define MAX_CREDIT_LINES 4

static vec4_t color_credit_background = { 0.25f, 0.25f, 0.25f, 0.5f };
static vec4_t color_credit_bar = { 0.66f, 0.66f, 0.66f, 1.0f };
static vec4_t color_credit_title = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t color_credit_text = { 1.0f, 1.0f, 0.0f, 1.0f };

static const float boxSize = 256.0;

// local functions
static void Credits_InitCreditSequence( qboolean nextSequence);

enum {
	TGA_NONE,
	TGA_FADEIN,
	TGA_SWIRLBLEND,
	TGA_SWIRL,

	// all modes from here are part of the "final" sequence
	// order is important, TGA_TOIMAGEBLEND is used to separate
	// from the group above
	TGA_TOIMAGEBLEND,
	TGA_TOIMAGE,
	TGA_HOLD,
	TGA_EXPLODE,
	TGA_BLANK
};

// used as index into effectData[]
enum {
	EFFECT_NONE,
	EFFECT_PULSEXYZ,
	EFFECT_PULSEXY,
	EFFECT_RIPPLE1,
	EFFECT_RIPPLE2,
	EFFECT_RUSHX,
	EFFECT_RUSHY,
	EFFECT_RUSHZ,
	EFFECT_JITTERSOME,
	EFFECT_JITTERALL,
	EFFECT_BOBBLE,
	EFFECT_FLIPX,
	EFFECT_FLIPY,
	EFFECT_FLIPZ,

	COUNT_EFFECTS
};

// parameters stored for each effect
typedef struct {
	int type;	// EFFECT_* flag

	int endtime;
	int duration;
	int param;	// extra data
} effectParams_t;

//
// the effects data table
//
typedef void (*effectHandler)(effectParams_t*);

typedef struct {
	int frequency;
	int mintime;
	int range;
	const char* name;
	effectHandler effectFunc;
} effectInfo_t;

// declare handler functions
static void Credits_Effect_RushX(effectParams_t* ep);
static void Credits_Effect_RushY(effectParams_t* ep);
static void Credits_Effect_RushZ(effectParams_t* ep);
static void Credits_Effect_PulseXYZ(effectParams_t* ep);
static void Credits_Effect_PulseXY(effectParams_t* ep);
static void Credits_Effect_Ripple1(effectParams_t* ep);
static void Credits_Effect_Ripple2(effectParams_t* ep);
static void Credits_Effect_JitterSome(effectParams_t* ep);
static void Credits_Effect_JitterAll(effectParams_t* ep);
static void Credits_Effect_Bobble(effectParams_t* ep);
static void Credits_Effect_FlipX(effectParams_t* ep);
static void Credits_Effect_FlipY(effectParams_t* ep);
static void Credits_Effect_FlipZ(effectParams_t* ep);

// All effects *must* be invariant on completion. This means
// that the xyz co-ordinates are unchanged after one period.
// This prevents pixel jumps from one spatial position to another
static effectInfo_t effectData[] = {
	{ 400, 3000, 4000, "None", 0 },	// EFFECT_NONE
	{ 15, 3000, 3000, "PulseXYZ", Credits_Effect_PulseXYZ },	// EFFECT_PULSEXYZ
	{ 15, 3000, 3000, "PulseXY", Credits_Effect_PulseXY },	// EFFECT_PULSEXY
	{ 15, 3000, 3000, "Ripple-1", Credits_Effect_Ripple1 },	// EFFECT_RIPPLE1
	{ 15, 3000, 3000, "Ripple-2", Credits_Effect_Ripple2 },	// EFFECT_RIPPLE2
	{ 10, 4000, 5000, "RushX", Credits_Effect_RushX },	// EFFECT_RUSHX
	{ 10, 4000, 5000, "RushY", Credits_Effect_RushY },	// EFFECT_RUSHY
	{ 10, 4000, 5000, "RushZ", Credits_Effect_RushZ },	// EFFECT_RUSHZ
	{ 15, 2000, 4000, "JitterSome", Credits_Effect_JitterSome },	// EFFECT_JITTERSOME
	{ 15, 2000, 4000, "JitterAll", Credits_Effect_JitterAll },	// EFFECT_JITTERALL
	{ 30, 2000, 4000, "Bobble", Credits_Effect_Bobble },	// EFFECT_BOBBLE
	{ 10, 3000, 2000, "Flip-X", Credits_Effect_FlipX},	// EFFECT_FLIPX
	{ 10, 3000, 2000, "Flip-Y", Credits_Effect_FlipY},	// EFFECT_FLIPY
	{ 10, 3000, 2000, "Flip-Z", Credits_Effect_FlipZ}	// EFFECT_FLIPZ
};

static const int effectDataCount = ARRAY_SIZEOF(effectData);

// stores image data for a composite targa image
typedef struct {
	int weight;

	const char* baseImage;
	float baseWeight;
	int baseResample;	// additional forced scaling factor when reducing large images

	const char* overlayImage;
	float overlayWeight;
	int overlayResample;	// additional forced scaling factor when reducing large images
} imageSource_t;

static imageSource_t uieImageList[] = {
	{ 100, "menu/default/logo1.tga", 1.0, 0, NULL, 0.0, 0 },	// displayed on first pass
	{ 100, "menu/default/logo1.tga", 1.0, 0, NULL, 0.9, 0 },
	{ 100, "menu/default/logo1.tga", 1.0, 0, NULL, 0.9, 0 }
};

// possible modes in which the credits may be displayed
// (some activate only after first or second pass complete)
#define CREDIT_NORMAL			0x0000
#define CREDIT_SILLYTITLE 		0x0002
#define CREDIT_REVERSETEXT 		0x0004
#define CREDIT_HIGHREVERSETEXT 	0x0008
#define CREDIT_SHUFFLETEXT		0x0010

// possible ways of interpreting the data in creditEntry_t
// CMODE_MODEL is currently not implemented
#define CMODE_TEXT 				0
#define CMODE_DUALTEXT			1
#define CMODE_MODEL				2
#define CMODE_SILLYTITLE		3
#define CMODE_QUOTE				4
#define CMODE_HARDTEXT			5

// a single credit title/names combination
// when title is NULL then its skipped during normal playback
// but used as a title during silly stuff
typedef struct {
	int mode;	// CMODE_* flags

	const char* title;
	const char* text[MAX_CREDIT_LINES];
} creditEntry_t;

static creditEntry_t uie_credits[] = {
//	{ CMODE_TEXT, "", { "", 0, 0, 0 } },
	{ CMODE_TEXT, "Quake Sandbox", {"Noire.dev", 0, 0, 0  } },
	{ CMODE_TEXT, "Game", {"Noire.dev", "Vovan_Vm", "teapxt", 0} },
	{ CMODE_TEXT, "Additional Mods", {"CannerZ45 (CZ45)", 0, 0, 0} },
	{ CMODE_TEXT, "Main Menu Music", {"iji Mae", 0, 0, 0} },
	{ CMODE_QUOTE, 0, {0, 0, 0, 0} }
};

// Replacement images for special dates
// these have a high priority for being shown
typedef struct {
	int day;	// 1-31
	int month;	// 0-11
	imageSource_t image;
} dateImageList_t;

static dateImageList_t uie_dateImages[] = {
	{ 31, 11, { 0, "menu/uie_art/imagenewyear.tga", 1.0, 0, NULL, 1.0, 0 }},
	{ 16, 7, { 0, "menu/uie_art/imagesecret.tga", 1.0, 0, NULL, 1.0, 0 }},
	{ 22, 2, { 0, "menu/uie_art/imagesecret.tga", 1.0, 0, NULL, 1.0, 0 }}
};

// a group of credits, associated with one background image
typedef struct {
	int modes;	// allowed display formats CREDIT_* (so we can do some silly things)
	creditEntry_t* credits;
	int numCredits;
	const char* discShader;

	const imageSource_t* imageList;
	int imageListSize;

	const dateImageList_t* dateImages;
	int dateImagesSize;

	float* color_title;
	float* color_text;
} creditList_t;

static creditList_t endCredits[] = {
	{ CREDIT_NORMAL|CREDIT_SILLYTITLE|CREDIT_REVERSETEXT,
		uie_credits, ARRAY_SIZEOF(uie_credits),
		"uie_creditdisc", uieImageList, ARRAY_SIZEOF(uieImageList),
		uie_dateImages, ARRAY_SIZEOF(uie_dateImages),
		color_credit_title, color_credit_text }
};

static const int numEndCredits = ARRAY_SIZEOF(endCredits);

// replacement title messages
// only used with a body of text
static char* sillyTitles[] = {
"404: Cast Not Found",
    "Hello World Coders",
    "Variable Name Ninjas",

    "Bug Squashers United",

    "Agile Sprint Runners",
    "Syntax Error Detectives",

    "Infinite Loop Survivors",
    "Null Pointer Defenders",

    "Version Control Heroes",

    "Stack Overflow Rescuers",
    "Hackathon Champions",
    "Unit Test Warriors",

    "Happy Debugging!",
    "Missing Semicolons",
    "Recursive Thinkers",
    "Open Source Crusaders",
    "Server Sidekick",
    "Prime Number Enthusiasts",
    "Bitwise Operators",
    "Binary Tree Navigators",
    "95% Done (Again)",
    "Refactoring Experts",
    "Cloud Architects",
    "Kernel Panic Survivors",
    "Coffee Script Addicts",
    "Ctrl+C/Ctrl+V Masters",
    "Algorithm Optimizers",
    "Regex Magicians"
};

static const int sillyTitlesCount = ARRAY_SIZEOF(sillyTitles);

// messages that make "sense" on their own
static char* sillyMessages[] = {
	"Server for Sale",
    "'Tis But a Segfault",
    "Throwing Exceptions is Fun",
    "print('Pthbbbttthhh!')",
    "try: except: Timmy!",
    "Regnagleppod: Undefined Variable",
    "One Script to Rule Them All",
    "127.0.0.1",
    "Unit Test AE-35 is Failing",
    "printf('Thunderbirds are Go!')",
    "The Voice of the Sysadmin",
    "Bring Back the Debugger!",
    "Oops! Bug Detected",
    "Conserving CPU Cycles",
    "Keyboard Error, Press Ctrl+Alt+Del",
    "printf('ETAOIN SHRDLU')",
    "70's Retro Code Sucks!",
    "Use the Source, Luke",
    "null",
    "That Bored, Running Lint?",
    "I Do Not Understand My Own Code",
    "Contracrostipunctus: Code Obfuscation",
    "All Your Base Are Belong to Us",
    "System.out.println('My god, it\'s full of stars!')"
};

static const int sillyMessagesCount = ARRAY_SIZEOF(sillyMessages);

// list of random quotes
typedef struct {
	int weight;
	creditEntry_t quote;
} quoteData_t;

static quoteData_t quoteList[] = {
	{ 10, { CMODE_HARDTEXT, NULL, {"\"Truth and Beauty are", "less common than semicolons\"", 0, 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"And so it begins", "You have forgotten to close a bracket\"", "Coder's Nightmare", 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"The Universe is run by the complex", "interweaving of three elements: energy,", "matter, and clean code\"", "Programmer's Philosophy" } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"You should never hand someone", "a debugger unless you're sure", "they know how to use it\"", "Senior Dev Advice" } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"Do not try the patience of programmers", "for they are subtle and quick to debug\"", 0, 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"Never try to outstubborn a legacy system\"", 0, 0, 0 } } },
    { 5, { CMODE_HARDTEXT, NULL, {"\"Never eat yellow snow\"", "And never trust user input", 0, 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"Never underestimate the power", "of infinite loops\"", 0, 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"How you behave towards documentation", "determines your success as a coder\"", 0, 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"LART: Luser Attitude Readjustment Tool.", "Rubber ducks are popular.\"", 0, 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"You can have performance.", "Or you can have readability.", "Don't ever count on having both at once.\"", 0 } } },
    { 10, { CMODE_HARDTEXT, NULL, {"\"Incoming message\"", "from the exception handler", 0, 0 } } }
};

static const int quoteListSize = ARRAY_SIZEOF(quoteList);

// credit display info
typedef struct {
	qboolean bDrawn;
	float transparency;

	int indexCredit;		// offset in credit group

	int indexData;	// text index, interpretation depends on CMODE_* flag

	int numLines;
	int duration;
	int finishTime;

	int offset_y;

	const char* titleString;

	// drawn background
	int width;
	int height;
} displayCredit_t;

// TGA image data
typedef struct {
	// targa image data
	byte targa[TARGA_BUFSIZE];

	int width;
	int height;

	int coloursize;	// bytes per colour

	float scale;	// normalization factor for screen
} imageTarga_t;

//
// the menu
//
typedef struct {
	menuframework_s	menu;

	//
	// title text
	//

	int textGroup;	// index to group of text being drawn

	displayCredit_t credit[MAX_CREDITS_ONSCREEN];

	int textPass;
	int activeCreditModes;
	int dualTextPhase;

	// stores indices of recently used values
	// to avoid repetition
	int recentMessage[MAX_RECENT_MEMORY];
	int recentMessagePos;

	int recentTitle[MAX_RECENT_MEMORY];
	int recentTitlePos;

	int recentQuote[MAX_RECENT_MEMORY];
	int recentQuotePos;

	//
	// drawing the image
	//

	// targa image data
	imageTarga_t tga;

	int numPoints;	// number of pixels in array
	int drawn[MAX_RENDEREDPIXELS];	// indices of pixels that will be drawn on screen
	vec3_t scr_pos[MAX_RENDEREDPIXELS];	// position after transforms
	vec3_t base_pos[MAX_RENDEREDPIXELS];	// position before transforms (constant for duration of image)

	// view animation data
	int imageMode;
	int imageFinishTime;	// when mode will finish

	// effect data
	effectParams_t effects[MAX_EFFECTS];

	// view point data
	vec3_t viewangles;
	vec3_t angledelta;	// angular rate per second
	vec3_t old_angledelta;	// used for blends
} creditsmenu_t;

static creditsmenu_t	s_credits;

/*
=================
ShuffleText
=================
*/
static void ShuffleText(char* text)
{
	int len, i;
	char c;
	int count;

	if (!text)
		return;

	count = 64*random();
	len = strlen(text);
	if (len < 2)
		return;

	while (count-- > 0) {
		do {
			i = len*random();
		} while (i == len);

		c = text[i];
		text[i] = text[count % len];
		text[count % len] = c;
	}
}

/*
=================
ReverseText
=================
*/
static void ReverseText(char* text)
{
	int len, i;
	char c;

	len = strlen(text);
	if (len < 2)
		return;

	for (i = 0; i < len/2; i++) {
		c = text[i];
		text[i] = text[len - i - 1];
		text[len - i - 1] = c;
	}
}

/*
=================
AngleAdd
=================
*/
static float AngleAdd(float a1, float a2)
{
	float a;

	a = a1 + a2;
	while (a > 180.0) {
		a -= 360.0;
	}
	while (a < -180.0) {
		a += 360.0;
	}
	return a;
}

/*
=================
AngleMA
=================
*/
static void AngleMA(vec3_t aa, float scale, vec3_t ab, vec3_t ac)
{
	ac[0] = AngleAdd(aa[0], scale * ab[0]);
	ac[1] = AngleAdd(aa[1], scale * ab[1]);
	ac[2] = AngleAdd(aa[2], scale * ab[2]);
}

/*
=================
AnglesAdd
=================
*/
static void AnglesAdd(vec3_t a1, vec3_t a2, vec3_t dest)
{
	dest[0] = AngleAdd(a1[0], a2[0]);
	dest[1] = AngleAdd(a1[1], a2[1]);
	dest[2] = AngleAdd(a1[2], a2[2]);
}

/*
=================
LerpAngles
=================
*/
static void LerpAngles(vec3_t from, vec3_t to, vec3_t dest, float frac)
{
	dest[0] = LerpAngle(from[0], to[0], frac);
	dest[1] = LerpAngle(from[1], to[1], frac);
	dest[2] = LerpAngle(from[2], to[2], frac);
}

/*
=================
ColorTransparency
=================
*/
static void ColorTransparency(vec4_t in, vec4_t out, float t)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3] * t;
}

/*
=================
Credits_GetTimeFrac
=================
*/
static float TimeFrac(int t, int step)
{
	float f;

	f = (float)(t - uis.realtime);
	f /= (float)step;

	if (f < 0.0)
		f = 0.0;

	if (f > 1.0)
		f = 1.0;	

	return f;
}

/*
=================
Credits_SetRandomRotate
=================
*/
static void Credits_SetRandomRotate(vec3_t v)
{
	int nulled;

	do {
		nulled = 0;

		v[PITCH] = (1.0 - 2.0*random()) * random() * MAXROTATE_DELTA;
		v[ROLL] = (1.0 - 2.0*random()) * random() * MAXROTATE_DELTA;
		v[YAW] = (1.0 - 2.0*random()) * random() * MAXROTATE_DELTA;

		if (fabsf(v[PITCH]) < 10) { //karin: abs -> fabsf
			nulled++;
			v[PITCH] = 0.0;
		}

		if (fabsf(v[ROLL]) < 10) { //karin: abs -> fabsf
			nulled++;
			v[ROLL] = 0.0;
		}

		if (fabsf(v[YAW]) < 10) { //karin: abs -> fabsf
			nulled++;
			v[YAW] = 0.0;
		}
	} while (nulled > 1);
}

/*
===============
Credits_ImplementRush
===============
*/
static void Credits_ImplementRush(effectParams_t* ep, int index)
{
	int i;
	float frac;
	float p;
	float s;

	if (!ep)
		return;

	frac = 0.5 - 0.5*cos(M_PI * TimeFrac(ep->endtime, ep->duration));

	for (i = 0; i < s_credits.numPoints; i++) {
		p = s_credits.scr_pos[i][index];
		s = (1.0 + (i % 4)) * frac;
		if (ep->param) {
			p += s;
			while (p > 0.5)
				p -= 1.0;
		}
		else {
			p -= s;
			while (p < -0.5)
				p += 1.0;
		}
		s_credits.scr_pos[i][index] = p;
	}
}

/*
===============
Credits_Effect_RushZ
===============
*/
static void Credits_Effect_RushZ(effectParams_t* ep)
{
	Credits_ImplementRush(ep, 0);
}

/*
===============
Credits_Effect_RushY
===============
*/
static void Credits_Effect_RushY(effectParams_t* ep)
{
	Credits_ImplementRush(ep, 2);
}

/*
===============
Credits_Effect_RushX
===============
*/
static void Credits_Effect_RushX(effectParams_t* ep)
{
	Credits_ImplementRush(ep, 1);
}

/*
===============
Credits_Effect_PulseXY
===============
*/
static void Credits_Effect_PulseXY(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float scale;

	if (!ep)
		return;

	amp = 0.02;
	frac = TimeFrac(ep->endtime, ep->duration);
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	frac *= frac;
	scale = 1.0 + amp*sin(12 * M_PI * frac);

	for (i = 0; i < s_credits.numPoints; i++) {
		s_credits.scr_pos[i][1] *= scale;
		s_credits.scr_pos[i][2] *= scale;
	}
}

/*
===============
Credits_Effect_PulseXYZ
===============
*/
static void Credits_Effect_PulseXYZ(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float scale;
	float* pt;

	if (!ep)
		return;

	amp = 0.02;
	frac = TimeFrac(ep->endtime, ep->duration);
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	frac *= frac;
	scale = 1.0 + amp*sin(4 * M_PI * frac );

	for (i = 0; i < s_credits.numPoints; i++) {
		pt  = s_credits.scr_pos[i];
		pt[1] *= scale;
		pt[2] *= scale;
		pt[0] = 0.5 + (pt[0] - 0.5) * scale;
	}
}

/*
===============
Credits_Effect_Ripple1
===============
*/
static void Credits_Effect_Ripple1(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float phase;

	if (!ep)
		return;

	frac = TimeFrac(ep->endtime, ep->duration);
	amp = 0.01;
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	for (i = 0; i < s_credits.numPoints; i++) {
		phase = s_credits.tga.width * frac - (s_credits.drawn[i] % s_credits.tga.width);

		s_credits.scr_pos[i][0] += amp * sin(0.1 * M_PI * phase);
	}
}

/*
===============
Credits_Effect_Ripple2
===============
*/
static void Credits_Effect_Ripple2(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float phase;

	if (!ep)
		return;

	frac = TimeFrac(ep->endtime, ep->duration);
	amp = 0.01;
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	for (i = 0; i < s_credits.numPoints; i++) {
		phase = s_credits.tga.height * frac - (s_credits.drawn[i] / s_credits.tga.width);

		s_credits.scr_pos[i][0] += amp * sin(0.1 * M_PI * phase);
	}
}

/*
===============
Credits_Effect_JitterAll
===============
*/
static void Credits_Effect_JitterAll(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float phase;

	if (!ep)
		return;

	frac = TimeFrac(ep->endtime, ep->duration);
	amp = 0.01;
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	for (i = 0; i < s_credits.numPoints; i++) {
		s_credits.scr_pos[i][1] += amp * (0.5 - random());
		s_credits.scr_pos[i][2] += amp * (0.5 - random());
	}
}

/*
===============
Credits_Effect_JitterSome
===============
*/
static void Credits_Effect_JitterSome(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float phase;

	if (!ep)
		return;

	frac = TimeFrac(ep->endtime, ep->duration);
	amp = 0.01;
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	for (i = 0; i < s_credits.numPoints; i++) {
		if (random() < 0.9)
			continue;
		s_credits.scr_pos[i][1] += amp * (0.5 - random());
		s_credits.scr_pos[i][2] += amp * (0.5 - random());
	}
}

/*
===============
Credits_Effect_Bobble
===============
*/
static void Credits_Effect_Bobble(effectParams_t* ep)
{
	int i;
	float frac;
	float amp;
	float phase;

	if (!ep)
		return;

	frac = TimeFrac(ep->endtime, ep->duration);
	amp = 0.01;
	if (frac < 0.2) {
		amp *= (frac)/0.2;
	}
	else if (frac > 0.8) {
		amp *= (1.0 - frac)/0.2;
	}

	for (i = 0; i < s_credits.numPoints; i++) {
		s_credits.scr_pos[i][0] += amp * random();
		s_credits.scr_pos[i][1] += amp * (0.5 - random());
		s_credits.scr_pos[i][2] += amp * (0.5 - random());
	}
}

/*
===============
Credits_ImplementFlip
===============
*/
static void Credits_ImplementFlip(effectParams_t* ep, int axis1, int axis2)
{
	int i;
	float frac;
	float phase;
	float x, z;
	float c, s;

	if (!ep || axis1 < 0 || axis1 > 2 || axis2 < 0 || axis2 > 2)
		return;

	frac = TimeFrac(ep->endtime, ep->duration);

	if (frac < 0.2) {
		phase = frac*frac/0.2;
	}
	else if (frac > 0.8) {
		phase = 1.0 - (1.0 - frac)*(1.0 - frac)/0.2;
	}
	else {
		phase = frac;
	}

	c = cos(2 * M_PI * phase);
	s = sin(2 * M_PI * phase);

	for (i = 0; i < s_credits.numPoints; i++) {
		z = s_credits.scr_pos[i][axis1];
		x = s_credits.scr_pos[i][axis2];
		if (ep->param) {
			s_credits.scr_pos[i][axis1] = z*c - x*s;
			s_credits.scr_pos[i][axis2] = x*c + z*s;
		}
		else {
			s_credits.scr_pos[i][axis1] = z*c + x*s;
			s_credits.scr_pos[i][axis2] = x*c - z*s;
		}

	}
}

/*
===============
Credits_Effect_FlipX
===============
*/
static void Credits_Effect_FlipX(effectParams_t* ep)
{
	Credits_ImplementFlip(ep, 0, 1);
}

/*
===============
Credits_Effect_FlipY
===============
*/
static void Credits_Effect_FlipY(effectParams_t* ep)
{
	Credits_ImplementFlip(ep, 0, 2);
}

/*
===============
Credits_Effect_FlipZ
===============
*/
static void Credits_Effect_FlipZ(effectParams_t* ep)
{
	Credits_ImplementFlip(ep, 1, 2);
}

/*
===============
Credits_SetNewEffect

type == -1 will choose a random type
no new effects when we enter into the final animation
===============
*/
static void Credits_SetNewEffect(int index, int type)
{
	int i;
	int totalBias;
	int baseBias;
	int sel;
	effectParams_t* epp;

	epp = &s_credits.effects[index];

	// no effects in the final sequence
	if (s_credits.imageMode >= TGA_TOIMAGEBLEND)
		type = EFFECT_NONE;

	if (type < 0) {
		totalBias = 0;
		for (i = 0; i < COUNT_EFFECTS; i++) {
			totalBias += effectData[i].frequency;
		}

		baseBias = 0;
		type = EFFECT_NONE;
		sel = totalBias * random();
		for (i = 0; i < COUNT_EFFECTS; i++) {
			baseBias += effectData[i].frequency;
			if (sel < baseBias) {
				type = i;
				break;
			}
		}
	}

	epp->type = type;
	epp->duration = effectData[type].mintime + effectData[type].range * random();
	epp->endtime = uis.realtime + epp->duration;
	epp->param = 0;

	switch (type) {
	case EFFECT_RUSHX:
	case EFFECT_RUSHY:
	case EFFECT_RUSHZ:
	case EFFECT_FLIPZ:
	case EFFECT_FLIPY:
	case EFFECT_FLIPX:
		if (random() > 0.5)
			epp->param = 1;
		break;
		
	default:
		break;
	}
}

/*
===============
Credits_CheckNextEffect
===============
*/
static void Credits_CheckNextEffect(void)
{
	int i;

	for (i = 0; i < MAX_EFFECTS; i++) {
		if (uis.realtime >= s_credits.effects[i].endtime) {
			Credits_SetNewEffect(i, -1);
		}
	}
}

/*
===============
Credits_SetNextImageAnimation

anim == -1 selects the next in sequence
===============
*/
static void Credits_SetNextImageAnimation(int anim)
{
	int time;
	int i;

	// automatic selection of next animation in sequence
	if (anim == -1) {
		switch (s_credits.imageMode) {
		case TGA_FADEIN:
			anim = TGA_SWIRL;
			break;

		case TGA_SWIRL:
			anim = TGA_SWIRLBLEND;
			break;

		case TGA_SWIRLBLEND:
			anim = TGA_SWIRL;
			break;

		case TGA_TOIMAGEBLEND:
			anim = TGA_TOIMAGE;
			break;

		case TGA_TOIMAGE:
			anim = TGA_HOLD;
			break;

		case TGA_HOLD:
			anim = TGA_EXPLODE;
			break;

		case TGA_EXPLODE:
			anim = TGA_BLANK;
			break;

		case TGA_BLANK:
			// prepare for next animation round
			Credits_InitCreditSequence(qtrue);
			anim = TGA_FADEIN;
			break;
		}
	}

	time = 0;
	switch (anim) {
	case TGA_FADEIN:
		time = FADEIN_TIME;
		break;

	case TGA_SWIRL:
		time = SWIRL_TIME;
		break;

	case TGA_SWIRLBLEND:
		time = SWIRLBLEND_TIME;
		VectorCopy(s_credits.angledelta, s_credits.old_angledelta);
		Credits_SetRandomRotate(s_credits.angledelta);
		break;

	case TGA_TOIMAGEBLEND:
		// begin the return to a (0,0,0) view
		VectorCopy(s_credits.angledelta, s_credits.old_angledelta);

		// ensures that the imageblend is sufficiently long
		// that the last credit can finish showing
		for (i = 0; i < MAX_CREDITS_ONSCREEN; i++) {
			if (s_credits.credit[i].bDrawn && time < s_credits.credit[i].finishTime - uis.realtime)
				time = s_credits.credit[i].finishTime - uis.realtime;
		}

		// end time when last effect has finished
		for (i = 0; i < MAX_EFFECTS; i++) {
			if (time < s_credits.effects[i].endtime - uis.realtime)
				time = s_credits.effects[i].endtime - uis.realtime;
		}

		time += TOIMAGEBLEND_TIME;

		// normalize angular step, and negate it
		VectorScale(s_credits.viewangles, -500.0/time, s_credits.angledelta);

		break;

	case TGA_TOIMAGE:
		// at the end of this step we should be at an angle (0,0,0)

		// normalize angular step, and negate it
		time = TOIMAGE_TIME;
		VectorScale(s_credits.viewangles, -1000.0/time, s_credits.angledelta);
		break;

	case TGA_HOLD:
		VectorClear(s_credits.viewangles);
		VectorClear(s_credits.angledelta);
		VectorClear(s_credits.old_angledelta);

		time = HOLD_TIME;

		break;

	case TGA_EXPLODE:
		time = EXPLODE_TIME;
		break;

	case TGA_BLANK:
		time = BLANK_TIME;
		break;

	default:
		return;
	}

	s_credits.imageMode = anim;
	s_credits.imageFinishTime = uis.realtime + time;
}

/*
===============
Credits_ApplyPixelTransforms

Works out the position of the pixel in the 3d volume
subject to the various effects

Result is then transformed into volume co-ordinates
===============
*/
static void Credits_ApplyPixelTransforms(void)
{
	int i;
	float scale;
	float *pt;
	effectParams_t* ep;

	// copy original values over
	for (i = 0; i < s_credits.numPoints; i++) {
		VectorCopy(s_credits.base_pos[i], s_credits.scr_pos[i]);
		if (s_credits.imageMode == TGA_HOLD)
			s_credits.scr_pos[i][0] = 0.0;
	}

	// apply transforms
	for (i = 0; i < MAX_EFFECTS; i++) {
		ep = &s_credits.effects[i];
		if (effectData[ep->type].effectFunc)
			effectData[ep->type].effectFunc(ep);
	}

	// scale to size
	scale = 1.0;
	if (s_credits.imageMode == TGA_TOIMAGE) {
		scale = TimeFrac(s_credits.imageFinishTime, TOIMAGE_TIME);
		scale = sqrt(scale);
	}
	else if (s_credits.imageMode == TGA_EXPLODE) {
		scale = 1.0 - TimeFrac(s_credits.imageFinishTime, EXPLODE_TIME);
		scale *= scale;
	}

	for (i = 0; i < s_credits.numPoints; i++) {
		pt = s_credits.scr_pos[i];
		pt[1] *= s_credits.tga.scale * s_credits.tga.width;
		pt[2] *= s_credits.tga.scale * s_credits.tga.height;


		switch (s_credits.imageMode) {
		case TGA_FADEIN:
		case TGA_TOIMAGEBLEND:
		case TGA_SWIRLBLEND:
		case TGA_SWIRL:
			pt[0] = boxSize * pt[0];
			break;

		case TGA_HOLD:
			// allow a small amount of effect through
			pt[0] = boxSize * (0.05 * pt[0] + 0.5);
			break;

		case TGA_TOIMAGE:
			pt[0] = boxSize * ((pt[0] - 0.5)* scale + 0.5);
			break;

		case TGA_EXPLODE:
			pt[0] = boxSize * (0.5 - (pt[0] + scale + 0.5) * scale);
			pt[1] *= (1.0 - 0.9*scale);
			pt[2] *= (1.0 - 0.9*scale);
			break;

		case TGA_NONE:
		default:
			break;
		}

	}
}

/*
===============
Credits_RenderView

Sets the viewpoint into the render list and
draws everything on screen
===============
*/
static void Credits_RenderView(void)
{
	refdef_t	refdef;
	float ix, iy, iw, ih;	// viewport size
	float step, frac;
	vec3_t angle;
	int blendtime;
	float scrx;
	float scry;
	
	trap_GetGlconfig( &uis.glconfig );
	
	scrx = uis.glconfig.vidWidth;
	scry = uis.glconfig.vidHeight;

	memset(&refdef, 0, sizeof(refdef));

	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );

	ix = 0;
	iy = 0;
	iw = scrx;
	ih = scry;
//	UI_AdjustFrom640(&ix, &iy, &iw, &ih);

	refdef.x = ix;
	refdef.y = iy;
	refdef.width = iw;
	refdef.height = ih;

	refdef.time = uis.realtime;

	refdef.fov_x = STATICVIEW_FOV;
	refdef.fov_y = STATICVIEW_FOV;

	step = (float)uis.frametime / 1000.0;

	switch (s_credits.imageMode) {
		case TGA_EXPLODE:
		case TGA_HOLD:
		case TGA_BLANK:
			break;

		case TGA_TOIMAGEBLEND:
		case TGA_SWIRLBLEND:
			if (s_credits.imageMode == TGA_TOIMAGE) {
				blendtime = TOIMAGEBLEND_TIME;
			}
			else {
				blendtime = SWIRLBLEND_TIME;
			}

			// set view angle
			frac = 1.0 - TimeFrac(s_credits.imageFinishTime, blendtime);
			LerpAngles(s_credits.old_angledelta, s_credits.angledelta, angle, frac);

			AngleMA(s_credits.viewangles, step, angle, s_credits.viewangles);
			AnglesToAxis(s_credits.viewangles, refdef.viewaxis);
			break;

		case TGA_FADEIN:
		case TGA_TOIMAGE:
		case TGA_SWIRL:
			// set view angle
			AngleMA(s_credits.viewangles, step, s_credits.angledelta, s_credits.viewangles);
			AnglesToAxis(s_credits.viewangles, refdef.viewaxis);
			break;

		case TGA_NONE:
			// should never get here
			return;

		default:
			break;
	}

	AngleVectors(s_credits.viewangles, refdef.vieworg, 0, 0);
	VectorScale(refdef.vieworg, -boxSize/2, refdef.vieworg);
	trap_R_RenderScene(&refdef);
}

/*
===============
Credits_DrawTargaImage

We can only draw up to 1024 entities on screen at once.
If there are too many active pixels then we'll get an
incomplete image
===============
*/
static void Credits_DrawTargaImage( void )
{
	int i;
	refEntity_t re;
	int colpos;
	byte* colptr;
	float fade_in;
	imageTarga_t* image;

	if (s_credits.imageMode == TGA_NONE)
		return;

	if (uis.realtime > s_credits.imageFinishTime)
		Credits_SetNextImageAnimation(-1);

	Credits_CheckNextEffect();

	if (s_credits.imageMode == TGA_BLANK)
		return;

	trap_R_ClearScene();

	// setup common refEntity_t values
	memset(&re, 0, sizeof(re));
	re.reType = RT_SPRITE;
	re.rotation = 0.0;
	re.customShader = trap_R_RegisterShaderNoMip(endCredits[s_credits.textGroup].discShader);

	image = &s_credits.tga;
	if (s_credits.imageMode == TGA_TOIMAGE) {
		re.radius = 0.5 * image->scale/(1.00 + TimeFrac(s_credits.imageFinishTime, TOIMAGE_TIME));
	}
	else if (s_credits.imageMode == TGA_HOLD) {
		re.radius = image->scale/2.0;
	}
	else if (s_credits.imageMode == TGA_EXPLODE) {
		re.radius = 0.5 * image->scale/ (2.0 - TimeFrac(s_credits.imageFinishTime, EXPLODE_TIME));
	}
	else {
		re.radius = image->scale/4.0;
	}

	if (s_credits.imageMode == TGA_FADEIN) {
		fade_in = 1.0 - TimeFrac(s_credits.imageFinishTime, FADEIN_TIME);
	}
	else
		fade_in = 1.0;

	// apply transforms to all pixels
	Credits_ApplyPixelTransforms();

	// run through all drawn pixels
	for (i = 0; i < s_credits.numPoints; i++)
	{
		// set the colour
		colpos = s_credits.drawn[i] * image->coloursize /* + image->offset*/;
		colptr = &image->targa[colpos];

		re.shaderRGBA[0] = colptr[2];
		re.shaderRGBA[1] = colptr[1];
		re.shaderRGBA[2] = colptr[0];

		if (image->coloursize == 4) {
			re.shaderRGBA[3] = colptr[3];
		}
		else {
			re.shaderRGBA[3] = 0xff;
		}

		if (s_credits.imageMode == TGA_FADEIN) {
			int tmp = re.shaderRGBA[3];
			tmp *= (int)(100 * fade_in);
			tmp /= 100;
			re.shaderRGBA[3] = (tmp & 0xff);
		}

		// position of sprite
		VectorCopy(s_credits.scr_pos[i], re.origin);
		trap_R_AddRefEntityToScene(&re);
	}

	// draw the view
	Credits_RenderView();
}

/*
===============
Credits_DoReverseText
===============
*/
static qboolean Credits_DoReverseText( void )
{
	if (s_credits.textPass < 2)
		return qfalse;

	if (endCredits[s_credits.textGroup].modes & s_credits.activeCreditModes & CREDIT_REVERSETEXT)
		return qtrue;
	if (endCredits[s_credits.textGroup].modes & s_credits.activeCreditModes & CREDIT_HIGHREVERSETEXT)
		return qtrue;
	return qfalse;
}

/*
===============
Credits_DoShuffleText
===============
*/
static qboolean Credits_DoShuffleText( displayCredit_t* dc )
{
	if (s_credits.textPass < 2)
		return qfalse;

	if (uis.realtime > dc->finishTime - dc->duration + CREDIT_SHUFFLE_TIME)
		return qfalse;

	if (endCredits[s_credits.textGroup].modes & s_credits.activeCreditModes & CREDIT_SHUFFLETEXT)
		return qtrue;
	return qfalse;
}

/*
===============
Credits_DoSillyTitle
===============
*/
static qboolean Credits_DoSillyTitle( void )
{
	if (s_credits.textPass == 0)
		return qfalse;

	if (endCredits[s_credits.textGroup].modes & s_credits.activeCreditModes & CREDIT_SILLYTITLE)
		return qtrue;
	return qfalse;
}

/*
===============
Credits_ChooseNextItem
===============
*/
static int Credits_ChooseNextItem( int range, int history[], int* pPos )
{
	int i;
	int sel;
	int pos;
	int max;
	int index;
	qboolean bMatch;

	if (!history || range < 2)
		return 0;

	pos = *pPos;
	max = MAX_RECENT_MEMORY;
	if (range < MAX_RECENT_MEMORY) {
		max = range - 3;
		if (max < 0)
			max = 0;
	}

	do {
		do {
			sel = range*random();
		} while (sel == range);

		bMatch = qfalse;
		for (i = 0; i < max; i++) {
			index = (pos + MAX_RECENT_MEMORY - i - 1) % MAX_RECENT_MEMORY;
			if (history[ index ] == sel) {
				bMatch = qtrue;
				break;
			}
		}

	} while (bMatch);

	history[pos] = sel;

	*pPos = (pos+1) % MAX_RECENT_MEMORY;

	return sel;
}

/*
===============
Credits_ChooseSillyTitleString
===============
*/
static const char* Credits_ChooseSillyTitleString( creditEntry_t* ce )
{
	int index;

	if (ce->text[0]) {
		index = Credits_ChooseNextItem(sillyTitlesCount, s_credits.recentTitle, &s_credits.recentTitlePos);
		return sillyTitles[index];
	}
	else {
		index = Credits_ChooseNextItem(sillyMessagesCount, s_credits.recentMessage, &s_credits.recentMessagePos);
		return sillyMessages[index];
	}
}

/*
===============
Credits_GetCreditTransparency
===============
*/
static float Credits_GetCreditTransparency( displayCredit_t* dc )
{
	int start;
	int window;

	window = CREDIT_DURATION / 10;
	start = dc->finishTime - dc->duration;

	if (uis.realtime - start < window)
		return 1.0 - TimeFrac(start + window, window);

	if (uis.realtime > dc->finishTime - window)
		return TimeFrac(dc->finishTime, window);

	return 1.0;
}

/*
===============
Credits_PrepareNextCredit
===============
*/
static void Credits_PrepareNextCredit( displayCredit_t* dc, int string )
{
	int i;
	creditList_t* cl;
	creditEntry_t* ce;
	float scale;
	int width, w, h;

	cl = &endCredits[s_credits.textGroup];
	if (string < 0) {
		dc->bDrawn = qfalse;
		return;
	}

	// close down credit display, we've shown them all
	if (string >= cl->numCredits) {
		dc->bDrawn = qfalse;
		return;
	}

	// size credit box
	dc->bDrawn = qtrue;
	dc->indexCredit = string;

	scale = UI_ProportionalSizeScale(UI_SMALLFONT, 0);
	ce = &cl->credits[string];
	dc->numLines = 0;
	width = 0;
	h = 0;
	dc->titleString = NULL;
	if (ce->mode == CMODE_QUOTE) {
		dc->indexData = Credits_ChooseNextItem(quoteListSize, s_credits.recentQuote, &s_credits.recentQuotePos);
		ce = &quoteList[dc->indexData].quote;
		dc->titleString = ce->title;
	}
	else if (Credits_DoSillyTitle() && ce->mode != CMODE_HARDTEXT) {
		dc->titleString = Credits_ChooseSillyTitleString(ce);
	}
	else if (ce->title) {
		dc->titleString = ce->title;
	}

	if (dc->titleString) {
		width = UI_ProportionalStringWidth(dc->titleString);
		h = PROP_HEIGHT;
		dc->numLines++;
	}

	// find the widest text
	for (i = 0; ce->text[i] && i < MAX_CREDIT_LINES; i++) {
		w = UI_ProportionalStringWidth(ce->text[i]) * scale;
		h += PROP_HEIGHT * scale;
		dc->numLines++;
		if (w > width)
			width = w;
	}

	dc->width = width;
	dc->height = h;

	// set the offset and duration of the credit
	if (ce->mode == CMODE_DUALTEXT) {
		dc->duration = 3 * CREDIT_DURATION / 2;
		dc->offset_y = 120 + 240 * s_credits.dualTextPhase;

		s_credits.dualTextPhase = 1 - s_credits.dualTextPhase;
	}
	else {
		dc->duration = CREDIT_DURATION;
		if (dc->numLines > 3)
			dc->duration += CREDIT_DURATION/2;
		dc->offset_y = 240;
	}
	dc->finishTime = uis.realtime + dc->duration;

	dc->transparency = Credits_GetCreditTransparency(dc);
}

/*
===============
Credits_FindNextCredit

Returns 0 if no more credits in list
===============
*/
static int Credits_FindNextCredit( int start, qboolean skipDual )
{
	int currentMode, nextMode;
	creditList_t* cl;
	int passCount;
	int next;

	cl = &endCredits[s_credits.textGroup];
	currentMode = cl->credits[start].mode;
	passCount = 1;
	for (next = start + 1; next < cl->numCredits; next++) {
		nextMode = cl->credits[next].mode;

		if (nextMode == CMODE_SILLYTITLE) {
			if (Credits_DoSillyTitle())
				break;
			else {
				continue;
			}
		}

		if (currentMode == CMODE_DUALTEXT && skipDual) {
			if (nextMode != CMODE_DUALTEXT)
				break;

			// skip one if in dual display mode
			if (passCount == 0)
				break;

			passCount--;
			continue;
		}

		// can process this credit
		break;
	}

	if (next == cl->numCredits)
		next = 0;

	return next;
}

/*
===============
Credits_NextFreeCreditSlot

returns -1 if not slot can be found
===============
*/
static int Credits_NextFreeCreditSlot( int start )
{
	int i;
	int index;

	if (start < 0 || start >= MAX_CREDITS_ONSCREEN)
		return -1;

	index = start;
	do {
		index++;
		if (index >= MAX_CREDITS_ONSCREEN)
			index = 0;
		if (!s_credits.credit[index].bDrawn)
			return index;
	} while (index != start);

	return -1;
}

/*
===============
Credits_StartDualCredits
===============
*/
static void Credits_StartDualCredits( creditList_t* cl, int firstcredit , int startslot)
{
	int nextFree;
	int next;

	nextFree = Credits_NextFreeCreditSlot(startslot);
	if (nextFree < 0)
		return;

	// find the next credit that will pair with this one
	next = Credits_FindNextCredit(firstcredit, qfalse);
	if (next <= 0)
		return;

	if (cl->credits[next].mode != CMODE_DUALTEXT)
		return;

	// finish setup
	Credits_PrepareNextCredit(&s_credits.credit[nextFree], next);
}

/*
===============
Credits_StopDualCredits
===============
*/
static void Credits_StopDualCredits( displayCredit_t* activeDC )
{
	int i;
	displayCredit_t *dc;
	creditList_t* cl;

	// We've not found another consecutive DUALTEXT entry
	// but we might have a dualtext entry that's still active.
	// If so then we kill this credit, because the still active one
	// will pick up the correct credit data next time
	cl = &endCredits[s_credits.textGroup];
	for (i = 0; i < MAX_CREDITS_ONSCREEN; i++) {
		dc = &s_credits.credit[i];
		if (dc->bDrawn && cl->credits[dc->indexCredit].mode == CMODE_DUALTEXT) {
			activeDC->bDrawn = qfalse;
			return;
		}
	}
}

/*
===============
Credits_UpdateCredits
===============
*/
static qboolean Credits_UpdateCredits( void )
{
	int i;
	int next;
	int currentMode, nextMode;
	displayCredit_t *dc;
	creditList_t* cl;
	qboolean bDone;
	qboolean bFound;

	cl = &endCredits[s_credits.textGroup];
	bDone = qfalse;

	for (i = 0; i < MAX_CREDITS_ONSCREEN; i++) {
		dc = &s_credits.credit[i];
		if (!dc->bDrawn)
			continue;

		dc->transparency = Credits_GetCreditTransparency(dc);
		if (uis.realtime < dc->finishTime)
			continue;

		// credit has expired, find next one
		next = Credits_FindNextCredit(dc->indexCredit, qtrue);

		if (next <= 0) {
			dc->bDrawn = qfalse;
			bDone = qtrue;
			continue;
		}

		currentMode = cl->credits[dc->indexCredit].mode;
		nextMode = cl->credits[next].mode;

		Credits_PrepareNextCredit(dc, next);

		// check for transition in/out of dual text
		// values in dc have changed after call to PrepareNextCredit
		if (currentMode != nextMode) {
			if (nextMode == CMODE_DUALTEXT) {
				Credits_StartDualCredits(cl, next, i);
			}

			if (currentMode == CMODE_DUALTEXT) {
				Credits_StopDualCredits(dc);
			}
		}
	}

	// start end sequence if on last credit
	if (bDone)
		Credits_SetNextImageAnimation(TGA_TOIMAGEBLEND);

	return qtrue;
}

/*
===============
Credits_DrawCreditText
===============
*/
static void Credits_DrawCreditText( displayCredit_t* dc )
{
	int i;
	int x, y, w, h;
	creditEntry_t* ce;
	creditList_t* cl;
	int line;
	float scale;
	float frac;
	vec4_t color;
	char text[MAX_CREDIT_TEXT];
	qboolean reverse;
	qboolean shuffle;

	cl = &endCredits[s_credits.textGroup];
	ce = &cl->credits[dc->indexCredit];
	if (ce->mode == CMODE_QUOTE) {
		ce = &quoteList[dc->indexData].quote;
	}

	x = 320 - dc->width/2 - CREDIT_BORDER;
	y = dc->offset_y - dc->height/2 - CREDIT_BORDER;
	w = dc->width + 2*CREDIT_BORDER;
	h = dc->height + 2*CREDIT_BORDER;

	ColorTransparency(color_credit_background, color, dc->transparency);
	UI_FillRect(x, y, w, h, color);

	// bar is offset 2 pixels in from border
	ColorTransparency(color_credit_bar, color, dc->transparency);
	UI_FillRect(x+2, y+2, w - 4, CREDIT_BAR, color);
	UI_FillRect(x+2, y + h - 2 - CREDIT_BAR, w - 4, CREDIT_BAR, color);

	// draw the text
	y += CREDIT_BORDER;
	line = 0;
	text[0] = '\0';

	reverse = Credits_DoReverseText();
	shuffle = Credits_DoShuffleText(dc);
	if (dc->titleString) {
		Q_strncpyz(text, dc->titleString, MAX_CREDIT_TEXT);

		if (reverse)
			ReverseText(text);
		if (shuffle)
			ShuffleText(text);

		ColorTransparency(cl->color_title, color, dc->transparency);
		UI_DrawString(320, y, text, UI_CENTER|UI_DROPSHADOW, color);
		line += PROP_HEIGHT;
	}

	scale = UI_ProportionalSizeScale(UI_SMALLFONT, 0);
	for (i = 0; i < MAX_CREDIT_LINES && ce->text[i]; i++) {
		ColorTransparency(cl->color_text, color, dc->transparency);
		Q_strncpyz(text, ce->text[i], MAX_CREDIT_TEXT);
		if (reverse)
			ReverseText(text);
		if (shuffle)
			ShuffleText(text);

		UI_DrawString(320, y + line, text, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color);
		line += PROP_HEIGHT*scale;
	}
}

/*
===============
Credits_DrawCredits
===============
*/
static void Credits_DrawCredits( void )
{
	int i;
	displayCredit_t* dc;

	if (!Credits_UpdateCredits())
		return;

	for (i = 0; i < MAX_CREDITS_ONSCREEN; i++) {
		dc = &s_credits.credit[i];
		if (!dc->bDrawn)
			continue;

		Credits_DrawCreditText(dc);
	}
}

/*
===============
Credits_IntAtOffset
===============
*/
static int Credits_IntAtOffset(byte* image, int offset)
{
	int value;

	value = (int)image[offset+1];
	value *= 256;
	value += (int)image[offset];

	return value;
}

/*
===============
Credits_CreateRenderList
===============
*/
static void Credits_CreateRenderList(const imageTarga_t* image)
{
	int i;
	int imageArea;
	float *pt;
	const byte* colptr;
	int count;

	// run through all pixels, select those that will be drawn
	s_credits.numPoints = 0;
	imageArea = image->width * image->height;
	count = 0;
	for (i = 0; i < imageArea; i++)
	{
		// set the colour
		colptr = &image->targa[i * image->coloursize /*+ image->offset*/];

		// remove near black and faint pixels
		if ((colptr[0] < 0x08) && (colptr[1] < 0x08) && (colptr[2] < 0x08))
			continue;

		if (image->coloursize == 4 && colptr[3] < 0x08)
			continue;

		// accurate pixel count, but don't store overflowed data
		count++;
		if (s_credits.numPoints == MAX_RENDEREDPIXELS)
			continue;

		s_credits.drawn[s_credits.numPoints] = i;

		pt = s_credits.base_pos[s_credits.numPoints];
		pt[0] = random() - 0.5;
		pt[1] = 0.5 - (float)(i % image->width) / (image->width) ;
		pt[2] = -0.5 + (float)(i / image->width) / (image->height);

		s_credits.numPoints++;
	}

	if (s_credits.numPoints == MAX_RENDEREDPIXELS) {
		Com_Printf("Credits image: contains %i drawn pixels (limit %i), will render incomplete\n", count, MAX_RENDEREDPIXELS);
	}
}

/*
===============
Credits_ScaleTargaImageData

Best results with images that have ^2 dimensions
other isze images are effectively clipped down to ^2 sizes
===============
*/
static void Credits_ScaleTargaImageData(imageTarga_t* image, byte* colourData, int resample)
{
	int i,j,k;
	int width, height;
	int sample;
	int imageArea;
	int colourBytes;
	int mask, masksum;
	int laststep;
	int colour[4];
	byte* destPtr;
	byte* srcPtr;
	byte* samplePtr;

	sample = 1;

	width = image->width;
	height = image->height;
	colourBytes = image->coloursize;

	while (width > MAX_TARGA_WIDTH || height > MAX_TARGA_HEIGHT) {
		width /= 2;
		height /= 2;
		sample *= 2;
	}

	while (resample > 0) {
		width /= 2;
		height /= 2;
		sample *= 2;

		resample--;
	}

	// correction for dropped pixels on end of line
    laststep = (image->width % sample);

	imageArea = height * width;
	srcPtr = colourData;
	destPtr = image->targa;
	mask = 0xff;
	for (i = 0; i < imageArea; i++) {
		colour[0] = 0;
		colour[1] = 0;
		colour[2] = 0;
		colour[3] = 0;

		samplePtr = srcPtr;
		masksum = 0;
		for (j = 0; j < sample * sample; j++) {
			if (colourBytes == 4)
				mask = samplePtr[3];

			colour[0] += mask*samplePtr[0];
			colour[1] += mask*samplePtr[1];
			colour[2] += mask*samplePtr[2];
			colour[3] += mask*mask;

            masksum += mask;

			// get next colour position
			if ((j + 1)%sample == 0)
				samplePtr += (image->width - sample + 1)*colourBytes;
			else
				samplePtr += colourBytes;
		}

		if (masksum) {
			colour[0] /= masksum;
			colour[1] /= masksum;
			colour[2] /= masksum;
		}
		else {
			colour[0] = 0;
			colour[1] = 0;
			colour[2] = 0;
		}

		if (colour[0] > 0xff)
			colour[0] = 0xff;

		if (colour[1] > 0xff)
			colour[1] = 0xff;

		if (colour[2] > 0xff)
			colour[2] = 0xff;

		destPtr[0] = (byte)colour[0];
		destPtr[1] = (byte)colour[1];
		destPtr[2] = (byte)colour[2];

		if (colourBytes == 4) {
			if (masksum) {
				colour[3] /= masksum;
				if (colour[3] > 0xff)
					colour[3] = 0xff;
			}
			else
				colour[3] = 0;
			destPtr[3] = (byte)colour[3];
		}

		// get start of next block
		destPtr += colourBytes;
		if ((i + 1) % width == 0)
			srcPtr += (image->width*(sample-1) + sample + laststep)*colourBytes;
		else
			srcPtr += sample*colourBytes;
	}

	image->width = width;
	image->height = height;
}

/*
===============
Credits_ProcessTargaImage
===============
*/
static qboolean Credits_ProcessTargaImage(const char* imageFile, imageTarga_t* image, int resample)
{
	static byte targaFile[MAX_TARGA_FILESIZE];

	fileHandle_t file;
	int len;
	int rgbsize;
	int minsize;
	int imageType;
	int i, j;
	int imageArea;
	int pixelCount;
	int offset;

	if (!imageFile)
		return qfalse;

	// load image
	len = trap_FS_FOpenFile(imageFile, &file, FS_READ);
	if (len >= MAX_TARGA_FILESIZE) {
		Com_Printf("Credits image: %s too large (%i bytes)\n", imageFile, len);
		return qfalse;
	}

	if (len < 0) {
		Com_Printf("Credits image: %s not found\n", imageFile);
		return qfalse;
	}

	trap_FS_Read(targaFile, len, file);
	trap_FS_FCloseFile(file);

	// validate image, check targa type
	imageType = targaFile[2];
	if ( imageType != 2) {
		Com_Printf("Credits image: %s is type %i, only type 2 supported\n", imageFile, imageType );
		return qfalse;
	}

	// validate image, check we can read the colour format
	rgbsize = targaFile[16];

	if (rgbsize != 24 && rgbsize != 32) {
		Com_Printf("Credits image: unsupported colour format (%i bits)\n", rgbsize);
		return qfalse;
	}

	// convert to bytes from bits
	image->coloursize = rgbsize/8;


	// find the offset to the start of the rgb data
	offset = 18;
	offset += (int)targaFile[0];
	if (targaFile[1]) {
		Com_Printf("Credits image: %s image map ignored\n", imageFile);
		offset += Credits_IntAtOffset(targaFile, 5) * (image->targa[7]/8);
	}

	// validate image, check the size
	image->width = Credits_IntAtOffset(targaFile, 12);
	image->height = Credits_IntAtOffset(targaFile, 14);

	if (image->width <= 0 || image->height <= 0) {
		Com_Printf("Credits image: zero or negative image size (!!!)\n");
		return qfalse;
	}

	// copy image data into targa buffer
	pixelCount = image->width * image->height;
	if (pixelCount > MAX_TARGA_WIDTH*MAX_TARGA_HEIGHT || image->width > 2*MAX_TARGA_WIDTH || image->height > 2*MAX_TARGA_HEIGHT || resample > 0) {
		if (uis.debug)
			Com_Printf("Credits image: %s too large (%ix%i), scaling\n", imageFile, image->width, image->height);

		Credits_ScaleTargaImageData(image, targaFile + offset, resample);
	}
	else {
		imageArea = pixelCount * image->coloursize;
		memcpy(image->targa, targaFile + offset, imageArea);
	}

	if (image->height > image->width) {
		image->scale = 10.0 * (32.0 / image->height);
	}
	else {
		image->scale = 10.0 * (32.0 / image->width);
	}

	// ready to go
//	Com_Printf(va("Credits image %s loaded.\n", imageFile));

	return qtrue;
}

/*
===============
Credits_ColourMergeMix
===============
*/
static byte Credits_ColorMergeMix(byte c1, byte c2, byte m1, byte m2)
{
	int weight;
	int col;

	weight = m1 + m2;
	if (weight == 0)
		return 0x00;

	col = m1*c1 + m2*c2;
	col /= weight;

	if (col > 0xff)
		col = 0xff;

	return (byte)col;
}

/*
===============
Credits_ColourMergeAdd
===============
*/
static byte Credits_ColorMergeAdd(byte s, byte d, byte ms, byte md)
{
	unsigned int weight;
	unsigned int col;

	weight = ms + 0xff;
	if (weight == 0)
		return 0x00;

	col = ms*s + 0xff*d;
	col /= weight;

	if (col > 0xff)
		col = 0xff;

	return (byte)col;
}

/*
===============
Credits_MergeSecondTargaImage
===============
*/
static qboolean Credits_MergeSecondTargaImage(imageTarga_t* dest, const char* imageFile, float d_fade, float s_fade, int resample)
{
	static imageTarga_t s_image;

	int i;
	int imageArea;
	byte s_mask, d_mask;
	byte *s_offset, *d_offset;

	if (!imageFile || !dest)
		return qfalse;

	// load second image data
	if (!Credits_ProcessTargaImage(imageFile, &s_image, resample))
		return qfalse;

	// compare data formats and prepare data
	if (dest->width != s_image.width || dest->height != s_image.height) {
		Com_Printf("2nd Credit image: different size\n");
		return qfalse;
	}

	imageArea = s_image.width * s_image.height;

	// merge images to data format of first image
	s_mask = (byte)(0xff * s_fade);
	d_mask = (byte)(0xff * d_fade);
	for (i = 0; i < imageArea; i++) {
		s_offset = &s_image.targa[i * s_image.coloursize];
		d_offset = &dest->targa[i * dest->coloursize];

		// grab transparency if in data
		if (s_image.coloursize == 4)
			s_mask = (byte)(s_offset[3] * s_fade);

		if (dest->coloursize == 4)
			d_mask = d_offset[3] * d_fade;

		d_offset[0] = Credits_ColorMergeAdd(s_offset[0], d_offset[0], s_mask, d_mask);
		d_offset[1] = Credits_ColorMergeAdd(s_offset[1], d_offset[1], s_mask, d_mask);
		d_offset[2] = Credits_ColorMergeAdd(s_offset[2], d_offset[2], s_mask, d_mask);

		if (dest->coloursize == 4)
			d_offset[3] = Credits_ColorMergeMix(0xff, 0xff, s_mask, d_mask);
	}

	return qtrue;
}

/*
=================
Credits_ProcessBackgroundImage
=================
*/
static void Credits_ProcessBackgroundImage( imageTarga_t* dest, const imageSource_t* is )
{
	if (Credits_ProcessTargaImage(is->baseImage, dest, is->baseResample))
		Credits_MergeSecondTargaImage(dest, is->overlayImage, is->baseWeight, is->overlayWeight, is->overlayResample);
}

/*
=================
Credits_SelectDateImage
=================
*/
static const imageSource_t* Credits_SelectDateImage( const creditList_t* cl )
{
	int i;
	qtime_t qt;
	const dateImageList_t* dil;
	const imageSource_t* is;
	int select, countImages;

	trap_RealTime(&qt);

	dil = cl->dateImages;
	if (!dil)
		return NULL;

	// count number of images on a given day
	countImages = 0;
	for (i = 0; i < cl->dateImagesSize; i++) {
		if (dil[i].day == qt.tm_mday && dil[i].month == qt.tm_mon)
			countImages++;
	}

	if (countImages == 0)
		return NULL;

	do {
		select = countImages*random();
	} while (select == countImages);

	// find that image
	is = NULL;
	for (i = 0; i < cl->dateImagesSize; i++) {
		if (dil[i].day == qt.tm_mday && dil[i].month == qt.tm_mon) {
			select--;
			if (select < 0) {
				is = &dil[i].image;
				break;
			}
		}
	}

	return is;
}

/*
=================
Credits_ChooseBackgroundImage
=================
*/
static void Credits_ChooseBackgroundImage( void )
{
	int i;
	int imageIndex;
	int weight, weightTotal;
	const creditList_t* cl;
	const imageSource_t* is;

	s_credits.imageMode = TGA_NONE;

	cl = &endCredits[s_credits.textGroup];
	if (!cl->imageList) {
		return;
	}

	// choose the backdrop image
	is = Credits_SelectDateImage(cl);

	if (!(is && random() < 0.5)) {
		if (s_credits.textPass == 0) {
			// always choose first image on first pass
			imageIndex = 0;
			is = &cl->imageList[0];
		}
		else {
			// choose a random image
			weightTotal = 0;
			for (i = 0; i < cl->imageListSize; i++) {
				weightTotal += cl->imageList[i].weight;
			}

			do {
				weight = weightTotal * random();
			} while (imageIndex == weightTotal);

			imageIndex = 0;
			for (i = 0; i < cl->imageListSize; i++) {
				weight-= cl->imageList[i].weight;
				if (weight < 0) {
					is = &cl->imageList[i];
					break;
				}
			}
		}
	}

	if (is)
		Credits_ProcessBackgroundImage(&s_credits.tga, is);
}

/*
=================
Credits_InitCreditSequence
=================
*/
static void Credits_InitCreditSequence( qboolean nextGroup )
{
	int i;
	int w, width, h;
	float scale;
	const creditEntry_t *ce;

	s_credits.dualTextPhase = 0;
	if (nextGroup) {
		s_credits.textGroup++;
		if (s_credits.textGroup >= numEndCredits) {
			s_credits.textGroup = 0;
			s_credits.textPass++;
		}
	}
	else {
		s_credits.textPass = 0;
		s_credits.textGroup = 0;
	}

	s_credits.activeCreditModes = CREDIT_NORMAL;
	if (random() < 0.3)
		s_credits.activeCreditModes |= CREDIT_REVERSETEXT;
	if (random() < 0.8)
		s_credits.activeCreditModes |= CREDIT_HIGHREVERSETEXT;
	if (random() < 0.9)
		s_credits.activeCreditModes |= CREDIT_SILLYTITLE;
	if (random() < 0.3)
		s_credits.activeCreditModes |= CREDIT_SHUFFLETEXT;

	Credits_ChooseBackgroundImage();

	Credits_CreateRenderList(&s_credits.tga);
	Credits_SetRandomRotate(s_credits.angledelta);

	s_credits.viewangles[PITCH] = 180 - 360.0 * random();
	s_credits.viewangles[YAW] = 360.0 * random();
	s_credits.viewangles[ROLL] = 180 - 360.0 * random();

	for (i = 0; i < MAX_EFFECTS; i++)
		Credits_SetNewEffect(i, EFFECT_NONE);

	Credits_PrepareNextCredit(&s_credits.credit[0], 0);
	for (i = 1; i < MAX_CREDITS_ONSCREEN; i++) {
		Credits_PrepareNextCredit(&s_credits.credit[i], -1);
	}
}

/*
=================
Credits_Init
=================
*/
static void Credits_Init( int num )
{
	int i;

	for (i = 0; i < MAX_RECENT_MEMORY; i++) {
		s_credits.recentMessage[i] = -1;
		s_credits.recentTitle[i] = -1;
		s_credits.recentQuote[i] = -1;
	}

	s_credits.recentMessagePos = 0;
	s_credits.recentTitlePos = 0;
	s_credits.recentQuotePos = 0;

	// setup image and effects
	Credits_InitCreditSequence(qfalse);
	if(num == 0){
	Credits_SetNextImageAnimation(TGA_FADEIN);
	}
	if(num == 1){
	Credits_SetNextImageAnimation(TGA_TOIMAGE);
	}
}

/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}
if(s_credits.menu.number == 0){
	trap_Cmd_ExecuteText( EXEC_APPEND,"quit\n" );
}
if(s_credits.menu.number == 1){
		uis.hideCursor = qfalse;
		trap_Cmd_ExecuteText( EXEC_APPEND, "music music/main_menu\n");
		UI_PopMenu();
}
	return 0;
}

/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {
	int		y;
	int i;
	float r;

	Credits_DrawTargaImage();

	if (s_credits.imageMode < TGA_TOIMAGE) {
		r = random();
		Credits_DrawCredits();
		if(s_credits.menu.number == 1){
		uis.hideCursor = qfalse;
		trap_Cmd_ExecuteText( EXEC_APPEND, "music music/main_menu\n");
		UI_PopMenu();
		}
	}

	// debug info
	if (uis.debug) {
		y = 180;
		for (i = 0; i < MAX_EFFECTS; i++) {
			UI_DrawString( 0-uis.wideoffset, y, va("%s, param=%i", effectData[s_credits.effects[i].type].name, s_credits.effects[i].param), UI_SMALLFONT, color_white );
			y += SMALLCHAR_HEIGHT;
		}

		UI_DrawString( 0-uis.wideoffset, y, va("Mode: %i", s_credits.imageMode), UI_SMALLFONT, color_white );
		y += SMALLCHAR_HEIGHT;
		UI_DrawString( 0-uis.wideoffset, y, va("NumPoints: %i", s_credits.numPoints), UI_SMALLFONT, color_white );
	}
}

/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( int num ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	s_credits.menu.number = num;

	Credits_Init( num );

	uis.hideCursor = qtrue;

	UI_PushMenu ( &s_credits.menu );
}
