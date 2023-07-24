// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETACCOUNT_H__ )
#define __SDNETACCOUNT_H__

#if defined( _XENON )
	// TODO: live stuff!
#elif defined( _WIN32 ) || defined( __linux__ ) || defined( MACOS_X )
#if defined( SD_DEMO_BUILD )
	#include "SDNetAccount_Anonymous.h"
#else
	#include "SDNetAccount_Auth.h"
	//#include "SDNetAccount_CDKey.h"
#endif
#endif

#endif /* !__SDNETACCOUNT_H__ */
