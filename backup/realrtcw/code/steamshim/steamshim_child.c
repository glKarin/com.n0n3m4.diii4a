
#ifdef WIN32

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
typedef HANDLE PipeType;
#define NULLPIPE NULL
typedef unsigned __int8 uint8;
typedef __int32 int32;
typedef __int32 uint32;
typedef unsigned __int64 uint64;

#else

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
typedef uint8_t uint8;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int PipeType;
#define NULLPIPE -1

#endif

#include "steamshim_child.h"
#include <stdio.h>
#define DEBUGPIPE
#ifdef DEBUGPIPE

static void dbgpipe(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    FILE *f = fopen("steamshim.log", "a");
 
    // fprintf(f, "%s", fmt);

    while (*fmt != '\0')
    {
        if (*(fmt++) != '%')
        {
            fprintf(f, "%c", *(fmt - 1));

            continue;
        }

        switch (*(fmt++))
        {
            case 'd':
            case 'i':
                fprintf(f, "%i", va_arg(args, int));
                break;
            case 'u':
                fprintf(f, "%i", va_arg(args, unsigned int));
                break;
            case 'c':
                fprintf(f, "%c", va_arg(args, char));
                break;
            case 'f':
                fprintf(f, "%f", va_arg(args, float));
                break;
            case 's':
                fprintf(f, "%s", va_arg(args, const char *));
                break;
            default:
                break;
        }
    }

    fclose(f);
 
    va_end(args);
}

#else

static inline void dbgpipe(const char *fmt, ...) {}

#endif

static int writePipe(PipeType fd, const void *buf, const unsigned int _len);
static int readPipe(PipeType fd, void *buf, const unsigned int _len);
static void closePipe(PipeType fd);
static char *getEnvVar(const char *key, char *buf, const size_t buflen);
static int pipeReady(PipeType fd);


#ifdef WIN32

static int pipeReady(PipeType fd)
{
    DWORD avail = 0;
    return (PeekNamedPipe(fd, NULL, 0, NULL, &avail, NULL) && (avail > 0));
} /* pipeReady */

static int writePipe(PipeType fd, const void *buf, const unsigned int _len)
{
    const DWORD len = (DWORD) _len;
    DWORD bw = 0;
    return ((WriteFile(fd, buf, len, &bw, NULL) != 0) && (bw == len));
} /* writePipe */

static int readPipe(PipeType fd, void *buf, const unsigned int _len)
{
    const DWORD len = (DWORD) _len;
    DWORD br = 0;
    return ReadFile(fd, buf, len, &br, NULL) ? (int) br : -1;
} /* readPipe */

static void closePipe(PipeType fd)
{
    CloseHandle(fd);
} /* closePipe */

static char *getEnvVar(const char *key, char *buf, const size_t _buflen)
{
    const DWORD buflen = (DWORD) _buflen;
    const DWORD rc = GetEnvironmentVariableA(key, buf, buflen);
    dbgpipe("buflen = %d rc = %d\n", buflen, rc);
    dbgpipe("buffer = %s\n", buf);
    /* rc doesn't count null char, hence "<". */
    return ((rc > 0) && (rc < buflen)) ? buf : NULL;
} /* getEnvVar */

#else

static int pipeReady(PipeType fd)
{
    int rc;
    struct pollfd pfd = { fd, POLLIN | POLLERR | POLLHUP, 0 };
    while (((rc = poll(&pfd, 1, 0)) == -1) && (errno == EINTR)) { /*spin*/ }
    return (rc == 1);
} /* pipeReady */

static int writePipe(PipeType fd, const void *buf, const unsigned int _len)
{
    const ssize_t len = (ssize_t) _len;
    ssize_t bw;
    while (((bw = write(fd, buf, len)) == -1) && (errno == EINTR)) { /*spin*/ }
    return (bw == len);
} /* writePipe */

static int readPipe(PipeType fd, void *buf, const unsigned int _len)
{
    const ssize_t len = (ssize_t) _len;
    ssize_t br;
    while (((br = read(fd, buf, len)) == -1) && (errno == EINTR)) { /*spin*/ }
    return (int) br;
} /* readPipe */

static void closePipe(PipeType fd)
{
    close(fd);
} /* closePipe */

static char *getEnvVar(const char *key, char *buf, const size_t buflen)
{
    const char *envr = getenv(key);
    if (!envr || (strlen(envr) >= buflen))
        return NULL;
    strcpy(buf, envr);
    return buf;
} /* getEnvVar */

#endif


static PipeType GPipeRead = NULLPIPE;
static PipeType GPipeWrite = NULLPIPE;

typedef enum ShimCmd
{
    SHIMCMD_BYE,
    SHIMCMD_PUMP,
    SHIMCMD_REQUESTSTATS,
    SHIMCMD_STORESTATS,
    SHIMCMD_SETACHIEVEMENT,
    SHIMCMD_GETACHIEVEMENT,
    SHIMCMD_RESETSTATS,
    SHIMCMD_SETSTATI,
    SHIMCMD_GETSTATI,
    SHIMCMD_SETSTATF,
    SHIMCMD_GETSTATF,
    SHIMCMD_RESTARTAPP,
    SHIMCMD_SETRICHPRESENCE,
} ShimCmd;

static int write1ByteCmd(const uint8 b1)
{
    const uint8 buf[] = { 1, b1 };
    return writePipe(GPipeWrite, buf, sizeof (buf));
} /* write1ByteCmd */

static int write2ByteCmd(const uint8 b1, const uint8 b2)
{
    const uint8 buf[] = { 2, b1, b2 };
    return writePipe(GPipeWrite, buf, sizeof (buf));
} /* write2ByteCmd */

static inline int writeBye(void)
{
    dbgpipe("Child sending SHIMCMD_BYE().\n");
    return write1ByteCmd(SHIMCMD_BYE);
} // writeBye

static int initPipes(void)
{
    char buf[64];
    unsigned long long val;

    if (!getEnvVar("STEAMSHIM_READHANDLE", buf, sizeof (buf)))
        return 0;
    else if (sscanf(buf, "%llu", &val) != 1)
        return 0;
    else
        GPipeRead = (PipeType) val;

    if (!getEnvVar("STEAMSHIM_WRITEHANDLE", buf, sizeof (buf)))
        return 0;
    else if (sscanf(buf, "%llu", &val) != 1)
        return 0;
    else
        GPipeWrite = (PipeType) val;
    
    return ((GPipeRead != NULLPIPE) && (GPipeWrite != NULLPIPE));
} /* initPipes */


int STEAMSHIM_init(void)
{
    dbgpipe("Child init start.\n");
    if (!initPipes())
    {
        dbgpipe("Child init failed.\n");
        return 0;
    } /* if */

#ifndef WIN32

    signal(SIGPIPE, SIG_IGN);

#endif

    dbgpipe("Child init success!\n");
    return 1;
} /* STEAMSHIM_init */

void STEAMSHIM_deinit(void)
{
    dbgpipe("Child deinit.\n");
    if (GPipeWrite != NULLPIPE)
    {
        writeBye();
        closePipe(GPipeWrite);
    } /* if */

    if (GPipeRead != NULLPIPE)
        closePipe(GPipeRead);

    GPipeRead = GPipeWrite = NULLPIPE;

#ifndef WIN32

    signal(SIGPIPE, SIG_DFL);

#endif

} /* STEAMSHIM_deinit */

static inline int isAlive(void)
{
    return ((GPipeRead != NULLPIPE) && (GPipeWrite != NULLPIPE));
} /* isAlive */

static inline int isDead(void)
{
    return !isAlive();
} /* isDead */

int STEAMSHIM_alive(void)
{
    return isAlive();
} /* STEAMSHIM_alive */

static const STEAMSHIM_Event *processEvent(const uint8 *buf, size_t buflen)
{
    static STEAMSHIM_Event event;
    const STEAMSHIM_EventType type = (STEAMSHIM_EventType) *(buf++);
    buflen--;

    memset(&event, '\0', sizeof (event));
    event.type = type;
    event.okay = 1;

    #ifdef DEBUGPIPE
    if (0) {}
    #define PRINTGOTEVENT(x) else if (type == x) printf("Child got " #x ".\n")
    PRINTGOTEVENT(SHIMEVENT_BYE);
    PRINTGOTEVENT(SHIMEVENT_STATSRECEIVED);
    PRINTGOTEVENT(SHIMEVENT_STATSSTORED);
    PRINTGOTEVENT(SHIMEVENT_SETACHIEVEMENT);
    PRINTGOTEVENT(SHIMEVENT_GETACHIEVEMENT);
    PRINTGOTEVENT(SHIMEVENT_RESETSTATS);
    PRINTGOTEVENT(SHIMEVENT_SETSTATI);
    PRINTGOTEVENT(SHIMEVENT_GETSTATI);
    PRINTGOTEVENT(SHIMEVENT_SETSTATF);
    PRINTGOTEVENT(SHIMEVENT_GETSTATF);
    PRINTGOTEVENT(SHIMEVENT_APPRESTARTED);
    PRINTGOTEVENT(SHIMEVENT_SETRICHPRESENCE);
    #undef PRINTGOTEVENT
    else printf("Child got unknown shimevent %d.\n", (int) type);
    #endif

    switch (type)
    {
        case SHIMEVENT_BYE:
            break;

        case SHIMEVENT_STATSRECEIVED:
        case SHIMEVENT_STATSSTORED:
            if (!buflen) return NULL;
            event.okay = *(buf++) ? 1 : 0;
            break;

        case SHIMEVENT_SETACHIEVEMENT:
            if (buflen < 3) return NULL;
            event.ivalue = *(buf++) ? 1 : 0;
            event.okay = *(buf++) ? 1 : 0;
            strcpy(event.name, (const char *) buf);
            break;

        case SHIMEVENT_GETACHIEVEMENT:
            if (buflen < 10) return NULL;
            event.ivalue = (int) *(buf++);
            if (event.ivalue == 2)
                event.ivalue = event.okay = 0;
            event.epochsecs = (long long unsigned) *((uint64 *) buf);
            buf += sizeof (uint64);
            strcpy(event.name, (const char *) buf);
            break;

        case SHIMEVENT_RESETSTATS:
            if (buflen != 2) return NULL;
            event.ivalue = *(buf++) ? 1 : 0;
            event.okay = *(buf++) ? 1 : 0;
            break;

        case SHIMEVENT_SETSTATI:
        case SHIMEVENT_GETSTATI:
            event.okay = *(buf++) ? 1 : 0;
            event.ivalue = (int) *((int32 *) buf);
            buf += sizeof (int32);
            strcpy(event.name, (const char *) buf);
            break;

        case SHIMEVENT_SETSTATF:
        case SHIMEVENT_GETSTATF:
            event.okay = *(buf++) ? 1 : 0;
            event.fvalue = (int) *((float *) buf);
            buf += sizeof (float);
            strcpy(event.name, (const char *) buf);
            break;

        case SHIMEVENT_APPRESTARTED:
            event.okay = *(buf++) ? 1 : 0;
            break;

        case SHIMEVENT_SETRICHPRESENCE:
            if (buflen < 2) return NULL;
            event.okay = *(buf++) ? 1 : 0;
            strcpy(event.name, (const char *) buf);
            break;

        default:  /* uh oh */
            return NULL;
    } /* switch */

    return &event;
} /* processEvent */

const STEAMSHIM_Event *STEAMSHIM_pump(void)
{
    static uint8 buf[256];
    static int br = 0;
    int evlen = (br > 0) ? ((int) buf[0]) : 0;

    if (isDead())
        return NULL;

    if (br <= evlen)  /* we have an incomplete commmand. Try to read more. */
    {
        if (pipeReady(GPipeRead))
        {
            const int morebr = readPipe(GPipeRead, buf + br, sizeof (buf) - br);
            if (morebr > 0)
                br += morebr;
            else  /* uh oh */
            {
                dbgpipe("Child readPipe failed! Shutting down.\n");
                STEAMSHIM_deinit();   /* kill it all. */
            } /* else */
        } /* if */
    } /* if */

    if (evlen && (br > evlen))
    {
        const STEAMSHIM_Event *retval = processEvent(buf+1, evlen);
        br -= evlen + 1;
        if (br > 0)
            memmove(buf, buf+evlen+1, br);
        return retval;
    } /* if */

    /* Run Steam event loop. */
    if (br == 0)
    {
        // dbgpipe("Child sending SHIMCMD_PUMP().\n");
        write1ByteCmd(SHIMCMD_PUMP);
    } /* if */

    return NULL;
} /* STEAMSHIM_pump */

void STEAMSHIM_requestStats(void)
{
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_REQUESTSTATS().\n");
    write1ByteCmd(SHIMCMD_REQUESTSTATS);
} /* STEAMSHIM_requestStats */

void STEAMSHIM_storeStats(void)
{
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_STORESTATS().\n");
    write1ByteCmd(SHIMCMD_STORESTATS);
} /* STEAMSHIM_storeStats */

void STEAMSHIM_setAchievement(const char *name, const int enable)
{
    uint8 buf[256];
    uint8 *ptr = buf+1;
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_SETACHIEVEMENT('%s', %senable).\n", name, enable ? "" : "!");
    *(ptr++) = (uint8) SHIMCMD_SETACHIEVEMENT;
    *(ptr++) = enable ? 1 : 0;
    strcpy((char *) ptr, name);
    ptr += strlen(name) + 1;
    buf[0] = (uint8) ((ptr-1) - buf);
    writePipe(GPipeWrite, buf, buf[0] + 1);
} /* STEAMSHIM_setAchievement */

void STEAMSHIM_getAchievement(const char *name)
{
    uint8 buf[256];
    uint8 *ptr = buf+1;
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_GETACHIEVEMENT('%s').\n", name);
    *(ptr++) = (uint8) SHIMCMD_GETACHIEVEMENT;
    strcpy((char *) ptr, name);
    ptr += strlen(name) + 1;
    buf[0] = (uint8) ((ptr-1) - buf);
    writePipe(GPipeWrite, buf, buf[0] + 1);
} /* STEAMSHIM_getAchievement */

void STEAMSHIM_resetStats(const int bAlsoAchievements)
{
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_RESETSTATS(%salsoAchievements).\n", bAlsoAchievements ? "" : "!");
    write2ByteCmd(SHIMCMD_RESETSTATS, bAlsoAchievements ? 1 : 0);
} /* STEAMSHIM_resetStats */

void STEAMSHIM_restartIfNecessary(unsigned int unOwnAppID)
{
    uint8 buf[6];
    uint8 *ptr = buf + 1;
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_RESTARTAPP.\n");
    *(ptr++) = (uint8) SHIMCMD_RESTARTAPP;
    *(uint32*)ptr = unOwnAppID;
    ptr += 4;
    buf[0] = (uint8) (ptr - buf);
    writePipe(GPipeWrite, buf, buf[0]);
}  /* STEAMSHIM_restartIfNecessary */

static void writeStatThing(const ShimCmd cmd, const char *name, const void *val, const size_t vallen)
{
    uint8 buf[256];
    uint8 *ptr = buf+1;
    if (isDead()) return;
    *(ptr++) = (uint8) cmd;
    if (vallen)
    {
        memcpy(ptr, val, vallen);
        ptr += vallen;
    } /* if */
    strcpy((char *) ptr, name);
    ptr += strlen(name) + 1;
    buf[0] = (uint8) ((ptr-1) - buf);
    writePipe(GPipeWrite, buf, buf[0] + 1);
} /* writeStatThing */

void STEAMSHIM_setStatI(const char *name, const int _val)
{
    const int32 val = (int32) _val;
    dbgpipe("Child sending SHIMCMD_SETSTATI('%s', val %d).\n", name, val);
    writeStatThing(SHIMCMD_SETSTATI, name, &val, sizeof (val));
} /* STEAMSHIM_setStatI */

void STEAMSHIM_getStatI(const char *name)
{
    dbgpipe("Child sending SHIMCMD_GETSTATI('%s').\n", name);
    writeStatThing(SHIMCMD_GETSTATI, name, NULL, 0);
} /* STEAMSHIM_getStatI */

void STEAMSHIM_setStatF(const char *name, const float val)
{
    dbgpipe("Child sending SHIMCMD_SETSTATF('%s', val %f).\n", name, val);
    writeStatThing(SHIMCMD_SETSTATF, name, &val, sizeof (val));
} /* STEAMSHIM_setStatF */

void STEAMSHIM_getStatF(const char *name)
{
    dbgpipe("Child sending SHIMCMD_GETSTATF('%s').\n", name);
    writeStatThing(SHIMCMD_GETSTATF, name, NULL, 0);
} /* STEAMSHIM_getStatF */

void STEAMSHIM_setRichPresence(const char* key, const char* value)
{
    uint8 buf[128 + 2];
    uint8 *ptr = buf+1;
    if (isDead()) return;
    dbgpipe("Child sending SHIMCMD_SETRICHPRESENCE('%s', '%s').\n", key, value);
    *(ptr++) = (uint8) SHIMCMD_SETRICHPRESENCE;
    strcpy((char *) ptr, key);
    ptr += strlen(key) + 1;
    strcpy((char *) ptr, value);
    ptr += strlen(value) + 1;
    buf[0] = (uint8) ((ptr-1) - buf);
    writePipe(GPipeWrite, buf, buf[0] + 1);
} /* STEAMSHIM_setRichPresence */

/* end of steamshim_child.c ... */

