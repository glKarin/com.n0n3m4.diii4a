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
#include "state_machine.h"

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

static unsigned leftmargin[TEXTROWS];
static unsigned rightmargin[TEXTROWS];
static EColorRange textcolor;
static FTextureID picnum;
static FTexture *backgroundFlat = NULL;
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

static const char *text(wl_state_t *state)
{
	return state->article.GetChars() + state->textposition;
}

static char nextchar(wl_state_t *state)
{
	return (state->article.GetChars() + state->textposition++)[0];
}


static void RipToEOL (wl_state_t *state)
{
	while (nextchar(state) != '\n')         // scan to end of line
		;
}


/*
=====================
=
= ParseNumber
=
=====================
*/

static int ParseNumber (wl_state_t *state)
{
	//
	// scan until a number is found
	//
	char ch = text(state)[0];
	while (ch < '0' || ch >'9')
		ch = nextchar(state);

	//
	// copy the number out
	//
	char num[80];
	char *numptr = num;
	do
	{
		*numptr++ = ch;
		nextchar(state);
		ch = text(state)[0];
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

static void ParsePicCommand (wl_state_t *state, bool helphack, bool norip=false)
{
	state->picy=ParseNumber(state);
	state->picx=ParseNumber(state);

	// Skip over whitespace
	while(text(state)[0] == ' ' || text(state)[0] == '\t')
		nextchar(state);

	if(text(state)[0] == '[')
	{
		const char* texName = text(state) + 1;
		unsigned int len = 0;
		while(nextchar(state), text(state)[0] != ']')
			++len;
		picnum = TexMan.GetTexture(FString(texName, len), FTexture::TEX_Any);
		nextchar(state);
	}
	else
	{
		int num=ParseNumber(state);

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
		RipToEOL (state);
}


static void ParseTimedCommand (wl_state_t *state, bool helphack)
{
	ParsePicCommand(state, helphack, true);
	state->picdelay=ParseNumber(state);
	RipToEOL (state);
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

static void TimedPicCommand (wl_state_t *state, bool helphack)
{
	ParseTimedCommand (state, helphack);

	//
	// update the screen, and wait for time delay
	//
	VW_UpdateScreen ();

	//
	// wait for time
	//
	Delay(state->picdelay);

	//
	// draw pic
	//
	if(picnum.isValid())
		VWB_DrawGraphic (TexMan(picnum), state->picx&~7, state->picy, MENU_CENTER);
}


/*
=====================
=
= HandleCommand
=
=====================
*/

static void HandleCommand (wl_state_t *state, bool helphack)
{
	nextchar(state);
	switch (toupper(text(state)[0]))
	{
		case 'B':
		{
			double bx = ParseNumber(state);
			double by = ParseNumber(state);
			double bw = ParseNumber(state);
			double bh = ParseNumber(state);
			MenuToRealCoords(bx, by, bw, bh, MENU_CENTER);
			VWB_DrawFill(backgroundFlat, (int)bx, (int)by, (int)(bx+bw), (int)(by+bh));
			RipToEOL(state);
			break;
		}
		case ';':               // comment
			RipToEOL(state);
			break;
		case 'P':               // ^P is start of next page, ^E is end of file
		case 'E':
			state->layoutdone = true;
			state->textposition--;             // back up to the '^'
			break;

		case 'C':               // ^c<hex digit> changes text color
		{
			nextchar(state);
			char i = toupper(text(state)[0]);
			if(i == '[') // Textcolo translation
			{
				state->fontcolor = 255;
				const BYTE *colorname = (const BYTE *)(text);
				textcolor = V_ParseFontColor(colorname, CR_UNTRANSLATED, CR_UNTRANSLATED+1);
				while(nextchar(state) != ']');
			}
			else
			{
				textcolor = CR_UNTRANSLATED;

				if (i>='0' && i<='9')
					state->fontcolor = i-'0';
				else if (i>='A' && i<='F')
					state->fontcolor = i-'A'+10;

				state->fontcolor *= 16;
				nextchar(state);
				i = toupper(text(state)[0]);
				if (i>='0' && i<='9')
					state->fontcolor += i-'0';
				else if (i>='A' && i<='F')
					state->fontcolor += i-'A'+10;
				nextchar(state);
			}
			break;
		}

		case '>':
			px = 160;
			nextchar(state);
			break;

		case 'L':
			py=ParseNumber(state);
			state->rowon = (py-TOPMARGIN)/FONTHEIGHT;
			py = TOPMARGIN+state->rowon*FONTHEIGHT;
			px=ParseNumber(state);
			while (nextchar(state) != '\n')         // scan to end of line
				;
			break;

		case 'T':               // ^Tyyy,xxx,ppp,ttt waits ttt tics, then draws pic
			TimedPicCommand (state, helphack);
			break;

		case 'G':               // ^Gyyy,xxx,ppp draws graphic
		{
			int margin,top,bottom;
			int picmid;

			ParsePicCommand (state, helphack);

			if(!picnum.isValid())
				break;
			FTexture *picture = TexMan(picnum);
			VWB_DrawGraphic (picture, state->picx&~7,state->picy, MENU_CENTER);

			//
			// adjust margins
			//
			picmid = state->picx + picture->GetScaledWidth()/2;
			if (picmid > SCREENMID)
				margin = state->picx-PICMARGIN;                        // new right margin
			else
				margin = state->picx+picture->GetScaledWidth()+PICMARGIN;       // new left margin

			top = (state->picy-TOPMARGIN)/FONTHEIGHT;
			if (top<0)
				top = 0;
			bottom = (state->picy+picture->GetScaledHeight()-TOPMARGIN)/FONTHEIGHT;
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
			if (px < (int) leftmargin[state->rowon])
				px = leftmargin[state->rowon];
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

static void NewLine (wl_state_t *state)
{
	if (++state->rowon == TEXTROWS)
	{
		//
		// overflowed the page, so skip until next page break
		//
		state->layoutdone = true;
		do
		{
			if (text(state)[0] == '^')
			{
				char ch = toupper(text(state)[1]);
				if (ch == 'E' || ch == 'P')
				{
					state->layoutdone = true;
					return;
				}
			}
			else if (text(state)[0] == '\0')
			{
				state->layoutdone = true;
				return;
			}
			nextchar(state);
		} while (1);
	}
	px = leftmargin[state->rowon];
	py+= FONTHEIGHT;
}



/*
=====================
=
= HandleCtrls
=
=====================
*/

static void HandleCtrls (wl_state_t *state)
{
	char ch = nextchar(state); // get the character and advance

	if (ch == '\n')
	{
		NewLine (state);
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

static void HandleWord (wl_state_t *state)
{
	char    wword[WORDLIMIT];
	int     wordindex;
	word    wwidth,wheight,newpos;


	//
	// copy the next word into [word]
	//
	wword[0] = nextchar(state);
	wordindex = 1;
	while (byte(text(state)[0])>32)
	{
		wword[wordindex] = nextchar(state);
		if (++wordindex == WORDLIMIT)
			I_FatalError ("PageLayout: Word limit exceeded");
	}
	wword[wordindex] = 0;            // stick a null at end for C

	//
	// see if it fits on this line
	//
	VW_MeasurePropString (SmallFont, wword,wwidth,wheight);

	while (px+wwidth > (int) rightmargin[state->rowon])
	{
		NewLine (state);
		if (state->layoutdone)
			return;         // overflowed page
	}

	//
	// print it
	//
	newpos = px+wwidth;
	if(state->fontcolor == 255 || textcolor != CR_UNTRANSLATED)
		VWB_DrawPropString (SmallFont, wword, textcolor);
	else
		VWB_DrawPropString (SmallFont, wword, CR_UNTRANSLATED, true, state->fontcolor);
	px = newpos;

	//
	// suck up any extra spaces
	//
	while (text(state)[0] == ' ')
	{
		px += SPACEWIDTH;
		nextchar(state);
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

static void PageLayout (wl_state_t *state, bool shownumber, bool helphack)
{
	const int oldfontcolor = state->fontcolor;

	state->fontcolor = 0;

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
	state->rowon = 0;
	state->layoutdone = false;

	//
	// make sure we are starting layout text (^P first command)
	// [BL] Why? How about assuming ^P?
	//
	while (byte(text(state)[0]) <= 32)
		nextchar(state);

	if (text(state)[0] == '^' && toupper(text(state)[1]) == 'P')
	{
		while (nextchar(state) != '\n')
			;
	}


	//
	// process text stream
	//
	do
	{
		unsigned char ch = text(state)[0];

		if (ch == '^')
			HandleCommand (state, helphack);
		else if(ch == '\0')
		{
			// Simulate ^E if one does not exist.
			state->layoutdone = true;
		}
		else
			if (ch == 9)
			{
				px = (px+8)&0xf8;
				state->textposition++;
			}
			else if (ch <= 32)
				HandleCtrls (state);
			else
				HandleWord (state);

	} while (!state->layoutdone);

	state->pagenum++;

	if (shownumber)
	{
		FString str;
		str.Format("pg %d of %d", (int) state->pagenum, (int) state->numpages);
		px = 213;
		py = 183;

		VWB_DrawPropString (SmallFont, str, gameinfo.FontColors[GameInfo::PAGEINDEX]);
	}

	state->fontcolor = oldfontcolor;
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

static void BackPage (wl_state_t *state)
{
	state->pagenum--;
	do
	{
		state->textposition--;
		if (text(state)[0] == '^' && toupper(text(state)[1]) == 'P')
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
static int CountPages (wl_state_t *state)
{
	const char *bombpoint = text(state)+30000;
	int numpages = 0;

	do
	{
		if (text(state)[0] == '^')
		{
			nextchar(state);
			char ch = toupper(text(state)[0]);
			if (ch == 'P')          // start of a page
				numpages++;
			if (ch == 'E')          // end of file, so load graphics and return
			{
				state->textposition = 0;
				return numpages;
			}
			if (ch == 'G')          // draw graphic command, so mark graphics
			{
				ParsePicCommand (state, false);
			}
			if (ch == 'T')          // timed draw graphic command, so mark graphics
			{
				ParseTimedCommand (state, false);
			}
		}
		else if (text(state)[0] == '\0')
		{
			state->textposition = 0;
			return numpages;
		}
		else
			nextchar(state);

	} while (text(state)<bombpoint);

	state->textposition = 0;
	I_FatalError ("CacheLayoutGraphics: No ^E to terminate file!");
	return numpages;
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
static void ShowArticle (wl_state_t *state, const FString &article, wl_stage_t nextStage, bool helphack=false)
{
	state->article = article;
	state->textposition = 0;
	state->numpages = CountPages(state);
	state->textposition = 0;
	state->pagenum = 0;
	printf ("numpages = %d\n", state->numpages);
	if(state->numpages == 0)
	{
		// No pages?  Show S3DNA style briefing.
		ShowBriefing(article);
		
		State_FadeIn(state, 0,255,10);
		State_Ack(state);
		state->stage = nextStage;
		return;
	}
	VWB_Clear(GPalette.BlackIndex, 0, 0, screenWidth, screenHeight);

	state->stage = TEXT_READER_STEP;
	state->newpage = true;
	state->firstpage = true;
	state->fontcolor = 255;
	textcolor = CR_UNTRANSLATED;
	
	return;
}

bool TextReaderStep(wl_state_t *state, const wl_input_state_t *input) {
	if (state->newpage)
	{
		state->newpage = false;
		PageLayout (state, true, false);
		if (state->firstpage)
		{
			state->firstpage = false;
			State_FadeIn(state, 0,255,10);
		}
	}
	
	if (input->menuDir == dir_North || input->menuDir == dir_West)
	{
		if (state->pagenum>1)
		{
			BackPage (state);
			BackPage (state);
			state->newpage = true;
		}
	} else if (input->menuDir == dir_South || input->menuDir == dir_East || input->menuEnter)
	{
		if (state->pagenum<state->numpages)
		{
			state->newpage = true;
		}
	} else if (input->menuBack) {
		state->stage = state->stageAfterIntermission;
	}
	VW_UpdateScreen ();
	return true;
}

//===========================================================================

/*
=================
=
= HelpScreens
=
=================
*/
#ifdef TODO
void HelpScreens (wl_state_t *state)
{
	int lumpNum = Wads.CheckNumForName("HELPART", ns_global);
	if(lumpNum != -1)
	{
		FMemLump lump = Wads.ReadLump(lumpNum);

		backgroundFlat = TexMan(gameinfo.FinaleFlat);
		ShowArticle(state,reinterpret_cast<const char*>(lump.GetMem()));
	}

	State_FadeOut(state);
}
#endif

static bool ShowText(wl_state_t *state, const FString exitText, const FString flat, const FString music, ClusterInfo::ExitType type,
		     wl_stage_t nextStage)
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
			state->stage = nextStage;

			State_Ack (state);
			return false;
	
		case ClusterInfo::EXIT_LUMP:
		{
			int lumpNum = Wads.CheckNumForName(exitText, ns_global);
			if(lumpNum != -1)
			{
				FMemLump lump = Wads.ReadLump(lumpNum);

				if(!music.IsEmpty())
					StartCPMusic(music);
				ShowArticle(state, reinterpret_cast<const char*>(lump.GetMem()), nextStage,
					    !!(IWad::GetGame().Flags & IWad::HELPHACK));
			}

			break;
		}

		default:
			if(!music.IsEmpty())
				StartCPMusic(music);
			ShowArticle(state, exitText, nextStage, !!(IWad::GetGame().Flags & IWad::HELPHACK));
			break;
	}
	return true;
}

//
// END ARTICLES
//
bool TransitionText (wl_state_t *state, int exitClusterNum, int enterClusterNum)
{
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

	if(exitClusterNum >= 0 && (exitText.IsEmpty() || exitSlideshow.IsEmpty()))
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
	if(!exitText.IsEmpty())
	{
		if(!ShowText(state, exitText, flat, music, type, LEVEL_TRANSITION_INTERMISSION_START))
			return false;
	} else
		state->stage = LEVEL_TRANSITION_INTERMISSION_START;
	state->transitionSlideshow = exitSlideshow;
	return false;
}

bool LevelEnterIntermissionStart(wl_state_t *state)
{
	if(state->transitionSlideshow.IsEmpty())
	{
		state->stage = state->stageAfterIntermission;
		return false;
	}

	InitIntermission(&state->intermission, state->transitionSlideshow, false);
	state->wasAcked = false;
	state->stage = PLAY_INTERMISSION;
	return false;
}

