/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWPRTCL.H -- LightWave Particles
 */
#ifndef LWSDK_PRTCL_H
#define LWSDK_PRTCL_H

#include <lwmeshes.h>
#include <lwrender.h>
#include <lwio.h>
#include <lwglobsrv.h>

#define LWPSYSFUNCS_GLOBAL      "Particle Services"
/*
The particle services provide some functions to create and manage particle data,
this data is made accessible to other clients by query functions that lookup particle
data from itemIDs. These services can be used by particle simulator to publish their particle
information, and by particle renderers to access the particle informations.

A "particle system" designates a group of particles sharing the same type of data.
The data is allocated by type in buffers. For example if the particles in a system have 
the following informations: Age, Position, Color. The particle system will have 3 data
buffers, one for each type and the create function will take the following flags:
LWPSB_POS | LWPSB_AGE | LWPSB_VEL.

Once the buffers are created and filled in, they must be attached to an item in the scene.
This can be done with the attach\detach functions, the query can be done with the getPSys
function.
*/

typedef struct st_LWPSys                *LWPSysID;
typedef struct st_LWPSBuf               *LWPSBufID;

/* particle types */
#define LWPST_PRTCL     0
#define LWPST_TRAIL     1
/* buffer flags */
#define LWPSB_POS       (1<<0) /* position (float[3]) */
#define LWPSB_SIZ       (1<<1) /* size (float) */
#define LWPSB_SCL       (1<<2) /* scale (float[3]) */
#define LWPSB_ROT       (1<<3) /* rotation (float[3]) */
#define LWPSB_VEL       (1<<4) /* velocity (float[3]) */
#define LWPSB_AGE       (1<<5) /* age (float) */
#define LWPSB_FCE       (1<<6) /* force (float) */
#define LWPSB_PRS       (1<<7) /* pressure     (float) */
#define LWPSB_TMP       (1<<8) /* temperature (float) */
#define LWPSB_MAS       (1<<9) /* mass (float) */
#define LWPSB_LNK       (1<<10)/* link to particle (for trails) (int) */
#define LWPSB_ID        (1<<11)/* particle ID, unique identifier for a particle (int) */
#define LWPSB_ENB       (1<<12)/* particle state (alive/limbo/dead) (char) */
#define LWPSB_RGBA      (1<<13)/* particle color,alpha (for display) (char[4]) */
#define LWPSB_CAGE      (1<<14)/* collision age: time since last collision (float) */

typedef struct st_LWPSBufDesc { 
        const char      *name;
        int             dataType,dataSize;
}LWPSBufDesc;
/* data types */
#define LWPSBT_FLOAT    0
#define LWPSBT_INT      1
#define LWPSBT_CHAR     2

/* Particle state values */
#define LWPST_DEAD              0
#define LWPST_ALIVE             1
#define LWPST_LIMBO             2
/*
"Alive": that is the normal state. Particles in that state will be rendered.
"Limbo": the idea behind that state is to allow particles to subsist in the database
for a while after they die, before being totally removed. When switching a particle to limbo
state, the particle age should be reset to 0 since the age is relative to the state. 
"Dead": particles will simply be ignored, it is possible to create them however merely
for convenience.
*/

typedef struct st_LWPSysFuncs {
        LWPSysID        (*create)(int flags,int type);                  /* creates a particle system with */
        int             (*destroy)(LWPSysID     ps);                    /* destroys a particle system */
        int             (*init)(LWPSysID        ps,int  np);    /* allocates all buffers with specified number of particles */
        void            (*cleanup) (LWPSysID    ps);            /* frees all buffers */
        void            (*load)(LWPSysID        ps,LWLoadState *lstate);
        void            (*save)(LWPSysID        ps,LWSaveState *sstate);
        int             (*getPCount)(LWPSysID       ps);    /* returns number of particles */

        void            (*attach)(LWPSysID      ps,LWItemID     item);  /* attaches data to an item, multiple PSys can be attached to an item and multiple items can share the same data */
        void            (*detach)(LWPSysID      ps,LWItemID     item);  /* detaches data */
        LWPSysID        *(*getPSys)(LWItemID    item);                  /* returns null terminated list of PSys attached to item. */

        LWPSBufID       (*addBuf)(LWPSysID      ps,LWPSBufDesc  desc);  /* adds a new data buffer to the system */
        LWPSBufID       (*getBufID)(LWPSysID    ps,int  bufFlag);               /* gets a buffer ID, given its flag */
        void            (*setBufData)(LWPSBufID buf,void        *data); /* sets a buffer en-masse by performing a memcopy from the given pointer */
        void            (*getBufData)(LWPSBufID buf,void        *data); /* gets data from a buffer */

        int             (*addParticle)(LWPSysID                 ps);    /* adds a particle to the system */
        void            (*setParticle)(LWPSBufID        buf,int idx,void        *data); /* sets a particle for the given data buffer */
        void            (*getParticle)(LWPSBufID        buf,int idx,void        *data); /* gets a particle data from the given buffer */
        void                    (*remParticle)(LWPSysID                 ps,int idx); /* removes a particle from the system */
} LWPSysFuncs;

#endif  LWSDK_PRTCL_H
