#ifndef HOST_H
#define HOST_H

#include <setjmp.h>
#include "qtypes.h"
#include "qdefs.h"
#include "cmd.h"
#include "cvar.h"

extern cvar_t developer;
extern cvar_t developer_entityparsing;
extern cvar_t developer_extra;
extern cvar_t developer_insane;
extern cvar_t developer_loadfile;
extern cvar_t developer_loading;
extern cvar_t host_isclient;
extern cvar_t sessionid;

struct cmd_state_s;

typedef enum host_state_e
{
	host_init,
	host_loading,
	host_active,
	// states >= host_shutdown cause graceful shutdown, see Sys_HandleCrash() comments
	host_shutdown,
	host_failing, ///< crashing
	host_failed ///< crashed or aborted, SDL dialog open
} host_state_t;

typedef struct host_static_s
{
	jmp_buf abortframe;
	int state;
	unsigned int framecount; // incremented every frame, never reset (checked by Host_Error and Host_SaveConfig_f)
	double realtime; // the accumulated mainloop time since application started (with filtering), without any slowmo or clamping
	double dirtytime; // the main loop wall time for this frame, equal to Sys_DirtyTime() at the start of this host frame
	double sleeptime; // time spent sleeping after the last frame
	qbool restless; // don't sleep
	qbool paused; // global paused state, pauses both client and server
	cmd_buf_t *cbuf;

	struct
	{
		void (*ConnectLocal)(void);
		void (*Disconnect)(qbool, const char *, ... );
		void (*ToggleMenu)(void);
		qbool (*CL_Intermission)(void); // Quake compatibility
		void (*CL_SendCvar)(struct cmd_state_s *);
		void (*SV_SendCvar)(struct cmd_state_s *);
		void (*SV_Shutdown)(void);
	} hook;
} host_static_t;

extern host_static_t host;

void Host_Main(void);
void Host_Error(const char *error, ...) DP_FUNC_PRINTF(1) DP_FUNC_NORETURN;
void Host_LockSession(void);
void Host_UnlockSession(void);
void Host_AbortCurrentFrame(void) DP_FUNC_NORETURN;
void Host_SaveConfig(const char *file);

#endif
