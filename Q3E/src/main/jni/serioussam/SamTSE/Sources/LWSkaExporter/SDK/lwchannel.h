/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWCHANNEL.H -- LightWave Channel Filters
 */
#ifndef LWSDK_CHANNEL_H
#define LWSDK_CHANNEL_H

#include <lwrender.h>
#include <lwtypes.h>

#define LWCHANNEL_HCLASS        "ChannelHandler"
#define LWCHANNEL_ICLASS        "ChannelInterface"
#define LWCHANNEL_VERSION       4


typedef void *            LWChannelID;

typedef struct st_LWChannelAccess {
        LWChannelID       chan;
        LWFrame           frame;
        LWTime            time;
        double            value;
        void            (*getChannel)  (LWChannelID chan, LWTime t, double *value);
        void            (*setChannel)  (LWChannelID chan, const double value);
        const char *    (*channelName) (LWChannelID chan);
} LWChannelAccess;

typedef struct st_LWChannelHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        void            (*evaluate) (LWInstance, const LWChannelAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWChannelHandler;

#endif
