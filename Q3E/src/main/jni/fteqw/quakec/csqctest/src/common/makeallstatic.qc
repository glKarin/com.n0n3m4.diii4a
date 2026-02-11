#ifdef FTEQCC
//	#define DOTHESTATICTHING
#endif

#ifdef DOTHESTATICTHING
	#pragma defaultstatic 1
#else
#ifndef FTEQCC
	#define static
	#define nonstatic
#endif
#endif