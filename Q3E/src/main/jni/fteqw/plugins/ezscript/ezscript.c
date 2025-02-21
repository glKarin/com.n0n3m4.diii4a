#include "../plugin.h"
#define RELEASE "__DATE__"

#define ezscriptcvars "ezScript Console Variables"
vmcvar_t	ezscript_silentmode = {"ezscript_silentmode", "1", ezscriptcvars, 0};
#undef ezscriptcvars

vmcvar_t	*cvarlist[] ={
	&ezscript_silentmode
};

void ezScript_InitCvars(void)
{
	vmcvar_t *v;
	int i;

	for (v = cvarlist[0],i=0; i < sizeof(cvarlist)/sizeof(cvarlist[0]); v++, i++)
		v->handle = Cvar_Register(v->name, v->string, v->flags, v->group);
}

int ezScript_CvarUpdate(void)
{
	vmcvar_t *v;
	int i;
	for (v = cvarlist[0],i=0; i < sizeof(cvarlist)/sizeof(cvarlist[0]); v++, i++)
		v->modificationcount = Cvar_Update(v->handle, &v->modificationcount, v->string, &v->value);
	return 0;
}

int Plug_ExecuteCommand(int *args)
{
	char cmd[256];
	char param[256];
	char *cvar;

	Cmd_Argv(0, cmd, sizeof(cmd));

	     if (!strcmp("loadsky",				cmd))	cvar = "r_skybox";
	else if (!strcmp("r_skyname",			cmd))	cvar = "r_skybox";
	else if (!strcmp("r_skycolor",			cmd))	cvar = "r_fastskycolour"; // note the england spelling, spike is a englander
	else if (!strcmp("cl_physfps",			cmd))	cvar = "cl_netfps";
	else if (!strcmp("fps_skycolor",		cmd))	cvar = "r_fastskycolour"; // note the england spelling, spike is a englander
	else if (!strcmp("fps_sky",				cmd))	cvar = "r_fastsky";
	else if (!strcmp("gl_consolefont",		cmd))	cvar = "gl_font";
	else if (!strcmp("gl_bounceparticles",	cmd))	cvar = "r_bouncysparks";
	else if (!strcmp("gl_loadlitfiles",		cmd))	cvar = "r_loadlit";
	else if (!strcmp("gl_weather_rain",		cmd))	cvar = "r_part_rain";
	else if (!strcmp("cl_bonusflash",		cmd))	cvar = "v_bonusflash";
	else if (!strcmp("sw_gamma",			cmd))	cvar = "gamma";
	else if (!strcmp("sw_contrast",			cmd))	cvar = "contrast";
	else if (!strcmp("s_nosound",			cmd))	cvar = "nosound";
	else if (!strcmp("scr_conback",			cmd))	cvar = "gl_conback";
	else if (!strcmp("sv_maxpitch",			cmd))	cvar = "serverinfo maxpitch";
	else if (!strcmp("sv_minpitch",			cmd))	cvar = "serverinfo minpitch";
	else if (!strcmp("sv_zombietime",		cmd))	cvar = "zombietime";
	else if (!strcmp("tp_triggers",			cmd))	cvar = "cl_triggers";
	else if (!strcmp("teamskin",			cmd))	cvar = "cl_teamskin";
	else if (!strcmp("enemyskin",			cmd))	cvar = "cl_enemyskin";
	else if (!strcmp("cl_predictPlayers",	cmd))	cvar = "cl_predict_players"; //lets not forget there is a cl_predict_players2
	else if (!strcmp("sshot_format",		cmd))	cvar = "scr_sshot_type";
	else if (!strcmp("cl_solidPlayers",		cmd))	cvar = "cl_solid_players";
	else if (!strcmp("fps_muzzleflash",		cmd))	cvar = "cl_muzzleflash";
	else if (!strcmp("in_m_mwhook",			cmd))	cvar = "in_mwhook";
	else if (!strcmp("r_floorcolor",		cmd))	cvar = "r_floorcolour";
	else if (!strcmp("r_wallcolor",			cmd))	cvar = "r_wallcolour";
	else if (!strcmp("r_farclip",			cmd))	cvar = "gl_maxdist";
	else if (!strcmp("vid_colorbits",		cmd))	cvar = "vid_bpp";
	else if (!strcmp("vid_customheight",	cmd))	cvar = "vid_height";
	else if (!strcmp("vid_customwidth",		cmd))	cvar = "vid_width";
	else if (!strcmp("vid_hwgammacontrol",	cmd))	cvar = "vid_hardwaregamma";
	else if (!strcmp("vid_vsync",			cmd))	cvar = "_vid_wait_override";
	else if (!strcmp("gl_lighting_vertex",	cmd))	cvar = "r_vertexlight";
	else if (!strcmp("bgmvolume",			cmd))	cvar = "musicvolume";
	else if (!strcmp("scr_menualpha",		cmd))	cvar = "scr_conalpha";
	else if (!strcmp("cl_fakeshaft",		cmd))	cvar = "cl_truelightning";
	else cvar = NULL;

	if (cvar)
	{
		if (Cmd_Argc() == 1)	//a query
		{
			if (!Cvar_GetString(cvar, param, sizeof(param)))
				Con_Printf("ezScript: %s(%s) is BAD\n", cmd, cvar);
			else
				Con_Printf("ezScript: %s(%s) is \"%s\"\n", cmd, cvar, param);
		}
		else
		{
			Cmd_Argv(1, param, sizeof(param));

			Cvar_SetString(cvar,param);

			ezScript_CvarUpdate();

			if (ezscript_silentmode.value == 0) { Con_Printf("-------------------------------------\n^7ezScript: ^1%s^7 is a ^3Fuh/ez/Z/More quakeworld ^7cvar, sending '^6%s^7' to ^2%s^7\n-------------------------------------\n",cmd,param,cvar); }

		}
		return 1;
	}

	return 0;
}

void ezScript_InitCommands(void) // not really needed actually
{
	//skyboxes
	Cmd_AddCommand("loadsky");
	Cmd_AddCommand("r_skyname");
	Cmd_AddCommand("r_skycolor");
	Cmd_AddCommand("fps_sky");
	Cmd_AddCommand("fps_skycolor");
	//gl stuff
	Cmd_AddCommand("gl_consolefont");
	Cmd_AddCommand("gl_bounceparticles");
	Cmd_AddCommand("gl_loadlitfiles");
	Cmd_AddCommand("gl_weather_rain");
	Cmd_AddCommand("r_farclip");
	Cmd_AddCommand("vid_vsync");
	Cmd_AddCommand("gl_lighting_vertex");
	Cmd_AddCommand("scr_conback");
	//misc
	Cmd_AddCommand("cl_bonusflash");
	Cmd_AddCommand("cl_fakeshaft");
	Cmd_AddCommand("r_floorcolor");
	Cmd_AddCommand("r_wallcolor");
	//gamma
	Cmd_AddCommand("sw_gamma");
	Cmd_AddCommand("sw_contrast");
	//sound
	Cmd_AddCommand("s_nosound");
	//teamplay
	Cmd_AddCommand("tp_triggers");
	Cmd_AddCommand("teamskin");
	Cmd_AddCommand("enemyskin");
	//console
	Cmd_AddCommand("scr_menualpha");
	//misc
	Cmd_AddCommand("cl_predictPlayers");
	Cmd_AddCommand("sshot_format");
	Cmd_AddCommand("cl_solidPlayers");
	Cmd_AddCommand("fps_muzzleflash");
	Cmd_AddCommand("scr_menualpha");
	Cmd_AddCommand("in_m_mwhook");
	//sound
	Cmd_AddCommand("bgmvolume");
	//fps
	Cmd_AddCommand("cl_physfps");
	//video
	Cmd_AddCommand("vid_colorbits");
	Cmd_AddCommand("vid_customheight");
	Cmd_AddCommand("vid_cumstomwidth");
	Cmd_AddCommand("vid_hwgammacontrol");
	//server
	Cmd_AddCommand("sv_maxpitch");
	Cmd_AddCommand("sv_minpitch");
	Cmd_AddCommand("sv_zombietime");
}

int Plug_Init(int *args)
{
	if (!Plug_Export("ExecuteCommand", Plug_ExecuteCommand))
	{
		Con_Printf("ezScript Plugin failed\n");
		return false;
	}

	Con_Printf(va("ezScript Plugin %s Loaded\n",RELEASE));
	ezScript_InitCvars();
	ezScript_InitCommands();
	return true;
}
