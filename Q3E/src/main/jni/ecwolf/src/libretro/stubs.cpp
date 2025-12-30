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
