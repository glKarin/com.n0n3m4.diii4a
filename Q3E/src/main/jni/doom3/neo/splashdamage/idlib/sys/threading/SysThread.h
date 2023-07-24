// Copyright (C) 2007 Id Software, Inc.
//

// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYSTHREAD_H__
#define __SYSTHREAD_H__

class sdSysThread {
public:
	static bool			Create( threadProc_t proc, void* parms, threadHandle_t& handle, threadPriority_e priority = THREAD_NORMAL, unsigned int stackSize = 0 );
	static bool			Start( threadHandle_t& handle );
	static bool			Suspend( threadHandle_t& handle );
	static bool			Resume( threadHandle_t& handle );
	static unsigned int	Exit( unsigned int retVal );
	static void			Join( threadHandle_t& handle );
	static void			Destroy( threadHandle_t& handle );
	static void			SetPriority( threadHandle_t& handle, const threadPriority_e priority );
	static void			SetProcessor( threadHandle_t& handle, const unsigned int processor );
	static void			SetName( threadHandle_t& handle, const char* name );

private:
						sdSysThread() {}

#ifndef _WIN32
	static void*		setPriorityProc( void *parms );

	static threadProc_t createProc;
	static void*		createParms;
	static int			priority;
#endif
};

#endif /* !__SYSTHREAD_H__ */
