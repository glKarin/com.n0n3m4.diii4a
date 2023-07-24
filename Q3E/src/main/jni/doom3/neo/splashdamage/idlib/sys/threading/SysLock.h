// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYSLOCK_H__
#define __SYSLOCK_H__

#ifndef _WIN32
#include <pthread.h>
#endif

class sdSysLock {
public:
	static void				Init( lockHandle_t& handle );
	static void				Destroy( lockHandle_t& handle );
	static bool				Acquire( lockHandle_t& handle, bool blocking );
	static void				Release( lockHandle_t& handle );

private:
							sdSysLock() {}
};

#endif /* !__SYSLOCK_H__ */
