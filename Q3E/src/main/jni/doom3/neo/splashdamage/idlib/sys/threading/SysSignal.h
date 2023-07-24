// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYSSIGNAL_H__
#define __SYSSIGNAL_H__

#ifndef _WIN32
#include <pthread.h>
#endif

class sdSysSignal {
public:
	static void				Create( signalHandle_t& handle );
	static void				Destroy( signalHandle_t& handle );
	static void				Set( signalHandle_t& handle );
	static void				Clear( signalHandle_t& handle );
	static bool				Wait( signalHandle_t& handle, int timeout );
	static bool				SignalAndWait( signalHandle_t& signal, signalHandle_t& handle, int timeout );
};

#endif /* !__SYSSIGNAL_H__ */
