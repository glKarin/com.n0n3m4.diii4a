
#include "quakedef.h"

#define MAXRENDERMODULES 20

typedef struct rendermodule_s
{
	int active; // set by start, cleared by shutdown
	const char *name;
	void(*start)(void);
	void(*shutdown)(void);
	void(*newmap)(void);
	void(*devicelost)(void);
	void(*devicerestored)(void);
}
rendermodule_t;

rendermodule_t rendermodule[MAXRENDERMODULES];

void R_Modules_Init(void)
{
	Cmd_AddCommand(CF_CLIENT, "r_restart", R_Modules_Restart_f, "restarts renderer");
}

void R_RegisterModule(const char *name, void(*start)(void), void(*shutdown)(void), void(*newmap)(void), void(*devicelost)(void), void(*devicerestored)(void))
{
	int i;
	for (i = 0;i < MAXRENDERMODULES;i++)
	{
		if (rendermodule[i].name == NULL)
			break;
		if (!strcmp(name, rendermodule[i].name))
		{
			Con_Printf("R_RegisterModule: module \"%s\" registered twice\n", name);
			return;
		}
	}
	if (i >= MAXRENDERMODULES)
		Sys_Error("R_RegisterModule: ran out of renderer module slots (%i)", MAXRENDERMODULES);
	rendermodule[i].active = 0;
	rendermodule[i].name = name;
	rendermodule[i].start = start;
	rendermodule[i].shutdown = shutdown;
	rendermodule[i].newmap = newmap;
	rendermodule[i].devicelost = devicelost;
	rendermodule[i].devicerestored = devicerestored;
}

void R_Modules_Start(void)
{
	int i;
	for (i = 0;i < MAXRENDERMODULES;i++)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (rendermodule[i].active)
		{
			Con_Printf ("R_Modules_Start: module \"%s\" already active\n", rendermodule[i].name);
			continue;
		}
		Con_DPrintf("Starting render module \"%s\"\n", rendermodule[i].name);
		rendermodule[i].active = 1;
		rendermodule[i].start();
	}
}

void R_Modules_Shutdown(void)
{
	int i;
	// shutdown in reverse
	for (i = MAXRENDERMODULES - 1;i >= 0;i--)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		Con_DPrintf("Stopping render module \"%s\"\n", rendermodule[i].name);
		rendermodule[i].active = 0;
		rendermodule[i].shutdown();
	}
}

void R_Modules_Restart_f(cmd_state_t *cmd)
{
	CL_StartVideo();
	Con_Print("Restarting renderer\n");
	SCR_BeginLoadingPlaque(false);
	R_Modules_Shutdown();
	R_Modules_Start();
}

void R_Modules_NewMap(void)
{
	int i;
	R_SkinFrame_PrepareForPurge();
	for (i = 0;i < MAXRENDERMODULES;i++)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		rendermodule[i].newmap();
	}
	R_SkinFrame_Purge();
}

void R_Modules_DeviceLost(void)
{
	int i;
	for (i = 0;i < MAXRENDERMODULES;i++)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		if (!rendermodule[i].devicelost)
			continue;
		rendermodule[i].devicelost();
	}
}


void R_Modules_DeviceRestored(void)
{
	int i;
	for (i = 0;i < MAXRENDERMODULES;i++)
	{
		if (rendermodule[i].name == NULL)
			continue;
		if (!rendermodule[i].active)
			continue;
		if (!rendermodule[i].devicerestored)
			continue;
		rendermodule[i].devicerestored();
	}
}

