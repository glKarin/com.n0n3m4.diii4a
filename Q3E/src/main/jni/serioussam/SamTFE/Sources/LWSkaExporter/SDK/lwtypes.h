/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWTYPES.H -- LightWave Common Types
 *
 * This header contains type declarations common to all aspects of
 * LightWave.
 */
#ifndef LWSDK_TYPES_H
#define LWSDK_TYPES_H


typedef void *           LWImageID;
typedef float            LWBufferValue;
typedef void *           LWPixmapID;
typedef void *           LWTextureID;

typedef int              LWFrame;
typedef double           LWTime;

typedef float            LWFVector[3];
typedef double           LWDVector[3];

typedef unsigned long    LWID;
#define LWID_(a,b,c,d)   ((((long)a)<<24)|(((long)b)<<16)|(((long)c)<<8)|(d))

typedef int              LWCommandCode;

/*
 * Persistent instances are just some opaque data object referenced
 * by void pointer.  Errors from handler functions are human-readable
 * strings, where a null string pointer indicates no error.
 */
typedef void *           LWInstance;
typedef const char *     LWError;

#ifndef NULL
 #define NULL           ((void *) 0)
#endif

#endif

