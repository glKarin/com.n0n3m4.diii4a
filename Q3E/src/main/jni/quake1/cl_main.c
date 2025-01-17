/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_main.c  -- client main loop

#include "quakedef.h"
#include "cl_collision.h"
#include "cl_video.h"
#include "image.h"
#include "csprogs.h"
#include "r_shadow.h"
#include "libcurl.h"
#include "snd_main.h"
#include "cdaudio.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

cvar_t csqc_progname = {CF_CLIENT | CF_SERVER, "csqc_progname","csprogs.dat","name of csprogs.dat file to load"};
cvar_t csqc_progcrc = {CF_CLIENT | CF_READONLY, "csqc_progcrc","-1","CRC of csprogs.dat file to load (-1 is none), only used during level changes and then reset to -1"};
cvar_t csqc_progsize = {CF_CLIENT | CF_READONLY, "csqc_progsize","-1","file size of csprogs.dat file to load (-1 is none), only used during level changes and then reset to -1"};
cvar_t csqc_usedemoprogs = {CF_CLIENT, "csqc_usedemoprogs","1","use csprogs stored in demos"};
cvar_t csqc_polygons_defaultmaterial_nocullface = {CF_CLIENT, "csqc_polygons_defaultmaterial_nocullface", "0", "use 'cull none' behavior in the default shader for rendering R_PolygonBegin - warning: enabling this is not consistent with FTEQW behavior on this feature"};
cvar_t csqc_lowres = {CF_CLIENT, "csqc_lowres", "0", "make EXT_CSQC functions CSQC_UpdateView(), setproperty(), getproperty() use the virtual 2D resolution (FTEQW/QSS behaviour) instead of the real resolution (DP behaviour); this mode is always used for the CSQC_SIMPLE (aka hud-only) CSQC_DrawHud() parameters; see cvars vid_conheight and vid_conwidth"};

cvar_t cl_shownet = {CF_CLIENT, "cl_shownet","0","1 = print packet size, 2 = print packet message list"};
cvar_t cl_nolerp = {CF_CLIENT, "cl_nolerp", "0","network update smoothing"};
cvar_t cl_lerpexcess = {CF_CLIENT, "cl_lerpexcess", "0","maximum allowed lerp excess (hides, not fixes, some packet loss)"};
cvar_t cl_lerpanim_maxdelta_server = {CF_CLIENT, "cl_lerpanim_maxdelta_server", "0.1","maximum frame delta for smoothing between server-controlled animation frames (when 0, one network frame)"};
cvar_t cl_lerpanim_maxdelta_framegroups = {CF_CLIENT, "cl_lerpanim_maxdelta_framegroups", "0.1","maximum frame delta for smoothing between framegroups (when 0, one network frame)"};

cvar_t cl_itembobheight = {CF_CLIENT, "cl_itembobheight", "0","how much items bob up and down (try 8)"};
cvar_t cl_itembobspeed = {CF_CLIENT, "cl_itembobspeed", "0.5","how frequently items bob up and down"};

cvar_t lookspring = {CF_CLIENT | CF_ARCHIVE, "lookspring","0","returns pitch to level with the floor when no longer holding a pitch key"};
cvar_t lookstrafe = {CF_CLIENT | CF_ARCHIVE, "lookstrafe","0","move instead of turning"};
cvar_t sensitivity = {CF_CLIENT | CF_ARCHIVE, "sensitivity","3","mouse speed multiplier"};

cvar_t m_pitch = {CF_CLIENT | CF_ARCHIVE, "m_pitch","0.022","mouse pitch speed multiplier"};
cvar_t m_yaw = {CF_CLIENT | CF_ARCHIVE, "m_yaw","0.022","mouse yaw speed multiplier"};
cvar_t m_forward = {CF_CLIENT | CF_ARCHIVE, "m_forward","1","mouse forward speed multiplier"};
cvar_t m_side = {CF_CLIENT | CF_ARCHIVE, "m_side","0.8","mouse side speed multiplier"};

cvar_t freelook = {CF_CLIENT | CF_ARCHIVE, "freelook", "1","mouse controls pitch instead of forward/back"};

cvar_t cl_autodemo = {CF_CLIENT | CF_ARCHIVE, "cl_autodemo", "0", "records every game played, using the date/time and map name to name the demo file" };
cvar_t cl_autodemo_nameformat = {CF_CLIENT | CF_ARCHIVE, "cl_autodemo_nameformat", "autodemos/%Y-%m-%d_%H-%M", "The format of the cl_autodemo filename, followed by the map name (the date is encoded using strftime escapes)" };
cvar_t cl_autodemo_delete = {CF_CLIENT, "cl_autodemo_delete", "0", "3: automatically delete every newly recorded demo unless this cvar is set to 2 during a game, in case something interesting happened (cvar will be automatically set back to 3);  0: keep every newly recorded demo unless this cvar is set to 1 during a game, in case nothing interesting happened (cvar will be automatically set back to 0).  Technically speaking, the value is a bitmask: bit 1 defines behaviour for all demos, bit 0 overrides behaviour for the demo currently being recorded" };
cvar_t cl_startdemos = {CF_CLIENT | CF_ARCHIVE, "cl_startdemos", "1", "1 enables the `startdemos` loop used in Quake and some mods, 0 goes straight to the menu"};

cvar_t r_draweffects = {CF_CLIENT, "r_draweffects", "1","renders temporary sprite effects"};

cvar_t cl_explosions_alpha_start = {CF_CLIENT | CF_ARCHIVE, "cl_explosions_alpha_start", "1.5","starting alpha of an explosion shell"};
cvar_t cl_explosions_alpha_end = {CF_CLIENT | CF_ARCHIVE, "cl_explosions_alpha_end", "0","end alpha of an explosion shell (just before it disappears)"};
cvar_t cl_explosions_size_start = {CF_CLIENT | CF_ARCHIVE, "cl_explosions_size_start", "16","starting size of an explosion shell"};
cvar_t cl_explosions_size_end = {CF_CLIENT | CF_ARCHIVE, "cl_explosions_size_end", "128","ending alpha of an explosion shell (just before it disappears)"};
cvar_t cl_explosions_lifetime = {CF_CLIENT | CF_ARCHIVE, "cl_explosions_lifetime", "0.5","how long an explosion shell lasts"};

cvar_t cl_stainmaps = {CF_CLIENT | CF_ARCHIVE, "cl_stainmaps", "0","stains lightmaps, much faster than decals but blurred"};
cvar_t cl_stainmaps_clearonload = {CF_CLIENT | CF_ARCHIVE, "cl_stainmaps_clearonload", "1","clear stainmaps on map restart"};

cvar_t cl_beams_polygons = {CF_CLIENT | CF_ARCHIVE, "cl_beams_polygons", "1","use beam polygons instead of models"};
cvar_t cl_beams_quakepositionhack = {CF_CLIENT | CF_ARCHIVE, "cl_beams_quakepositionhack", "1", "makes your lightning gun appear to fire from your waist (as in Quake and QuakeWorld)"};
cvar_t cl_beams_instantaimhack = {CF_CLIENT | CF_ARCHIVE, "cl_beams_instantaimhack", "0", "makes your lightning gun aiming update instantly"};
cvar_t cl_beams_lightatend = {CF_CLIENT | CF_ARCHIVE, "cl_beams_lightatend", "0", "make a light at the end of the beam"};

cvar_t cl_deathfade = {CF_CLIENT | CF_ARCHIVE, "cl_deathfade", "0", "fade screen to dark red when dead, value represents how fast the fade is (higher is faster)"};

cvar_t cl_noplayershadow = {CF_CLIENT | CF_ARCHIVE, "cl_noplayershadow", "0","hide player shadow"};

cvar_t cl_dlights_decayradius = {CF_CLIENT | CF_ARCHIVE, "cl_dlights_decayradius", "1", "reduces size of light flashes over time"};
cvar_t cl_dlights_decaybrightness = {CF_CLIENT | CF_ARCHIVE, "cl_dlights_decaybrightness", "1", "reduces brightness of light flashes over time"};

cvar_t qport = {CF_CLIENT, "qport", "0", "identification key for playing on qw servers (allows you to maintain a connection to a quakeworld server even if your port changes)"};

cvar_t cl_prydoncursor = {CF_CLIENT, "cl_prydoncursor", "0", "enables a mouse pointer which is able to click on entities in the world, useful for point and click mods, see PRYDON_CLIENTCURSOR extension in dpextensions.qc"};
cvar_t cl_prydoncursor_notrace = {CF_CLIENT, "cl_prydoncursor_notrace", "0", "disables traceline used in prydon cursor reporting to the game, saving some cpu time"};

cvar_t cl_deathnoviewmodel = {CF_CLIENT, "cl_deathnoviewmodel", "1", "hides gun model when dead"};

cvar_t cl_locs_enable = {CF_CLIENT | CF_ARCHIVE, "locs_enable", "1", "enables replacement of certain % codes in chat messages: %l (location), %d (last death location), %h (health), %a (armor), %x (rockets), %c (cells), %r (rocket launcher status), %p (powerup status), %w (weapon status), %t (current time in level)"};
cvar_t cl_locs_show = {CF_CLIENT, "locs_show", "0", "shows defined locations for editing purposes"};

cvar_t cl_minfps = {CF_CLIENT | CF_ARCHIVE, "cl_minfps", "40", "minimum fps target - while the rendering performance is below this, it will drift toward lower quality"};
cvar_t cl_minfps_fade = {CF_CLIENT | CF_ARCHIVE, "cl_minfps_fade", "1", "how fast the quality adapts to varying framerate"};
cvar_t cl_minfps_qualitymax = {CF_CLIENT | CF_ARCHIVE, "cl_minfps_qualitymax", "1", "highest allowed drawdistance multiplier"};
cvar_t cl_minfps_qualitymin = {CF_CLIENT | CF_ARCHIVE, "cl_minfps_qualitymin", "0.25", "lowest allowed drawdistance multiplier"};
cvar_t cl_minfps_qualitymultiply = {CF_CLIENT | CF_ARCHIVE, "cl_minfps_qualitymultiply", "0.2", "multiplier for quality changes in quality change per second render time (1 assumes linearity of quality and render time)"};
cvar_t cl_minfps_qualityhysteresis = {CF_CLIENT | CF_ARCHIVE, "cl_minfps_qualityhysteresis", "0.05", "reduce all quality increments by this to reduce flickering"};
cvar_t cl_minfps_qualitystepmax = {CF_CLIENT | CF_ARCHIVE, "cl_minfps_qualitystepmax", "0.1", "maximum quality change in a single frame"};
cvar_t cl_minfps_force = {CF_CLIENT, "cl_minfps_force", "0", "also apply quality reductions in timedemo/capturevideo"};
cvar_t cl_maxfps = {CF_CLIENT | CF_ARCHIVE, "cl_maxfps", "0", "maximum fps cap, 0 = unlimited, if game is running faster than this it will wait before running another frame (useful to make cpu time available to other programs)"};
cvar_t cl_maxfps_alwayssleep = {CF_CLIENT | CF_ARCHIVE, "cl_maxfps_alwayssleep", "0", "gives up some processing time to other applications each frame, value in milliseconds, disabled if a timedemo is running"};
cvar_t cl_maxidlefps = {CF_CLIENT | CF_ARCHIVE, "cl_maxidlefps", "20", "maximum fps cap when the game is not the active window (makes cpu time available to other programs"};

cvar_t cl_areagrid_link_SOLID_NOT = {CF_CLIENT, "cl_areagrid_link_SOLID_NOT", "1", "set to 0 to prevent SOLID_NOT entities from being linked to the area grid, and unlink any that are already linked (in the code paths that would otherwise link them), for better performance"};
cvar_t cl_gameplayfix_nudgeoutofsolid_separation = {CF_CLIENT, "cl_gameplayfix_nudgeoutofsolid_separation", "0.03125", "keep objects this distance apart to prevent collision issues on seams"};


client_static_t	cls;
client_state_t	cl;

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState(void)
{
	int i;
	entity_t *ent;

	CL_VM_ShutDown();

// wipe the entire cl structure
	Mem_EmptyPool(cls.levelmempool);
	memset (&cl, 0, sizeof(cl));

	S_StopAllSounds();

	// reset the view zoom interpolation
	cl.mviewzoom[0] = cl.mviewzoom[1] = 1;
	cl.sensitivityscale = 1.0f;

	// enable rendering of the world and such
	cl.csqc_vidvars.drawworld = r_drawworld.integer != 0;
	cl.csqc_vidvars.drawenginesbar = true;
	cl.csqc_vidvars.drawcrosshair = true;

	// set up the float version of the stats array for easier access to float stats
	cl.statsf = (float *)cl.stats;

	cl.num_entities = 0;
	cl.num_static_entities = 0;
	cl.num_brushmodel_entities = 0;

	// tweak these if the game runs out
	cl.max_csqcrenderentities = 0;
	cl.max_entities = MAX_ENTITIES_INITIAL;
	cl.max_static_entities = MAX_STATICENTITIES;
	cl.max_effects = MAX_EFFECTS;
	cl.max_beams = MAX_BEAMS;
	cl.max_dlights = MAX_DLIGHTS;
	cl.max_lightstyle = MAX_LIGHTSTYLES;
	cl.max_brushmodel_entities = MAX_EDICTS;
	cl.max_particles = MAX_PARTICLES_INITIAL; // grows dynamically
	cl.max_showlmps = 0;

	cl.num_dlights = 0;
	cl.num_effects = 0;
	cl.num_beams = 0;

	cl.csqcrenderentities = NULL;
	cl.entities = (entity_t *)Mem_Alloc(cls.levelmempool, cl.max_entities * sizeof(entity_t));
	cl.entities_active = (unsigned char *)Mem_Alloc(cls.levelmempool, cl.max_brushmodel_entities * sizeof(unsigned char));
	cl.static_entities = (entity_t *)Mem_Alloc(cls.levelmempool, cl.max_static_entities * sizeof(entity_t));
	cl.effects = (cl_effect_t *)Mem_Alloc(cls.levelmempool, cl.max_effects * sizeof(cl_effect_t));
	cl.beams = (beam_t *)Mem_Alloc(cls.levelmempool, cl.max_beams * sizeof(beam_t));
	cl.dlights = (dlight_t *)Mem_Alloc(cls.levelmempool, cl.max_dlights * sizeof(dlight_t));
	cl.lightstyle = (lightstyle_t *)Mem_Alloc(cls.levelmempool, cl.max_lightstyle * sizeof(lightstyle_t));
	cl.brushmodel_entities = (int *)Mem_Alloc(cls.levelmempool, cl.max_brushmodel_entities * sizeof(int));
	cl.particles = (particle_t *) Mem_Alloc(cls.levelmempool, cl.max_particles * sizeof(particle_t));
	cl.showlmps = NULL;

	// LadyHavoc: have to set up the baseline info for alpha and other stuff
	for (i = 0;i < cl.max_entities;i++)
	{
		cl.entities[i].state_baseline = defaultstate;
		cl.entities[i].state_previous = defaultstate;
		cl.entities[i].state_current = defaultstate;
	}

	if (IS_NEXUIZ_DERIVED(gamemode))
	{
		VectorSet(cl.playerstandmins, -16, -16, -24);
		VectorSet(cl.playerstandmaxs, 16, 16, 45);
		VectorSet(cl.playercrouchmins, -16, -16, -24);
		VectorSet(cl.playercrouchmaxs, 16, 16, 25);
	}
	else
	{
		VectorSet(cl.playerstandmins, -16, -16, -24);
		VectorSet(cl.playerstandmaxs, 16, 16, 24);
		VectorSet(cl.playercrouchmins, -16, -16, -24);
		VectorSet(cl.playercrouchmaxs, 16, 16, 24);
	}

	// disable until we get textures for it
	R_ResetSkyBox();

	ent = &cl.entities[0];
	// entire entity array was cleared, so just fill in a few fields
	ent->state_current.active = true;
	ent->render.model = cl.worldmodel = NULL; // no world model yet
	ent->render.alpha = 1;
	ent->render.flags = RENDER_SHADOW | RENDER_LIGHT;
	Matrix4x4_CreateFromQuakeEntity(&ent->render.matrix, 0, 0, 0, 0, 0, 0, 1);
	ent->render.allowdecals = true;
	CL_UpdateRenderEntity(&ent->render);

	// noclip is turned off at start
	noclip_anglehack = false;

	// mark all frames invalid for delta
	memset(cl.qw_deltasequence, -1, sizeof(cl.qw_deltasequence));

	// set bestweapon data back to Quake data
	IN_BestWeapon_ResetData();

	CL_Screen_NewMap();
}

extern cvar_t cl_topcolor;
extern cvar_t cl_bottomcolor;

void CL_SetInfo(const char *key, const char *value, qbool send, qbool allowstarkey, qbool allowmodel, qbool quiet)
{
	int i;
	qbool fail = false;
	char vabuf[1024];
	if (!allowstarkey && key[0] == '*')
		fail = true;
	if (!allowmodel && (!strcasecmp(key, "pmodel") || !strcasecmp(key, "emodel")))
		fail = true;
	for (i = 0;key[i];i++)
		if (ISWHITESPACE(key[i]) || key[i] == '\"')
			fail = true;
	for (i = 0;value[i];i++)
		if (value[i] == '\r' || value[i] == '\n' || value[i] == '\"')
			fail = true;
	if (fail)
	{
		if (!quiet)
			Con_Printf("Can't setinfo \"%s\" \"%s\"\n", key, value);
		return;
	}
	InfoString_SetValue(cls.userinfo, sizeof(cls.userinfo), key, value);
	if (cls.state == ca_connected && cls.netcon)
	{
		if (cls.protocol == PROTOCOL_QUAKEWORLD)
		{
			MSG_WriteByte(&cls.netcon->message, qw_clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "setinfo \"%s\" \"%s\"", key, value));
		}
		else if (!strcasecmp(key, "_cl_name") || !strcasecmp(key, "name"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "name \"%s\"", value));
		}
		else if (!strcasecmp(key, "playermodel"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "playermodel \"%s\"", value));
		}
		else if (!strcasecmp(key, "playerskin"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "playerskin \"%s\"", value));
		}
		else if (!strcasecmp(key, "topcolor"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "color %i %i", atoi(value), cl_bottomcolor.integer));
		}
		else if (!strcasecmp(key, "bottomcolor"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "color %i %i", cl_topcolor.integer, atoi(value)));
		}
		else if (!strcasecmp(key, "rate"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "rate \"%s\"", value));
		}
		else if (!strcasecmp(key, "rate_burstsize"))
		{
			MSG_WriteByte(&cls.netcon->message, clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, va(vabuf, sizeof(vabuf), "rate_burstsize \"%s\"", value));
		}
	}
}

void CL_ExpandEntities(int num)
{
	int i, oldmaxentities;
	entity_t *oldentities;
	if (num >= cl.max_entities)
	{
		if (!cl.entities)
			Sys_Error("CL_ExpandEntities: cl.entities not initialized");
		if (num >= MAX_EDICTS)
			Host_Error("CL_ExpandEntities: num %i >= %i", num, MAX_EDICTS);
		oldmaxentities = cl.max_entities;
		oldentities = cl.entities;
		cl.max_entities = (num & ~255) + 256;
		cl.entities = (entity_t *)Mem_Alloc(cls.levelmempool, cl.max_entities * sizeof(entity_t));
		memcpy(cl.entities, oldentities, oldmaxentities * sizeof(entity_t));
		Mem_Free(oldentities);
		for (i = oldmaxentities;i < cl.max_entities;i++)
		{
			cl.entities[i].state_baseline = defaultstate;
			cl.entities[i].state_previous = defaultstate;
			cl.entities[i].state_current = defaultstate;
		}
	}
}

void CL_ExpandCSQCRenderEntities(int num)
{
	int i;
	int oldmaxcsqcrenderentities;
	entity_render_t *oldcsqcrenderentities;
	if (num >= cl.max_csqcrenderentities)
	{
		if (num >= MAX_EDICTS)
			Host_Error("CL_ExpandEntities: num %i >= %i", num, MAX_EDICTS);
		oldmaxcsqcrenderentities = cl.max_csqcrenderentities;
		oldcsqcrenderentities = cl.csqcrenderentities;
		cl.max_csqcrenderentities = (num & ~255) + 256;
		cl.csqcrenderentities = (entity_render_t *)Mem_Alloc(cls.levelmempool, cl.max_csqcrenderentities * sizeof(entity_render_t));
		if (oldcsqcrenderentities)
		{
			memcpy(cl.csqcrenderentities, oldcsqcrenderentities, oldmaxcsqcrenderentities * sizeof(entity_render_t));
			for (i = 0;i < r_refdef.scene.numentities;i++)
				if(r_refdef.scene.entities[i] >= oldcsqcrenderentities && r_refdef.scene.entities[i] < (oldcsqcrenderentities + oldmaxcsqcrenderentities))
					r_refdef.scene.entities[i] = cl.csqcrenderentities + (r_refdef.scene.entities[i] - oldcsqcrenderentities);
			Mem_Free(oldcsqcrenderentities);
		}
	}
}

static void CL_ToggleMenu_Hook(void)
{
#ifdef CONFIG_MENU
	// remove menu
	if (key_dest == key_menu || key_dest == key_menu_grabbed)
		MR_ToggleMenu(0);
#endif
	key_dest = key_game;
}

extern cvar_t rcon_secure;

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/

void CL_DisconnectEx(qbool kicked, const char *fmt, ... )
{
	va_list argptr;
	char reason[512];

	if (cls.state == ca_dedicated)
		return;

	if(fmt)
	{
		va_start(argptr,fmt);
		dpvsnprintf(reason,sizeof(reason),fmt,argptr);
		va_end(argptr);
	}
	else
	{
		dpsnprintf(reason, sizeof(reason), "Disconnect by user");
	}

	if (Sys_CheckParm("-profilegameonly"))
		Sys_AllowProfiling(false);

	Curl_Clear_forthismap();

	Con_DPrintf("CL_Disconnect\n");

	Cvar_SetValueQuick(&csqc_progcrc, -1);
	Cvar_SetValueQuick(&csqc_progsize, -1);
	CL_VM_ShutDown();
	// stop sounds (especially looping!)
	S_StopAllSounds();
	// prevent dlcache assets from this server from interfering with the next one
	FS_UnloadPacks_dlcache();

	cl.parsingtextexpectingpingforscores = 0; // just in case no reply has come yet

	// clear contents blends
	cl.cshifts[0].percent = 0;
	cl.cshifts[1].percent = 0;
	cl.cshifts[2].percent = 0;
	cl.cshifts[3].percent = 0;

	cl.worldmodel = NULL;

	CL_Parse_ErrorCleanUp();

	if (cls.demoplayback)
		CL_StopPlayback();
	else if (cls.netcon)
	{
		sizebuf_t buf;
		unsigned char bufdata[520];
		if (cls.demorecording)
			CL_Stop_f(cmd_local);

		if(!kicked)
		{
			// send disconnect message 3 times to improve chances of server
			// receiving it (but it still fails sometimes)
			memset(&buf, 0, sizeof(buf));
			buf.data = bufdata;
			buf.maxsize = sizeof(bufdata);
			if (cls.protocol == PROTOCOL_QUAKEWORLD)
			{
				Con_DPrint("Sending drop command\n");
				MSG_WriteByte(&buf, qw_clc_stringcmd);
				MSG_WriteString(&buf, "drop");
			}
			else
			{
				Con_DPrint("Sending clc_disconnect\n");
				MSG_WriteByte(&buf, clc_disconnect);
				if(cls.protocol == PROTOCOL_DARKPLACES8)
					MSG_WriteString(&buf, reason);
				// DP8 TODO: write a simpler func that Sys_HandleCrash() calls
				// to send a disconnect message indicating we crashed
			}
			NetConn_SendUnreliableMessage(cls.netcon, &buf, cls.protocol, 10000, 0, false);
			NetConn_SendUnreliableMessage(cls.netcon, &buf, cls.protocol, 10000, 0, false);
			NetConn_SendUnreliableMessage(cls.netcon, &buf, cls.protocol, 10000, 0, false);
		}

		NetConn_Close(cls.netcon);
		cls.netcon = NULL;

		// It's possible for a server to disconnect a player with an empty reason
		// which is checked here rather than above so we don't print "Disconnect by user".
		if(fmt && reason[0] != '\0')
			dpsnprintf(cl_connect_status, sizeof(cl_connect_status), "Disconnect: %s", reason);
		else
			dp_strlcpy(cl_connect_status, "Disconnected", sizeof(cl_connect_status));
		Con_Printf("%s\n", cl_connect_status);
	}
	cls.state = ca_disconnected;
	cl.islocalgame = false;
	cls.signon = 0;
	cls.demoplayback = cls.timedemo = host.restless = false;
	Cvar_Callback(&vid_vsync); // might need to re-enable vsync

	Cvar_Callback(&cl_netport);

	// If we're dropped mid-connection attempt, it won't clear otherwise.
	SCR_ClearLoadingScreen(false);

	if(host.hook.SV_Shutdown)
		host.hook.SV_Shutdown();
}

void CL_Disconnect(void)
{
	CL_DisconnectEx(false, NULL);
}

/*
==================
CL_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
static void CL_Reconnect_f(cmd_state_t *cmd)
{
	char temp[128];
	// if not connected, reconnect to the most recent server
	if (!cls.netcon)
	{
		// if we have connected to a server recently, the userinfo
		// will still contain its IP address, so get the address...
		InfoString_GetValue(cls.userinfo, "*ip", temp, sizeof(temp));
		if (temp[0])
			CL_EstablishConnection(temp, -1);
		else
			Con_Printf(CON_WARN "Reconnect to what server?  (you have not connected to a server yet)\n");
		return;
	}
	// if connected, do something based on protocol
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
	{
		// quakeworld can just re-login
		if (cls.qw_downloadmemory)  // don't change when downloading
			return;

		S_StopAllSounds();

		if (cls.state == ca_connected)
		{
			Con_Printf("Server is changing level...\n");
			MSG_WriteChar(&cls.netcon->message, qw_clc_stringcmd);
			MSG_WriteString(&cls.netcon->message, "new");
		}
	}
	else
	{
		// netquake uses reconnect on level changes (silly)
		if (Cmd_Argc(cmd) != 1)
		{
			Con_Print("reconnect : wait for signon messages again\n");
			return;
		}
		if (!cls.signon)
		{
			Con_Print("reconnect: no signon, ignoring reconnect\n");
			return;
		}
		cls.signon = 0;		// need new connection messages
	}
}

/*
=====================
CL_Connect_f

User command to connect to server
=====================
*/
static void CL_Connect_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) < 2)
	{
		Con_Print("connect <serveraddress> [<key> <value> ...]: connect to a multiplayer game\n");
		return;
	}
	// clear the rcon password, to prevent vulnerability by stuffcmd-ing a connect command
	if(rcon_secure.integer <= 0)
		Cvar_SetQuick(&rcon_password, "");
	CL_EstablishConnection(Cmd_Argv(cmd, 1), 2);
}

void CL_Disconnect_f(cmd_state_t *cmd)
{
	Cmd_Argc(cmd) < 1 ? CL_Disconnect() : CL_DisconnectEx(false, Cmd_Argv(cmd, 1));
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address
=====================
*/
void CL_EstablishConnection(const char *address, int firstarg)
{
	if (cls.state == ca_dedicated)
		return;

	// don't connect to a server if we're benchmarking a demo
	if (Sys_CheckParm("-benchmark"))
		return;

	// make sure the client ports are open before attempting to connect
	NetConn_UpdateSockets();

	if (LHNETADDRESS_FromString(&cls.connect_address, address, 26000) && (cls.connect_mysocket = NetConn_ChooseClientSocketForAddress(&cls.connect_address)))
	{
		cls.connect_trying = true;
		cls.connect_remainingtries = 10;
		cls.connect_nextsendtime = 0;

		// only NOW, set connect_userinfo
		if(firstarg >= 0)
		{
			int i;
			*cls.connect_userinfo = 0;
			for(i = firstarg; i+2 <= Cmd_Argc(cmd_local); i += 2)
				InfoString_SetValue(cls.connect_userinfo, sizeof(cls.connect_userinfo), Cmd_Argv(cmd_local, i), Cmd_Argv(cmd_local, i+1));
		}
		else if(firstarg < -1)
		{
			// -1: keep as is (reconnect)
			// -2: clear
			*cls.connect_userinfo = 0;
		}

		dp_strlcpy(cl_connect_status, "Connect: pending...", sizeof(cl_connect_status));
		SCR_BeginLoadingPlaque(false);
	}
	else
	{
		Con_Printf(CON_ERROR "Connect: failed, unable to find a network socket suitable to reach %s\n", address);
		dp_strlcpy(cl_connect_status, "Connect: failed, no network", sizeof(cl_connect_status));
	}
}

static void CL_EstablishConnection_Local(void)
{
	if(cls.state == ca_disconnected)
		CL_EstablishConnection("local:1", -2);
}


/*
==============
CL_PrintEntities_f
==============
*/
static void CL_PrintEntities_f(cmd_state_t *cmd)
{
	entity_t *ent;
	int i;

	for (i = 0, ent = cl.entities;i < cl.num_entities;i++, ent++)
	{
		const char* modelname;

		if (!ent->state_current.active)
			continue;

		if (ent->render.model)
			modelname = ent->render.model->name;
		else
			modelname = "--no model--";
		Con_Printf("%3i: %-25s:%4i (%5i %5i %5i) [%3i %3i %3i] %4.2f %5.3f\n", i, modelname, ent->render.framegroupblend[0].frame, (int) ent->state_current.origin[0], (int) ent->state_current.origin[1], (int) ent->state_current.origin[2], (int) ent->state_current.angles[0] % 360, (int) ent->state_current.angles[1] % 360, (int) ent->state_current.angles[2] % 360, ent->render.scale, ent->render.alpha);
	}
}

/*
===============
CL_ModelIndexList_f

List information on all models in the client modelindex
===============
*/
static void CL_ModelIndexList_f(cmd_state_t *cmd)
{
	int i;
	model_t *model;

	// Print Header
	Con_Printf("%3s: %-30s %-8s %-8s\n", "ID", "Name", "Type", "Triangles");

	for (i = -MAX_MODELS;i < MAX_MODELS;i++)
	{
		model = CL_GetModelByIndex(i);
		if (!model)
			continue;
		if(model->loaded || i == 1)
			Con_Printf("%3i: %-30s %-8s %-10i\n", i, model->name, model->modeldatatypestring, model->surfmesh.num_triangles);
		else
			Con_Printf("%3i: %-30s %-30s\n", i, model->name, "--no local model found--");
		i++;
	}
}

/*
===============
CL_SoundIndexList_f

List all sounds in the client soundindex
===============
*/
static void CL_SoundIndexList_f(cmd_state_t *cmd)
{
	int i = 1;

	while(cl.sound_precache[i] && i != MAX_SOUNDS)
	{ // Valid Sound
		Con_Printf("%i : %s\n", i, cl.sound_precache[i]->name);
		i++;
	}
}

/*
===============
CL_UpdateRenderEntity

Updates inversematrix, animation interpolation factors, scale, and mins/maxs
===============
*/
void CL_UpdateRenderEntity(entity_render_t *ent)
{
	vec3_t org;
	vec_t scale;
	model_t *model = ent->model;
	// update the inverse matrix for the renderer
	Matrix4x4_Invert_Simple(&ent->inversematrix, &ent->matrix);
	// update the animation blend state
	VM_FrameBlendFromFrameGroupBlend(ent->frameblend, ent->framegroupblend, ent->model, cl.time);
	// we need the matrix origin to center the box
	Matrix4x4_OriginFromMatrix(&ent->matrix, org);
	// update entity->render.scale because the renderer needs it
	ent->scale = scale = Matrix4x4_ScaleFromMatrix(&ent->matrix);
	if (model)
	{
		// NOTE: this directly extracts vector components from the matrix, which relies on the matrix orientation!
#ifdef MATRIX4x4_OPENGLORIENTATION
		if (ent->matrix.m[0][2] != 0 || ent->matrix.m[1][2] != 0)
#else
		if (ent->matrix.m[2][0] != 0 || ent->matrix.m[2][1] != 0)
#endif
		{
			// pitch or roll
			VectorMA(org, scale, model->rotatedmins, ent->mins);
			VectorMA(org, scale, model->rotatedmaxs, ent->maxs);
		}
#ifdef MATRIX4x4_OPENGLORIENTATION
		else if (ent->matrix.m[1][0] != 0 || ent->matrix.m[0][1] != 0)
#else
		else if (ent->matrix.m[0][1] != 0 || ent->matrix.m[1][0] != 0)
#endif
		{
			// yaw
			VectorMA(org, scale, model->yawmins, ent->mins);
			VectorMA(org, scale, model->yawmaxs, ent->maxs);
		}
		else
		{
			VectorMA(org, scale, model->normalmins, ent->mins);
			VectorMA(org, scale, model->normalmaxs, ent->maxs);
		}
	}
	else
	{
		ent->mins[0] = org[0] - 16;
		ent->mins[1] = org[1] - 16;
		ent->mins[2] = org[2] - 16;
		ent->maxs[0] = org[0] + 16;
		ent->maxs[1] = org[1] + 16;
		ent->maxs[2] = org[2] + 16;
	}
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
static float CL_LerpPoint(void)
{
	float f;

	if (cl_nettimesyncboundmode.integer == 1)
		cl.time = bound(cl.mtime[1], cl.time, cl.mtime[0]);

	// LadyHavoc: lerp in listen games as the server is being capped below the client (usually)
	if (cl.mtime[0] <= cl.mtime[1])
	{
		cl.time = cl.mtime[0];
		return 1;
	}

	f = (cl.time - cl.mtime[1]) / (cl.mtime[0] - cl.mtime[1]);
	return bound(0, f, 1 + cl_lerpexcess.value);
}

void CL_ClearTempEntities (void)
{
	r_refdef.scene.numtempentities = 0;
	// grow tempentities buffer on request
	if (r_refdef.scene.expandtempentities)
	{
		Con_Printf("CL_NewTempEntity: grow maxtempentities from %i to %i\n", r_refdef.scene.maxtempentities, r_refdef.scene.maxtempentities * 2);
		r_refdef.scene.maxtempentities *= 2;
		r_refdef.scene.tempentities = (entity_render_t *)Mem_Realloc(cls.permanentmempool, r_refdef.scene.tempentities, sizeof(entity_render_t) * r_refdef.scene.maxtempentities);
		r_refdef.scene.expandtempentities = false;
	}
}

entity_render_t *CL_NewTempEntity(double shadertime)
{
	entity_render_t *render;

	if (r_refdef.scene.numentities >= r_refdef.scene.maxentities)
		return NULL;
	if (r_refdef.scene.numtempentities >= r_refdef.scene.maxtempentities)
	{
		r_refdef.scene.expandtempentities = true; // will be reallocated next frame since current frame may have pointers set already
		return NULL;
	}
	render = &r_refdef.scene.tempentities[r_refdef.scene.numtempentities++];
	memset (render, 0, sizeof(*render));
	r_refdef.scene.entities[r_refdef.scene.numentities++] = render;

	render->shadertime = shadertime;
	render->alpha = 1;
	VectorSet(render->colormod, 1, 1, 1);
	VectorSet(render->glowmod, 1, 1, 1);
	return render;
}

void CL_Effect(vec3_t org, model_t *model, int startframe, int framecount, float framerate)
{
	int i;
	cl_effect_t *e;
	if (!model) // sanity check
		return;
	if (framerate < 1)
	{
		Con_Printf("CL_Effect: framerate %f is < 1\n", framerate);
		return;
	}
	if (framecount < 1)
	{
		Con_Printf("CL_Effect: framecount %i is < 1\n", framecount);
		return;
	}
	for (i = 0, e = cl.effects;i < cl.max_effects;i++, e++)
	{
		if (e->active)
			continue;
		e->active = true;
		VectorCopy(org, e->origin);
		e->model = model;
		e->starttime = cl.time;
		e->startframe = startframe;
		e->endframe = startframe + framecount;
		e->framerate = framerate;

		e->frame = 0;
		e->frame1time = cl.time;
		e->frame2time = cl.time;
		cl.num_effects = max(cl.num_effects, i + 1);
		break;
	}
}

void CL_AllocLightFlash(entity_render_t *ent, matrix4x4_t *matrix, float radius, float red, float green, float blue, float decay, float lifetime, char *cubemapname, int style, int shadowenable, vec_t corona, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags)
{
	int i;
	dlight_t *dl;

// then look for anything else
	dl = cl.dlights;
	for (i = 0;i < cl.max_dlights;i++, dl++)
		if (!dl->radius)
			break;

	// unable to find one
	if (i == cl.max_dlights)
		return;

	//Con_Printf("dlight %i : %f %f %f : %f %f %f\n", i, org[0], org[1], org[2], red * radius, green * radius, blue * radius);
	memset (dl, 0, sizeof(*dl));
	cl.num_dlights = max(cl.num_dlights, i + 1);
	Matrix4x4_Normalize(&dl->matrix, matrix);
	dl->ent = ent;
	Matrix4x4_OriginFromMatrix(&dl->matrix, dl->origin);
	CL_FindNonSolidLocation(dl->origin, dl->origin, 6);
	Matrix4x4_SetOrigin(&dl->matrix, dl->origin[0], dl->origin[1], dl->origin[2]);
	dl->radius = radius;
	dl->color[0] = red;
	dl->color[1] = green;
	dl->color[2] = blue;
	dl->initialradius = radius;
	dl->initialcolor[0] = red;
	dl->initialcolor[1] = green;
	dl->initialcolor[2] = blue;
	dl->decay = decay / radius; // changed decay to be a percentage decrease
	dl->intensity = 1; // this is what gets decayed
	if (lifetime)
		dl->die = cl.time + lifetime;
	else
		dl->die = 0;
	dl->cubemapname[0] = 0;
	if (cubemapname && cubemapname[0])
		dp_strlcpy(dl->cubemapname, cubemapname, sizeof(dl->cubemapname));
	dl->style = style;
	dl->shadow = shadowenable;
	dl->corona = corona;
	dl->flags = flags;
	dl->coronasizescale = coronasizescale;
	dl->ambientscale = ambientscale;
	dl->diffusescale = diffusescale;
	dl->specularscale = specularscale;
}

static void CL_DecayLightFlashes(void)
{
	int i, oldmax;
	dlight_t *dl;
	float time;

	time = bound(0, cl.time - cl.oldtime, 0.1);
	oldmax = cl.num_dlights;
	cl.num_dlights = 0;
	for (i = 0, dl = cl.dlights;i < oldmax;i++, dl++)
	{
		if (dl->radius)
		{
			dl->intensity -= time * dl->decay;
			if (cl.time < dl->die && dl->intensity > 0)
			{
				if (cl_dlights_decayradius.integer)
					dl->radius = dl->initialradius * dl->intensity;
				else
					dl->radius = dl->initialradius;
				if (cl_dlights_decaybrightness.integer)
					VectorScale(dl->initialcolor, dl->intensity, dl->color);
				else
					VectorCopy(dl->initialcolor, dl->color);
				cl.num_dlights = i + 1;
			}
			else
				dl->radius = 0;
		}
	}
}

// called before entity relinking
void CL_RelinkLightFlashes(void)
{
	int i, j, k, l;
	dlight_t *dl;
	float frac, f;
	matrix4x4_t tempmatrix;

	if (r_dynamic.integer)
	{
		for (i = 0, dl = cl.dlights;i < cl.num_dlights && r_refdef.scene.numlights < MAX_DLIGHTS;i++, dl++)
		{
			if (dl->radius)
			{
				tempmatrix = dl->matrix;
				Matrix4x4_Scale(&tempmatrix, dl->radius, 1);
				// we need the corona fading to be persistent
				R_RTLight_Update(&dl->rtlight, false, &tempmatrix, dl->color, dl->style, dl->cubemapname, dl->shadow, dl->corona, dl->coronasizescale, dl->ambientscale, dl->diffusescale, dl->specularscale, dl->flags);
				r_refdef.scene.lights[r_refdef.scene.numlights++] = &dl->rtlight;
			}
		}
	}

	if (!cl.lightstyle)
	{
		for (j = 0;j < cl.max_lightstyle;j++)
		{
			r_refdef.scene.rtlightstylevalue[j] = 1;
			r_refdef.scene.lightstylevalue[j] = 256;
		}
		return;
	}

// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	f = cl.time * 10;
	i = (int)floor(f);
	frac = f - i;
	for (j = 0;j < cl.max_lightstyle;j++)
	{
		if (!cl.lightstyle[j].length)
		{
			r_refdef.scene.rtlightstylevalue[j] = 1;
			r_refdef.scene.lightstylevalue[j] = 256;
			continue;
		}
		// static lightstyle "=value"
		if (cl.lightstyle[j].map[0] == '=')
		{
			r_refdef.scene.rtlightstylevalue[j] = atof(cl.lightstyle[j].map + 1);
			if ( r_lerplightstyles.integer || ((int)f - f) < 0.01)
				r_refdef.scene.lightstylevalue[j] = r_refdef.scene.rtlightstylevalue[j];
			continue;
		}
		k = i % cl.lightstyle[j].length;
		l = (i-1) % cl.lightstyle[j].length;
		k = cl.lightstyle[j].map[k] - 'a';
		l = cl.lightstyle[j].map[l] - 'a';
		// rtlightstylevalue is always interpolated because it has no bad
		// consequences for performance
		// lightstylevalue is subject to a cvar for performance reasons;
		// skipping lightmap updates on most rendered frames substantially
		// improves framerates (but makes light fades look bad)
		r_refdef.scene.rtlightstylevalue[j] = ((k*frac)+(l*(1-frac)))*(22/256.0f);
		r_refdef.scene.lightstylevalue[j] = r_lerplightstyles.integer ? (unsigned short)(((k*frac)+(l*(1-frac)))*22) : k*22;
	}
}

static void CL_AddQWCTFFlagModel(entity_t *player, int skin)
{
	int frame = player->render.framegroupblend[0].frame;
	float f;
	entity_render_t *flagrender;
	matrix4x4_t flagmatrix;

	// this code taken from QuakeWorld
	f = 14;
	if (frame >= 29 && frame <= 40)
	{
		if (frame >= 29 && frame <= 34)
		{ //axpain
			if      (frame == 29) f = f + 2;
			else if (frame == 30) f = f + 8;
			else if (frame == 31) f = f + 12;
			else if (frame == 32) f = f + 11;
			else if (frame == 33) f = f + 10;
			else if (frame == 34) f = f + 4;
		}
		else if (frame >= 35 && frame <= 40)
		{ // pain
			if      (frame == 35) f = f + 2;
			else if (frame == 36) f = f + 10;
			else if (frame == 37) f = f + 10;
			else if (frame == 38) f = f + 8;
			else if (frame == 39) f = f + 4;
			else if (frame == 40) f = f + 2;
		}
	}
	else if (frame >= 103 && frame <= 118)
	{
		if      (frame >= 103 && frame <= 104) f = f + 6;  //nailattack
		else if (frame >= 105 && frame <= 106) f = f + 6;  //light
		else if (frame >= 107 && frame <= 112) f = f + 7;  //rocketattack
		else if (frame >= 112 && frame <= 118) f = f + 7;  //shotattack
	}
	// end of code taken from QuakeWorld

	flagrender = CL_NewTempEntity(player->render.shadertime);
	if (!flagrender)
		return;

	flagrender->model = CL_GetModelByIndex(cl.qw_modelindex_flag);
	flagrender->skinnum = skin;
	flagrender->alpha = 1;
	VectorSet(flagrender->colormod, 1, 1, 1);
	VectorSet(flagrender->glowmod, 1, 1, 1);
	// attach the flag to the player matrix
	Matrix4x4_CreateFromQuakeEntity(&flagmatrix, -f, -22, 0, 0, 0, -45, 1);
	Matrix4x4_Concat(&flagrender->matrix, &player->render.matrix, &flagmatrix);
	CL_UpdateRenderEntity(flagrender);
}

matrix4x4_t viewmodelmatrix_withbob;
matrix4x4_t viewmodelmatrix_nobob;

static const vec3_t muzzleflashorigin = {18, 0, 0};

void CL_SetEntityColormapColors(entity_render_t *ent, int colormap)
{
	const unsigned char *cbcolor;
	if (colormap >= 0)
	{
		cbcolor = palette_rgb_pantscolormap[colormap & 0xF];
		VectorScale(cbcolor, (1.0f / 255.0f), ent->colormap_pantscolor);
		cbcolor = palette_rgb_shirtcolormap[(colormap & 0xF0) >> 4];
		VectorScale(cbcolor, (1.0f / 255.0f), ent->colormap_shirtcolor);
	}
	else
	{
		VectorClear(ent->colormap_pantscolor);
		VectorClear(ent->colormap_shirtcolor);
	}
}

// note this is a recursive function, recursionlimit should be 32 or so on the initial call
static void CL_UpdateNetworkEntity(entity_t *e, int recursionlimit, qbool interpolate)
{
	const matrix4x4_t *matrix;
	matrix4x4_t blendmatrix, tempmatrix, matrix2;
	int frame;
	vec_t origin[3], angles[3], lerp;
	entity_t *t;
	entity_render_t *r;
	//entity_persistent_t *p = &e->persistent;
	//entity_render_t *r = &e->render;
	// skip inactive entities and world
	if (!e->state_current.active || e == cl.entities)
		return;
	if (recursionlimit < 1)
		return;
	e->render.alpha = e->state_current.alpha * (1.0f / 255.0f); // FIXME: interpolate?
	e->render.scale = e->state_current.scale * (1.0f / 16.0f); // FIXME: interpolate?
	e->render.flags = e->state_current.flags;
	e->render.effects = e->state_current.effects;
	VectorScale(e->state_current.colormod, (1.0f / 32.0f), e->render.colormod);
	VectorScale(e->state_current.glowmod, (1.0f / 32.0f), e->render.glowmod);
	if(e >= cl.entities && e < cl.entities + cl.num_entities)
		e->render.entitynumber = e - cl.entities;
	else
		e->render.entitynumber = 0;
	if (e->state_current.flags & RENDER_COLORMAPPED)
		CL_SetEntityColormapColors(&e->render, e->state_current.colormap);
	else if (e->state_current.colormap > 0 && e->state_current.colormap <= cl.maxclients && cl.scores != NULL)
		CL_SetEntityColormapColors(&e->render, cl.scores[e->state_current.colormap-1].colors);
	else
		CL_SetEntityColormapColors(&e->render, -1);
	e->render.skinnum = e->state_current.skin;
	if (e->state_current.tagentity)
	{
		// attached entity (gun held in player model's hand, etc)
		// if the tag entity is currently impossible, skip it
		if (e->state_current.tagentity >= cl.num_entities)
			return;
		t = cl.entities + e->state_current.tagentity;
		// if the tag entity is inactive, skip it
		if (t->state_current.active)
		{
			// update the parent first
			CL_UpdateNetworkEntity(t, recursionlimit - 1, interpolate);
			r = &t->render;
		}
		else
		{
			// it may still be a CSQC entity... trying to use its
			// info from last render frame (better than nothing)
			if(!cl.csqc_server2csqcentitynumber[e->state_current.tagentity])
				return;
			r = cl.csqcrenderentities + cl.csqc_server2csqcentitynumber[e->state_current.tagentity];
			if(!r->entitynumber)
				return; // neither CSQC nor legacy entity... can't attach
		}
		// make relative to the entity
		matrix = &r->matrix;
		// some properties of the tag entity carry over
		e->render.flags |= r->flags & (RENDER_EXTERIORMODEL | RENDER_VIEWMODEL);
		// if a valid tagindex is used, make it relative to that tag instead
		if (e->state_current.tagentity && e->state_current.tagindex >= 1 && r->model)
		{
			if(!Mod_Alias_GetTagMatrix(r->model, r->frameblend, r->skeleton, e->state_current.tagindex - 1, &blendmatrix)) // i.e. no error
			{
				// concat the tag matrices onto the entity matrix
				Matrix4x4_Concat(&tempmatrix, &r->matrix, &blendmatrix);
				// use the constructed tag matrix
				matrix = &tempmatrix;
			}
		}
	}
	else if (e->render.flags & RENDER_VIEWMODEL)
	{
		// view-relative entity (guns and such)
		if (e->render.effects & EF_NOGUNBOB)
			matrix = &viewmodelmatrix_nobob; // really attached to view
		else
			matrix = &viewmodelmatrix_withbob; // attached to gun bob matrix
	}
	else
	{
		// world-relative entity (the normal kind)
		matrix = &identitymatrix;
	}

	// movement lerp
	// if it's the predicted player entity, update according to client movement
	// but don't lerp if going through a teleporter as it causes a bad lerp
	// also don't use the predicted location if fixangle was set on both of
	// the most recent server messages, as that cause means you are spectating
	// someone or watching a cutscene of some sort
	if (cl_nolerp.integer || cls.timedemo)
		interpolate = false;

	if (e == cl.entities + cl.playerentity && cl.movement_predicted && (!cl.fixangle[1] || !cl.fixangle[0]))
	{
		VectorCopy(cl.movement_origin, origin);
		VectorSet(angles, 0, cl.viewangles[1], 0);
	}
	else if (interpolate && e->persistent.lerpdeltatime > 0 && (lerp = (cl.time - e->persistent.lerpstarttime) / e->persistent.lerpdeltatime) < 1 + cl_lerpexcess.value)
	{
		// interpolate the origin and angles
		lerp = max(0, lerp);
		VectorLerp(e->persistent.oldorigin, lerp, e->persistent.neworigin, origin);
#if 0
		// this fails at the singularity of euler angles
		VectorSubtract(e->persistent.newangles, e->persistent.oldangles, delta);
		if (delta[0] < -180) delta[0] += 360;else if (delta[0] >= 180) delta[0] -= 360;
		if (delta[1] < -180) delta[1] += 360;else if (delta[1] >= 180) delta[1] -= 360;
		if (delta[2] < -180) delta[2] += 360;else if (delta[2] >= 180) delta[2] -= 360;
		VectorMA(e->persistent.oldangles, lerp, delta, angles);
#else
		{
			vec3_t f0, u0, f1, u1;
			AngleVectors(e->persistent.oldangles, f0, NULL, u0);
			AngleVectors(e->persistent.newangles, f1, NULL, u1);
			VectorMAM(1-lerp, f0, lerp, f1, f0);
			VectorMAM(1-lerp, u0, lerp, u1, u0);
			AnglesFromVectors(angles, f0, u0, false);
		}
#endif
	}
	else
	{
		// no interpolation
		VectorCopy(e->persistent.neworigin, origin);
		VectorCopy(e->persistent.newangles, angles);
	}

	// model setup and some modelflags
	frame = e->state_current.frame;
	e->render.model = CL_GetModelByIndex(e->state_current.modelindex);
	if (e->render.model)
	{
		if (e->render.skinnum >= e->render.model->numskins)
			e->render.skinnum = 0;
		if (frame >= e->render.model->numframes)
			frame = 0;
		// models can set flags such as EF_ROCKET
		// this 0xFF800000 mask is EF_NOMODELFLAGS plus all the higher EF_ flags such as EF_ROCKET
		if (!(e->render.effects & 0xFF800000))
			e->render.effects |= e->render.model->effects;
		// if model is alias or this is a tenebrae-like dlight, reverse pitch direction
		if (e->render.model->type == mod_alias)
			angles[0] = -angles[0];
		if ((e->render.effects & EF_SELECTABLE) && cl.cmd.cursor_entitynumber == e->state_current.number)
		{
			VectorScale(e->render.colormod, 2, e->render.colormod);
			VectorScale(e->render.glowmod, 2, e->render.glowmod);
		}
	}
	// if model is alias or this is a tenebrae-like dlight, reverse pitch direction
	else if (e->state_current.lightpflags & PFLAGS_FULLDYNAMIC)
		angles[0] = -angles[0];
		// NOTE: this must be synced to SV_GetPitchSign!

	if ((e->render.effects & EF_ROTATE) && !(e->render.flags & RENDER_VIEWMODEL))
	{
		angles[1] = ANGLEMOD(100*cl.time);
		if (cl_itembobheight.value)
			origin[2] += (cos(cl.time * cl_itembobspeed.value * (2.0 * M_PI)) + 1.0) * 0.5 * cl_itembobheight.value;
	}

	// animation lerp
	e->render.skeleton = NULL;
	if (e->render.flags & RENDER_COMPLEXANIMATION)
	{
		e->render.framegroupblend[0] = e->state_current.framegroupblend[0];
		e->render.framegroupblend[1] = e->state_current.framegroupblend[1];
		e->render.framegroupblend[2] = e->state_current.framegroupblend[2];
		e->render.framegroupblend[3] = e->state_current.framegroupblend[3];
		if (e->state_current.skeletonobject.model && e->state_current.skeletonobject.relativetransforms)
			e->render.skeleton = &e->state_current.skeletonobject;
	}
	else if (e->render.framegroupblend[0].frame == frame)
	{
		// update frame lerp fraction
		e->render.framegroupblend[0].lerp = 1;
		e->render.framegroupblend[1].lerp = 0;
		if (e->render.framegroupblend[0].start > e->render.framegroupblend[1].start)
		{
			// make sure frame lerp won't last longer than 100ms
			// (this mainly helps with models that use framegroups and
			// switch between them infrequently)
			float maxdelta = cl_lerpanim_maxdelta_server.value;
			if(e->render.model)
			if(e->render.model->animscenes)
			if(e->render.model->animscenes[e->render.framegroupblend[0].frame].framecount > 1 || e->render.model->animscenes[e->render.framegroupblend[1].frame].framecount > 1)
				maxdelta = cl_lerpanim_maxdelta_framegroups.value;
			maxdelta = max(maxdelta, cl.mtime[0] - cl.mtime[1]);
			e->render.framegroupblend[0].lerp = (cl.time - e->render.framegroupblend[0].start) / min(e->render.framegroupblend[0].start - e->render.framegroupblend[1].start, maxdelta);
			e->render.framegroupblend[0].lerp = bound(0, e->render.framegroupblend[0].lerp, 1);
			e->render.framegroupblend[1].lerp = 1 - e->render.framegroupblend[0].lerp;
		}
	}
	else
	{
		// begin a new frame lerp
		e->render.framegroupblend[1] = e->render.framegroupblend[0];
		e->render.framegroupblend[1].lerp = 1;
		e->render.framegroupblend[0].frame = frame;
		e->render.framegroupblend[0].start = cl.time;
		e->render.framegroupblend[0].lerp = 0;
	}

	// set up the render matrix
	if (matrix)
	{
		// attached entity, this requires a matrix multiply (concat)
		// FIXME: e->render.scale should go away
		Matrix4x4_CreateFromQuakeEntity(&matrix2, origin[0], origin[1], origin[2], angles[0], angles[1], angles[2], e->render.scale);
		// concat the matrices to make the entity relative to its tag
		Matrix4x4_Concat(&e->render.matrix, matrix, &matrix2);
		// get the origin from the new matrix
		Matrix4x4_OriginFromMatrix(&e->render.matrix, origin);
	}
	else
	{
		// unattached entities are faster to process
		Matrix4x4_CreateFromQuakeEntity(&e->render.matrix, origin[0], origin[1], origin[2], angles[0], angles[1], angles[2], e->render.scale);
	}

	// tenebrae's sprites are all additive mode (weird)
	if (gamemode == GAME_TENEBRAE && e->render.model && e->render.model->type == mod_sprite)
		e->render.flags |= RENDER_ADDITIVE;
	// player model is only shown with chase_active on
	if (e->state_current.number == cl.viewentity)
		e->render.flags |= RENDER_EXTERIORMODEL;
	// either fullbright or lit
	if(!r_fullbright.integer)
	{
		if (!(e->render.effects & EF_FULLBRIGHT))
			e->render.flags |= RENDER_LIGHT;
	}
	// hide player shadow during intermission or nehahra movie
	if (!(e->render.effects & (EF_NOSHADOW | EF_ADDITIVE | EF_NODEPTHTEST))
	 && (e->render.alpha >= 1)
	 && !(e->render.flags & RENDER_VIEWMODEL)
	 && (!(e->render.flags & RENDER_EXTERIORMODEL) || (!cl.intermission && cls.protocol != PROTOCOL_NEHAHRAMOVIE && !cl_noplayershadow.integer)))
		e->render.flags |= RENDER_SHADOW;
	if (e->render.flags & RENDER_VIEWMODEL)
		e->render.flags |= RENDER_NOSELFSHADOW;
	if (e->render.effects & EF_NOSELFSHADOW)
		e->render.flags |= RENDER_NOSELFSHADOW;
	if (e->render.effects & EF_NODEPTHTEST)
		e->render.flags |= RENDER_NODEPTHTEST;
	if (e->render.effects & EF_ADDITIVE)
		e->render.flags |= RENDER_ADDITIVE;
	if (e->render.effects & EF_DOUBLESIDED)
		e->render.flags |= RENDER_DOUBLESIDED;
	if (e->render.effects & EF_DYNAMICMODELLIGHT)
		e->render.flags |= RENDER_DYNAMICMODELLIGHT;

	// make the other useful stuff
	e->render.allowdecals = true;
	CL_UpdateRenderEntity(&e->render);
}

// creates light and trails from an entity
static void CL_UpdateNetworkEntityTrail(entity_t *e)
{
	effectnameindex_t trailtype;
	vec3_t origin;

	// bmodels are treated specially since their origin is usually '0 0 0' and
	// their actual geometry is far from '0 0 0'
	if (e->render.model && e->render.model->soundfromcenter)
	{
		vec3_t o;
		VectorMAM(0.5f, e->render.model->normalmins, 0.5f, e->render.model->normalmaxs, o);
		Matrix4x4_Transform(&e->render.matrix, o, origin);
	}
	else
		Matrix4x4_OriginFromMatrix(&e->render.matrix, origin);

	// handle particle trails and such effects now that we know where this
	// entity is in the world...
	trailtype = EFFECT_NONE;
	// LadyHavoc: if the entity has no effects, don't check each
	if (e->render.effects & (EF_BRIGHTFIELD | EF_FLAME | EF_STARDUST))
	{
		if (e->render.effects & EF_BRIGHTFIELD)
		{
			if (IS_NEXUIZ_DERIVED(gamemode))
				trailtype = EFFECT_TR_NEXUIZPLASMA;
			else
				CL_EntityParticles(e);
		}
		if (e->render.effects & EF_FLAME)
			CL_ParticleTrail(EFFECT_EF_FLAME, bound(0, cl.time - cl.oldtime, 0.1), origin, origin, vec3_origin, vec3_origin, NULL, 0, false, true, NULL, NULL, 1);
		if (e->render.effects & EF_STARDUST)
			CL_ParticleTrail(EFFECT_EF_STARDUST, bound(0, cl.time - cl.oldtime, 0.1), origin, origin, vec3_origin, vec3_origin, NULL, 0, false, true, NULL, NULL, 1);
	}
	if (e->render.internaleffects & (INTEF_FLAG1QW | INTEF_FLAG2QW))
	{
		// these are only set on player entities
		CL_AddQWCTFFlagModel(e, (e->render.internaleffects & INTEF_FLAG2QW) != 0);
	}
	// muzzleflash fades over time
	if (e->persistent.muzzleflash > 0)
		e->persistent.muzzleflash -= bound(0, cl.time - cl.oldtime, 0.1) * 20;
	// LadyHavoc: if the entity has no effects, don't check each
	if (e->render.effects && !(e->render.flags & RENDER_VIEWMODEL))
	{
		if (e->render.effects & EF_GIB)
			trailtype = EFFECT_TR_BLOOD;
		else if (e->render.effects & EF_ZOMGIB)
			trailtype = EFFECT_TR_SLIGHTBLOOD;
		else if (e->render.effects & EF_TRACER)
			trailtype = EFFECT_TR_WIZSPIKE;
		else if (e->render.effects & EF_TRACER2)
			trailtype = EFFECT_TR_KNIGHTSPIKE;
		else if (e->render.effects & EF_ROCKET)
			trailtype = EFFECT_TR_ROCKET;
		else if (e->render.effects & EF_GRENADE)
		{
			// LadyHavoc: e->render.alpha == -1 is for Nehahra dem compatibility (cigar smoke)
			trailtype = e->render.alpha == -1 ? EFFECT_TR_NEHAHRASMOKE : EFFECT_TR_GRENADE;
		}
		else if (e->render.effects & EF_TRACER3)
			trailtype = EFFECT_TR_VORESPIKE;
	}
	// do trails
	if (e->render.flags & RENDER_GLOWTRAIL)
		trailtype = EFFECT_TR_GLOWTRAIL;
	if (e->state_current.traileffectnum)
		trailtype = (effectnameindex_t)e->state_current.traileffectnum;
	// check if a trail is allowed (it is not after a teleport for example)
	if (trailtype && e->persistent.trail_allowed)
	{
		float len;
		vec3_t vel;
		VectorSubtract(e->state_current.origin, e->state_previous.origin, vel);
		len = e->state_current.time - e->state_previous.time;
		if (len > 0)
			len = 1.0f / len;
		VectorScale(vel, len, vel);
		// pass time as count so that trails that are time based (such as an emitter) will emit properly as long as they don't use trailspacing
		CL_ParticleTrail(trailtype, bound(0, cl.time - cl.oldtime, 0.1), e->persistent.trail_origin, origin, vel, vel, e, e->state_current.glowcolor, false, true, NULL, NULL, 1);
	}
	// now that the entity has survived one trail update it is allowed to
	// leave a real trail on later frames
	e->persistent.trail_allowed = true;
	VectorCopy(origin, e->persistent.trail_origin);
}


/*
===============
CL_UpdateViewEntities
===============
*/
void CL_UpdateViewEntities(void)
{
	int i;
	// update any RENDER_VIEWMODEL entities to use the new view matrix
	for (i = 1;i < cl.num_entities;i++)
	{
		if (cl.entities_active[i])
		{
			entity_t *ent = cl.entities + i;
			if ((ent->render.flags & RENDER_VIEWMODEL) || ent->state_current.tagentity)
				CL_UpdateNetworkEntity(ent, 32, true);
		}
	}
	// and of course the engine viewmodel needs updating as well
	CL_UpdateNetworkEntity(&cl.viewent, 32, true);
}

/*
===============
CL_UpdateNetworkCollisionEntities
===============
*/
static void CL_UpdateNetworkCollisionEntities(void)
{
	entity_t *ent;
	int i;

	// start on the entity after the world
	cl.num_brushmodel_entities = 0;
	for (i = cl.maxclients + 1;i < cl.num_entities;i++)
	{
		if (cl.entities_active[i])
		{
			ent = cl.entities + i;
			if (ent->state_current.active && ent->render.model && ent->render.model->name[0] == '*' && ent->render.model->TraceBox)
			{
				// do not interpolate the bmodels for this
				CL_UpdateNetworkEntity(ent, 32, false);
				cl.brushmodel_entities[cl.num_brushmodel_entities++] = i;
			}
		}
	}
}

/*
===============
CL_UpdateNetworkEntities
===============
*/
static void CL_UpdateNetworkEntities(void)
{
	entity_t *ent;
	int i;

	// start on the entity after the world
	for (i = 1;i < cl.num_entities;i++)
	{
		if (cl.entities_active[i])
		{
			ent = cl.entities + i;
			if (ent->state_current.active)
			{
				CL_UpdateNetworkEntity(ent, 32, true);
				// view models should never create light/trails
				if (!(ent->render.flags & RENDER_VIEWMODEL))
					CL_UpdateNetworkEntityTrail(ent);
			}
			else
			{
				R_DecalSystem_Reset(&ent->render.decalsystem);
				cl.entities_active[i] = false;
			}
		}
	}
}

static void CL_UpdateViewModel(void)
{
	entity_t *ent;
	ent = &cl.viewent;
	ent->state_previous = ent->state_current;
	ent->state_current = defaultstate;
	ent->state_current.time = cl.time;
	ent->state_current.number = (unsigned short)-1;
	ent->state_current.active = true;
	ent->state_current.modelindex = cl.stats[STAT_WEAPON];
	ent->state_current.frame = cl.stats[STAT_WEAPONFRAME];
	ent->state_current.flags = RENDER_VIEWMODEL;
	if ((cl.stats[STAT_HEALTH] <= 0 && cl_deathnoviewmodel.integer) || cl.intermission)
		ent->state_current.modelindex = 0;
	else if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
	{
		if (gamemode == GAME_TRANSFUSION)
			ent->state_current.alpha = 128;
		else
			ent->state_current.modelindex = 0;
	}
	ent->state_current.alpha = cl.entities[cl.viewentity].state_current.alpha;
	ent->state_current.effects = EF_NOSHADOW | (cl.entities[cl.viewentity].state_current.effects & (EF_ADDITIVE | EF_FULLBRIGHT | EF_NODEPTHTEST | EF_NOGUNBOB));

	// reset animation interpolation on weaponmodel if model changed
	if (ent->state_previous.modelindex != ent->state_current.modelindex)
	{
		ent->render.framegroupblend[0].frame = ent->render.framegroupblend[1].frame = ent->state_current.frame;
		ent->render.framegroupblend[0].start = ent->render.framegroupblend[1].start = cl.time;
		ent->render.framegroupblend[0].lerp = 1;ent->render.framegroupblend[1].lerp = 0;
	}
	CL_UpdateNetworkEntity(ent, 32, true);
}

// note this is a recursive function, but it can never get in a runaway loop (because of the delayedlink flags)
static void CL_LinkNetworkEntity(entity_t *e)
{
	effectnameindex_t trailtype;
	vec3_t origin;
	vec3_t dlightcolor;
	vec_t dlightradius;
	char vabuf[1024];

	// skip inactive entities and world
	if (!e->state_current.active || e == cl.entities)
		return;
	if (e->state_current.tagentity)
	{
		// if the tag entity is currently impossible, skip it
		if (e->state_current.tagentity >= cl.num_entities)
			return;
		// if the tag entity is inactive, skip it
		if (!cl.entities[e->state_current.tagentity].state_current.active)
		{
			if(!cl.csqc_server2csqcentitynumber[e->state_current.tagentity])
				return;
			if(!cl.csqcrenderentities[cl.csqc_server2csqcentitynumber[e->state_current.tagentity]].entitynumber)
				return;
			// if we get here, it's properly csqc networked and attached
		}
	}

	// create entity dlights associated with this entity
	if (e->render.model && e->render.model->soundfromcenter)
	{
		// bmodels are treated specially since their origin is usually '0 0 0'
		vec3_t o;
		VectorMAM(0.5f, e->render.model->normalmins, 0.5f, e->render.model->normalmaxs, o);
		Matrix4x4_Transform(&e->render.matrix, o, origin);
	}
	else
		Matrix4x4_OriginFromMatrix(&e->render.matrix, origin);
	trailtype = EFFECT_NONE;
	dlightradius = 0;
	dlightcolor[0] = 0;
	dlightcolor[1] = 0;
	dlightcolor[2] = 0;
	// LadyHavoc: if the entity has no effects, don't check each
	if (e->render.effects & (EF_BRIGHTFIELD | EF_DIMLIGHT | EF_BRIGHTLIGHT | EF_RED | EF_BLUE | EF_FLAME | EF_STARDUST))
	{
		if (e->render.effects & EF_BRIGHTFIELD)
		{
			if (IS_NEXUIZ_DERIVED(gamemode))
				trailtype = EFFECT_TR_NEXUIZPLASMA;
		}
		if (e->render.effects & EF_DIMLIGHT)
		{
			dlightradius = max(dlightradius, 200);
			dlightcolor[0] += 1.50f;
			dlightcolor[1] += 1.50f;
			dlightcolor[2] += 1.50f;
		}
		if (e->render.effects & EF_BRIGHTLIGHT)
		{
			dlightradius = max(dlightradius, 400);
			dlightcolor[0] += 3.00f;
			dlightcolor[1] += 3.00f;
			dlightcolor[2] += 3.00f;
		}
		// LadyHavoc: more effects
		if (e->render.effects & EF_RED) // red
		{
			dlightradius = max(dlightradius, 200);
			dlightcolor[0] += 1.50f;
			dlightcolor[1] += 0.15f;
			dlightcolor[2] += 0.15f;
		}
		if (e->render.effects & EF_BLUE) // blue
		{
			dlightradius = max(dlightradius, 200);
			dlightcolor[0] += 0.15f;
			dlightcolor[1] += 0.15f;
			dlightcolor[2] += 1.50f;
		}
		if (e->render.effects & EF_FLAME)
			CL_ParticleTrail(EFFECT_EF_FLAME, 1, origin, origin, vec3_origin, vec3_origin, NULL, 0, true, false, NULL, NULL, 1);
		if (e->render.effects & EF_STARDUST)
			CL_ParticleTrail(EFFECT_EF_STARDUST, 1, origin, origin, vec3_origin, vec3_origin, NULL, 0, true, false, NULL, NULL, 1);
	}
	// muzzleflash fades over time, and is offset a bit
	if (e->persistent.muzzleflash > 0 && r_refdef.scene.numlights < MAX_DLIGHTS)
	{
		vec3_t v2;
		vec3_t color;
		trace_t trace;
		matrix4x4_t tempmatrix;
		Matrix4x4_Transform(&e->render.matrix, muzzleflashorigin, v2);
		trace = CL_TraceLine(origin, v2, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID | SUPERCONTENTS_SKY, 0, 0, collision_extendmovelength.value, true, false, NULL, false, false);
		Matrix4x4_Normalize(&tempmatrix, &e->render.matrix);
		Matrix4x4_SetOrigin(&tempmatrix, trace.endpos[0], trace.endpos[1], trace.endpos[2]);
		Matrix4x4_Scale(&tempmatrix, 150, 1);
		VectorSet(color, e->persistent.muzzleflash * 4.0f, e->persistent.muzzleflash * 4.0f, e->persistent.muzzleflash * 4.0f);
		R_RTLight_Update(&r_refdef.scene.templights[r_refdef.scene.numlights], false, &tempmatrix, color, -1, NULL, true, 0, 0.25, 0, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
		r_refdef.scene.lights[r_refdef.scene.numlights] = &r_refdef.scene.templights[r_refdef.scene.numlights];r_refdef.scene.numlights++;
	}
	// LadyHavoc: if the model has no flags, don't check each
	if (e->render.model && e->render.effects && !(e->render.flags & RENDER_VIEWMODEL))
	{
		if (e->render.effects & EF_GIB)
			trailtype = EFFECT_TR_BLOOD;
		else if (e->render.effects & EF_ZOMGIB)
			trailtype = EFFECT_TR_SLIGHTBLOOD;
		else if (e->render.effects & EF_TRACER)
			trailtype = EFFECT_TR_WIZSPIKE;
		else if (e->render.effects & EF_TRACER2)
			trailtype = EFFECT_TR_KNIGHTSPIKE;
		else if (e->render.effects & EF_ROCKET)
			trailtype = EFFECT_TR_ROCKET;
		else if (e->render.effects & EF_GRENADE)
		{
			// LadyHavoc: e->render.alpha == -1 is for Nehahra dem compatibility (cigar smoke)
			trailtype = e->render.alpha == -1 ? EFFECT_TR_NEHAHRASMOKE : EFFECT_TR_GRENADE;
		}
		else if (e->render.effects & EF_TRACER3)
			trailtype = EFFECT_TR_VORESPIKE;
	}
	// LadyHavoc: customizable glow
	if (e->state_current.glowsize)
	{
		// * 4 for the expansion from 0-255 to 0-1023 range,
		// / 255 to scale down byte colors
		dlightradius = max(dlightradius, e->state_current.glowsize * 4);
		VectorMA(dlightcolor, (1.0f / 255.0f), palette_rgb[e->state_current.glowcolor], dlightcolor);
	}
	// custom rtlight
	if ((e->state_current.lightpflags & PFLAGS_FULLDYNAMIC) && r_refdef.scene.numlights < MAX_DLIGHTS)
	{
		matrix4x4_t dlightmatrix;
		vec4_t light;
		VectorScale(e->state_current.light, (1.0f / 256.0f), light);
		light[3] = e->state_current.light[3];
		if (light[0] == 0 && light[1] == 0 && light[2] == 0)
			VectorSet(light, 1, 1, 1);
		if (light[3] == 0)
			light[3] = 350;
		// FIXME: add ambient/diffuse/specular scales as an extension ontop of TENEBRAE_GFX_DLIGHTS?
		Matrix4x4_Normalize(&dlightmatrix, &e->render.matrix);
		Matrix4x4_Scale(&dlightmatrix, light[3], 1);
		R_RTLight_Update(&r_refdef.scene.templights[r_refdef.scene.numlights], false, &dlightmatrix, light, e->state_current.lightstyle, e->state_current.skin > 0 ? va(vabuf, sizeof(vabuf), "cubemaps/%i", e->state_current.skin) : NULL, !(e->state_current.lightpflags & PFLAGS_NOSHADOW), (e->state_current.lightpflags & PFLAGS_CORONA) != 0, 0.25, 0, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
		r_refdef.scene.lights[r_refdef.scene.numlights] = &r_refdef.scene.templights[r_refdef.scene.numlights];r_refdef.scene.numlights++;
	}
	// make the glow dlight
	else if (dlightradius > 0 && (dlightcolor[0] || dlightcolor[1] || dlightcolor[2]) && !(e->render.flags & RENDER_VIEWMODEL) && r_refdef.scene.numlights < MAX_DLIGHTS)
	{
		matrix4x4_t dlightmatrix;
		Matrix4x4_Normalize(&dlightmatrix, &e->render.matrix);
		// hack to make glowing player light shine on their gun
		//if (e->state_current.number == cl.viewentity/* && !chase_active.integer*/)
		//	Matrix4x4_AdjustOrigin(&dlightmatrix, 0, 0, 30);
		Matrix4x4_Scale(&dlightmatrix, dlightradius, 1);
		R_RTLight_Update(&r_refdef.scene.templights[r_refdef.scene.numlights], false, &dlightmatrix, dlightcolor, -1, NULL, true, 1, 0.25, 0, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
		r_refdef.scene.lights[r_refdef.scene.numlights] = &r_refdef.scene.templights[r_refdef.scene.numlights];r_refdef.scene.numlights++;
	}
	// do trail light
	if (e->render.flags & RENDER_GLOWTRAIL)
		trailtype = EFFECT_TR_GLOWTRAIL;
	if (e->state_current.traileffectnum)
		trailtype = (effectnameindex_t)e->state_current.traileffectnum;
	if (trailtype)
		CL_ParticleTrail(trailtype, 1, origin, origin, vec3_origin, vec3_origin, NULL, e->state_current.glowcolor, true, false, NULL, NULL, 1);

	// don't show entities with no modelindex (note: this still shows
	// entities which have a modelindex that resolved to a NULL model)
	if (e->render.model && !(e->render.effects & EF_NODRAW) && r_refdef.scene.numentities < r_refdef.scene.maxentities)
		r_refdef.scene.entities[r_refdef.scene.numentities++] = &e->render;
	//if (cl.viewentity && e->state_current.number == cl.viewentity)
	//	Matrix4x4_Print(&e->render.matrix);
}

static void CL_RelinkWorld(void)
{
	entity_t *ent = &cl.entities[0];
	// FIXME: this should be done at load
	ent->render.matrix = identitymatrix;
	ent->render.flags = RENDER_SHADOW;
	if (!r_fullbright.integer)
		ent->render.flags |= RENDER_LIGHT;
	VectorSet(ent->render.colormod, 1, 1, 1);
	VectorSet(ent->render.glowmod, 1, 1, 1);
	ent->render.allowdecals = true;
	CL_UpdateRenderEntity(&ent->render);
	r_refdef.scene.worldentity = &ent->render;
	r_refdef.scene.worldmodel = cl.worldmodel;

	// if the world is q2bsp, animate the textures
	if (ent->render.model && ent->render.model->brush.isq2bsp)
		ent->render.framegroupblend[0].frame = (int)(cl.time * 2.0f);
}

static void CL_RelinkStaticEntities(void)
{
	int i;
	entity_t *e;
	for (i = 0, e = cl.static_entities;i < cl.num_static_entities && r_refdef.scene.numentities < r_refdef.scene.maxentities;i++, e++)
	{
		e->render.flags = 0;
		// if the model was not loaded when the static entity was created we
		// need to re-fetch the model pointer
		e->render.model = CL_GetModelByIndex(e->state_baseline.modelindex);
		// either fullbright or lit
		if(!r_fullbright.integer)
		{
			if (!(e->render.effects & EF_FULLBRIGHT))
				e->render.flags |= RENDER_LIGHT;
		}
		// hide player shadow during intermission or nehahra movie
		if (!(e->render.effects & (EF_NOSHADOW | EF_ADDITIVE | EF_NODEPTHTEST)) && (e->render.alpha >= 1))
			e->render.flags |= RENDER_SHADOW;
		VectorSet(e->render.colormod, 1, 1, 1);
		VectorSet(e->render.glowmod, 1, 1, 1);
		VM_FrameBlendFromFrameGroupBlend(e->render.frameblend, e->render.framegroupblend, e->render.model, cl.time);
		e->render.allowdecals = true;
		CL_UpdateRenderEntity(&e->render);
		r_refdef.scene.entities[r_refdef.scene.numentities++] = &e->render;
	}
}

/*
===============
CL_RelinkEntities
===============
*/
static void CL_RelinkNetworkEntities(void)
{
	entity_t *ent;
	int i;

	// start on the entity after the world
	for (i = 1;i < cl.num_entities;i++)
	{
		if (cl.entities_active[i])
		{
			ent = cl.entities + i;
			if (ent->state_current.active)
				CL_LinkNetworkEntity(ent);
			else
				cl.entities_active[i] = false;
		}
	}
}

static void CL_RelinkEffects(void)
{
	int i, intframe;
	cl_effect_t *e;
	entity_render_t *entrender;
	float frame;

	for (i = 0, e = cl.effects;i < cl.num_effects;i++, e++)
	{
		if (e->active)
		{
			frame = (cl.time - e->starttime) * e->framerate + e->startframe;
			intframe = (int)frame;
			if (intframe < 0 || intframe >= e->endframe)
			{
				memset(e, 0, sizeof(*e));
				while (cl.num_effects > 0 && !cl.effects[cl.num_effects - 1].active)
					cl.num_effects--;
				continue;
			}

			if (intframe != e->frame)
			{
				e->frame = intframe;
				e->frame1time = e->frame2time;
				e->frame2time = cl.time;
			}

			// if we're drawing effects, get a new temp entity
			// (NewTempEntity adds it to the render entities list for us)
			if (r_draweffects.integer && (entrender = CL_NewTempEntity(e->starttime)))
			{
				// interpolation stuff
				entrender->framegroupblend[0].frame = intframe;
				entrender->framegroupblend[0].lerp = 1 - frame - intframe;
				entrender->framegroupblend[0].start = e->frame1time;
				if (intframe + 1 >= e->endframe)
				{
					entrender->framegroupblend[1].frame = 0; // disappear
					entrender->framegroupblend[1].lerp = 0;
					entrender->framegroupblend[1].start = 0;
				}
				else
				{
					entrender->framegroupblend[1].frame = intframe + 1;
					entrender->framegroupblend[1].lerp = frame - intframe;
					entrender->framegroupblend[1].start = e->frame2time;
				}

				// normal stuff
				entrender->model = e->model;
				entrender->alpha = 1;
				VectorSet(entrender->colormod, 1, 1, 1);
				VectorSet(entrender->glowmod, 1, 1, 1);

				Matrix4x4_CreateFromQuakeEntity(&entrender->matrix, e->origin[0], e->origin[1], e->origin[2], 0, 0, 0, 1);
				CL_UpdateRenderEntity(entrender);
			}
		}
	}
}

void CL_Beam_CalculatePositions(const beam_t *b, vec3_t start, vec3_t end)
{
	VectorCopy(b->start, start);
	VectorCopy(b->end, end);

	// if coming from the player, update the start position
	if (b->entity == cl.viewentity)
	{
		if (cl_beams_quakepositionhack.integer && !chase_active.integer)
		{
			// LadyHavoc: this is a stupid hack from Quake that makes your
			// lightning appear to come from your waist and cover less of your
			// view
			// in Quake this hack was applied to all players (causing the
			// infamous crotch-lightning), but in darkplaces and QuakeWorld it
			// only applies to your own lightning, and only in first person
			Matrix4x4_OriginFromMatrix(&cl.entities[cl.viewentity].render.matrix, start);
		}
		if (cl_beams_instantaimhack.integer)
		{
			vec3_t dir, localend;
			vec_t len;
			// LadyHavoc: this updates the beam direction to match your
			// viewangles
			VectorSubtract(end, start, dir);
			len = VectorLength(dir);
			VectorNormalize(dir);
			VectorSet(localend, len, 0, 0);
			Matrix4x4_Transform(&r_refdef.view.matrix, localend, end);
		}
	}
}

void CL_RelinkBeams(void)
{
	int i;
	beam_t *b;
	vec3_t dist, org, start, end;
	float d;
	entity_render_t *entrender;
	double yaw, pitch;
	float forward;
	matrix4x4_t tempmatrix;

	for (i = 0, b = cl.beams;i < cl.num_beams;i++, b++)
	{
		if (!b->model)
			continue;
		if (b->endtime < cl.time)
		{
			b->model = NULL;
			continue;
		}

		CL_Beam_CalculatePositions(b, start, end);

		if (b->lightning)
		{
			if (cl_beams_lightatend.integer && r_refdef.scene.numlights < MAX_DLIGHTS)
			{
				// FIXME: create a matrix from the beam start/end orientation
				vec3_t dlightcolor;
				VectorSet(dlightcolor, 0.3, 0.7, 1);
				Matrix4x4_CreateFromQuakeEntity(&tempmatrix, end[0], end[1], end[2], 0, 0, 0, 200);
				R_RTLight_Update(&r_refdef.scene.templights[r_refdef.scene.numlights], false, &tempmatrix, dlightcolor, -1, NULL, true, 1, 0.25, 1, 0, 0, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
				r_refdef.scene.lights[r_refdef.scene.numlights] = &r_refdef.scene.templights[r_refdef.scene.numlights];r_refdef.scene.numlights++;
			}
			if (cl_beams_polygons.integer)
			{
				CL_Beam_AddPolygons(b);
				continue;
			}
		}

		// calculate pitch and yaw
		// (this is similar to the QuakeC builtin function vectoangles)
		VectorSubtract(end, start, dist);
		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = atan2(dist[1], dist[0]) * 180 / M_PI;
			if (yaw < 0)
				yaw += 360;

			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = atan2(dist[2], forward) * 180 / M_PI;
			if (pitch < 0)
				pitch += 360;
		}

		// add new entities for the lightning
		VectorCopy (start, org);
		d = VectorNormalizeLength(dist);
		while (d > 0)
		{
			entrender = CL_NewTempEntity (0);
			if (!entrender)
				return;
			entrender->model = b->model;
			Matrix4x4_CreateFromQuakeEntity(&entrender->matrix, org[0], org[1], org[2], -pitch, yaw, lhrandom(0, 360), 1);
			CL_UpdateRenderEntity(entrender);
			VectorMA(org, 30, dist, org);
			d -= 30;
		}
	}

	while (cl.num_beams > 0 && !cl.beams[cl.num_beams - 1].model)
		cl.num_beams--;
}

static void CL_RelinkQWNails(void)
{
	int i;
	vec_t *v;
	entity_render_t *entrender;

	for (i = 0;i < cl.qw_num_nails;i++)
	{
		v = cl.qw_nails[i];

		// if we're drawing effects, get a new temp entity
		// (NewTempEntity adds it to the render entities list for us)
		if (!(entrender = CL_NewTempEntity(0)))
			continue;

		// normal stuff
		entrender->model = CL_GetModelByIndex(cl.qw_modelindex_spike);
		entrender->alpha = 1;
		VectorSet(entrender->colormod, 1, 1, 1);
		VectorSet(entrender->glowmod, 1, 1, 1);

		Matrix4x4_CreateFromQuakeEntity(&entrender->matrix, v[0], v[1], v[2], v[3], v[4], v[5], 1);
		CL_UpdateRenderEntity(entrender);
	}
}

static void CL_LerpPlayer(float frac)
{
	int i;

	cl.viewzoom = cl.mviewzoom[1] + frac * (cl.mviewzoom[0] - cl.mviewzoom[1]);
	for (i = 0;i < 3;i++)
	{
		cl.punchangle[i] = cl.mpunchangle[1][i] + frac * (cl.mpunchangle[0][i] - cl.mpunchangle[1][i]);
		cl.punchvector[i] = cl.mpunchvector[1][i] + frac * (cl.mpunchvector[0][i] - cl.mpunchvector[1][i]);
		cl.velocity[i] = cl.mvelocity[1][i] + frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);
	}

	// interpolate the angles if playing a demo or spectating someone
	if (cls.demoplayback || cl.fixangle[0])
	{
		for (i = 0;i < 3;i++)
		{
			float d = cl.mviewangles[0][i] - cl.mviewangles[1][i];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
			cl.viewangles[i] = cl.mviewangles[1][i] + frac * d;
		}
	}
}

void CSQC_RelinkAllEntities (int drawmask)
{
	// link stuff
	CL_RelinkWorld();
	// the scene mesh is added first for easier debugging (consistent spot in render entities list)
	CL_MeshEntities_Scene_AddRenderEntity();
	CL_RelinkStaticEntities();
	CL_RelinkBeams();
	CL_RelinkEffects();
	CL_RelinkLightFlashes();

	// link stuff
	if (drawmask & ENTMASK_ENGINE)
	{
		CL_RelinkNetworkEntities();
		if (drawmask & ENTMASK_ENGINEVIEWMODELS)
			CL_LinkNetworkEntity(&cl.viewent); // link gun model
		CL_RelinkQWNails();
	}

	// update view blend
	V_CalcViewBlend();
}

/*
===============
CL_UpdateWorld

Update client game world for a new frame
===============
*/
void CL_UpdateWorld(void)
{
	r_refdef.scene.numentities = 0;
	r_refdef.scene.numlights = 0;
	r_refdef.view.matrix = identitymatrix;
	r_refdef.view.quality = 1;
		
	cl.num_brushmodel_entities = 0;

	if (cls.state == ca_connected && cls.signon == SIGNONS)
	{
		// prepare for a new frame
		CL_LerpPlayer(CL_LerpPoint());
		CL_DecayLightFlashes();
		CL_ClearTempEntities();
		V_DriftPitch();
		V_FadeViewFlashs();

		// if prediction is enabled we have to update all the collidable
		// network entities before the prediction code can be run
		CL_UpdateNetworkCollisionEntities();

		// now update the player prediction
		CL_ClientMovement_Replay();

		// update the player entity (which may be predicted)
		CL_UpdateNetworkEntity(cl.entities + cl.viewentity, 32, true);

		// now update the view (which depends on that player entity)
		V_CalcRefdef();

		// now update all the network entities and create particle trails
		// (some entities may depend on the view)
		CL_UpdateNetworkEntities();

		// update the engine-based viewmodel
		CL_UpdateViewModel();

		// when csqc is loaded, it will call this in CSQC_UpdateView
		if (!CLVM_prog->loaded || CLVM_prog->flag & PRVM_CSQC_SIMPLE)
		{
			// clear the CL_Mesh_Scene() used for some engine effects
			CL_MeshEntities_Scene_Clear();
			// add engine entities and effects
			CSQC_RelinkAllEntities(ENTMASK_ENGINE | ENTMASK_ENGINEVIEWMODELS);
		}

		// decals, particles, and explosions will be updated during rneder
	}

	r_refdef.scene.time = cl.time;
}

/*
======================
CL_Fog_f
======================
*/
static void CL_Fog_f(cmd_state_t *cmd)
{
	if (Cmd_Argc (cmd) == 1)
	{
		Con_Printf("\"fog\" is \"%f %f %f %f %f %f %f %f %f\"\n", r_refdef.fog_density, r_refdef.fog_red, r_refdef.fog_green, r_refdef.fog_blue, r_refdef.fog_alpha, r_refdef.fog_start, r_refdef.fog_end, r_refdef.fog_height, r_refdef.fog_fadedepth);
		return;
	}
	FOG_clear(); // so missing values get good defaults
	if(Cmd_Argc(cmd) > 1)
		r_refdef.fog_density = atof(Cmd_Argv(cmd, 1));
	if(Cmd_Argc(cmd) > 2)
		r_refdef.fog_red = atof(Cmd_Argv(cmd, 2));
	if(Cmd_Argc(cmd) > 3)
		r_refdef.fog_green = atof(Cmd_Argv(cmd, 3));
	if(Cmd_Argc(cmd) > 4)
		r_refdef.fog_blue = atof(Cmd_Argv(cmd, 4));
	if(Cmd_Argc(cmd) > 5)
		r_refdef.fog_alpha = atof(Cmd_Argv(cmd, 5));
	if(Cmd_Argc(cmd) > 6)
		r_refdef.fog_start = atof(Cmd_Argv(cmd, 6));
	if(Cmd_Argc(cmd) > 7)
		r_refdef.fog_end = atof(Cmd_Argv(cmd, 7));
	if(Cmd_Argc(cmd) > 8)
		r_refdef.fog_height = atof(Cmd_Argv(cmd, 8));
	if(Cmd_Argc(cmd) > 9)
		r_refdef.fog_fadedepth = atof(Cmd_Argv(cmd, 9));
}

/*
======================
CL_FogHeightTexture_f
======================
*/
static void CL_Fog_HeightTexture_f(cmd_state_t *cmd)
{
	if (Cmd_Argc (cmd) < 11)
	{
		Con_Printf("\"fog_heighttexture\" is \"%f %f %f %f %f %f %f %f %f %s\"\n", r_refdef.fog_density, r_refdef.fog_red, r_refdef.fog_green, r_refdef.fog_blue, r_refdef.fog_alpha, r_refdef.fog_start, r_refdef.fog_end, r_refdef.fog_height, r_refdef.fog_fadedepth, r_refdef.fog_height_texturename);
		return;
	}
	FOG_clear(); // so missing values get good defaults
	r_refdef.fog_density = atof(Cmd_Argv(cmd, 1));
	r_refdef.fog_red = atof(Cmd_Argv(cmd, 2));
	r_refdef.fog_green = atof(Cmd_Argv(cmd, 3));
	r_refdef.fog_blue = atof(Cmd_Argv(cmd, 4));
	r_refdef.fog_alpha = atof(Cmd_Argv(cmd, 5));
	r_refdef.fog_start = atof(Cmd_Argv(cmd, 6));
	r_refdef.fog_end = atof(Cmd_Argv(cmd, 7));
	r_refdef.fog_height = atof(Cmd_Argv(cmd, 8));
	r_refdef.fog_fadedepth = atof(Cmd_Argv(cmd, 9));
	dp_strlcpy(r_refdef.fog_height_texturename, Cmd_Argv(cmd, 10), sizeof(r_refdef.fog_height_texturename));
}


/*
====================
CL_TimeRefresh_f

For program optimization
====================
*/
static void CL_TimeRefresh_f(cmd_state_t *cmd)
{
	int i;
	double timestart, timedelta;

	timestart = Sys_DirtyTime();
	for (i = 0;i < 128;i++)
	{
		Matrix4x4_CreateFromQuakeEntity(&r_refdef.view.matrix, r_refdef.view.origin[0], r_refdef.view.origin[1], r_refdef.view.origin[2], 0, i / 128.0 * 360.0, 0, 1);
		r_refdef.view.quality = 1;
		CL_UpdateScreen();
	}
	timedelta = Sys_DirtyTime() - timestart;

	Con_Printf("%f seconds (%f fps)\n", timedelta, 128/timedelta);
}

static void CL_AreaStats_f(cmd_state_t *cmd)
{
	World_PrintAreaStats(&cl.world, "client");
}

cl_locnode_t *CL_Locs_FindNearest(const vec3_t point)
{
	int i;
	cl_locnode_t *loc;
	cl_locnode_t *best;
	vec3_t nearestpoint;
	vec_t dist, bestdist;
	best = NULL;
	bestdist = 0;
	for (loc = cl.locnodes;loc;loc = loc->next)
	{
		for (i = 0;i < 3;i++)
			nearestpoint[i] = bound(loc->mins[i], point[i], loc->maxs[i]);
		dist = VectorDistance2(nearestpoint, point);
		if (bestdist > dist || !best)
		{
			bestdist = dist;
			best = loc;
			if (bestdist < 1)
				break;
		}
	}
	return best;
}

void CL_Locs_FindLocationName(char *buffer, size_t buffersize, vec3_t point)
{
	cl_locnode_t *loc;
	loc = CL_Locs_FindNearest(point);
	if (loc)
		dp_strlcpy(buffer, loc->name, buffersize);
	else
		dpsnprintf(buffer, buffersize, "LOC=%.0f:%.0f:%.0f", point[0], point[1], point[2]);
}

static void CL_Locs_FreeNode(cl_locnode_t *node)
{
	cl_locnode_t **pointer, **next;
	for (pointer = &cl.locnodes;*pointer;pointer = next)
	{
		next = &(*pointer)->next;
		if (*pointer == node)
		{
			*pointer = node->next;
			Mem_Free(node);
			return;
		}
	}
	Con_Printf("CL_Locs_FreeNode: no such node! (%p)\n", (void *)node);
}

static void CL_Locs_AddNode(vec3_t mins, vec3_t maxs, const char *name)
{
	cl_locnode_t *node, **pointer;
	int namelen;
	if (!name)
		name = "";
	namelen = (int)strlen(name);
	node = (cl_locnode_t *) Mem_Alloc(cls.levelmempool, sizeof(cl_locnode_t) + namelen + 1);
	VectorSet(node->mins, min(mins[0], maxs[0]), min(mins[1], maxs[1]), min(mins[2], maxs[2]));
	VectorSet(node->maxs, max(mins[0], maxs[0]), max(mins[1], maxs[1]), max(mins[2], maxs[2]));
	node->name = (char *)(node + 1);
	memcpy(node->name, name, namelen);
	node->name[namelen] = 0;
	// link it into the tail of the list to preserve the order
	for (pointer = &cl.locnodes;*pointer;pointer = &(*pointer)->next)
		;
	*pointer = node;
}

static void CL_Locs_Add_f(cmd_state_t *cmd)
{
	vec3_t mins, maxs;
	if (Cmd_Argc(cmd) != 5 && Cmd_Argc(cmd) != 8)
	{
		Con_Printf("usage: %s x y z[ x y z] name\n", Cmd_Argv(cmd, 0));
		return;
	}
	mins[0] = atof(Cmd_Argv(cmd, 1));
	mins[1] = atof(Cmd_Argv(cmd, 2));
	mins[2] = atof(Cmd_Argv(cmd, 3));
	if (Cmd_Argc(cmd) == 8)
	{
		maxs[0] = atof(Cmd_Argv(cmd, 4));
		maxs[1] = atof(Cmd_Argv(cmd, 5));
		maxs[2] = atof(Cmd_Argv(cmd, 6));
		CL_Locs_AddNode(mins, maxs, Cmd_Argv(cmd, 7));
	}
	else
		CL_Locs_AddNode(mins, mins, Cmd_Argv(cmd, 4));
}

static void CL_Locs_RemoveNearest_f(cmd_state_t *cmd)
{
	cl_locnode_t *loc;
	loc = CL_Locs_FindNearest(r_refdef.view.origin);
	if (loc)
		CL_Locs_FreeNode(loc);
	else
		Con_Printf("no loc point or box found for your location\n");
}

static void CL_Locs_Clear_f(cmd_state_t *cmd)
{
	while (cl.locnodes)
		CL_Locs_FreeNode(cl.locnodes);
}

static void CL_Locs_Save_f(cmd_state_t *cmd)
{
	cl_locnode_t *loc;
	qfile_t *outfile;
	char locfilename[MAX_QPATH];
	char timestring[128];

	if (!cl.locnodes)
	{
		Con_Printf("No loc points/boxes exist!\n");
		return;
	}
	if (cls.state != ca_connected || !cl.worldmodel)
	{
		Con_Printf("No level loaded!\n");
		return;
	}
	dpsnprintf(locfilename, sizeof(locfilename), "%s.loc", cl.worldnamenoextension);

	outfile = FS_OpenRealFile(locfilename, "w", false);
	if (!outfile)
		return;
	// if any boxes are used then this is a proquake-format loc file, which
	// allows comments, so add some relevant information at the start
	for (loc = cl.locnodes;loc;loc = loc->next)
		if (!VectorCompare(loc->mins, loc->maxs))
			break;
	if (loc)
	{
		Sys_TimeString(timestring, sizeof(timestring), "%Y-%m-%d");
		FS_Printf(outfile, "// %s %s saved by %s\n// x,y,z,x,y,z,\"name\"\n\n", locfilename, timestring, engineversion);
		for (loc = cl.locnodes;loc;loc = loc->next)
			if (VectorCompare(loc->mins, loc->maxs))
				break;
		if (loc)
			Con_Printf(CON_WARN "Warning: writing loc file containing a mixture of qizmo-style points and proquake-style boxes may not work in qizmo or proquake!\n");
	}
	for (loc = cl.locnodes;loc;loc = loc->next)
	{
		if (VectorCompare(loc->mins, loc->maxs))
		{
			int len;
			const char *s;
			const char *in = loc->name;
			char name[MAX_INPUTLINE];
			for (len = 0;len < (int)sizeof(name) - 1 && *in;)
			{
				if (*in == ' ') {s = "$loc_name_separator";in++;}
				else if (!strncmp(in, "SSG", 3)) {s = "$loc_name_ssg";in += 3;}
				else if (!strncmp(in, "NG", 2)) {s = "$loc_name_ng";in += 2;}
				else if (!strncmp(in, "SNG", 3)) {s = "$loc_name_sng";in += 3;}
				else if (!strncmp(in, "GL", 2)) {s = "$loc_name_gl";in += 2;}
				else if (!strncmp(in, "RL", 2)) {s = "$loc_name_rl";in += 2;}
				else if (!strncmp(in, "LG", 2)) {s = "$loc_name_lg";in += 2;}
				else if (!strncmp(in, "GA", 2)) {s = "$loc_name_ga";in += 2;}
				else if (!strncmp(in, "YA", 2)) {s = "$loc_name_ya";in += 2;}
				else if (!strncmp(in, "RA", 2)) {s = "$loc_name_ra";in += 2;}
				else if (!strncmp(in, "MEGA", 4)) {s = "$loc_name_mh";in += 4;}
				else s = NULL;
				if (s)
				{
					while (len < (int)sizeof(name) - 1 && *s)
						name[len++] = *s++;
					continue;
				}
				name[len++] = *in++;
			}
			name[len] = 0;
			FS_Printf(outfile, "%.0f %.0f %.0f %s\n", loc->mins[0]*8, loc->mins[1]*8, loc->mins[2]*8, name);
		}
		else
			FS_Printf(outfile, "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,\"%s\"\n", loc->mins[0], loc->mins[1], loc->mins[2], loc->maxs[0], loc->maxs[1], loc->maxs[2], loc->name);
	}
	FS_Close(outfile);
}

void CL_Locs_Reload_f(cmd_state_t *cmd)
{
	int i, linenumber, limit, len;
	const char *s;
	char *filedata, *text, *textend, *linestart, *linetext, *lineend;
	fs_offset_t filesize;
	vec3_t mins, maxs;
	char locfilename[MAX_QPATH];
	char name[MAX_INPUTLINE];

	if (cls.state != ca_connected || !cl.worldmodel)
	{
		Con_Printf("No level loaded!\n");
		return;
	}

	CL_Locs_Clear_f(cmd);

	// try maps/something.loc first (LadyHavoc: where I think they should be)
	dpsnprintf(locfilename, sizeof(locfilename), "%s.loc", cl.worldnamenoextension);
	filedata = (char *)FS_LoadFile(locfilename, cls.levelmempool, false, &filesize);
	if (!filedata)
	{
		// try proquake name as well (LadyHavoc: I hate path mangling)
		dpsnprintf(locfilename, sizeof(locfilename), "locs/%s.loc", cl.worldbasename);
		filedata = (char *)FS_LoadFile(locfilename, cls.levelmempool, false, &filesize);
		if (!filedata)
			return;
	}
	text = filedata;
	textend = filedata + filesize;
	for (linenumber = 1;text < textend;linenumber++)
	{
		linestart = text;
		for (;text < textend && *text != '\r' && *text != '\n';text++)
			;
		lineend = text;
		if (text + 1 < textend && *text == '\r' && text[1] == '\n')
			text++;
		if (text < textend)
			text++;
		// trim trailing whitespace
		while (lineend > linestart && ISWHITESPACE(lineend[-1]))
			lineend--;
		// trim leading whitespace
		while (linestart < lineend && ISWHITESPACE(*linestart))
			linestart++;
		// check if this is a comment
		if (linestart + 2 <= lineend && !strncmp(linestart, "//", 2))
			continue;
		linetext = linestart;
		limit = 3;
		for (i = 0;i < limit;i++)
		{
			if (linetext >= lineend)
				break;
			// note: a missing number is interpreted as 0
			if (i < 3)
				mins[i] = atof(linetext);
			else
				maxs[i - 3] = atof(linetext);
			// now advance past the number
			while (linetext < lineend && !ISWHITESPACE(*linetext) && *linetext != ',')
				linetext++;
			// advance through whitespace
			if (linetext < lineend)
			{
				if (*linetext == ',')
				{
					linetext++;
					limit = 6;
					// note: comma can be followed by whitespace
				}
				if (ISWHITESPACE(*linetext))
				{
					// skip whitespace
					while (linetext < lineend && ISWHITESPACE(*linetext))
						linetext++;
				}
			}
		}
		// if this is a quoted name, remove the quotes
		if (i == 6)
		{
			if (linetext >= lineend || *linetext != '"')
				continue; // proquake location names are always quoted
			lineend--;
			linetext++;
			len = min(lineend - linetext, (int)sizeof(name) - 1);
			memcpy(name, linetext, len);
			name[len] = 0;
			// add the box to the list
			CL_Locs_AddNode(mins, maxs, name);
		}
		// if a point was parsed, it needs to be scaled down by 8 (since
		// point-based loc files were invented by a proxy which dealt
		// directly with quake protocol coordinates, which are *8), turn
		// it into a box
		else if (i == 3)
		{
			// interpret silly fuhquake macros
			for (len = 0;len < (int)sizeof(name) - 1 && linetext < lineend;)
			{
				if (*linetext == '$')
				{
					if (linetext + 18 <= lineend && !strncmp(linetext, "$loc_name_separator", 19)) {s = " ";linetext += 19;}
					else if (linetext + 13 <= lineend && !strncmp(linetext, "$loc_name_ssg", 13)) {s = "SSG";linetext += 13;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_ng", 12)) {s = "NG";linetext += 12;}
					else if (linetext + 13 <= lineend && !strncmp(linetext, "$loc_name_sng", 13)) {s = "SNG";linetext += 13;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_gl", 12)) {s = "GL";linetext += 12;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_rl", 12)) {s = "RL";linetext += 12;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_lg", 12)) {s = "LG";linetext += 12;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_ga", 12)) {s = "GA";linetext += 12;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_ya", 12)) {s = "YA";linetext += 12;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_ra", 12)) {s = "RA";linetext += 12;}
					else if (linetext + 12 <= lineend && !strncmp(linetext, "$loc_name_mh", 12)) {s = "MEGA";linetext += 12;}
					else s = NULL;
					if (s)
					{
						while (len < (int)sizeof(name) - 1 && *s)
							name[len++] = *s++;
						continue;
					}
				}
				name[len++] = *linetext++;
			}
			name[len] = 0;
			// add the point to the list
			VectorScale(mins, (1.0 / 8.0), mins);
			CL_Locs_AddNode(mins, mins, name);
		}
		else
			continue;
	}
}

entity_t cl_meshentities[NUM_MESHENTITIES];
model_t cl_meshentitymodels[NUM_MESHENTITIES];
const char *cl_meshentitynames[NUM_MESHENTITIES] =
{
	"MESH_SCENE",
	"MESH_UI",
};

static void CL_MeshEntities_Restart(void)
{
	int i;
	entity_t *ent;
	for (i = 0; i < NUM_MESHENTITIES; i++)
	{
		ent = cl_meshentities + i;
		Mod_Mesh_Destroy(ent->render.model);
		Mod_Mesh_Create(ent->render.model, cl_meshentitynames[i]);
	}
}

static void CL_MeshEntities_Start(void)
{
	int i;
	entity_t *ent;
	for(i = 0; i < NUM_MESHENTITIES; i++)
	{
		ent = cl_meshentities + i;
		Mod_Mesh_Create(ent->render.model, cl_meshentitynames[i]);
	}
}

static void CL_MeshEntities_Shutdown(void)
{
	int i;
	entity_t *ent;
	for(i = 0; i < NUM_MESHENTITIES; i++)
	{
		ent = cl_meshentities + i;
		Mod_Mesh_Destroy(ent->render.model);
	}
}

void CL_MeshEntities_Init(void)
{
	int i;
	entity_t *ent;
	for (i = 0; i < NUM_MESHENTITIES; i++)
	{
		ent = cl_meshentities + i;
		ent->state_current.active = true;
		ent->render.model = cl_meshentitymodels + i;
		Mod_Mesh_Create(ent->render.model, cl_meshentitynames[i]);	
		ent->render.alpha = 1;
		ent->render.flags = RENDER_SHADOW | RENDER_LIGHT;
		ent->render.framegroupblend[0].lerp = 1;
		ent->render.frameblend[0].lerp = 1;
		VectorSet(ent->render.colormod, 1, 1, 1);
		VectorSet(ent->render.glowmod, 1, 1, 1);
		VectorSet(ent->render.custommodellight_ambient, 1, 1, 1);
		VectorSet(ent->render.custommodellight_diffuse, 0, 0, 0);
		VectorSet(ent->render.custommodellight_lightdir, 0, 0, 1);
		VectorSet(ent->render.render_fullbright, 1, 1, 1);
		VectorSet(ent->render.render_glowmod, 0, 0, 0);
		VectorSet(ent->render.render_modellight_ambient, 1, 1, 1);
		VectorSet(ent->render.render_modellight_diffuse, 0, 0, 0);
		VectorSet(ent->render.render_modellight_specular, 0, 0, 0);
		VectorSet(ent->render.render_modellight_lightdir_world, 0, 0, 1);
		VectorSet(ent->render.render_modellight_lightdir_local, 0, 0, 1); // local doesn't matter because no diffuse/specular color
		VectorSet(ent->render.render_lightmap_ambient, 0, 0, 0);
		VectorSet(ent->render.render_lightmap_diffuse, 1, 1, 1);
		VectorSet(ent->render.render_lightmap_specular, 1, 1, 1);
		VectorSet(ent->render.render_rtlight_diffuse, 1, 1, 1);
		VectorSet(ent->render.render_rtlight_specular, 1, 1, 1);

		Matrix4x4_CreateIdentity(&ent->render.matrix);
		CL_UpdateRenderEntity(&ent->render);
	}
	cl_meshentities[MESH_UI].render.flags = RENDER_NOSELFSHADOW;
	R_RegisterModule("CL_MeshEntities", CL_MeshEntities_Start, CL_MeshEntities_Shutdown, CL_MeshEntities_Restart, CL_MeshEntities_Restart, CL_MeshEntities_Restart);
}

void CL_MeshEntities_Scene_Clear(void)
{
	Mod_Mesh_Reset(CL_Mesh_Scene());
}

void CL_MeshEntities_Scene_AddRenderEntity(void)
{
	entity_t* ent = &cl_meshentities[MESH_SCENE];
	r_refdef.scene.entities[r_refdef.scene.numentities++] = &ent->render;
}

void CL_MeshEntities_Scene_FinalizeRenderEntity(void)
{
	entity_t *ent = &cl_meshentities[MESH_SCENE];
	Mod_Mesh_Finalize(ent->render.model);
	VectorCopy(ent->render.model->normalmins, ent->render.mins);
	VectorCopy(ent->render.model->normalmaxs, ent->render.maxs);
}

extern cvar_t r_overheadsprites_pushback;
extern cvar_t r_fullbright_directed_pitch_relative;
extern cvar_t r_fullbright_directed_pitch;
extern cvar_t r_fullbright_directed_ambient;
extern cvar_t r_fullbright_directed_diffuse;
extern cvar_t r_fullbright_directed;
extern cvar_t r_hdr_glowintensity;

static void CL_UpdateEntityShading_GetDirectedFullbright(vec3_t ambient, vec3_t diffuse, vec3_t worldspacenormal)
{
	vec3_t angles;

	VectorSet(ambient, r_fullbright_directed_ambient.value, r_fullbright_directed_ambient.value, r_fullbright_directed_ambient.value);
	VectorSet(diffuse, r_fullbright_directed_diffuse.value, r_fullbright_directed_diffuse.value, r_fullbright_directed_diffuse.value);

	// Use cl.viewangles and not r_refdef.view.forward here so it is the
	// same for all stereo views, and to better handle pitches outside
	// [-90, 90] (in_pitch_* cvars allow that).
	VectorCopy(cl.viewangles, angles);
	if (r_fullbright_directed_pitch_relative.integer) {
		angles[PITCH] += r_fullbright_directed_pitch.value;
	}
	else {
		angles[PITCH] = r_fullbright_directed_pitch.value;
	}
	AngleVectors(angles, worldspacenormal, NULL, NULL);
	VectorNegate(worldspacenormal, worldspacenormal);
}

static void CL_UpdateEntityShading_Entity(entity_render_t *ent)
{
	float shadingorigin[3], a[3], c[3], dir[3];
	int q;

	for (q = 0; q < 3; q++)
		a[q] = c[q] = dir[q] = 0;

	ent->render_lightgrid = false;
	ent->render_modellight_forced = false;
	ent->render_rtlight_disabled = false;

	// pick an appropriate value for render_modellight_origin - if this is an
	// attachment we want to use the parent's render_modellight_origin so that
	// shading is the same (also important for r_shadows to cast shadows in the
	// same direction)
	if (VectorLength2(ent->custommodellight_origin))
	{
		// CSQC entities always provide this (via CL_GetTagMatrix)
		for (q = 0; q < 3; q++)
			shadingorigin[q] = ent->custommodellight_origin[q];
	}
	else if (ent->entitynumber > 0 && ent->entitynumber < cl.num_entities)
	{
		// network entity - follow attachment chain back to a root entity,
		int entnum = ent->entitynumber, recursion;
		for (recursion = 32; recursion > 0; --recursion)
		{
			int parentnum = cl.entities[entnum].state_current.tagentity;
			if (parentnum < 1 || parentnum >= cl.num_entities || !cl.entities_active[parentnum])
				break;
			entnum = parentnum;
		}
		// grab the root entity's origin
		Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, shadingorigin);
	}
	else
	{
		// not a CSQC entity (which sets custommodellight_origin), not a network
		// entity - so it's probably not attached to anything
		Matrix4x4_OriginFromMatrix(&ent->matrix, shadingorigin);
	}

	if (!(ent->flags & RENDER_LIGHT) || r_fullbright.integer)
	{
		// intentionally EF_FULLBRIGHT entity
		// the only type that is not scaled by r_refdef.scene.lightmapintensity
		// CSQC can still provide its own customized modellight values
		ent->render_rtlight_disabled = true;
		ent->render_modellight_forced = true;
		if (ent->flags & RENDER_CUSTOMIZEDMODELLIGHT)
		{
			// custom colors provided by CSQC
			for (q = 0; q < 3; q++)
			{
				a[q] = ent->custommodellight_ambient[q];
				c[q] = ent->custommodellight_diffuse[q];
				dir[q] = ent->custommodellight_lightdir[q];
			}
		}
		else if (r_fullbright_directed.integer)
			CL_UpdateEntityShading_GetDirectedFullbright(a, c, dir);
		else
			for (q = 0; q < 3; q++)
				a[q] = 1;
	}
	else
	{
		// fetch the lighting from the worldmodel data

		// CSQC can provide its own customized modellight values
		if (ent->flags & RENDER_CUSTOMIZEDMODELLIGHT)
		{
			ent->render_modellight_forced = true;
			for (q = 0; q < 3; q++)
			{
				a[q] = ent->custommodellight_ambient[q];
				c[q] = ent->custommodellight_diffuse[q];
				dir[q] = ent->custommodellight_lightdir[q];
			}
		}
		else if (ent->model->type == mod_sprite && !(ent->model->data_textures[0].basematerialflags & MATERIALFLAG_FULLBRIGHT))
		{
			if (ent->model->sprite.sprnum_type == SPR_OVERHEAD) // apply offset for overhead sprites
				shadingorigin[2] = shadingorigin[2] + r_overheadsprites_pushback.value;
			R_CompleteLightPoint(a, c, dir, shadingorigin, LP_LIGHTMAP | LP_RTWORLD | LP_DYNLIGHT, r_refdef.scene.lightmapintensity, r_refdef.scene.ambientintensity);
			ent->render_modellight_forced = true;
			ent->render_rtlight_disabled = true;
		}
		else if (((ent->model && !ent->model->lit) || (ent->model == r_refdef.scene.worldmodel ? mod_q3bsp_lightgrid_world_surfaces.integer : mod_q3bsp_lightgrid_bsp_surfaces.integer))
			&& r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->lit && r_refdef.scene.worldmodel->brushq3.lightgridtexture && mod_q3bsp_lightgrid_texture.integer)
		{
			ent->render_lightgrid = true;
			// no need to call R_CompleteLightPoint as we base it on render_lightmap_*
		}
		else if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->lit && r_refdef.scene.worldmodel->brush.LightPoint)
			R_CompleteLightPoint(a, c, dir, shadingorigin, LP_LIGHTMAP, r_refdef.scene.lightmapintensity, r_refdef.scene.ambientintensity);
		else if (r_fullbright_directed.integer)
			CL_UpdateEntityShading_GetDirectedFullbright(a, c, dir);
		else
			R_CompleteLightPoint(a, c, dir, shadingorigin, LP_LIGHTMAP, r_refdef.scene.lightmapintensity, r_refdef.scene.ambientintensity);
	}

	for (q = 0; q < 3; q++)
	{
		ent->render_fullbright[q] = ent->colormod[q];
		ent->render_glowmod[q] = ent->glowmod[q] * r_hdr_glowintensity.value;
		ent->render_modellight_ambient[q] = a[q] * ent->colormod[q];
		ent->render_modellight_diffuse[q] = c[q] * ent->colormod[q];
		ent->render_modellight_specular[q] = c[q];
		ent->render_modellight_lightdir_world[q] = dir[q];
		ent->render_lightmap_ambient[q] = ent->colormod[q] * r_refdef.scene.ambientintensity;
		ent->render_lightmap_diffuse[q] = ent->colormod[q] * r_refdef.scene.lightmapintensity;
		ent->render_lightmap_specular[q] = r_refdef.scene.lightmapintensity;
		ent->render_rtlight_diffuse[q] = ent->colormod[q];
		ent->render_rtlight_specular[q] = 1;
	}

	// these flags disable code paths, make sure it's obvious if they're ignored by storing 0 1 2
	if (ent->render_modellight_forced)
		for (q = 0; q < 3; q++)
			ent->render_lightmap_ambient[q] = ent->render_lightmap_diffuse[q] = ent->render_lightmap_specular[q] = q;
	if (ent->render_rtlight_disabled)
		for (q = 0; q < 3; q++)
			ent->render_rtlight_diffuse[q] = ent->render_rtlight_specular[q] = q;

	if (VectorLength2(ent->render_modellight_lightdir_world) == 0)
		VectorSet(ent->render_modellight_lightdir_world, 0, 0, 1); // have to set SOME valid vector here
	VectorNormalize(ent->render_modellight_lightdir_world);
	// transform into local space for the entity as well
	Matrix4x4_Transform3x3(&ent->inversematrix, ent->render_modellight_lightdir_world, ent->render_modellight_lightdir_local);
	VectorNormalize(ent->render_modellight_lightdir_local);
}


void CL_UpdateEntityShading(void)
{
	int i;
	CL_UpdateEntityShading_Entity(r_refdef.scene.worldentity);
	for (i = 0; i < r_refdef.scene.numentities; i++)
		CL_UpdateEntityShading_Entity(r_refdef.scene.entities[i]);
}

qbool vid_opened = false;
void CL_StartVideo(void)
{
	if (!vid_opened && cls.state != ca_dedicated)
	{
		vid_opened = true;
#ifdef WIN32
		// make sure we open sockets before opening video because the Windows Firewall "unblock?" dialog can screw up the graphics context on some graphics drivers
		NetConn_UpdateSockets();
#endif
		VID_Start();
	}
}

extern cvar_t host_framerate;
extern cvar_t host_speeds;
extern uint8_t serverlist_querystage;
double CL_Frame (double time)
{
	static double clframetime;
	static double cl_timer = 0;
	static double time1 = 0, time2 = 0, time3 = 0;
	int pass1, pass2, pass3;
	float maxfps;

	CL_VM_PreventInformationLeaks();

	/*
	 * If the accumulator hasn't become positive, don't
	 * run the frame. Everything that happens before this
	 * point will happen even if we're sleeping this frame.
	 */

	// limit the frametime steps to no more than 100ms each
	cl_timer = min(cl_timer + time, 0.1);

	// Run at full speed when querying servers, compared to waking up early to parse
	// this is simpler and gives pings more representative of what can be expected when playing.
	maxfps = (vid_activewindow || serverlist_querystage ? cl_maxfps : cl_maxidlefps).value;

	if (cls.state != ca_dedicated && (cl_timer > 0 || host.restless || maxfps <= 0))
	{
		R_TimeReport("---");
		Collision_Cache_NewFrame();
		R_TimeReport("photoncache");
#ifdef CONFIG_VIDEO_CAPTURE
		// decide the simulation time
		if (cls.capturevideo.active)
		{
			if (cls.capturevideo.realtime)
				clframetime = cl.realframetime = max(time, 1.0 / cls.capturevideo.framerate);
			else
			{
				clframetime = 1.0 / cls.capturevideo.framerate;
				cl.realframetime = max(time, clframetime);
			}
		}
		else
#endif
		{
			if (maxfps <= 0 || cls.timedemo)
				clframetime = cl.realframetime = cl_timer;
			else
				// networking assumes at least 10fps
				clframetime = cl.realframetime = bound(cl_timer, 1 / maxfps, 0.1);

			// on some legacy systems, we need to sleep to keep input responsive
			if (cl_maxfps_alwayssleep.value > 0 && !cls.timedemo)
				Sys_Sleep(min(cl_maxfps_alwayssleep.value / 1000, 0.05));
		}

		// apply slowmo scaling
		clframetime *= cl.movevars_timescale;
		// scale playback speed of demos by slowmo cvar
		if (cls.demoplayback)
		{
			clframetime *= host_timescale.value;
			// if demo playback is paused, don't advance time at all
			if (cls.demopaused)
				clframetime = 0;
		}
		else
		{
			// host_framerate overrides all else
			if (host_framerate.value)
				clframetime = host_framerate.value;

			if (cl.paused || host.paused)
				clframetime = 0;
		}

		// deduct the frame time from the accumulator
		cl_timer -= cl.realframetime;

		cl.oldtime = cl.time;
		cl.time += clframetime;

		// update video
		if (host_speeds.integer)
			time1 = Sys_DirtyTime();
		R_TimeReport("pre-input");

		// Collect input into cmd
		CL_Input();

		R_TimeReport("input");

		// check for new packets
		NetConn_ClientFrame();

		// read a new frame from a demo if needed
		CL_ReadDemoMessage();
		R_TimeReport("clientnetwork");

		// now that packets have been read, send input to server
		CL_SendMove();
		R_TimeReport("sendmove");

		// update client world (interpolate entities, create trails, etc)
		CL_UpdateWorld();
		R_TimeReport("lerpworld");

		CL_Video_Frame();

		R_TimeReport("client");

		CL_UpdateScreen();
		R_TimeReport("render");

		if (host_speeds.integer)
			time2 = Sys_DirtyTime();

		// update audio
		if(cl.csqc_usecsqclistener)
		{
			S_Update(&cl.csqc_listenermatrix);
			cl.csqc_usecsqclistener = false;
		}
		else
			S_Update(&r_refdef.view.matrix);

		CDAudio_Update();
		R_TimeReport("audio");

		// reset gathering of mouse input
		in_mouse_x = in_mouse_y = 0;

		if (host_speeds.integer)
		{
			pass1 = (int)((time1 - time3)*1000000);
			time3 = Sys_DirtyTime();
			pass2 = (int)((time2 - time1)*1000000);
			pass3 = (int)((time3 - time2)*1000000);
			Con_Printf("%6ius total %6ius server %6ius gfx %6ius snd\n",
						pass1+pass2+pass3, pass1, pass2, pass3);
		}
	}
	// if there is some time remaining from this frame, reset the timer
	return cl_timer >= 0 ? 0 : cl_timer;
}

/*
===========
CL_Shutdown
===========
*/
void CL_Shutdown (void)
{
	// be quiet while shutting down
	S_StopAllSounds();
	
	// disconnect client from server if active
	CL_Disconnect();
	
	CL_Video_Shutdown();

#ifdef CONFIG_MENU
	// Shutdown menu
	if(MR_Shutdown)
		MR_Shutdown();
#endif

	S_Terminate ();
	
	R_Modules_Shutdown();
	VID_Shutdown();

	CL_Screen_Shutdown();
	CL_Particles_Shutdown();
	CL_Parse_Shutdown();
	CL_MeshEntities_Shutdown();

	Key_Shutdown();

	Mem_FreePool (&cls.permanentmempool);
	Mem_FreePool (&cls.levelmempool);
}

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	if (cls.state == ca_dedicated)
	{
		Cmd_AddCommand(CF_SERVER, "disconnect", CL_Disconnect_f, "disconnect from server (or disconnect all clients if running a server)");
	}
	else
	{
		Con_Printf("Initializing client\n");

		Cvar_SetValueQuick(&host_isclient, 1);

		R_Modules_Init();
		Palette_Init();
#ifdef CONFIG_MENU
		MR_Init_Commands();
#endif
		VID_Shared_Init();
		VID_Init();
		Render_Init();
		S_Init();
		Key_Init();
		V_Init();

		cls.levelmempool = Mem_AllocPool("client (per-level memory)", 0, NULL);
		cls.permanentmempool = Mem_AllocPool("client (long term memory)", 0, NULL);

		memset(&r_refdef, 0, sizeof(r_refdef));
		// max entities sent to renderer per frame
		r_refdef.scene.maxentities = MAX_EDICTS + 256 + 512;
		r_refdef.scene.entities = (entity_render_t **)Mem_Alloc(cls.permanentmempool, sizeof(entity_render_t *) * r_refdef.scene.maxentities);

		// max temp entities
		r_refdef.scene.maxtempentities = MAX_TEMPENTITIES;
		r_refdef.scene.tempentities = (entity_render_t *)Mem_Alloc(cls.permanentmempool, sizeof(entity_render_t) * r_refdef.scene.maxtempentities);

		CL_InitInput ();

	//
	// register our commands
	//
		CL_InitCommands();

		Cvar_RegisterVariable (&cl_upspeed);
		Cvar_RegisterVariable (&cl_forwardspeed);
		Cvar_RegisterVariable (&cl_backspeed);
		Cvar_RegisterVariable (&cl_sidespeed);
		Cvar_RegisterVariable (&cl_movespeedkey);
		Cvar_RegisterVariable (&cl_yawspeed);
		Cvar_RegisterVariable (&cl_pitchspeed);
		Cvar_RegisterVariable (&cl_anglespeedkey);
		Cvar_RegisterVariable (&cl_shownet);
		Cvar_RegisterVariable (&cl_nolerp);
		Cvar_RegisterVariable (&cl_lerpexcess);
		Cvar_RegisterVariable (&cl_lerpanim_maxdelta_server);
		Cvar_RegisterVariable (&cl_lerpanim_maxdelta_framegroups);
		Cvar_RegisterVariable (&cl_deathfade);
		Cvar_RegisterVariable (&lookspring);
		Cvar_RegisterVariable (&lookstrafe);
		Cvar_RegisterVariable (&sensitivity);
		Cvar_RegisterVariable (&freelook);

		Cvar_RegisterVariable (&m_pitch);
		Cvar_RegisterVariable (&m_yaw);
		Cvar_RegisterVariable (&m_forward);
		Cvar_RegisterVariable (&m_side);

		Cvar_RegisterVariable (&cl_itembobspeed);
		Cvar_RegisterVariable (&cl_itembobheight);

		CL_Demo_Init();

		Cmd_AddCommand(CF_CLIENT, "entities", CL_PrintEntities_f, "print information on network entities known to client");
		Cmd_AddCommand(CF_CLIENT, "disconnect", CL_Disconnect_f, "disconnect from server (or disconnect all clients if running a server)");
		Cmd_AddCommand(CF_CLIENT, "connect", CL_Connect_f, "connect to a server by IP address or hostname");
		Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "reconnect", CL_Reconnect_f, "reconnect to the last server you were on, or resets a quakeworld connection (do not use if currently playing on a netquake server)");

		// Support Client-side Model Index List
		Cmd_AddCommand(CF_CLIENT, "cl_modelindexlist", CL_ModelIndexList_f, "list information on all models in the client modelindex");
		// Support Client-side Sound Index List
		Cmd_AddCommand(CF_CLIENT, "cl_soundindexlist", CL_SoundIndexList_f, "list all sounds in the client soundindex");

		Cmd_AddCommand(CF_CLIENT, "fog", CL_Fog_f, "set global fog parameters (density red green blue [alpha [mindist [maxdist [top [fadedepth]]]]])");
		Cmd_AddCommand(CF_CLIENT, "fog_heighttexture", CL_Fog_HeightTexture_f, "set global fog parameters (density red green blue alpha mindist maxdist top depth textures/mapname/fogheight.tga)");

		Cmd_AddCommand(CF_CLIENT, "cl_areastats", CL_AreaStats_f, "prints statistics on entity culling during collision traces");

		Cvar_RegisterVariable(&r_draweffects);
		Cvar_RegisterVariable(&cl_explosions_alpha_start);
		Cvar_RegisterVariable(&cl_explosions_alpha_end);
		Cvar_RegisterVariable(&cl_explosions_size_start);
		Cvar_RegisterVariable(&cl_explosions_size_end);
		Cvar_RegisterVariable(&cl_explosions_lifetime);
		Cvar_RegisterVariable(&cl_stainmaps);
		Cvar_RegisterVariable(&cl_stainmaps_clearonload);
		Cvar_RegisterVariable(&cl_beams_polygons);
		Cvar_RegisterVariable(&cl_beams_quakepositionhack);
		Cvar_RegisterVariable(&cl_beams_instantaimhack);
		Cvar_RegisterVariable(&cl_beams_lightatend);
		Cvar_RegisterVariable(&cl_noplayershadow);
		Cvar_RegisterVariable(&cl_dlights_decayradius);
		Cvar_RegisterVariable(&cl_dlights_decaybrightness);

		Cvar_RegisterVariable(&cl_prydoncursor);
		Cvar_RegisterVariable(&cl_prydoncursor_notrace);

		Cvar_RegisterVariable(&cl_deathnoviewmodel);

		// for QW connections
		Cvar_RegisterVariable(&qport);
		// multiplying by RAND_MAX necessary for Windows, for which RAND_MAX is only 32767.
		Cvar_SetValueQuick(&qport, ((unsigned int)rand() * RAND_MAX + (unsigned int)rand()) & 0xffff);

		Cmd_AddCommand(CF_CLIENT, "timerefresh", CL_TimeRefresh_f, "turn quickly and print rendering statistcs");

		Cvar_RegisterVariable(&cl_locs_enable);
		Cvar_RegisterVariable(&cl_locs_show);
		Cmd_AddCommand(CF_CLIENT, "locs_add", CL_Locs_Add_f, "add a point or box location (usage: x y z[ x y z] \"name\", if two sets of xyz are supplied it is a box, otherwise point)");
		Cmd_AddCommand(CF_CLIENT, "locs_removenearest", CL_Locs_RemoveNearest_f, "remove the nearest point or box (note: you need to be very near a box to remove it)");
		Cmd_AddCommand(CF_CLIENT, "locs_clear", CL_Locs_Clear_f, "remove all loc points/boxes");
		Cmd_AddCommand(CF_CLIENT, "locs_reload", CL_Locs_Reload_f, "reload .loc file for this map");
		Cmd_AddCommand(CF_CLIENT, "locs_save", CL_Locs_Save_f, "save .loc file for this map containing currently defined points and boxes");

		Cvar_RegisterVariable(&csqc_polygons_defaultmaterial_nocullface);
		Cvar_RegisterVariable(&csqc_lowres);

		Cvar_RegisterVariable (&cl_minfps);
		Cvar_RegisterVariable (&cl_minfps_fade);
		Cvar_RegisterVariable (&cl_minfps_qualitymax);
		Cvar_RegisterVariable (&cl_minfps_qualitymin);
		Cvar_RegisterVariable (&cl_minfps_qualitystepmax);
		Cvar_RegisterVariable (&cl_minfps_qualityhysteresis);
		Cvar_RegisterVariable (&cl_minfps_qualitymultiply);
		Cvar_RegisterVariable (&cl_minfps_force);
		Cvar_RegisterVariable (&cl_maxfps);
		Cvar_RegisterVariable (&cl_maxfps_alwayssleep);
		Cvar_RegisterVariable (&cl_maxidlefps);

		Cvar_RegisterVariable (&cl_areagrid_link_SOLID_NOT);
		Cvar_RegisterVariable (&cl_gameplayfix_nudgeoutofsolid_separation);

		CL_Parse_Init();
		CL_Particles_Init();
		CL_Screen_Init();

		CL_Video_Init();

		Cvar_Callback(&cl_netport);

		host.hook.ConnectLocal = CL_EstablishConnection_Local;
		host.hook.Disconnect = CL_DisconnectEx;
		host.hook.ToggleMenu = CL_ToggleMenu_Hook;
	}
}
