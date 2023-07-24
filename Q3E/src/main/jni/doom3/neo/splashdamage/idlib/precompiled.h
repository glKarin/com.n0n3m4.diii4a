// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __IDLIB_PRECOMPILED_H__
#define __IDLIB_PRECOMPILED_H__

//-----------------------------------------------------

#include "../idlib/LibOS.h"

//-----------------------------------------------------

//#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>
#include <wchar.h>	// wmemset
#include <limits.h>

//-----------------------------------------------------

#include "../common/common.h"

// non-portable system services
#include "../sys/sys_public.h"

// GSS
#ifndef _DEBUG
	#ifdef GSS_ENABLED
		#ifdef _WIN32
			#include "../sys/gss/GSSCodes.h"
		#else
			#undef GSS_ENABLED
		#endif	/* _WIN32 */
	#endif	/* GSS_ENABLED */
#endif	/* !_DEBUG */

#include "../framework/BuildDefines.h"

// id lib
#include "../idlib/Lib.h"

// framework
#include "../framework/Common_public.h"
#include "../libs/filelib/File.h"
#include "../framework/File_InZip.h"
#include "../libs/filelib/File64.h"
#include "../framework/FileSystem.h"

// renderer
#include "../renderer/Model_public.h"

#include "../idlib/LibImpl.h"

#ifdef EB_WITH_PB
	#include "../libs/punkbuster/pbcommon.h"
#endif /* EB_WITH_PB */

#endif /* !__IDLIB_PRECOMPILED_H__ */
