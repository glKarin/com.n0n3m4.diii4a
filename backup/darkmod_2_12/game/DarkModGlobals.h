/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2004            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * DESCRIPTION: This file contains all global identifiers, variables and
 * structures. Please note that global variables should be kept to a minimum
 * and only what is really neccessary should go in here.
 *
 *****************************************************************************/

#ifndef DARKMODGLOBALS_H
#define DARKMODGLOBALS_H

#include <stdio.h>
#include "Game_local.h"
#include "../framework/Licensee.h"

enum VersionCheckResult
{
	EQUAL,
	OLDER,
	NEWER,
};

// Compares the version pair <major, minor> to <toMajor, toMinor> and returns the result
// @returns: OLDER when <major, minor> is OLDER than <toMajor, toMinor>
VersionCheckResult CompareVersion(int major, int minor, int toMajor, int toMinor);

/*!
Darkmod LAS
*/
#include "darkModLAS.h"

class IniFile;
typedef std::shared_ptr<IniFile> IniFilePtr;

typedef enum {
	LT_INIT,
	LT_FORCE,			// Never use this
	LT_ERROR,			// Errormessage
	LT_BEGIN,			// Begin function
	LT_END,				// Leave function
	LT_WARNING,
	LT_INFO,
	LT_DEBUG,
	LT_COUNT
} LT_LogType;

// The log class determines a class or group of
// actions belonging together. This does not neccessarily mean 
// that this class is a C++ type class. For example. We have a class
// Frobbing which contains all loginfo concerning frobbing an item
// independent of it's class (like AI, item, doors, switche, etc.).

// greebo: NOTE: Keep these in accordance to the ones in scripts/tdm_defs.script!
typedef enum {
	LC_INIT,
	LC_FORCE,			// Never use this
	LC_MISC,
	LC_SYSTEM,			// Initialization, INI file and such stuff
	LC_FROBBING,		// Everything that has to do with frobbing
	LC_AI,				// same for AI
	LC_SOUND,			// same for sound
	LC_FUNCTION,		// general logging for functions (being, end, etc).
	LC_ENTITY,
	LC_INVENTORY,		// Everything that has to do with inventory
	LC_LIGHT,
	LC_WEAPON,
	LC_MATH,
	LC_MOVEMENT,		// mantling, leaning, ledge hanging, etc...
	LC_LOCKPICK,
	LC_FRAME,			// This is intended only as a framemarker and will always switched on if at least one other option is on.
	LC_STIM_RESPONSE,
	LC_OBJECTIVES,
	LC_DIFFICULTY,		// anything difficulty-related
	LC_CONVERSATION,	// conversation/dialogue stuff
	LC_MAINMENU,		// main menu logging
	LC_AAS,				// grayman - AAS area logging
	LC_STATE,			// grayman #3559 - State logging
	LC_COUNT
} LC_LogClass;

class idCmdArgs;
class CDarkModPlayer;

class CGlobal {
public:
	CGlobal();
	~CGlobal();

	void Shutdown();
	void Init();

	void LogPlane(idStr const &Name, idPlane const &Plane);
	void LogVector(idStr const &Name, idVec3 const &Vector);
	void LogMat3(idStr const &Name, idMat3 const &Matrix);
	void LogString(const char *Format, ...);

	/**
	* Lookup the name of a the surface for a given material
	* Needed to incorporate new surface types
	* Stores the result in the strIn argument.
	* If the surface is not found or invalid, stores "none"
	**/
	void GetSurfName(const idMaterial *material, idStr &strIn);

	/**
	 * greebo: Returns the surface name for the given material
	 * or an empty string if not found.
	 **/
	idStr GetSurfName(const idMaterial* material);

	/** 
	 * greebo: Returns the surface hardness string ("soft", "hard")
	 * for the given material type.
	 */
	const idStr& GetSurfaceHardness(const char* surfName);

	// Returns the darkmod path
	static std::string GetDarkmodPath();

	// Helper to retrieve the path of a mod, e.g. C:\Games\Doom3\alchemist
	static std::string GetModPath(const std::string& modName);

	// Converts a string to a logclass (LC_COUNT) if nothing found.
	static LC_LogClass GetLogClassForString(const char* str);

	static void ArgCompletion_LogClasses( const idCmdArgs &args, void(*callback)( const char *s ) );

private:
	void LoadINISettings(const IniFilePtr& iniFile);

	void CheckLogArray(const IniFilePtr& iniFile, const char* key, LT_LogType logType);
	void CheckLogClass(const IniFilePtr& iniFile, const char* key, LC_LogClass logClass);

	// Sets up the surface hardness mapping
	void InitSurfaceHardness();

	// A table for retrieving indices out of input strings
	idHashIndex m_SurfaceHardnessHash;
	
	// A list of hardness strings ("hard", "soft")
	idStringList m_SurfaceHardness;

public:
	/**
	 * LogFile is initialized to NULL if no Logfile is in use. Otherwise it
	 * contains a filepointer which can be used to write debugging information
	 * to the logfile. The logsettings are switched on in the INI file.
	 */
	FILE *m_LogFile;
	bool m_LogArray[LT_COUNT];
	bool m_ClassArray[LC_COUNT];

	LC_LogClass		m_LogClass;
	LT_LogType		m_LogType;
	int			m_Frame;
	const char		*m_Filename;
	char			m_DriveLetter;		// Remember the last driveletter
	int				m_Linenumber;

public:

	/*!
	* Global game settings, default values
	*/

	/**
	* Maximum distance of reach for frobbing (updated based on map objects)
	**/
	float m_MaxFrobDistance;

	/**
	* List of AI Acuities
	**/
	idStrList m_AcuityNames;
	idHashIndex m_AcuityHash;
};

extern CGlobal g_Global;
extern const char *g_LCString[];

#define LOGBUILD

#ifdef LOGBUILD
#define DM_LOG(lc, lt)				if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global
#define LOGSTRING					.LogString
#define LOGVECTOR					.LogVector
#define DM_LOGVECTOR3(lc, lt, s, v)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogVector(s, v)
#define DM_LOGPLANE(lc, lt, s, p)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogPlane(s, p)
#define DM_LOGMAT3(lc, lt, s, m)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogMat3(s, m)
#else
#define DM_LOG(lc, lt)
#define LOGSTRING 
#define LOGVECTOR
#define DM_LOGVECTOR3(lc, lt, s, v)
#define DM_LOGPLANE(lc, lt, s, p)
#define DM_LOGMAT3(lc, lt, s, m)
#endif

/**
*	Message pragma so we can show file and line info in comments easily
*	Same principle as the one below but simpler to implement and use.
*   Been using it for about 8 or 9 years not sure where I found it
*	but I did have a subscription to windows developer journal so maybe thats where.
*	Usage: #pragma Message( "your message goes here")
*	
*	Submitted by Thelvyn
*/
#ifndef MacroStr2
#define MacroStr(x)   #x
#define MacroStr2(x)  MacroStr(x)
#define Message(desc) message(__FILE__ "(" MacroStr2(__LINE__) ") :" #desc)
#endif

/**
* The DARKMOD_NOTE macro makes it easy to add reminders which are shown when code is compiled. 
* You can double click on a reminder in the Output Window and jump to the line when using VC. 
* Adapted from highprogrammer.com (Originally from Windows Developer Journal).
*
* Usage: #pragma message(DARKMOD_NOTE "your reminder goes here")
*
* Submitted by Zaccheus
*/

#define DARKMOD_NOTE_AUX_STR( _S_ )             #_S_ 
#define DARKMOD_NOTE_AUX_MAKESTR( _M_, _L_ )    _M_(_L_) 
#define DARKMOD_NOTE_AUX_LINE                   DARKMOD_NOTE_AUX_MAKESTR(DARKMOD_NOTE_AUX_STR,__LINE__) 
#define DARKMOD_NOTE                            __FILE__ "(" DARKMOD_NOTE_AUX_LINE ") : DARKMOD_NOTE: " 

// A generic function to handle linear interpolation. J.C.Denton
template<class T> ID_INLINE T Lerp( const T &v1, const T &v2, const float l ) {
	
	T tRetVal;
	if ( l <= 0.0f ) {
		tRetVal = v1;
	} else if ( l >= 1.0f ) {
		tRetVal = v2;
	} else {
		tRetVal = v1 + l * ( v2 - v1 );
	}

	return tRetVal;
}


#endif
