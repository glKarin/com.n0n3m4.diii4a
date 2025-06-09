/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWGLOBSERV.H -- LightWave Global Server
 *
 * This header contains declarations necessary to define a "Global"
 * class server.
 */
#ifndef LWSDK_GLOBSERV_H
#define LWSDK_GLOBSERV_H

#define LWGLOBALSERVICE_CLASS   "Global"
#define LWGLOBALSERVICE_VERSION 1


typedef struct st_LWGlobalService {
        const char      *id;
        void            *data;
} LWGlobalService;

#endif

