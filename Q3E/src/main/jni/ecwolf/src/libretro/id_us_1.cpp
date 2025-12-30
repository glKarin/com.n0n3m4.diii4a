//
//	ID Engine
//	ID_US_1.c - User Manager - General routines
//	v1.1d1
//	By Jason Blochowiak
//	Hacked up for Catacomb 3D
//

//
//	This module handles dealing with user input & feedback
//
//	Depends on: Input Mgr, View Mgr, some variables from the Sound, Caching,
//		and Refresh Mgrs, Memory Mgr for background save/restore
//
//	Globals:
//		ingame - Flag set by game indicating if a game is in progress
//		loadedgame - Flag set if a game was loaded
//		PrintX, PrintY - Where the User Mgr will print (global coords)
//		WindowX,WindowY,WindowW,WindowH - The dimensions of the current
//			window
//

#include "wl_def.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "id_in.h"
#include "id_vh.h"
#include "id_us.h"
#include "compat/msvc.h"

//	Global variables
		word		PrintX,PrintY;
		word		WindowX,WindowY,WindowW,WindowH;

//	Internal variables
#define	ConfigVersion	1

HighScore	Scores[MaxScores] =
			{
				{"id software-'92",10000,"1",""},
				{"Adrian Carmack",10000,"1",""},
				{"John Carmack",10000,"1",""},
				{"Kevin Cloud",10000,"1",""},
				{"Tom Hall",10000,"1",""},
				{"John Romero",10000,"1",""},
				{"Jay Wilbur",10000,"1",""},
			};

//	Internal routines

//	Public routines

//	Window/Printing routines

///////////////////////////////////////////////////////////////////////////
//
//	US_Print() - Prints a string in the current window. Newlines are
//		supported.
//
///////////////////////////////////////////////////////////////////////////
void US_Print(FFont *font, const char *sorg, EColorRange translation)
{
	static word width, height, finalWidth, finalHeight;

	px = PrintX;
	py = PrintY;
	VW_MeasurePropString(font, "A", width, finalHeight);
	VW_MeasurePropString(font, sorg, width, height, &finalWidth);
	VWB_DrawPropString(font, sorg, translation);
	PrintX = px + finalWidth;
	PrintY = py + height - finalHeight;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintUnsigned() - Prints an unsigned long
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintUnsigned(longword n)
{
	char	buffer[32];
	sprintf(buffer, "%lu", static_cast<long unsigned int> (n));

	US_Print(SmallFont, buffer);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintSigned() - Prints a signed long
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintSigned(int32_t n)
{
	char	buffer[32];

	snprintf(buffer, 30, "%ld", (long int) n);

	US_Print(SmallFont, buffer);
}

///////////////////////////////////////////////////////////////////////////
//
//	USL_PrintInCenter() - Prints a string in the center of the given rect
//
///////////////////////////////////////////////////////////////////////////
void
USL_PrintInCenter(const char *s,Rect r)
{
	word	w,h,
			rw,rh;

	VW_MeasurePropString(SmallFont, s,w,h);
	rw = r.lr.x - r.ul.x;
	rh = r.lr.y - r.ul.y;

	px = r.ul.x + ((rw - w) / 2);
	py = r.ul.y + ((rh - h) / 2);
	VWB_DrawPropString(SmallFont, s);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintCentered() - Prints a string centered in the current window.
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintCentered(const char *s)
{
	Rect	r;

	r.ul.x = WindowX;
	r.ul.y = WindowY;
	r.lr.x = r.ul.x + WindowW;
	r.lr.y = r.ul.y + WindowH;

	USL_PrintInCenter(s,r);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CPrintLine() - Prints a string centered on the current line and
//		advances to the next line. Newlines are not supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_CPrintLine(FFont *font, const char *s, EColorRange translation)
{
	word	w,h;

	VW_MeasurePropString(font, s,w,h);

	px = WindowX + ((WindowW - w) / 2);
	py = PrintY;
	VWB_DrawPropString(font, s, translation);
	PrintY += h;
}

///////////////////////////////////////////////////////////////////////////
//
//  US_CPrint() - Prints a string centered in the current window.
//      Newlines are supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_CPrint(FFont *font, const char *sorg, EColorRange translation)
{
	char	c;
	char *sstart = strdup(sorg);
	char *s = sstart;
	char *se;

	while (*s)
	{
		se = s;
		while ((c = *se)!=0 && (c != '\n'))
			se++;
		*se = '\0';

		US_CPrintLine(font, s, translation);

		s = se;
		if (c)
		{
			*se = c;
			s++;
		}
	}
	free(sstart);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_ClearWindow() - Clears the current window to white and homes the
//		cursor
//
///////////////////////////////////////////////////////////////////////////
void
US_ClearWindow(void)
{
	VWB_Clear(GPalette.WhiteIndex, WindowX, WindowY, WindowX+WindowW, WindowY+WindowH);
	PrintX = WindowX;
	PrintY = WindowY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_DrawWindow() - Draws a frame and sets the current window parms
//
///////////////////////////////////////////////////////////////////////////
void US_DrawWindow(word x,word y,word w,word h)
{
	enum
	{
		BOX_UPPERLEFT,
		BOX_UPPER,
		BOX_UPPERRIGHT,
		BOX_LEFT,
		BOX_RIGHT,
		BOX_LOWERLEFT,
		BOX_LOWER,
		BOX_LOWERRIGHT,

		BOX_START = 24
	};

	WindowX = x*8;
	WindowY = y*8;
	WindowW = w*8;
	WindowH = h*8;

	w += 2;
	h += 2;

	const unsigned int strSize = w*h + h;
	char* windowString = new char[strSize];
	memset(windowString, ' ', strSize);

	for(unsigned int p = 0;p < strSize;p += w+1)
	{
		windowString[p] = BOX_START+BOX_LEFT;
		windowString[p+w-1] = BOX_START+BOX_RIGHT;

		windowString[p+w] = '\n';
	}
	windowString[0] = BOX_START+BOX_UPPERLEFT;
	windowString[w-1] = BOX_START+BOX_UPPERRIGHT;
	windowString[strSize-w-1] = BOX_START+BOX_LOWERLEFT;
	windowString[strSize-2] = BOX_START+BOX_LOWERRIGHT;
	windowString[strSize-1] = 0;
	memset(windowString+1, BOX_START+BOX_UPPER, w-2);
	memset(windowString+strSize-w, BOX_START+BOX_LOWER, w-2);

	py = y*8 - Tile8Font->GetHeight();
	px = x*8 - Tile8Font->GetCharWidth(BOX_START+BOX_UPPERLEFT);
	VWB_DrawPropString(Tile8Font, windowString, CR_UNTRANSLATED);

	int cx = WindowX, cy = WindowY, cw = WindowW, ch = WindowH;
	MenuToRealCoords(cx, cy, cw, ch, MENU_CENTER);
	VWB_Clear(GPalette.WhiteIndex, cx, cy, cx+cw, cy+ch);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CenterWindow() - Generates a window of a given width & height in the
//		middle of the screen
//
///////////////////////////////////////////////////////////////////////////
void US_CenterWindow(word w,word h)
{
	US_DrawWindow(((MaxX / 8) - w) / 2,((MaxY / 8) - h) / 2,w,h);
	PrintX = WindowX;
	PrintY = WindowY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_SaveWindow() - Saves the current window parms into a record for
//		later restoration
//
///////////////////////////////////////////////////////////////////////////
void
US_SaveWindow(WindowRec *win)
{
	win->x = WindowX;
	win->y = WindowY;
	win->w = WindowW;
	win->h = WindowH;

	win->px = PrintX;
	win->py = PrintY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_RestoreWindow() - Sets the current window parms to those held in the
//		record
//
///////////////////////////////////////////////////////////////////////////
void
US_RestoreWindow(WindowRec *win)
{
	WindowX = win->x;
	WindowY = win->y;
	WindowW = win->w;
	WindowH = win->h;

	PrintX = win->px;
	PrintY = win->py;
}

//	Input routines

///////////////////////////////////////////////////////////////////////////
//
//	USL_XORICursor() - XORs the I-bar text cursor. Used by US_LineInput()
//
///////////////////////////////////////////////////////////////////////////
static void USL_XORICursor(FFont *font, int x,int y,const char *s,word cursor,EColorRange translation)
{
	static	bool	status;		// VGA doesn't XOR...
	char	buf[MaxString];
	word	w,h;

	strcpy(buf,s);
	buf[cursor] = '\0';
 	VW_MeasurePropString(font, buf,w,h);

	px = x + w - 1;
	py = y;
	if (status^=1)
	{
		const char cursorString[2] = {font->GetCursor(), 0};
		VWB_DrawPropString(font, cursorString, translation);
	}
}

char USL_RotateChar(char ch, int dir)
{
	static const char charSet[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ.,-!?0123456789";
	const int numChars = sizeof(charSet) / sizeof(char) - 1;
	int i;
	for(i = 0; i < numChars; i++)
	{
		if(ch == charSet[i]) break;
	}

	if(i == numChars) i = 0;

	i += dir;
	if(i < 0) i = numChars - 1;
	else if(i >= numChars) i = 0;
	return charSet[i];
}

///////////////////////////////////////////////////////////////////////////
//
//	US_LineInput() - Gets a line of user input at (x,y), the string defaults
//		to whatever is pointed at by def. Input is restricted to maxchars
//		chars or maxwidth pixels wide. If the user hits escape (and escok is
//		true), nothing is copied into buf, and false is returned. If the
//		user hits return, the current string is copied into buf, and true is
//		returned
//
///////////////////////////////////////////////////////////////////////////
bool US_LineInput(FFont *font, int x,int y,char *buf,const char *def,bool escok,
				int maxchars,int maxwidth, byte clearcolor, EColorRange translation)
{
	return 1;
}
