/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#ifndef SE_INCL_THREADLOCALSTORAGE_H
#define SE_INCL_THREADLOCALSTORAGE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#ifdef SINGLE_THREADED
#define THREADLOCAL(type, name, defval) type name = defval
#define EXTERNTHREADLOCAL(type, name) extern type name
#elif (defined _MSC_VER)
#define THREADLOCAL(type, name, defval) type __declspec(thread) name = defval
#define EXTERNTHREADLOCAL(type, name) extern type __declspec(thread) name
#else
// Pretty much everything has __thread now. If it doesn't, do SINGLE_THREADED or
//  roll something with pthread_key or SDL or whatever. See revision history for a start.
#define THREADLOCAL(type, name, defval) __thread type name = defval;
#define EXTERNTHREADLOCAL(type, name) extern __thread type name;
#endif

#endif  // include-once blocker.


// end of ThreadLocalStorage.h ...

