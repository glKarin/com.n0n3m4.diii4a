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

#include "precompiled.h"
#pragma hdrstop



#include "DarkModGlobals.h"
#include "SndPropLoader.h"
#include "SndProp.h"
#include "Relations.h"
#include "ModMenu.h"
#include "ai/AI.h"
#include "IniFile.h"
#include "Debug.h"

#ifdef MACOS_X
#include <mach-o/dyld.h>
#endif

#include <StdFilesystem.h>
namespace fs = stdext;

class idAI;

VersionCheckResult CompareVersion(int major, int minor, int toMajor, int toMinor)
{
	// Equality check
	if (major == toMajor && minor == toMinor) return EQUAL;

	// Not equal, find the difference
	if (major < toMajor)
	{
		return OLDER; // major version is older
	}
	else if (major > toMajor)
	{
		return NEWER;
	}
	else // Major version equal, check minor versions
	{
		return (minor < toMinor) ? OLDER : NEWER;
	}
}

// Name of the logfile to use for Dark Mod logging
#if defined(__linux__)
const char* DARKMOD_LOGFILE = "/tmp/DarkMod.log";
#elif MACOS_X
const char* DARKMOD_LOGFILE = "~/Library/Logs/DarkMod.log";
#else // Windows
const char* DARKMOD_LOGFILE = "DarkMod.temp.log";
#endif

static const char *LTString[LT_COUNT+1] = {
	"INI",
	"FRC",
	"ERR",
	"BEG",
	"END",
	"WAR",
	"INF",
	"DEB",
	"---"
};

static const char *LCString[LC_COUNT+1] = {
	"INIT",
	"FORCE",
	"MISC",
	"SYSTEM",
	"FROBBING",
	"AI",
	"SOUND",
	"FUNCTION",
	"ENTITY",
	"INVENTORY",
	"LIGHT",
	"WEAPON",
	"MATH",
	"MOVEMENT",
	"LOCKPICK",
	"FRAME",
	"STIMRESP",
	"OBJECTIVES",
	"DIFFICULTY",
	"CONVERSATION",
	"MAINMENU",
	"(empty)"
};

#define INI_DEBUG_SECTION "Debug"

// declare various global objects
CsndPropLoader	g_SoundPropLoader;
CsndProp		g_SoundProp;

#ifdef MACOS_X
// Platform-specific method to expand the ~ in a path with a full path
// adjusted from http://developer.apple.com/library/mac/#qa/qa2007/qa1549.html

#include <glob.h>

idStr GetExpandedTildePath(const char* path)
{
	assert(path != NULL); // don't accept bogus input

	idStr result = path;

	// Check if the path starts with a tilde 
	if (result.CmpPrefix("~") == 0)
	{
		// Got a tilde at the front, replace that with the home folder using glob()
		glob_t globbuf;

		if (glob("~", GLOB_TILDE, NULL, &globbuf) == 0)
		{
			char** v = globbuf.gl_pathv; // list of matched pathnames
	        
			// Replace the tilde
			result.StripLeading('~');
			result = v[0] + result;

			globfree(&globbuf);
		}
	}

	return result;
}
#endif

CGlobal::CGlobal()
{
	//stgatilov: please, write only necessary zero-initialization here!
	//move all real initialization logic into Init method
	memset(m_LogArray, 0, sizeof(m_LogArray));
	memset(m_ClassArray, 0, sizeof(m_ClassArray));
	m_LogFile = 0;
	m_MaxFrobDistance = 0;
	m_LogClass = LC_SYSTEM;
	m_LogType = LT_DEBUG;
	m_Filename = "undefined";
	m_DriveLetter = 0;
	m_Linenumber = 0;
}

CGlobal::~CGlobal()
{
	Shutdown();
}

void CGlobal::Shutdown() {
	m_SurfaceHardness.ClearFree();
	m_SurfaceHardnessHash.ClearFree();
	if (m_LogFile != NULL)
	{
		fclose(m_LogFile);
		m_LogFile = 0;
	}
	m_AcuityNames.ClearFree();
	m_AcuityHash.ClearFree();
}

void CGlobal::Init()
{
	// Initialise all logtypes to false
	for (int i = 0; i < LT_COUNT; ++i)
	{
		m_LogArray[i] = false;
	}

	// Except for these two
	m_LogArray[LT_INIT] = true;			// This is always on
	m_LogArray[LT_FORCE] = true;			// This is always on
	
	// Initialise all logging values to false
	for (int i = 0; i < LC_COUNT; ++i)
	{
		m_ClassArray[i] = false;
	}

	// Except for these two, these are always on
	m_ClassArray[LC_INIT] = true;
	m_ClassArray[LC_FORCE] = true;


	idStr logFilePath = DARKMOD_LOGFILE;

#ifdef MACOS_X
	// In OSX we need to resolve the user's home folder first
	logFilePath = GetExpandedTildePath(logFilePath);
#endif
	
	m_LogFile = fopen(logFilePath.c_str(), "w+b");

	if (m_LogFile != NULL)
	{
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Initializing mod logging\r");
	}

	// initialize the AI Acuities hash

	/**
	* Define AI Acuities Here:
	* NOTE: If you add an acuity, your total number of acuities
	* must be below s_MAXACUITIES defined in AI.h, unless you
	* want to chagne that and recompile everything.
	**/
	m_AcuityNames.Append("vis");
	m_AcuityNames.Append("aud");
	m_AcuityNames.Append("tact");
	m_AcuityNames.Append("env");
	m_AcuityNames.Append("other");

	m_AcuityNames.Condense();

	for( int i=0; i < m_AcuityNames.Num(); i++ )
	{
		m_AcuityHash.Add( m_AcuityHash.GenerateKey( m_AcuityNames[i].c_str(), false ), i );
	}

	// Report the darkmod path for diagnostic purposes
	LogString("Darkmod path is %s\r", GetDarkmodPath().c_str());

#ifdef _WINDOWS

	std::string iniPath = GetDarkmodPath();
	iniPath += "\\darkmod.ini";

#elif MACOS_X

	std::string iniPath = GetDarkmodPath();
	iniPath += "/darkmod.ini";

#else   // LINUX
	
	std::string iniPath = GetDarkmodPath();
	iniPath += "/darkmod.ini";
	
#endif

	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Trying to open %s\r", iniPath.c_str());

	// Load the INI file into memory
	IniFilePtr darkmodIni = IniFile::ConstructFromFile(iniPath);

	if (darkmodIni)
	{
		LoadINISettings(darkmodIni);
	}
	else
	{
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Unable to open darkmod.ini: %s\r", iniPath.c_str());
	}

	// Map the surface types to strings
	InitSurfaceHardness();
}

void CGlobal::LogPlane(idStr const &Name, idPlane const &Plane)
{
	float a, b, c, d;

	Plane.GetPlaneParams(a, b, c, d);
	LogString("Plane %s:    a: %f   b: %f   c: %f   d: %f\r", Name.c_str(), a, b, c, d);
}

void CGlobal::LogVector(idStr const &Name, idVec3 const &Vector)
{
	LogString("Vector %s:    x: %f   y: %f   z: %f\r", Name.c_str(), Vector.x, Vector.y, Vector.z);
}

void CGlobal::LogMat3(idStr const &Name, idMat3 const &Mat)
{
	idVec3 a, b, c;

	Mat.GetMat3Params(a, b, c);
	LogString("Matrix %s:\r\t%f  %f  %f\r\t%f  %f  %f\r\t%f  %f  %f\r", Name.c_str(), 
		a.x, a.y, a.z,
		b.x, b.y, b.z,
		c.x, c.y, c.z
		);
}

void CGlobal::LogString(const char *fmt, ...)
{
	if(m_LogFile == NULL)
		return;

	LC_LogClass lc = m_LogClass;
	LT_LogType lt = m_LogType;

	if(m_ClassArray[lc] == false)
		return;

	if(m_LogArray[lt] == false)
		return;

	va_list arg;
	va_start(arg, fmt);

	const char *cleanFilename = CleanupSourceCodeFileName(m_Filename);
	fprintf(m_LogFile, "[%s (%4u):%s (%s) GT: %4u] ", cleanFilename, m_Linenumber, LTString[lt], LCString[lc], gameLocal.time);
	vfprintf(m_LogFile, fmt, arg);
	fprintf(m_LogFile, "\n");
	fflush(m_LogFile);

	va_end(arg);
}

void CGlobal::LoadINISettings(const IniFilePtr& iniFile)
{
	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Loading INI settings\r");

	// All logclasses are loaded from the INI file. Frame can be switched 
	// on explicitly. If any of the other classes are enabled, then Frame 
	// will also be enabled as a marker in the logfile.

// Don't use the LogFile section for OSX, always log to the original path in ~/Library/Logs
#ifndef MACOS_X
	std::string logFilePath = iniFile->GetValue(INI_DEBUG_SECTION, "LogFile");

	if (logFilePath.empty())
	{
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Logging disabled by darkmod.ini, closing logfile.\r");

		// No logfile defined, quit logging
		fclose(m_LogFile);
		m_LogFile = NULL;
	}
	else
	{
		FILE* logfile = fopen(logFilePath.c_str(), "w+b");

		if (logfile != NULL)
		{
			DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Switching logfile to %s.\r", logFilePath.c_str());

			if (m_LogFile != NULL)
			{
				fclose(m_LogFile);
				m_LogFile = logfile;
			}
		}
	}

#endif

	time_t timer = time(NULL);
	struct tm* t = localtime(&timer);

	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("LogFile created at %04u.%02u.%02u %02u:%02u:%02u\r",
		t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Executable last cleaned and rebuilt on " __DATE__ " " __TIME__ "\r");

	// BluePill #4539 - show whether this is a 32-bit or 64-bit binary
	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("%s %d.%02d/%u, code revision %d (%s)\r", 
		GAME_VERSION,
		TDM_VERSION_MAJOR,
		TDM_VERSION_MINOR,
		sizeof(void*) * 8,
		RevisionTracker::Instance().GetHighestRevision(),
		RevisionTracker::Instance().GetRevisionString());

	CheckLogArray(iniFile, "LogBegin", LT_BEGIN);
	CheckLogArray(iniFile, "LogEnd", LT_END);
	CheckLogArray(iniFile, "LogInfo", LT_INFO);
	CheckLogArray(iniFile, "LogDebug", LT_DEBUG);	
	CheckLogArray(iniFile, "LogWarning", LT_WARNING);
	CheckLogArray(iniFile, "LogError", LT_ERROR);
	
	CheckLogClass(iniFile, "LogClass_FRAME", LC_FRAME);
	CheckLogClass(iniFile, "LogClass_SYSTEM", LC_SYSTEM);
	CheckLogClass(iniFile, "LogClass_MISC", LC_MISC);
	CheckLogClass(iniFile, "LogClass_FROBBING", LC_FROBBING);
	CheckLogClass(iniFile, "LogClass_AI", LC_AI);
	CheckLogClass(iniFile, "LogClass_SOUND", LC_SOUND);
	CheckLogClass(iniFile, "LogClass_FUNCTION", LC_FUNCTION);
	CheckLogClass(iniFile, "LogClass_ENTITY", LC_ENTITY);
	CheckLogClass(iniFile, "LogClass_INVENTORY", LC_INVENTORY);
	CheckLogClass(iniFile, "LogClass_LIGHT", LC_LIGHT);
	CheckLogClass(iniFile, "LogClass_WEAPON", LC_WEAPON);
	CheckLogClass(iniFile, "LogClass_MATH", LC_MATH);
	CheckLogClass(iniFile, "LogClass_MOVEMENT", LC_MOVEMENT);
	CheckLogClass(iniFile, "LogClass_STIM_RESPONSE", LC_STIM_RESPONSE);
	CheckLogClass(iniFile, "LogClass_OBJECTIVES", LC_OBJECTIVES);
	CheckLogClass(iniFile, "LogClass_DIFFICULTY", LC_DIFFICULTY);
	CheckLogClass(iniFile, "LogClass_CONVERSATION", LC_CONVERSATION);
	CheckLogClass(iniFile, "LogClass_MAINMENU", LC_MAINMENU);
	CheckLogClass(iniFile, "LogClass_LOCKPICK", LC_LOCKPICK);
	CheckLogClass(iniFile, "LogClass_AAS", LC_AAS); // grayman
	CheckLogClass(iniFile, "LogClass_STATE", LC_STATE); // grayman
	// Tels: LogClass_INIT and LogClass_FORCE are always on
}

void CGlobal::CheckLogArray(const IniFilePtr& iniFile, const char* key, LT_LogType logType)
{
	if (iniFile->GetValue(INI_DEBUG_SECTION, key) == "1")
	{
		m_LogArray[logType] = true;
	}

	DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("%s: %d\r", key, m_LogArray[logType]);
}

void CGlobal::CheckLogClass(const IniFilePtr& iniFile, const char* key, LC_LogClass logClass)
{
	if (iniFile->GetValue(INI_DEBUG_SECTION, key) == "1")
	{
		m_ClassArray[logClass] = true;
	}

	DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("%s: %d\r", key, m_ClassArray[logClass]);
}

void DM_Printf(const char* fmt, ...)
{
	va_list arg;
	char text[1024];

	va_start( arg, fmt );
	vsprintf( text, fmt, arg );
	va_end( arg );

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Console output %s!\r", text);
}

void CGlobal::GetSurfName(const idMaterial *material, idStr &strIn )
{
	int end;
	int surftype;
	
	if (material == NULL) {
		goto Quit;
	}
	
	end = -1;
	surftype = material->GetSurfaceType();

	if( surftype != SURFTYPE_15 )
	{
		strIn = gameLocal.surfaceTypeNames[ surftype ];
		goto Quit;
	}

	// return the first word of the description if it has surftype_15
	strIn = material->GetDescription();
	end = strIn.Find(' ');

	if ( end == -1 )
	{
		goto Quit;
	}

	strIn = strIn.Left( end );

Quit:
	if( strIn.IsEmpty() )
		strIn = "none";

	//DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("GetSurfName: Found surface type name %s\r", strIn.c_str());

	return;
}

idStr CGlobal::GetSurfName(const idMaterial* material)
{
	if (material == NULL)
	{
		return idStr();
	}

	const surfTypes_t surftype = material->GetSurfaceType();

	if (surftype != SURFTYPE_15)
	{
		return gameLocal.surfaceTypeNames[ surftype ];
	}

	// return the first word of the description if it has surftype_15
	idStr desc = material->GetDescription();
	int end = desc.Find(' ');

	return ( end == -1 ) ? desc : desc.Left(end);
}

const idStr& CGlobal::GetSurfaceHardness(const char* surfName)
{
	// Generate the hash from the string and look up the index into the list
	int index = m_SurfaceHardnessHash.First( m_SurfaceHardnessHash.GenerateKey(surfName) );	

	return (index != -1) ? m_SurfaceHardness[index] : m_SurfaceHardness[0]; // fall back to "soft"
}

void CGlobal::InitSurfaceHardness()
{
	m_SurfaceHardness.Clear();

	int soft = m_SurfaceHardness.Append("soft");
	int hard = m_SurfaceHardness.Append("hard");

	// The hard ones
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("none"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("metal"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("stone"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("wood"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("glass"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("plastic"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("ricochet"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype10"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype11"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype12"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype13"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype14"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype15"), hard ); // shouldn't occur in normal operation
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("tile"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("gravel"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("rock"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("twigs"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("brokeglass"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("ice"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("squeakboard"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("puddle"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("ceramic"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("slate"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("armor_chain"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("armor_plate"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("climbable"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("hardwood"), hard ); // grayman #1421/1422

	// The soft ones
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("flesh"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("cardboard"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("liquid"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("carpet"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("dirt"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("grass"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("foliage"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("sand"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("mud"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("snow"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("moss"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("cloth"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("straw"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("armor_leath"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("paper"), soft );
}

std::string CGlobal::GetModPath(const std::string& modName)
{
	fs::path path = GetDarkmodPath();	// f.i. c:\games\tdm\darkmod
	path /= cv_tdm_fm_path.GetString();	// fms/
	path /= modName;					// <fs_currentfm>
    
	return path.string();
}

std::string CGlobal::GetDarkmodPath()
{
	std::string darkmodPath = fileSystem->ModPath();

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Resulting darkmod path is %s\r", darkmodPath.c_str());

	return darkmodPath.c_str();
}

LC_LogClass CGlobal::GetLogClassForString(const char* str)
{
	for (int i = 0; i < LC_COUNT; ++i)
	{
		if (idStr::Icmp(str, LCString[i]) == 0)
		{
			return static_cast<LC_LogClass>(i);
		}
	}

	return LC_COUNT;
}

// Auto-completion function for log-classes
void CGlobal::ArgCompletion_LogClasses( const idCmdArgs &args, void(*callback)( const char *s ) )
{
	for (int i = 0; i < LC_COUNT; ++i)
	{
		callback( va( "%s %s", args.Argv( 0 ), LCString[i] ) );
	}
}
