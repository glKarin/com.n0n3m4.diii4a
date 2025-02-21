#include "../plugin.h"
qboolean VPK_Init(void);
qboolean VTF_Init(void);
qboolean MDL_Init(void);
qboolean VMT_Init(void);
qboolean VBSP_Init(void);

qboolean Plug_Init(void)
{
	qboolean somethingisokay = false;
	char plugname[128];
	strcpy(plugname, "hl2");
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	if (!VPK_Init())	Con_Printf(CON_ERROR"%s: VPK support unavailable\n", plugname);	else	somethingisokay = true;
	if (!VTF_Init())	Con_Printf(CON_ERROR"%s: VTF support unavailable\n", plugname);	else	somethingisokay = true;
	if (!VMT_Init())	Con_Printf(CON_ERROR"%s: VMT support unavailable\n", plugname);	else	somethingisokay = true;
	if (!MDL_Init())	Con_Printf(CON_ERROR"%s: MDL support unavailable\n", plugname);	else	somethingisokay = true;
	if (!VBSP_Init())	Con_Printf(CON_ERROR"%s: BSP support unavailable\n", plugname);	else	somethingisokay = true;
	return somethingisokay;
}

