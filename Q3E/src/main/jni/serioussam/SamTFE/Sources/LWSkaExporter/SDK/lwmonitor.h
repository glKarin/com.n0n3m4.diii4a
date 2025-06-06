/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMONITOR.H -- LightWave Progress Monitor
 *
 * Monitors are simple data structures defining an interface which the
 * server can use to give feedback to the host on its progress in
 * performing some task.  They are sometimes passed servers to give
 * feedback on the progress of the particular operation, and can sometimes
 * be accessed from within a server that wants to show its progress on a
 * slow operation using the host's normal feedback display.
 */
#ifndef LWSDK_MONITOR_H
#define LWSDK_MONITOR_H


/*
 * An LWMonitor struct consists of some generic data and three functions:
 * init, step and done.  The 'init' function is called first with the
 * number of steps in the process to be monitored, which is computed by
 * the server.  As the task is processed, the 'step' function is called
 * with the number of steps just completed (often one).  These step
 * increments should eventually add up to the total number and then the
 * 'done' function is called, but 'done' may be called early if there was
 * a problem or the process was aborted.  The 'step' function will return
 * one if the user requested an abort and zero otherwise.
 *
 * The server is masked from any errors in the monitor that may occur on
 * the host side of the interface.  If there is a problem with putting up
 * a monitor, the functions will still return normally, since the monitor
 * is for user feedback and is not that critical.
 */
typedef struct st_LWMonitor {
        void             *data;
        void            (*init) (void *, unsigned int);
        int             (*step) (void *, unsigned int);
        void            (*done) (void *);
} LWMonitor;

/*
 * Macros are provided to call a monitor which will do nothing if the
 * monitor pointer is null.  MON_INCR is used for step sizes greater than
 * one and MON_STEP is used for step sizes exactly one.
 */
#define MON_INIT(mon,count)     if (mon) (*mon->init) (mon->data, count)
#define MON_INCR(mon,d)         (mon ? (*mon->step) (mon->data, d) : 0)
#define MON_STEP(mon)           MON_INCR (mon, 1)
#define MON_DONE(mon)           if (mon) (*mon->done) (mon->data)


#define LWLMONFUNCS_GLOBAL        "LWLMonFuncs"

/****
 * create:
 * Creates a new monitor instance
 *
 * setup:
 * Configures the progress monitor
 *   title    - Monitor Title
 *   uiflags  - a bitmask of desired LMO_* flags
 *   histfile - if given, filename where messages will also be written
 *   uiflags  - takes a bitmask of ui options
 *
 * setwinpos:
 *   xywh     - position and size of monitor window
 *
 * init:
 * Sets total number of steps and OOpens the monitor progress window
 *   total    - Total number of steps
 *   msg      - Initial message string for user
 *
 * step:
 * Increments the current step and sets the display message
 *   incr     - Increment to increase counter (0 is valid)
 *   msg      - Message string for user
 *
 * done:
 * Indicates the processing is complete. If review option enabled,
 * monitor remains open in a modal state for user review of output.
 * Control will not return to caller until the user dismisses the window.
 *
 * destroy:
 * Closes the window (if open) and destroys the instance.
 *
 ****
 */

/* defines */
/****
 * UI Option Flags
 * NOABORT:
 *  Abort button is disabled. Default is operation can be aborted.
 * REVIEW:
 *  Monitor remains open after 'done' method is called. This allows user
 *  to review messages. Default is no review and mediately from done method
 * HISTAPPEND:
 *  History file is appended with new messages. Default is overwrite.
 * IMMUPD:
 *  Enables immediate update of the monitor on every step. The default
 *  is to delay updates to avoid incurring too much overhead for rapid
 *  step events.
 ****
 */
#define LMO_NOABORT     (1<<0)    
#define LMO_REVIEW      (1<<1)
#define LMO_HISTAPPEND  (1<<2)
#define LMO_IMMUPD      (1<<3)

/* TypeDefs */
typedef struct st_LMONDATA *LWLMonID;

typedef struct st_LWLMonFuncs {
  LWLMonID   (*create)    ( void );
  void       (*setup)     ( LWLMonID mon, char *title,
                                                                                                                unsigned int uiflags, const char *histfile );
  void       (*setwinpos) ( LWLMonID mon, int x, int y, int w, int h );
  void       (*init)      ( LWLMonID mon, unsigned int total, const char *msg );
  int        (*step)      ( LWLMonID mon, unsigned int incr, const char *msg );
  void       (*done)      ( LWLMonID mon );
  void       (*destroy)   ( LWLMonID mon );
} LWLMonFuncs;


/****
 * Macros to simplify some common operations.
 ****
 */

/* Creates a monitor instance */
#define LMON_NEW(fun)              ((*fun->create)())

/* Initailizes a default monitor */
#define LMON_DFLT(fun,mon,tot)     {(*fun->setup)(mon,"Processing...",0,NULL);\
                                    (*fun->init)(mon,tot,NULL);}

#define LMON_WPOS(fun,mon,x,y,w,h) ((*fun->setwinpos)(mon,x,y,w,h))

#define LMON_INIT(fun,mon,tot)     ((*fun->init)(mon,tot,NULL))

/* The following are various incrementing macros */
#define LMON_STEP(fun,mon)         ((*fun->step)(mon,1,NULL))
#define LMON_INCR(fun,mon,s)       ((*fun->step)(mon,s,NULL))

#define LMON_MSG(fun,mon,msg)      ((*fun->step)(mon,0,msg))
#define LMON_MSGS(fun,mon,msg)     ((*fun->step)(mon,1,msg))
#define LMON_MSGI(fun,mon,s,msg)   ((*fun->step)(mon,s,msg))

/* These finish and destroy the monitor */
#define LMON_DONE(fun,mon)         ((*fun->done)(mon))
#define LMON_KILL(fun,mon)         ((*fun->destroy)(mon))

#endif

