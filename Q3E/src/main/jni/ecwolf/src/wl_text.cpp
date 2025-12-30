// WL_TEXT.C

#include "wl_def.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "v_palette.h"
#include "w_wad.h"
#include "wl_game.h"
#include "wl_iwad.h"
#include "wl_text.h"
#include "g_intermission.h"
#include "g_mapinfo.h"
#include "id_ca.h"
#include "textures/textures.h"

/*
=============================================================================

TEXT FORMATTING COMMANDS
------------------------
^C<hex digit>           Change text color
^E[enter]               End of layout (all pages)
^G<y>,<x>,<pic>[enter]  Draw a graphic and push margins
^P[enter]               start new page, must be the first chars in a layout
^L<x>,<y>[ENTER]        Locate to a specific spot, x in pixels, y in lines

=============================================================================
*/

/*
=============================================================================

												LOCAL CONSTANTS

=============================================================================
*/

#define WORDLIMIT       80
#define FONTHEIGHT      10
#define TOPMARGIN       16
#define BOTTOMMARGIN    32
#define LEFTMARGIN      16
#define RIGHTMARGIN     16
#define PICMARGIN       8
#define TEXTROWS        ((200-TOPMARGIN-BOTTOMMARGIN)/FONTHEIGHT)
#define SPACEWIDTH      7
#define SCREENPIXWIDTH  320
#define SCREENMID       (SCREENPIXWIDTH/2)

/*
=============================================================================

								LOCAL VARIABLES

=============================================================================
*/

static int pagenum;
static int numpages;

static unsigned leftmargin[TEXTROWS];
static unsigned rightmargin[TEXTROWS];
static const char* text;
static unsigned rowon;

static byte    fontcolor;
static EColorRange textcolor;
static int     picx;
static int     picy;
static FTextureID picnum;
static FTexture *backgroundFlat = NULL;
static int     picdelay;
static bool    layoutdone;
static FFont *font;
static ETSAlignment alignment;
static ETSAnchor anchor;

//===========================================================================

/*
=====================
=
= RipToEOL
=
=====================
*/

static void RipToEOL (void)
{
	while (*text++ != '\n')         // scan to end of line
		;
}


/*
=====================
=
= ParseNumber
=
=====================
*/

static int ParseNumber (void)
{
	//
	// scan until a number is found
	//
	char ch = *text;
	while (ch < '0' || ch >'9')
		ch = *++text;

	//
	// copy the number out
	//
	char num[80];
	char *numptr = num;
	do
	{
		*numptr++ = ch;
		ch = *++text;
	} while (ch >= '0' && ch <= '9');
	*numptr = 0;

	return atoi (num);
}



/*
=====================
=
= ParsePicCommand
=
= Call with text pointing just after a ^P
= Upon exit text points to the start of next line
=
=====================
*/

static void ParsePicCommand (bool helphack, bool norip=false)
{
	picy=ParseNumber();
	picx=ParseNumber();

	// Skip over whitespace
	while(*text == ' ' || *text == '\t')
		++text;

	if(*text == '[')
	{
		const char* texName = text+1;
		unsigned int len = 0;
		while(*++text != ']')
			++len;
		picnum = TexMan.GetTexture(FString(texName, len), FTexture::TEX_Any);
		++text;
	}
	else
	{
		int num=ParseNumber();

		if(helphack)
		{
			switch(num)
			{
				case 5:
					num = 11;
					break;
				case 11:
					num = 5;
					break;
				default:
					break;
			}
		}
		picnum = TexMan.GetArtIndex(num);
	}

	if(!norip)
		RipToEOL ();
}


static void ParseTimedCommand (bool helphack)
{
	ParsePicCommand(helphack, true);
	picdelay=ParseNumber();
	RipToEOL ();
}


/*
=====================
=
= TimedPicCommand
=
= Call with text pointing just after a ^P
= Upon exit text points to the start of next line
=
=====================
*/

static void TimedPicCommand (bool helphack)
{
	ParseTimedCommand (helphack);

	//
	// update the screen, and wait for time delay
	//
	VW_UpdateScreen ();

	//
	// wait for time
	//
	Delay(picdelay);

	//
	// draw pic
	//
	if(picnum.isValid())
		VWB_DrawGraphic (TexMan(picnum), picx&~7, picy, MENU_CENTER);
}


/*
=====================
=
= HandleCommand
=
=====================
*/

static void HandleCommand (bool helphack)
{
	switch (toupper(*++text))
	{
		case 'B':
		{
			double bx = ParseNumber();
			double by = ParseNumber();
			double bw = ParseNumber();
			double bh = ParseNumber();
			MenuToRealCoords(bx, by, bw, bh, MENU_CENTER);
			VWB_DrawFill(backgroundFlat, (int)bx, (int)by, (int)(bx+bw), (int)(by+bh));
			RipToEOL();
			break;
		}
		case ';':               // comment
			RipToEOL();
			break;
		case 'P':               // ^P is start of next page, ^E is end of file
		case 'E':
			layoutdone = true;
			text--;             // back up to the '^'
			break;

		case 'C':               // ^c<hex digit> changes text color
		{
			char i = toupper(*++text);
			if(i == '[') // Textcolo translation
			{
				fontcolor = 255;
				const BYTE *colorname = (const BYTE *)(text);
				textcolor = V_ParseFontColor(colorname, CR_UNTRANSLATED, CR_UNTRANSLATED+1);
				while(*text++ != ']');
			}
			else
			{
				textcolor = CR_UNTRANSLATED;

				if (i>='0' && i<='9')
					fontcolor = i-'0';
				else if (i>='A' && i<='F')
					fontcolor = i-'A'+10;

				fontcolor *= 16;
				i = toupper(*++text);
				if (i>='0' && i<='9')
					fontcolor += i-'0';
				else if (i>='A' && i<='F')
					fontcolor += i-'A'+10;
				text++;
			}
			break;
		}

		case '>':
			px = 160;
			text++;
			break;

		case 'L':
			py=ParseNumber();
			rowon = (py-TOPMARGIN)/FONTHEIGHT;
			py = TOPMARGIN+rowon*FONTHEIGHT;
			px=ParseNumber();
			while (*text++ != '\n')         // scan to end of line
				;
			break;

		case 'T':               // ^Tyyy,xxx,ppp,ttt waits ttt tics, then draws pic
			TimedPicCommand (helphack);
			break;

		case 'G':               // ^Gyyy,xxx,ppp draws graphic
		{
			int margin,top,bottom;
			int picmid;

			ParsePicCommand (helphack);

			if(!picnum.isValid())
				break;
			FTexture *picture = TexMan(picnum);
			VWB_DrawGraphic (picture, picx&~7,picy, MENU_CENTER);

			//
			// adjust margins
			//
			picmid = picx + picture->GetScaledWidth()/2;
			if (picmid > SCREENMID)
				margin = picx-PICMARGIN;                        // new right margin
			else
				margin = picx+picture->GetScaledWidth()+PICMARGIN;       // new left margin

			top = (picy-TOPMARGIN)/FONTHEIGHT;
			if (top<0)
				top = 0;
			bottom = (picy+picture->GetScaledHeight()-TOPMARGIN)/FONTHEIGHT;
			if (bottom>=TEXTROWS)
				bottom = TEXTROWS-1;

			for (int i=top;i<=bottom;i++)
			{
				if (picmid > SCREENMID)
					rightmargin[i] = margin;
				else
					leftmargin[i] = margin;
			}

			//
			// adjust this line if needed
			//
			if (px < (int) leftmargin[rowon])
				px = leftmargin[rowon];
			break;
		}
	}
}


/*
=====================
=
= NewLine
=
=====================
*/

static void NewLine (void)
{
	if (++rowon == TEXTROWS)
	{
		//
		// overflowed the page, so skip until next page break
		//
		layoutdone = true;
		do
		{
			if (*text == '^')
			{
				char ch = toupper(*(text+1));
				if (ch == 'E' || ch == 'P')
				{
					layoutdone = true;
					return;
				}
			}
			else if (*text == '\0')
			{
				layoutdone = true;
				return;
			}
			text++;
		} while (1);
	}
	px = leftmargin[rowon];
	py+= FONTHEIGHT;
}



/*
=====================
=
= HandleCtrls
=
=====================
*/

static void HandleCtrls (void)
{
	char ch = *text++; // get the character and advance

	if (ch == '\n')
	{
		NewLine ();
		return;
	}
}


/*
=====================
=
= HandleWord
=
=====================
*/

static void HandleWord (void)
{
	char    wword[WORDLIMIT];
	int     wordindex;
	word    wwidth,wheight,newpos;


	//
	// copy the next word into [word]
	//
	wword[0] = *text++;
	wordindex = 1;
	while (byte(*text)>32)
	{
		wword[wordindex] = *text++;
		if (++wordindex == WORDLIMIT)
			I_FatalError ("PageLayout: Word limit exceeded");
	}
	wword[wordindex] = 0;            // stick a null at end for C

	//
	// see if it fits on this line
	//
	VW_MeasurePropString (SmallFont, wword,wwidth,wheight);

	while (px+wwidth > (int) rightmargin[rowon])
	{
		NewLine ();
		if (layoutdone)
			return;         // overflowed page
	}

	//
	// print it
	//
	newpos = px+wwidth;
	if(fontcolor == 255 || textcolor != CR_UNTRANSLATED)
		VWB_DrawPropString (SmallFont, wword, textcolor);
	else
		VWB_DrawPropString (SmallFont, wword, CR_UNTRANSLATED, true, fontcolor);
	px = newpos;

	//
	// suck up any extra spaces
	//
	while (*text == ' ')
	{
		px += SPACEWIDTH;
		text++;
	}
}

/*
=====================
=
= PageLayout
=
= Clears the screen, draws the pics on the page, and word wraps the text.
= Returns a pointer to the terminating command
=
=====================
*/

static void PageLayout (bool shownumber, bool helphack)
{
	const int oldfontcolor = fontcolor;

	fontcolor = 0;

	//
	// clear the screen
	//
	int clearx = 0, cleary = 0, clearw = 320, clearh = 200;
	MenuToRealCoords(clearx, cleary, clearw, clearh, MENU_CENTER);
	VWB_DrawFill(backgroundFlat, clearx, cleary, clearx+clearw, cleary+clearh);
	VWB_DrawGraphic(TexMan("TOPWINDW"), 0, 0, MENU_CENTER);
	VWB_DrawGraphic(TexMan("LFTWINDW"), 0, 8, MENU_CENTER);
	VWB_DrawGraphic(TexMan("RGTWINDW"), 312, 8, MENU_CENTER);
	VWB_DrawGraphic(TexMan("BOTWINDW"), 8, 176, MENU_CENTER);

	for (int i=0; i<TEXTROWS; i++)
	{
		leftmargin[i] = LEFTMARGIN;
		rightmargin[i] = SCREENPIXWIDTH-RIGHTMARGIN;
	}

	px = LEFTMARGIN;
	py = TOPMARGIN;
	rowon = 0;
	layoutdone = false;

	//
	// make sure we are starting layout text (^P first command)
	// [BL] Why? How about assuming ^P?
	//
	while (byte(*text) <= 32)
		text++;

	if (*text == '^' && toupper(*(text+1)) == 'P')
	{
		++text;
		while (*text++ != '\n')
			;
	}


	//
	// process text stream
	//
	do
	{
		unsigned char ch = *text;

		if (ch == '^')
			HandleCommand (helphack);
		else if(ch == '\0')
		{
			// Simulate ^E if one does not exist.
			layoutdone = true;
		}
		else
			if (ch == 9)
			{
				px = (px+8)&0xf8;
				text++;
			}
			else if (ch <= 32)
				HandleCtrls ();
			else
				HandleWord ();

	} while (!layoutdone);

	pagenum++;

	if (shownumber)
	{
		FString str;
		str.Format(gameinfo.PageIndexText, pagenum, numpages);
		px = 213;
		py = 183;

		VWB_DrawPropString (SmallFont, str, gameinfo.FontColors[GameInfo::PAGEINDEX]);
	}

	fontcolor = oldfontcolor;
}

//===========================================================================

/*
=====================
=
= BackPage
=
= Scans for a previous ^P
=
=====================
*/

static void BackPage (void)
{
	pagenum--;
	do
	{
		text--;
		if (*text == '^' && toupper(*(text+1)) == 'P')
			return;
	} while (1);
}


//===========================================================================


/*
=====================
=
= CountPages
=
= Scans an entire layout file (until a ^E) marking all graphics used, and
= counting pages, then caches the graphics in
=
=====================
*/
static void CountPages (void)
{
	const char *textstart = text;
	const char *bombpoint = text+30000;
	numpages = pagenum = 0;

	do
	{
		if (*text == '^')
		{
			char ch = toupper(*++text);
			if (ch == 'P')          // start of a page
				numpages++;
			if (ch == 'E')          // end of file, so load graphics and return
			{
				text = textstart;
				return;
			}
			if (ch == 'G')          // draw graphic command, so mark graphics
			{
				ParsePicCommand (false);
			}
			if (ch == 'T')          // timed draw graphic command, so mark graphics
			{
				ParseTimedCommand (false);
			}
		}
		else if (*text == '\0')
		{
			text = textstart;
			return;
		}
		else
			text++;

	} while (text<bombpoint);

	I_FatalError ("CacheLayoutGraphics: No ^E to terminate file!");
}

/*
=====================
=
= ShowBreifing
=
=====================
*/

static void ShowBriefing(FString str)
{
	VWB_DrawFill(backgroundFlat, 0, 0, screenWidth, screenHeight);

	switch(alignment)
	{
		default:
			px = 8;
			break;
		case TS_Center:
			px = 160;
			break;
		case TS_Right:
			px = 312;
			break;
	}
	py = 8;

	DrawMultiLineText(str, font, textcolor, alignment, anchor);

	VL_FadeIn(0,255,10);
	IN_Ack();
}

void DrawMultiLineText(const FString str, FFont *font, EColorRange color, ETSAlignment align, ETSAnchor anchor)
{
	int oldpa = pa;
	pa = anchor;

	const int basepx = px;
	long pos = -1, oldpos;
	do
	{
		oldpos = pos+1;
		pos = str.IndexOf('\n', oldpos);
		const FString line = str.Mid(oldpos, pos - oldpos);

		word width, height;
		VW_MeasurePropString(font, line, width, height);

		switch(align)
		{
			default:
				px = basepx;
				break;
			case TS_Right:
				px = basepx - width;
				break;
			case TS_Center:
				px = basepx - width/2;
				break;
		}

		VWB_DrawPropString(font, line, color);

		py += font->GetHeight();
	}
	while(pos != -1);

	pa = oldpa;
}

/*
=====================
=
= ShowArticle
=
=====================
*/

// Helphack switches index 11 and 5 so that the keyboard/blaze pics are reversed.
static void ShowArticle (const char *article, bool helphack=false)
{
	bool newpage, firstpage;
	ControlInfo ci;

	text = article;
	CountPages();
	if(numpages == 0)
	{
		// No pages?  Show S3DNA style briefing.
		ShowBriefing(article);
		return;
	}

	VWB_Clear(GPalette.BlackIndex, 0, 0, screenWidth, screenHeight);

	newpage = true;
	firstpage = true;
	fontcolor = 255;
	textcolor = CR_UNTRANSLATED;

	do
	{
		if (newpage)
		{
			newpage = false;
			PageLayout (true, helphack);
			VW_UpdateScreen ();
			if (firstpage)
			{
				VL_FadeIn(0,255,10);
				firstpage = false;
			}
		}
		SDL_Delay(5);

		LastScan = 0;
		ReadAnyControl(&ci);
		Direction dir = ci.dir;
		switch(dir)
		{
			case dir_North:
			case dir_South:
				break;

			default:
				if(ci.button0) dir = dir_South;
				switch(LastScan)
				{
					case sc_UpArrow:
					case sc_PgUp:
					case sc_LeftArrow:
						dir = dir_North;
						break;

					case sc_Enter:
					case sc_DownArrow:
					case sc_PgDn:
					case sc_RightArrow:
						dir = dir_South;
						break;
				}
				break;
		}

		switch(dir)
		{
			default:
				break;
			case dir_North:
			case dir_West:
				if (pagenum>1)
				{
					BackPage ();
					BackPage ();
					newpage = true;
				}
				TicDelay(20);
				break;

			case dir_South:
			case dir_East:
				if (pagenum<numpages)
				{
					newpage = true;
				}
				TicDelay(20);
				break;
		}
	} while (LastScan != sc_Escape && !ci.button1);

	IN_ClearKeysDown ();
}


//===========================================================================

/*
=================
=
= HelpScreens
=
=================
*/
void HelpScreens (void)
{
	int lumpNum = Wads.CheckNumForName("HELPART", ns_global);
	if(lumpNum != -1)
	{
		FMemLump lump = Wads.ReadLump(lumpNum);

		backgroundFlat = TexMan(gameinfo.FinaleFlat);
		ShowArticle(reinterpret_cast<const char*>(lump.GetMem()));
	}

	VW_FadeOut();
}


static bool ShowText(const FString exitText, const FString flat, const FString music, ClusterInfo::ExitType type)
{
	// Use cluster background if set.
	if(!flat.IsEmpty())
		backgroundFlat = TexMan(flat);
	if(!backgroundFlat) // Get default if needed
		backgroundFlat = TexMan(gameinfo.FinaleFlat);

	switch(type)
	{
		case ClusterInfo::EXIT_MESSAGE:
			SD_PlaySound ("misc/1up");

			Message (exitText);

			IN_ClearKeysDown ();
			IN_Ack ();
			return false;
	
		case ClusterInfo::EXIT_LUMP:
		{
			int lumpNum = Wads.CheckNumForName(exitText, ns_global);
			if(lumpNum != -1)
			{
				FMemLump lump = Wads.ReadLump(lumpNum);

				if(!music.IsEmpty())
					StartCPMusic(music);
				ShowArticle(reinterpret_cast<const char*>(lump.GetMem()), !!(IWad::GetGame().Flags & IWad::HELPHACK));
			}

			break;
		}

		default:
			if(!music.IsEmpty())
				StartCPMusic(music);
			ShowArticle(exitText, !!(IWad::GetGame().Flags & IWad::HELPHACK));
			break;
	}

	IN_ClearKeysDown();
	if (MousePresent && IN_IsInputGrabbed())
		IN_CenterMouse();  // Clear accumulated mouse movement
	return true;
}

//
// END ARTICLES
//
bool EndText (int exitClusterNum, int enterClusterNum)
{
	static bool EndTextInProgress = false;

	if(EndTextInProgress)
		return false;
	EndTextInProgress = true;

	// Determine if we're using an exit text or enter text. The enter text
	// overrides the exit text since it's mainly used for entering secret levels.
	// Also collect any information
	FString exitSlideshow;
	FString exitText;
	FString flat;
	FString music;
	ClusterInfo::ExitType type = ClusterInfo::EXIT_STRING;

	if(enterClusterNum >= 0)
	{
		ClusterInfo &enterCluster = ClusterInfo::Find(enterClusterNum);
		if(!enterCluster.EnterText.IsEmpty())
		{
			exitText = enterCluster.EnterText;
			flat = enterCluster.Flat;
			music = enterCluster.Music;
			type = enterCluster.EnterTextType;
			textcolor = enterCluster.TextColor;
			font = enterCluster.TextFont;
			alignment = enterCluster.TextAlignment;
			anchor = enterCluster.TextAnchor;
		}

		exitSlideshow = enterCluster.EnterSlideshow;
	}

	if(exitText.IsEmpty() || exitSlideshow.IsEmpty())
	{
		ClusterInfo &exitCluster = ClusterInfo::Find(exitClusterNum);
		if(exitText.IsEmpty())
		{
			exitText = exitCluster.ExitText;
			flat = exitCluster.Flat;
			music = exitCluster.Music;
			type = exitCluster.ExitTextType;
			textcolor = exitCluster.TextColor;
			font = exitCluster.TextFont;
			alignment = exitCluster.TextAlignment;
			anchor = exitCluster.TextAnchor;
		}
		if(exitSlideshow.IsEmpty())
			exitSlideshow = exitCluster.ExitSlideshow;
	}

	// If there was no text then just carry on
	bool ret = false;
	if(!exitText.IsEmpty())
	{
		ret = true;

		if(type != ClusterInfo::EXIT_MESSAGE)
			VW_FadeOut();
		if(!ShowText(exitText, flat, music, type))
			return false;
	}

	if(!exitSlideshow.IsEmpty())
	{
		IntermissionInfo *intermission = IntermissionInfo::Find(exitSlideshow);
		ShowIntermission(intermission);
	}

	EndTextInProgress = false;
	return ret;
}

// Episode start execute entertext.
void EnterText(unsigned int cluster)
{
	ClusterInfo &clusterInfo = ClusterInfo::Find(cluster);

	if(!clusterInfo.EnterText.IsEmpty())
	{
		textcolor = clusterInfo.TextColor;
		font = clusterInfo.TextFont;
		alignment = clusterInfo.TextAlignment;
		anchor = clusterInfo.TextAnchor;

		if(clusterInfo.EnterTextType != ClusterInfo::EXIT_MESSAGE)
			VW_FadeOut();
		ShowText(clusterInfo.EnterText, clusterInfo.Flat, clusterInfo.Music, clusterInfo.EnterTextType);
	}

	if(!clusterInfo.EnterSlideshow.IsEmpty())
	{
		IntermissionInfo *intermission = IntermissionInfo::Find(clusterInfo.EnterSlideshow);
		ShowIntermission(intermission);
	}
}
