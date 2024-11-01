/* ---------------------------------------------------------------------------------------------------------------------------------
  _                                 _     
 | |                               | |    
 | | ___   __ _  __ _  ___ _ __    | |__  
 | |/ _ \ / _` |/ _` |/ _ \ '__|   | '_ \ 
 | | (_) | (_| | (_| |  __/ |    _ | | | |
 |_|\___/ \__, |\__, |\___|_|   (_)|_| |_|
           __/ | __/ |                    
          |___/ |___/                     

 Generic informational logging class

 ---------------------------------------------------------------------------------------------------------------------------------

 Restrictions & freedoms pertaining to usage and redistribution of this software:

  * This software is 100% free
  * If you use this software (in part or in whole) you must credit the author.
  * This software may not be re-distributed (in part or in whole) in a modified
    form without clear documentation on how to obtain a copy of the original work.
  * You may not use this software to directly or indirectly cause harm to others.
  * This software is provided as-is and without warrantee. Use at your own risk.

 For more information, visit HTTP://www.FluidStudios.com

 ---------------------------------------------------------------------------------------------------------------------------------
 Originally created on 07/06/2000 by Paul Nettle
 Modified Heavily by Jeremy Swigart

 Copyright 2000, Fluid Studios, Inc., all rights reserved.
 --------------------------------------------------------------------------------------------------------------------------------- */

#ifndef	_H_LOGGER
#define _H_LOGGER

#include <time.h>
#include <string>

#include "CodeAnalysis.h"

// ---------------------------------------------------------------------------------------------------------------------------------
// The global logger
// ---------------------------------------------------------------------------------------------------------------------------------

class Logger;
extern Logger g_Logger;

// ---------------------------------------------------------------------------------------------------------------------------------
// Macros (necessary evil to take advantage of __LINE__ and __FILE__)
// ---------------------------------------------------------------------------------------------------------------------------------

#define	LOG(_s) if(g_Logger.LogStarted() && (g_Logger.LogMask() & Logger::LOG_INFO)) {	\
	g_Logger.LimitFileSize(); \
	g_Logger.Stream() << g_Logger.HeaderString(Logger::LOG_INFO); \
	g_Logger.SourceLine() = __LINE__; \
	g_Logger.SourceFile() = __FILE__; \
	g_Logger.Stream() << _s << std::endl; }

#define	LOGERR(_s) if(g_Logger.LogStarted() && (g_Logger.LogMask() & Logger::LOG_ERR)) {	\
	g_Logger.LimitFileSize(); \
	g_Logger.Stream() << g_Logger.HeaderString(Logger::LOG_ERR); \
	g_Logger.SourceLine() = __LINE__; \
	g_Logger.SourceFile() = __FILE__; \
	g_Logger.Stream() << _s << std::endl; }

#define	LOGWARN(_s) if(g_Logger.LogStarted() && (g_Logger.LogMask() & Logger::LOG_WARN)) {	\
	g_Logger.LimitFileSize(); \
	g_Logger.Stream() << g_Logger.HeaderString(Logger::LOG_WARN); \
	g_Logger.SourceLine() = __LINE__; \
	g_Logger.SourceFile() = __FILE__; \
	g_Logger.Stream() << _s << std::endl; }

#define	LOGCRIT(_s) if(g_Logger.LogStarted() && g_Logger.LogMask() & Logger::LOG_CRIT) {	\
	g_Logger.LimitFileSize(); \
	g_Logger.Stream() << g_Logger.HeaderString(Logger::LOG_CRIT); \
	g_Logger.SourceLine() = __LINE__; \
	g_Logger.SourceFile() = __FILE__; \
	g_Logger.Stream() << _s << std::endl; }

#define	HEX			g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__,g_Logger.LogHex
#define	RAW			g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__,g_Logger.LogRaw
#define	INDENT		g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__,g_Logger.Indent
#define	UNDENT		g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__,g_Logger.Undent
#define	LOGBLOCK	g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__;LogBlock __lb__


//#ifndef __linux__
	#define	LOGFUNC		g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__;LogFlow __lf__(__FUNCTION__)
	#define	LOGFUNCBLOCK g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__;LogBlock __lb__(__FUNCTION__)
//#else
//	#define	LOGFUNC		g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__;LogFlow __lf__(__func__)
//	#define	LOGFUNCBLOCK g_Logger.SourceLine() = __LINE__, g_Logger.SourceFile() = __FILE__;LogBlock __lb__(__func__)
//#endif

// ---------------------------------------------------------------------------------------------------------------------------------
// The logger class: does the actual logging
// ---------------------------------------------------------------------------------------------------------------------------------

class Logger
{
public:
	enum LogFlags
	{
		LOG_INDENT = 1<<0,
		LOG_UNDENT = 1<<1,
		LOG_FLOW   = 1<<2,
		LOG_DATA   = 1<<3,
		LOG_INFO   = 1<<4,
		LOG_WARN   = 1<<5,
		LOG_ERR    = 1<<6,
		LOG_CRIT   = 1<<7,
		LOG_ALL    = 0xFFFFFFFF
	};

	enum OutputFlags
	{
		WRITE_FILE = (1<<0),
		WRITE_LINE = (1<<1),
		WRITE_DATE = (1<<2),
		WRITE_TIME = (1<<3),
		WRITE_TIMESTAMP = WRITE_DATE | WRITE_TIME,
		WRITE_ALL  = WRITE_FILE | WRITE_LINE | WRITE_TIMESTAMP,
	};

	// Construction/Destruction
	Logger();
	virtual ~Logger();

	// Accessors
	inline const bool&			LineCharsFlag() const	{return m_LineCharsFlag;}
	inline bool&				LineCharsFlag() 		{return m_LineCharsFlag;}

	inline const int&			FileSizeLimit() const	{return m_FileSizeLimit;}
	inline int&					FileSizeLimit()			{return m_FileSizeLimit;}

	inline const int&			LogMask() const			{return m_LogMask;}
	inline int&					LogMask()				{return m_LogMask;}

	inline const unsigned int	&SourceLine() const		{return m_SourceLine;}
	inline unsigned int&		SourceLine()			{return m_SourceLine;}

	inline const std::string&	SourceFile() const		{return m_SourceFile;}
	inline std::string&			SourceFile()			{return m_SourceFile;}

	inline std::fstream &		Stream()				{return m_LogFile;}

	bool						LogStarted();

	// Utilitarian (public)
	void		Start(const std::string &_filename, const bool reset);
	void		Stop();
	void		LogTex(const LogFlags logBits, CHECK_PRINTF_ARGS const char *s);
	void		LogRaw(const std::string &s);
	void		LogHex(const char *buffer, const unsigned int count, const LogFlags logBits = LOG_INFO);
	void		Indent(const std::string &s, const LogFlags logBits = LOG_INDENT);
	void		Undent(const std::string &s, const LogFlags logBits = LOG_UNDENT);

	void		LimitFileSize();
	const std::string&	HeaderString(const LogFlags logBits) const;
private:

	// Data
	std::fstream	m_LogFile;
	std::string		m_SourceFile;
	unsigned int	m_SourceLine;
	int				m_IndentCount;
	int				m_IndentChars;
	int				m_FileSizeLimit;
	int				m_LogMask;
	int				m_OutMask;
	bool			m_LineCharsFlag;

};

// ---------------------------------------------------------------------------------------------------------------------------------
// The LogBlock class: used for automatic indentation
// ---------------------------------------------------------------------------------------------------------------------------------

class LogBlock
{
public:
	inline LogBlock(const std::string &_s)/* :
		m_Str(_s)*/
	{
		g_Logger.Indent("Begin block: " + _s, Logger::LOG_INDENT);
	}
	inline ~LogBlock()
	{
		g_Logger.Undent("", Logger::LOG_UNDENT);
	}
private:
	//string		m_Str;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// The LogFlow class: used for logging code flow
// ---------------------------------------------------------------------------------------------------------------------------------

class LogFlow
{
public:
	inline LogFlow(const std::string &_function)/* :
		m_Str(_function)*/
	{
		g_Logger.Indent(_function, Logger::LOG_FLOW);
	}
	inline ~LogFlow()
	{
		g_Logger.Undent("", Logger::LOG_FLOW);
	}
private:
	//string		m_Str;
};

#endif
