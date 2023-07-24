// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_MISC_PROFILEHELPER_H__
#define __GAME_MISC_PROFILEHELPER_H__


class sdProfileHelper;
class sdProfileHelperManagerLocal;
typedef idLinkList< sdProfileHelper >	sdProfileHelperList;
typedef sdSingleton< sdProfileHelperManagerLocal > sdProfileHelperManager;


/*
===============================================================

	sdProfileHelper 
		handy helper class that uses a hash map & lots of mini-dump files to make a full graphable csv file
		of profiling information.
		graphs vs game time

===============================================================
*/

class sdProfileHelper {
protected:
	static const int			PROFILE_HELPER_MAX_FRAMES = 400;
	typedef struct {
		double					frameInfo[ PROFILE_HELPER_MAX_FRAMES ];
	} sampleSet_t;

	typedef idHashMap< sampleSet_t* >	sampleGroup_t;

public:

	sdProfileHelper( void );
	~sdProfileHelper( void );

	void			Init( const char* name, bool hasTotal = false, bool hasCount = false );
	void			Update( void );

	void			Start( void );
	void			Stop( void );
	void			LogValue( const char* sampleName, double value );

	bool			IsActive( void ) const;
	const char*		GetName( void ) const { return m_ProfileName.c_str(); }


	sdProfileHelperList		m_ProfileNode;


protected:
	void			DumpLog( const char* suffix, sampleGroup_t& group );
	void			AssembleLogs( const char* suffix );

	void			GetMiniLogName( const char* suffix, idStr& out, int logNum, int subLogNum );
	void			GetLogName( const char* suffix, idStr& out, int logNum );

	// profiler information
	idStr					m_ProfileName;
	bool					m_HasTotal;
	bool					m_HasCount;

	// sampling information
	sampleGroup_t			m_SampleValues;
	sampleGroup_t			m_SampleCounts;
	// TWTODO: Timing could be moved into the manager & have all profilers use the same time basis
	int						m_SampleTimes[ PROFILE_HELPER_MAX_FRAMES ];
	int						m_SampleUpto;
	int						m_LogSubFileUpto;
	int						m_LogFileUpto;
};

/*
===============================================================

	sdProfileHelperManagerLocal 
		Keeps track of all the active profile helpers and tells them to update, etc

===============================================================
*/

class sdProfileHelperManagerLocal {
public:
	sdProfileHelperManagerLocal( void );
	~sdProfileHelperManagerLocal( void );

	void				StopAll( void );
	void				Update( void );

	sdProfileHelper*	FindProfiler( const char* name );
	void				LogValue( const char* profileName, const char* sampleName, double value, bool create = true, bool hasTotal = false, bool hasCount = false );


	sdProfileHelperList	m_ActiveProfilers;
	sdProfileHelperList	m_InactiveProfilers;
};


/*
===============================================================

	sdProfileHelper_ScopeTimer 
		Handy helper, profiles the current scope if on the stack

===============================================================
*/

class sdProfileHelper_ScopeTimer {
public:
	static const int MAX_PROFILE_NAME = 16;
	static const int MAX_SAMPLE_NAME = 128;

	sdProfileHelper_ScopeTimer( const char* profileName, const char* sampleName, bool condition = true ) {
		m_Condition = condition;
		if ( m_Condition ) {
			idStr::Copynz( m_ProfileName, profileName, MAX_PROFILE_NAME );
			idStr::Copynz( m_SampleName, sampleName, MAX_SAMPLE_NAME );

			m_Timer.Start();
		}
	}

	~sdProfileHelper_ScopeTimer() {
		if ( m_Condition ) {
			m_Timer.Stop();
			double value = m_Timer.Milliseconds();

			sdProfileHelperManager::GetInstance().LogValue( m_ProfileName, m_SampleName, value, true, true, true );
		}
	}

protected:
	idTimer			m_Timer;
	char			m_ProfileName[ MAX_PROFILE_NAME ];
	char			m_SampleName[ MAX_SAMPLE_NAME ];
	bool			m_Condition;
};

#endif // __GAME_MISC_PROFILEHELPER_H__
