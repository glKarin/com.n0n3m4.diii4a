#ifndef Prof_INC_PROF_WIN32_H
#define Prof_INC_PROF_WIN32_H

typedef long long int Prof_Int64;

#ifdef __cplusplus
  inline
#elif _MSC_VER >= 1200
  __forceinline
#else
  static
#endif
      void Prof_get_timestamp(Prof_Int64 *res)
      {
      }
#endif
