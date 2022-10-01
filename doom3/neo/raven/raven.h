#ifndef _KARIN_RAVEN_H
#define _KARIN_RAVEN_H

#include "idlib/containers/Pair.h"
#include "idlib/rvMemSys.h"
#include "idlib/math/Interpolate.h"
#include "idlib/threads/AutoCrit.h"
#include "idlib/AutoPtr.h"
#include "idlib/LexerFactory.h"

#define TIME_THIS_SCOPE(x)
#define STRINGIZE_INDIRECT(F, X) F(X)
#define STRINGIZE(X) #X
#define __LINESTR__ STRINGIZE_INDIRECT(STRINGIZE, __LINE__)
#define __FILELINEFUNC__ (__FILE__ " " __LINESTR__ " " __FUNCTION__)
#define __FUNCLINE__ ( __FUNCTION__ " " __LINESTR__ )

#define PC_CVAR_ARCHIVE CVAR_ARCHIVE

// Str.h
// ddynerman: team colors
#define S_COLOR_MARINE				"^c683"
#define S_COLOR_STROGG				"^c950"
#define S_COLOR_ALERT				"^c920"
// ddynerman: MP icons
#define I_VOICE_ENABLED				"^ivce"
#define I_VOICE_DISABLED			"^ivcd"
#define I_FRIEND_ENABLED			"^ifde"
#define I_FRIEND_DISABLED			"^ifdd"
#define I_FLAG_MARINE				"^iflm"
#define I_FLAG_STROGG				"^ifls"
#define I_READY						"^iyrd"
#define I_NOT_READY					"^inrd"
// ddynerman: MP icons
#define I_VOICE_ENABLED				"^ivce"
#define I_VOICE_DISABLED			"^ivcd"
#define I_FRIEND_ENABLED			"^ifde"
#define I_FRIEND_DISABLED			"^ifdd"
#define I_FLAG_MARINE				"^iflm"
#define I_FLAG_STROGG				"^ifls"
#define I_READY						"^iyrd"
#define I_NOT_READY					"^inrd"

// RAVEN BEGIN
// cdr: AASTactical 

// feature bits
#define FEATURE_COVER				BIT(0)		// provides cover
#define FEATURE_LOOK_LEFT			BIT(1)		// attack by leaning left
#define FEATURE_LOOK_RIGHT			BIT(2)		// attack by leaning right
#define FEATURE_LOOK_OVER			BIT(3)		// attack by leaning over the cover
#define FEATURE_CORNER_LEFT			BIT(4)		// is a left corner
#define FEATURE_CORNER_RIGHT		BIT(5)		// is a right corner
#define FEATURE_PINCH				BIT(6)		// is a tight area connecting two larger areas
#define FEATURE_VANTAGE				BIT(7)		// provides a good view of the sampled area as a whole
// RAVEN END

class ThreadedAlloc;

// RAVEN BEGIN
// amccarthy:  tags for memory allocation tracking.  When updating this list please update the
// list of discriptions in Heap.cpp as well.
typedef enum {
	MA_NONE = 0,	
	
	MA_OPNEW,
	MA_DEFAULT,
	MA_LEXER,
	MA_PARSER,
	MA_AAS,
	MA_CLASS,
	MA_SCRIPT,
	MA_CM,
	MA_CVAR,
	MA_DECL,
	MA_FILESYS,
	MA_IMAGES,
	MA_MATERIAL,
	MA_MODEL,
	MA_FONT,
	MA_RENDER,
	MA_VERTEX,
	MA_SOUND,
	MA_WINDOW,
	MA_EVENT,
	MA_MATH,
	MA_ANIM,
	MA_DYNAMICBLOCK,
	MA_STRING,
	MA_GUI,
	MA_EFFECT,
	MA_ENTITY,
	MA_PHYSICS,
	MA_AI,
	MA_NETWORK,

	MA_DO_NOT_USE,		// neither of the two remaining enumerated values should be used (no use of MA_DO_NOT_USE prevents the second dword in a memory block from getting the value 0xFFFFFFFF)
	MA_MAX				// <- this enumerated value is a count and cannot exceed 32 (5 bits are used to encode tag within memory block with rvHeap.cpp)
} Mem_Alloc_Types_t;

#endif
