#ifndef __PROFILER_H__
#define __PROFILER_H__


// tmj: moved outside INGAME_PROFILER_ENABLED to work with typeinfo
#define PROFILE_DEBUG					0

#if INGAME_PROFILER_ENABLED

// Category modes, must match enumeration
#define	PROFMASK_NORMAL					0x0001
#define PROFMASK_RENDERING				0x0002
#define PROFMASK_AI						0x0004
#define PROFMASK_PHYSICS				0x0008
#define PROFMASK_COMBAT					0x0010
#define PROFMASK_ALL					0xffff

enum profileCategoryModes_e {
	PROFMODE_NORMAL=0,
	PROFMODE_RENDERING,
	PROFMODE_AI,
	PROFMODE_PHYSICS,
	PROFMODE_COMBAT,
	PROFMODE_NUM_MODES
};

enum EProfCommand{
	PROFCOMMAND_NONE,
	PROFCOMMAND_TRAVERSEOUT,
	PROFCOMMAND_STARTCAPTURE,
	PROFCOMMAND_STOPCAPTURE,
	PROFCOMMAND_TOGGLECAPTURE,
	PROFCOMMAND_ENABLE,
	PROFCOMMAND_DISABLE,
	PROFCOMMAND_TOGGLE,
	PROFCOMMAND_RELEASE,
	PROFCOMMAND_TOGGLEMODE,
	PROFCOMMAND_TOGGLEMS,
	PROFCOMMAND_TOGGLESMOOTHING,
	PROFCOMMAND_IN0,
	PROFCOMMAND_IN1,
	PROFCOMMAND_IN2,
	PROFCOMMAND_IN3,
	PROFCOMMAND_IN4,
	PROFCOMMAND_IN5,
	PROFCOMMAND_IN6,
	PROFCOMMAND_IN7,
	PROFCOMMAND_IN8, 
	PROFCOMMAND_IN9,
	PROFCOMMAND_IN10,
	PROFCOMMAND_IN11,
	PROFCOMMAND_IN12,
	PROFCOMMAND_IN13,
	PROFCOMMAND_IN14
};



class hhProfiler {
public:
	virtual void			DrawGraph()=0;
	virtual void			FrameBoundary()=0;
	virtual void			SubmitFrameCommand(EProfCommand command)=0;
	virtual bool			IsInteractive()=0;
	virtual cinData_t		ImageForTime( const int milliseconds )=0;

	// Hierarchical mode support
	virtual void			StartProfile(const char *timingName, bool AllowDuplicates)=0;
	virtual void			StopProfile(const char *timingName)=0;
	virtual void			StopProfile()=0;

	// Category mode support
	virtual void			StartProfileCategory(const char *timingName, int mode)=0;
	virtual void			StopProfileCategory(const char *timingName, int mode)=0;
	virtual void			StopProfileCategory(int mode)=0;
};

extern hhProfiler *profiler;


//
// ProfileScope
// dummy class used to make Start/Stop profiler calls based on scope
//
struct ProfileScope {
    inline ProfileScope(const char *zone, bool AllowDuplicates);
    inline ~ProfileScope();
};

inline ProfileScope::ProfileScope(const char *zone, bool AllowDuplicates) {
	profiler->StartProfile(zone, AllowDuplicates);
}

inline ProfileScope::~ProfileScope() {
	profiler->StopProfile();
}


//
// ProfileScopeCategory
// dummy class used to make Start/Stop profiler calls based on scope
//
class ProfileScopeCategory {
public:
	int localmode;
    inline ProfileScopeCategory(const char *zone, int mode);
    inline ~ProfileScopeCategory();
};

inline ProfileScopeCategory::ProfileScopeCategory(const char *zone, int mode) {
	localmode = mode;
	profiler->StartProfileCategory(zone, mode);
}

inline ProfileScopeCategory::~ProfileScopeCategory() {
	profiler->StopProfileCategory(localmode);
}

#if PROFILE_DEBUG
	#define PROFILE_START(n, m)				profiler->StartProfileCategory(n, m)
	#define PROFILE_STOP(n, m)				profiler->StopProfileCategory(n, m)
	#define PROFILE_SCOPE(n, m)				ProfileScopeCategory modescope_ ## __COUNTER__ (n, m)

	#define PROFILE_START_EXPENSIVE(n, m)	profiler->StartProfileCategory(n, m)
	#define PROFILE_STOP_EXPENSIVE(n, m)	profiler->StopProfileCategory(n, m)
	#define PROFILE_SCOPE_EXPENSIVE(n, m)	ProfileScopeCategory modescope_ ## __COUNTER__ (n, m)
#else
	#define PROFILE_START(n, m)				profiler->StartProfileCategory(n, m)
	#define PROFILE_STOP(n, m)				profiler->StopProfileCategory(m)
	#define PROFILE_SCOPE(n, m)				ProfileScopeCategory modescope_ ## __COUNTER__ (n, m)

	#define PROFILE_START_EXPENSIVE(n, m)	profiler->StartProfileCategory(n, m)
	#define PROFILE_STOP_EXPENSIVE(n, m)	profiler->StopProfileCategory(m)
	#define PROFILE_SCOPE_EXPENSIVE(n, m)	ProfileScopeCategory modescope_ ## __COUNTER__ (n, m)
#endif

#else	// INGAME_PROFILER_ENABLED

	// Profiling not enabled, compile it out
	#define PROFILE_START(n, m)		
	#define PROFILE_STOP(n, m)		
	#define PROFILE_SCOPE(n, m)		

	#define PROFILE_START_EXPENSIVE(n, m)
	#define PROFILE_STOP_EXPENSIVE(n, m)
	#define PROFILE_SCOPE_EXPENSIVE(n, m)

#endif	// INGAME_PROFILER_ENABLED

#endif /* __PROFILER_H__ */

