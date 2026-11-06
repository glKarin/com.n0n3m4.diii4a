//
// if include std C++ library headers
//

#ifndef _REMOVE_IDLIB_MACROS_H
#define _REMOVE_IDLIB_MACROS_H

#undef strcmp
#undef strncmp
#undef snprintf
#undef _snprintf
#undef vsnprintf
#undef _vsnprintf

#define FLT_EPSILON 1.192092896e-07f

#else

#define strcmp			idStr::Cmp		// use_idStr_Cmp
#define strncmp			use_idStr_Cmpn
#define snprintf		use_idStr_snPrintf
#define _snprintf		use_idStr_snPrintf
#define vsnprintf		use_idStr_vsnPrintf
#define _vsnprintf		use_idStr_vsnPrintf

#undef FLT_EPSILON

#undef _REMOVE_IDLIB_MACROS_H

#endif //_REMOVE_IDLIB_MACROS_H
