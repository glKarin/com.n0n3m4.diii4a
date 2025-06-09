#ifndef LWSDK_MTUTIL_H
#define LWSDK_MTUTIL_H

/****
 * 
 * Some plugin services may be called from within a LW render thread which
 * implies a need for methods for a plugin to protect its global data.  The
 * MTUtil functions provide that service thus allowing a plugin to be threaad 
 * safe.
 *
 * USAGE:
 *
 * create - this method returns a LWMTUtilID which must be passed to the lock and
 *          unlock methods.  Each MTUtilID allows up to 10 independent mutexes 
 *          (binary semaphores) to be used, indexed 0 - 9.  Return NULL on failure.
 *
 * lock   - takes a MTUtilID and a mutex index and blocks until the mutex becomes 
 *          available.  Returns success.
 *
 * unlock - releases the mutex specified by the index.  Returns success.
 */
 
 

typedef void *          LWMTUtilID;

#define LWMTUTILFUNCS_GLOBAL "MultiThreading Utilities"

typedef struct st_LWMTUtilFuncs {
        LWMTUtilID      (*create) (void);
        void            (*destroy) (LWMTUtilID mtid);
        int             (*lock) (LWMTUtilID mtid, int mutexID);
        int             (*unlock) (LWMTUtilID mtid, int mutexID);
} LWMTUtilFuncs;

#endif
