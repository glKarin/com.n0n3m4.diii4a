#ifndef __WL_PLAY_H__
#define __WL_PLAY_H__

/*
=============================================================================

							WL_PLAY DEFINITIONS

=============================================================================
*/

#define BASEMOVE                35
#define RUNMOVE                 70
#define BASETURN                35
#define RUNTURN                 70

#define JOYSCALE                2

extern  bool noadaptive;
extern  unsigned        tics;
extern  int             viewsize;

//
// current user input
//
struct TicCmd_t
{
	int controlx,controly, controlstrafe; // range from -100 to 100
	int controlpanx, controlpany;
	bool buttonstate[NUMBUTTONS], ambuttonstate[NUMAMBUTTONS];
	bool buttonheld[NUMBUTTONS], ambuttonheld[NUMAMBUTTONS];
};
extern unsigned int ConsolePlayer;
extern TicCmd_t control[MAXPLAYERS];
extern  exit_t      playstate;
extern  bool        madenoise;
extern  int         godmode;
extern	bool		notargetmode;

extern  bool        demorecord,demoplayback;
extern  int8_t      *demoptr, *lastdemoptr;
extern  memptr      demobuffer;

void    PlayLoop (void);

void    InitRedShifts (void);
void    FinishPaletteShifts (void);

void    PollControls (bool);
int     StopMusic(void);
void    StartMusic(void);
void    ContinueMusic(int offs);
void    StartDamageFlash (int damage);
void    StartBonusFlash (void);

void CalcTics();
void Delay(int wolfticks);
int32_t GetTimeCount();
void ResetTimeCount();

extern  int32_t     funnyticount;           // FOR FUNNY BJ FACE

extern  bool        noclip,ammocheat,mouselook;
extern  int         singlestep;
extern  unsigned int extravbls;

#endif
