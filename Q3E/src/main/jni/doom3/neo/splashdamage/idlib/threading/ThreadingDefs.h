// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __THREADINGDEFS_H__
#define __THREADINGDEFS_H__

#if defined( _WIN32 )
	struct threadHandle_t {
		HANDLE			handle;
		unsigned int	id;
	};

	typedef unsigned int (__stdcall *threadProc_t)( void* );

	#define signalHandle_t	HANDLE

	#define lockHandle_t	CRITICAL_SECTION
#else
	typedef void* (*threadProc_t)( void* );
	#define threadHandle_t	pthread_t
	struct signalHandle_t {
		pthread_mutex_t	mutex;
		pthread_cond_t	cond;
		bool			signaled;
		bool			waiting;
	};
	#define lockHandle_t	pthread_mutex_t
#endif

#endif /* !__THREADINGDEFS_H__ */
