#include "../plugin.h"

extern plugcorefuncs_t *plugfuncs;
extern plugcmdfuncs_t *cmdfuncs;
extern plugcvarfuncs_t *cvarfuncs;
static plug2dfuncs_t *drawfuncs;
static pluginputfuncs_t *inputfuncs;

static int K_UPARROW;
static int K_DOWNARROW;
static int K_LEFTARROW;
static int K_RIGHTARROW;
static int K_ESCAPE;
static int K_ENTER;
static int K_KP_ENTER;
static int K_MOUSE1;
static int K_MOUSE2;
static int K_HOME;
static int K_SHIFT;
static int K_MWHEELDOWN;
static int K_MWHEELUP;
static int K_PAGEUP;
static int K_PAGEDOWN;
static int K_BACKSPACE;

static qhandle_t con_chars;
static qhandle_t pic_cursor;

static float drawscalex;
static float drawscaley;

static unsigned char namebuffer[256];
static int insertpos;

static void LoadPics(void)
{
	char buffer[256];

//main bar (add cvars later)
	con_chars = drawfuncs->LoadImage("gfx/conchars.lmp");
	cvarfuncs->GetString("cl_cursor", buffer, sizeof(buffer));
	if (*buffer)
		pic_cursor = drawfuncs->LoadImage(buffer);
	else
		pic_cursor = 0;
}

static void DrawChar(unsigned int c, int x, int y)
{
	static const float size = 1.0f/16.0f;
	float s1 = size * (c&15);
	float t1 = size * (c>>4);
//	drawfuncs->Character(x, y, 0xe000|c);
	drawfuncs->Image((float)x*drawscalex, y*drawscaley, 16*drawscalex, 16*drawscaley, s1, t1, s1+size, t1+size, con_chars);
}

static qboolean AllowedChar(int c)
{
	//normalise away any unicode chars...
	if (c >= 0xe000 && c <= 0xe0ff)
		c &= 0xff;

	if (c < 0x00 || c > 0xff)
		return false;	//not a byte

	if (c == 0)		return false;	//block null chars
	if (c == '\\')	return false;	//invalid in infokeys
	if (c == '\"')	return false;	//breaks string escapes
	if (c == 255)	return false;	//block this byte, as it causes illegible server messages in vanilla

	return true;	//other chars are okay.
}
static void InsertChar(int newchar)
{
	int oldlen;
	if (!AllowedChar(newchar))
		return;

	oldlen = strlen(namebuffer);
	if (oldlen + 1 == sizeof(namebuffer))
		return;
	namebuffer[oldlen+1] = 0;
	for (; oldlen > insertpos; oldlen--)
		namebuffer[oldlen] = namebuffer[oldlen-1];

	namebuffer[insertpos++] = newchar;
}

static void KeyPress(int key, int unicode, int mx, int my)
{
	int oldlen;
	if (!key)
		; //invalid keys...
	else if (key == K_ESCAPE)
		inputfuncs->SetMenuFocus(false, NULL, 0, 0, 0); //release input focus
	else if (key == K_ENTER || key == K_KP_ENTER)
	{
		inputfuncs->SetMenuFocus(false, NULL, 0, 0, 0); //release input focus
		cvarfuncs->SetString("name", (char*)namebuffer);
	}
	else if (key == K_MOUSE1)
	{
		mx -= ((640 - (480-16))/2);
		my -= 16;
		mx /= (480-16)/16;
		my /= (480-16)/16;
		if (mx < 0 || my < 0 || mx >= 16 || my >= 16)
			return; //outside the grid

		InsertChar(mx + my*16);
	}
	else if (key == K_MOUSE2 || key == K_BACKSPACE)
	{
		if (insertpos > 0)
			insertpos--;
		for (oldlen = insertpos; namebuffer[oldlen]; oldlen++)
			namebuffer[oldlen] = namebuffer[oldlen+1];
	}
	else if (key == K_LEFTARROW)
	{
		insertpos--;
		if (insertpos < 0)
			insertpos = 0;
	}
	else if (key == K_RIGHTARROW)
	{
		insertpos++;
		if (insertpos > strlen(namebuffer))
			insertpos = strlen(namebuffer);
	}
	else if (key == K_SHIFT)
		return;
	else if ((unicode >= 0x20 && unicode <= 0x7f) || (unicode >= 0xe000 && unicode <= 0xe0ff))
		InsertChar(unicode);
}

static qboolean QDECL Plug_MenuEvent(int eventtype, int param, int unicode, float mousecursor_x, float mousecursor_y, float vidwidth, float vidheight)
{
	int i;
	quintptr_t currenttime;
	drawscalex = vidwidth/640.0f;
	drawscaley = vidheight/480.0f;

	mousecursor_x /= drawscalex;
	mousecursor_y /= drawscaley;

	switch(eventtype)
	{
	case 0:	//draw
		currenttime = plugfuncs->GetMilliseconds();

		drawfuncs->Colour4f(1,1,1,1);

		drawfuncs->Image(((640 - (480-16))/2)*drawscalex, 16*drawscaley, (480-16)*drawscalex, (480-16)*drawscaley, 0, 0, 1, 1, con_chars);

		for (i = 0; namebuffer[i]; i++)
			DrawChar(namebuffer[i], i*16, 0);
		DrawChar(10 + (((currenttime/250)&1)==1), insertpos*16, 0);
		break;
	case 1:	//keydown
		KeyPress(param, unicode, mousecursor_x, mousecursor_y);
		break;
	case 2:	//keyup
		break;
	case 3:	//menu closed (this is called even if we change it).
		break;
	case 4:	//mousemove
		break;
	}

	return 0;
}

static void Plug_NameMaker_f(void)
{
	inputfuncs->SetMenuFocus(true, NULL, 0, 0, 0); //grab input focus
	cvarfuncs->GetString("name", (char*)namebuffer, sizeof(namebuffer));
	insertpos = strlen(namebuffer);
}

qboolean Plug_Init(void)
{
	drawfuncs = plugfuncs->GetEngineInterface(plug2dfuncs_name, sizeof(*drawfuncs));
	inputfuncs = plugfuncs->GetEngineInterface(pluginputfuncs_name, sizeof(*inputfuncs));
	if (drawfuncs && inputfuncs &&
//		plugfuncs->ExportFunction("SbarBase", UI_StatusBar) &&
//		plugfuncs->ExportFunction("SbarOverlay", UI_ScoreBoard) &&
		plugfuncs->ExportFunction("MenuEvent", Plug_MenuEvent))
	{

		K_UPARROW		= inputfuncs->GetKeyCode("uparrow", NULL);
		K_DOWNARROW		= inputfuncs->GetKeyCode("downarrow", NULL);
		K_LEFTARROW		= inputfuncs->GetKeyCode("leftarrow", NULL);
		K_RIGHTARROW	= inputfuncs->GetKeyCode("rightarrow", NULL);
		K_ESCAPE		= inputfuncs->GetKeyCode("escape", NULL);
		K_ENTER			= inputfuncs->GetKeyCode("enter", NULL);
		K_KP_ENTER		= inputfuncs->GetKeyCode("kp_enter", NULL);
		K_HOME			= inputfuncs->GetKeyCode("home", NULL);
		K_MOUSE1		= inputfuncs->GetKeyCode("mouse1", NULL);
		K_MOUSE2		= inputfuncs->GetKeyCode("mouse2", NULL);
		K_MWHEELDOWN	= inputfuncs->GetKeyCode("mwheeldown", NULL);
		K_MWHEELUP		= inputfuncs->GetKeyCode("mwheelup", NULL);
		K_SHIFT			= inputfuncs->GetKeyCode("shift", NULL);
		K_PAGEUP		= inputfuncs->GetKeyCode("pgup", NULL);
		K_PAGEDOWN		= inputfuncs->GetKeyCode("pgdn", NULL);
		K_BACKSPACE		= inputfuncs->GetKeyCode("backspace", NULL);

		cmdfuncs->AddCommand("namemaker", Plug_NameMaker_f, "Provides a simple way to select from quake's glyphs.");

		LoadPics();

		return 1;
	}
	return 0;
}
