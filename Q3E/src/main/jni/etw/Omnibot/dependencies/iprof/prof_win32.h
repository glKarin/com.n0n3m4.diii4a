#ifndef Prof_INC_PROF_WIN32_H
#define Prof_INC_PROF_WIN32_H

typedef __int64 Prof_Int64;

#ifdef __cplusplus
  inline
#elif _MSC_VER >= 1200
  __forceinline
#else
  static
#endif
      void Prof_get_timestamp(Prof_Int64 *result)
      {
#ifndef _M_X64 // cs: TODO: 64bit support
         __asm {
            rdtsc;
            mov    ebx, result
            mov    [ebx], eax
            mov    [ebx+4], edx
         }
#endif
      }

#endif
