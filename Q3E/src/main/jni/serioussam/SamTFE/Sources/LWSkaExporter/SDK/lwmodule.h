/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMODULE.H -- LightWave Plug-in Modules
 *
 * The ModuleDescriptor is the lowest-level which describes a single
 * LightWave plug-in module.  Modules can contain multiple servers
 * but have one startup and shutdown each.  The synchronization codes
 * are used to assure that the module matches the expectations of the
 * host.
 */
#ifndef LWSDK_MODULE_H
#define LWSDK_MODULE_H

#include <lwserver.h>


typedef struct st_ModuleDescriptor {
        unsigned long            sysSync;
        unsigned long            sysVersion;
        unsigned long            sysMachine;
        void *                 (*startup)  (void);
        void                   (*shutdown) (void *);
        ServerRecord            *serverDefs;
} ModuleDescriptor;

#define MOD_SYSSYNC      0x04121994
#define MOD_SYSVER       3
#ifdef _XGL
 #define MOD_MACHINE     0x200
#endif
#ifdef _WIN32
 #ifdef _ALPHA_
  #define MOD_MACHINE    0x302
 #else
  #define MOD_MACHINE    0x300
 #endif
#endif
#ifdef _MACOS
 #define MOD_MACHINE     0x400
#endif

#endif
