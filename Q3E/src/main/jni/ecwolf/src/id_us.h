//
//	ID Engine
//	ID_US.h - Header file for the User Manager
//	v1.0d1
//	By Jason Blochowiak
//

#ifndef	__ID_US__
#define	__ID_US__

#include "v_font.h"

#ifdef	__DEBUG__
#define	__DEBUG_UserMgr__
#endif

typedef struct
{
	int x,y;
} Point;
typedef struct
{
	Point ul,lr;
} Rect;

//#define	HELPTEXTLINKED

#define	MaxX	320
#define	MaxY	200

#define	MaxHelpLines	500

#define	MaxHighName	57
#define	MaxScores	7
typedef	struct
{
	char	name[MaxHighName + 1];
	int32_t	score;
	FString	completed;
	char	graphic[9];
} HighScore;

#define	MaxString	128	// Maximum input string size

typedef	struct
{
	int	x,y,
		w,h,
		px,py;
} WindowRec;	// Record used to save & restore screen windows

extern	bool		ingame,		// Set by game code if a game is in progress
					loadedgame;	// Set if the current game was loaded
extern	word		PrintX,PrintY;	// Current printing location in the window
extern	word		WindowX,WindowY,// Current location of window
					WindowW,WindowH;// Current size of window

extern	bool		(*USL_SaveGame)(int),(*USL_LoadGame)(int);
extern	void		(*USL_ResetGame)(void);
extern	HighScore	Scores[];

#define	US_HomeWindow()	{PrintX = WindowX; PrintY = WindowY;}

void			US_TextScreen(void),
				US_UpdateTextScreen(void),
				US_FinishTextScreen(void);
void			US_DrawWindow(word x,word y,word w,word h);
void			US_CenterWindow(word,word);
void			US_SaveWindow(WindowRec *win),
				US_RestoreWindow(WindowRec *win);
void 			US_ClearWindow(void);
void			US_PrintCentered(const char *s),
				US_CPrint(FFont *font, const char *s, EColorRange translation=CR_UNTRANSLATED),
				US_CPrintLine(FFont *font, const char *s, EColorRange translation=CR_UNTRANSLATED),
				US_Print(FFont *font, const char *s, EColorRange translation=CR_UNTRANSLATED);

void			US_PrintUnsigned(longword n);
void			US_PrintSigned(int32_t n);
void			US_StartCursor(void),
				US_ShutCursor(void);
void			US_DisplayHighScores(int which);
extern	bool	US_UpdateCursor(void);
bool			US_LineInput(FFont *font, int x,int y,char *buf,const char *def,bool escok,
							int maxchars,int maxwidth, byte clearcolor, EColorRange translation=CR_UNTRANSLATED);

void	        USL_PrintInCenter(const char *s,Rect r);
char 	        *USL_GiveSaveName(word game);

#endif
