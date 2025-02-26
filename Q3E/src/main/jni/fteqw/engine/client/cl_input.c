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
// cl.input.c  -- builds an intended movement command to send to the server

#include "quakedef.h"

#ifdef _WIN32
#include "winquake.h"	//fps indep stuff.
#endif

float in_sensitivityscale = 1;

#ifdef NQPROT
static cvar_t	cl_movement = CVARD("cl_movement","1", "Specifies whether to send movement sequence info over DPP7 protocols (other protocols are unaffected). Unlike cl_nopred, this can result in different serverside behaviour.");
#endif

cvar_t	cl_nodelta = CVAR("cl_nodelta","0");

cvar_t	cl_c2sdupe = CVARD("cl_c2sdupe", "0", "Send duplicate copies of packets to the server. This avoids extra latency caused by packetloss, but could also make the problem worse.");
cvar_t	cl_c2spps = CVARD("cl_c2spps", "0", "Reduces outgoing packet rates by dropping up to a third of outgoing packets.");
cvar_t	cl_c2sImpulseBackup = CVARD("cl_c2sImpulseBackup","3", "Prevents the cl_c2spps setting from dropping redundant packets that contain impulses, in an attempt to keep impulses more reliable.");
static cvar_t	cl_c2sMaxRedundancy = CVARD("cl_c2sMaxRedundancy","5", "This is the maximum number of input frames to send in each input packet. Values greater than 1 provide redundancy and avoid prediction misses, though you might find cl_c2sdupe provides equivelent result and at lower latency. It is locked at 3 for vanilla quakeworld, and locked at 1 for vanilla netquake.");
cvar_t	cl_netfps = CVARFD("cl_netfps", "150", CVAR_ARCHIVE, "Send up to this many packets to the server per second. The rate used is also limited by the server which usually forces a cap to this setting of 77. Low packet rates can result in extra extrapolation to try to hide the resulting latencies.");
cvar_t  cl_queueimpulses = CVARD("cl_queueimpulses", "0", "Queues unsent impulses instead of replacing them. This avoids the need for extra wait commands (and the timing issues of such commands), but potentially increases latency and can cause scripts to be desynced with regard to buttons and impulses.");
cvar_t	cl_smartjump = CVARD("cl_smartjump", "1", "Makes the jump button act as +moveup when in water. This is typically quieter and faster.");
cvar_t	cl_iDrive = CVARFD("cl_iDrive", "1", CVAR_SEMICHEAT, "Effectively releases movement keys when the opposing key is pressed. This avoids dead-time when both keys are pressed. This can be emulated with various scripts, but that's messy.");
cvar_t	cl_run = CVARD("cl_run", "0", "Enables autorun, inverting the state of the +speed key.");
cvar_t	cl_fastaccel = CVARD("cl_fastaccel", "1", "Begin moving at full speed instantly, instead of waiting a frame or so.");
extern cvar_t cl_rollspeed;
static cvar_t cl_sendchatstate = CVARD("cl_sendchatstate", "1", "Announce your chat state to the server in a privacy-violating kind of way. This allows other players to see your afk/at-console status.");

cvar_t	cl_prydoncursor = CVAR("cl_prydoncursor", "");	//for dp protocol
cvar_t	cl_instantrotate = CVARF("cl_instantrotate", "1", CVAR_SEMICHEAT);
cvar_t in_xflip = CVAR("in_xflip", "0");
cvar_t in_vraim = CVARD("in_vraim", "1", "When set to 1, the 'view' angle sent to the server is controlled by your vr headset instead of separately. This is for fallback behaviour and blocks mouse+joy+gamepad aiming.");

cvar_t	prox_inmenu = CVAR("prox_inmenu", "0");

usercmd_t cl_pendingcmd[MAX_SPLITS];

/*kinda a hack...*/
unsigned int		con_splitmodifier;
cvar_t	cl_forceseat = CVARAD("in_forceseat", "0", "in_forcesplitclient", "Overrides the device identifiers to control a specific client from any device. This can be used for debugging mods, where you only have one keyboard/mouse.");
int CL_TargettedSplit(qboolean nowrap)
{
	int mod;

	//explicitly targetted at some seat number from the server
	if (Cmd_ExecLevel >= RESTRICT_SERVER)
		return Cmd_ExecLevel - RESTRICT_SERVER;

	//locally executed command.
	if (nowrap)
		mod = MAX_SPLITS;
	else
		mod = cl.splitclients;
	if (mod < 1)
		return 0;

	if (con_splitmodifier > 0)
		return (con_splitmodifier - 1) % mod;
	else if (cl_forceseat.ival > 0)
		return (cl_forceseat.ival-1) % cl.splitclients;
	else
		return 0;
}

void CL_Split_f(void)
{
	int tmp;
	char *c;
	c = Cmd_Argv(0);
	tmp = con_splitmodifier;
	if (*c == '+' || *c == '-')
	{
		con_splitmodifier = c[2]-'0';
		Cmd_ExecuteString(va("%c%s", *c, Cmd_Args()), Cmd_ExecLevel);
	}
	else
	{
		con_splitmodifier = c[1]-'0';
		Cmd_ExecuteString(Cmd_Args(), Cmd_ExecLevel);
	}
	con_splitmodifier = tmp;
}
void CL_SplitA_f(void)
{
	int tmp;
	char *c, *args;
	c = Cmd_Argv(0);
	args = COM_Parse(Cmd_Args());
	if (!args)
		return;
	while(*args == ' ' || *args == '\t')
		args++;
	tmp = con_splitmodifier;
	con_splitmodifier = atoi(com_token);
	if (*c == '+' || *c == '-')
		Cmd_ExecuteString(va("%c%s", *c, args), Cmd_ExecLevel);
	else
		Cmd_ExecuteString(args, Cmd_ExecLevel);
	con_splitmodifier = tmp;
}

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_strafe, in_speed;
static kbutton_t	in_klook;
static kbutton_t	in_left, in_right, in_forward, in_back;
static kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
static kbutton_t	in_use, in_jump, in_attack;
static kbutton_t	in_rollleft, in_rollright, in_up, in_down;

static kbutton_t	in_button[19+1];

#define IN_IMPULSECACHE 32
static int			in_impulse[MAX_SPLITS][IN_IMPULSECACHE];
static int			in_nextimpulse[MAX_SPLITS];
static int			in_impulsespending[MAX_SPLITS];
static void CL_QueueImpulse (int pnum, int newimp)
{
	if (cl_queueimpulses.ival)
	{
		if (in_impulsespending[pnum]>=IN_IMPULSECACHE)
		{
			Con_Printf("Too many impulses, ignoring %i\n", newimp);
			return;
		}
		in_impulse[pnum][(in_nextimpulse[pnum]+in_impulsespending[pnum])%IN_IMPULSECACHE] = newimp;
		in_impulsespending[pnum]++;
	}
	else
	{
		if (in_impulsespending[pnum])
			Con_DPrintf("Too many impulses, forgetting %i\n", in_impulse[pnum][(in_nextimpulse[pnum])%IN_IMPULSECACHE]);
		in_impulse[pnum][(in_nextimpulse[pnum])%IN_IMPULSECACHE] = newimp;
		in_impulsespending[pnum]=1;
	}
}

qboolean	cursor_active;





static qboolean KeyDown_Scan (kbutton_t *b, kbutton_t *anti, int k)
{
	int pnum = CL_TargettedSplit(false);
	
	if (k == b->down[pnum][0] || k == b->down[pnum][1])
		return false;		// repeating key
	
	if (!b->down[pnum][0])
		b->down[pnum][0] = k;
	else if (!b->down[pnum][1])
		b->down[pnum][1] = k;
	else
	{
		Con_DPrintf ("Three keys down for a button!\n");
		return false;
	}
	
	if (b->state[pnum] & 1)
		return false;		// still down
	b->state[pnum] |= 1 + 2;	// down + impulse down

	if (anti && (anti->state[pnum] & 1) && cl_iDrive.ival)
	{	//anti-keys are the opposing key. so +forward can auto-release +back for slightly faster-responding keypresses.
		b->suppressed[pnum] = anti;
		anti->suppressed[pnum] = NULL;
		anti->state[pnum] &= ~1;		// now up
		anti->state[pnum] |= 4; 		// impulse up
	}
	return true;
}
static void KeyDown (kbutton_t *b, kbutton_t *anti)
{
	int		k;
	char	*c;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	KeyDown_Scan(b, anti, k);
}

static qboolean KeyUp_Scan (kbutton_t *b, int k)
{
	int pnum = CL_TargettedSplit(false);

	if (k < 0)
	{ // typed manually at the console, assume for unsticking, so clear all
		b->suppressed[pnum] = NULL;
		b->down[pnum][0] = b->down[pnum][1] = 0;
		if (b->state[pnum] & ~4)
		{
			b->state[pnum] = 4;	// impulse up
			return true;
		}
		return false;
	}

	if (b->down[pnum][0] == k)
		b->down[pnum][0] = 0;
	else if (b->down[pnum][1] == k)
		b->down[pnum][1] = 0;
	else
		return false;		// key up without coresponding down (menu pass through)
	if (b->down[pnum][0] || b->down[pnum][1])
		return false;		// some other key is still holding it down

	if (!(b->state[pnum] & 1))
		return false;		// still up (this should not happen)
	b->state[pnum] &= ~1;		// now up
	b->state[pnum] |= 4; 		// impulse up

	if (b->suppressed[pnum])
	{
		if (b->suppressed[pnum]->down[pnum][0] || b->suppressed[pnum]->down[pnum][1])
			b->suppressed[pnum]->state[pnum] |= 1 + 2;
		b->suppressed[pnum] = NULL;
	}
	return true;
}
static qboolean KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;

	return KeyUp_Scan(b, k);
}



#ifdef QUAKESTATS
static cvar_t	cl_weaponhide = CVARD("cl_weaponhide", "0", "HACK: Attempt to switch weapon to another in order to cheat your killer out of any possible weapon upgrades.\n0: original behaviour\n1: switch away when +attack/+fire is released\n2: switch away only in deathmatch\n");
static cvar_t	cl_weaponhide_preference = CVARAD("cl_weaponhide_preference", "2 1", "cl_weaponhide_axe", "The weapon you would like to try to switch to when cl_weaponhide is active");
static cvar_t	cl_weaponpreselect = CVARD("cl_weaponpreselect", "0", "HACK: Controls the interaction between the ^aweapon^a and ^a+attack^a commands (does not affect ^aimpulse^a).\n0: weapon switch happens instantly\n1: weapon switch happens on next attack\n2: instant only when already firing, otherwise delayed\n3: delay until new attack only in deathmatch 1\n4: delay until any attack only in deathmatch 1");
static cvar_t	cl_weaponforgetorder = CVARD("cl_weaponforgetorder", "0", "The 'weapon' command will lock in its weapon choice, instead of choosing a different weapon between select+fire.");
cvar_t r_viewpreselgun = CVARD("r_viewpreselgun", "0", "HACK: Display the preselected weaponmodel, instead of the current weaponmodel.");
static int preselectedweapons[MAX_SPLITS];
static int preselectedweapon[MAX_SPLITS][32];
static kbutton_t in_wwheel;
static float wwheeldir[MAX_SPLITS][2];
static size_t wwheelsel[MAX_SPLITS];
static double wwheelseltime[MAX_SPLITS];

static struct weaponinfo_s {
	char shortname[32]; //must not look like an impulse.
	int impulse; //primary key...
	int items_mask;
	int items_val;
//	int weapon_val;	//unused...
	int ammostat;
	int ammomin;

	char *icons[3]; //unselected,selected,ammo
	char *viewmodel;
} *weaponinfo;
static size_t weaponinfo_count;

static int IN_NameToWeaponIdx(const char *name)
{
	size_t i;
	char *end;
	i = strtoul(name, &end, 10);
	if (i && !end)
	{	//select by impulse when they specified something numeric.
		for (i = 0; i < weaponinfo_count; i++)
		{
			if (weaponinfo[i].impulse == i)
				return i;
		}
	}
	else
	{
		for (i = 0; i < weaponinfo_count; i++)
		{
			if (!strcmp(name, weaponinfo[i].shortname))
				return i;
		}
	}
	return -1;
}
static int IN_RegisterWeapon(int impulse, const char *name, int items_mask, int items_val, int weapon_val, int ammostat, int ammomin, const char *viewmodel, const char *icon,const char *selicon, const char *ammoicon)
{
	size_t i;
	char *end;
	if (impulse <= 0 || impulse > 255)
		return -1;	//no. just no...
	if (strtol(name, &end, 10) != impulse)
	{
		if (!*end)
			return -1;	//parsed as a number, which didn't match its impulse... don't break the impulse command.
	}
	//slots are unique/replaced by impulse
	for (i = 0; i < weaponinfo_count; i++)
	{
		if (weaponinfo[i].impulse == impulse)
			break;	//replace...
	}
	if (i == weaponinfo_count)
		Z_ReallocElements((void**)&weaponinfo, &weaponinfo_count, i+1, sizeof(*weaponinfo));
	weaponinfo[i].impulse = impulse;

	Q_strncpyz(weaponinfo[i].shortname, name, sizeof(weaponinfo[i].shortname));
	weaponinfo[i].items_mask = items_mask;
	weaponinfo[i].items_val = items_val;
//	weaponinfo[i].weapon_val;	//unused...
	weaponinfo[i].ammostat = ammostat;
	weaponinfo[i].ammomin = ammomin;

#define Z_StrDupPtr2(v,s) do{Z_Free(*v),*(v) = (s&&*s)?strcpy(Z_Malloc(strlen(s)+1), s):NULL;}while(0)
	Z_StrDupPtr2(&weaponinfo[i].viewmodel, viewmodel);
	Z_StrDupPtr2(&weaponinfo[i].icons[0], icon);
	Z_StrDupPtr2(&weaponinfo[i].icons[1], selicon);
	Z_StrDupPtr2(&weaponinfo[i].icons[2], ammoicon);
	return i;
}
static void IN_RegisterWeapon_Clear(void)
{
	while(weaponinfo_count)
		Z_Free(weaponinfo[--weaponinfo_count].viewmodel);
	Z_Free(weaponinfo);
	weaponinfo = NULL;
}
void IN_RegisterWeapon_Reset(void)
{
	vfsfile_t *f;
	IN_RegisterWeapon_Clear();

	f = FS_OpenVFS("wwheel.txt", "rb", FS_GAME);
	if (f)
	{	//from the rerelease:
		/*
		slot N
		{
			impulse N
			icon "gfx/weapons/ww_foo_1.lmp"
			icon_sel "gfx/weapons/ww_foo_2.lmp"
			ammoicon "FOO"
			entvaroffs N
			weaponnum N
		}
		*/
		char line[1024];
		char *v;
		for(;;)
		{
			if (!VFS_GETS(f, line, sizeof(line)))
				break;
			v = COM_Parse(line);
			if (!strcmp(com_token, "slot"))	//slot N... just assume they're ordered.
			{
				//v = COM_Parse(line);
				//slot = com_token
				if (!VFS_GETS(f, line, sizeof(line)))
					break;
				v = COM_Parse(line);
				if (!strcmp(com_token, "{"))
				{
					int weaponbit=0;
					int impulse=0;
					int ammostat=-1;
					int ammocount=0;
					int field;
					char *icon = NULL;
					char *selicon = NULL;
					char *ammoicon = NULL;
					char *viewmodel = NULL;
					char *name = NULL;

					for (;;)
					{
						if (!VFS_GETS(f, line, sizeof(line)))
							break;
						v = COM_Parse(line);
						if (!strcmp(com_token, "}"))
						{	//end of this weapon.
							IN_RegisterWeapon(impulse, name?name:va("%i", impulse), weaponbit,weaponbit, weaponbit, ammostat,ammocount, viewmodel, icon, selicon, ammoicon);
							break;
						}
						else if (!strcmp(com_token, "impulse"))
						{
							v = COM_Parse(v);
							impulse = atoi(com_token);
						}
						else if (!strcmp(com_token, "weaponnum"))
						{
							v = COM_Parse(v);
							weaponbit = atoi(com_token);
						}
						else if (!strcmp(com_token, "icon"))
						{
							v = COM_Parse(v);
							Z_StrDupPtr(&icon, com_token);
						}
						else if (!strcmp(com_token, "icon_sel"))
						{
							v = COM_Parse(v);
							Z_StrDupPtr(&selicon, com_token);
						}
						else if (!strcmp(com_token, "ammoicon"))
						{
							v = COM_Parse(v);
							if (strchr(com_token, '.') || strchr(com_token, '/'))
								Z_StrDupPtr(&ammoicon, com_token);
							else
								Z_StrDupPtr(&ammoicon, va("gfx/%s", com_token));
						}
						else if (!strcmp(com_token, "entvaroffs"))
						{	//this seems to be a host-only thing. the server can poke the qc's fields directly but other clients can't.
							//remap known indexes to stats - this can only work for nq's system fields.
							//note that rogue does some windowing thing so our client can only know either nails or lavanails, not both. either way those are NOT the normal stats and these weapons will appear to have infinite ammo as a result, sorry.
							//they really should have used field names here, not numbers, but hey, not my spec... use our 'ammostat' instead for fancy mods.
							v = COM_Parse(v);
							field = atoi(com_token);
							switch(field)
							{
							case 216:	ammostat = STAT_SHELLS, ammocount = 1;	break;
							case 220:	ammostat = STAT_NAILS, ammocount = 1;	break;
							case 224:	ammostat = STAT_ROCKETS, ammocount = 1;	break;
							case 228:	ammostat = STAT_CELLS, ammocount = 1;	break;
							default:	ammostat = -1, ammocount = 0;	break;
							}
						}
						else if (!strcmp(com_token, "ammostat"))
						{	//non-qe
							v = COM_Parse(v);
							ammostat = atoi(com_token);
							if (ammocount <= 0)
								ammocount = 1;
						}
						else if (!strcmp(com_token, "ammomin"))
						{	//non-qe
							v = COM_Parse(v);
							ammocount = atoi(com_token);
						}
						else if (!strcmp(com_token, "viewmodel"))
						{	//non-qe, so preselect can show the correct model before its actually changed.
							v = COM_Parse(v);
							Z_StrDupPtr(&viewmodel, com_token);
						}
						else if (!strcmp(com_token, "shortname"))
						{	//non-qe, for `+fire nq`
							v = COM_Parse(v);
							Z_StrDupPtr(&name, com_token);
						}
						else if (*com_token)
							Con_Printf("Unexpected line in wwheel.txt: %s\n", line);
					}
					Z_Free(icon);
					Z_Free(selicon);
					Z_Free(ammoicon);
					Z_Free(viewmodel);
					Z_Free(name);
				}
				else
				{
					Con_Printf("missing block, found: %s\n", line);
					break;
				}
			}
			else if (*com_token)
				Con_Printf("Unexpected line in wwheel.txt: %s\n", line);
		}
		VFS_CLOSE(f);
	}
	else
	{
		IN_RegisterWeapon(2, "sg", IT_SHOTGUN,IT_SHOTGUN, IT_SHOTGUN, STAT_SHELLS,1, "progs/v_shot.mdl", NULL,NULL, "gfx/sb_shells");
		IN_RegisterWeapon(3, "ssg", IT_SUPER_SHOTGUN,IT_SUPER_SHOTGUN, IT_SUPER_SHOTGUN, STAT_SHELLS,1, "progs/v_shot2.mdl", NULL,NULL, "gfx/sb_shells");
		IN_RegisterWeapon(4, "ng", IT_NAILGUN,IT_NAILGUN, IT_NAILGUN, STAT_NAILS,1, "progs/v_nail.mdl", NULL,NULL, "gfx/sb_nails");
		IN_RegisterWeapon(5, "sng", IT_SUPER_NAILGUN,IT_SUPER_NAILGUN, IT_SUPER_NAILGUN, STAT_NAILS,1, "progs/v_nail2.mdl", NULL,NULL, "gfx/sb_nails");
		IN_RegisterWeapon(6, "gl", IT_GRENADE_LAUNCHER,IT_GRENADE_LAUNCHER, IT_GRENADE_LAUNCHER, STAT_ROCKETS,1, "progs/v_rock.mdl", NULL,NULL, "gfx/sb_rocket");
		IN_RegisterWeapon(7, "rl", IT_ROCKET_LAUNCHER,IT_ROCKET_LAUNCHER, IT_ROCKET_LAUNCHER, STAT_ROCKETS,1, "progs/v_rock2.mdl", NULL,NULL, "gfx/sb_rocket");
		IN_RegisterWeapon(8, "lg", IT_LIGHTNING,IT_LIGHTNING, IT_LIGHTNING, STAT_CELLS,1, "progs/v_light.mdl", NULL,NULL, "gfx/sb_cells");
		IN_RegisterWeapon(1, "axe", IT_AXE,IT_AXE, IT_AXE, -1,0, "progs/v_axe.mdl", NULL, NULL, NULL);
	}
}
static void IN_RegisterWeapon_f(void)
{
	if (Cmd_Argc() <= 2)
	{
		const char *arg = Cmd_Argv(1);
		if (!strcmp(arg, "clear"))
			IN_RegisterWeapon_Clear();
		else if (!strcmp(arg, "reset") || !strcmp(arg, "quake"))	//'quake' for compat with dp.
			IN_RegisterWeapon_Reset();
		else
			Con_Printf("Unknown arg %s\n", Cmd_Argv(1));
	}
	else
	{
		IN_RegisterWeapon(	atoi(Cmd_Argv(2)),	//impulse
							Cmd_Argv(1),		//name
							atoi(Cmd_Argv(3)),	//itemsmask
							atoi(Cmd_Argv(3)),	//itemsval
							atoi(Cmd_Argv(4)),	//weaponval
							atoi(Cmd_Argv(5)),	//ammostat
							atoi(Cmd_Argv(6)),	//ammomin
							Cmd_Argv(7),		//viewmodel
							Cmd_Argv(8),		//weaponicon
							Cmd_Argv(9),		//weaponicon_sel
							Cmd_Argv(10));		//ammoicon
	}
}

//hacks, because we have to guess what the mod is doing. we'll probably get it wrong, which sucks.
static qboolean IN_HaveWeapon_Idx(int pnum, size_t widx)
{
	if (widx < weaponinfo_count)
		if ((cl.playerview[pnum].stats[STAT_ITEMS]&weaponinfo[widx].items_mask) == weaponinfo[widx].items_val)	//we have the weapon
			if (weaponinfo[widx].ammostat < 0 || cl.playerview[pnum].stats[weaponinfo[widx].ammostat] >= weaponinfo[widx].ammomin)				//and we have enough ammo for it too.
				return true;
	return false;
}
static qboolean IN_HaveWeapon(int pnum, int impulse)
{
	size_t widx;
	for (widx = 0; widx < weaponinfo_count; widx++)
	{
		if (weaponinfo[widx].impulse == impulse)
			return IN_HaveWeapon_Idx(pnum, widx);
	}
	return false;	//we don't really know about it, but assume we can't because false negatives are better than false positives here.
}
static qboolean IN_HaveWeapon_Name(int pnum, char *name, int *impulse)
{
	int widx = IN_NameToWeaponIdx(name);
	if (widx < 0)
		return false;
	if (!IN_HaveWeapon_Idx(pnum, widx))
		return false;
	*impulse = weaponinfo[widx].impulse;
	return true;
}
static int IN_BestWeapon_Pre(unsigned int pnum);
//if we're using weapon preselection, then we probably also want to show which weapon will be selected, instead of showing the shotgun the whole time.
//this of course requires more hacks.
const char *IN_GetPreselectedViewmodelName(unsigned int pnum)
{
	if (r_viewpreselgun.ival && cl_weaponpreselect.ival && pnum < countof(preselectedweapons) && preselectedweapons[pnum])
	{
		int best = IN_BestWeapon_Pre(pnum);
		size_t widx;
		for (widx = 0; widx < weaponinfo_count; widx++)
		{
			if (weaponinfo[widx].impulse == best)
				return weaponinfo[widx].viewmodel;
		}
	}
	return NULL;
}

static int IN_BestWeapon_Args(unsigned int pnum, int firstarg, int argcount)
{	//returns impulses.
	int i, imp;
	unsigned int best = 0;

	for (i = firstarg + argcount; --i >= firstarg; )
	{
		if (IN_HaveWeapon_Name(pnum, Cmd_Argv(i), &imp))
			best = imp;
	}

	return best;
}
static int IN_BestWeapon_Pre(unsigned int pnum)
{
	int i, imp;
	unsigned int best = 0;

	for (i = preselectedweapons[pnum]; i-- > 0; )
	{
		imp = preselectedweapon[pnum][i];
		if (IN_HaveWeapon(pnum, imp))
			best = imp;
	}

	return best;
}

static void IN_DoPostSelect(void)
{
	int pnum = CL_TargettedSplit(false);
	if (cl_weaponpreselect.ival)
	{
		int best = IN_BestWeapon_Pre(pnum);
		if (best)
			CL_QueueImpulse(pnum, best);
	}

	if (in_wwheel.state[pnum]&1)
	{
		in_wwheel.state[pnum] = 0;
		pnum = CL_TargettedSplit(false);
		if (wwheelsel[pnum] < weaponinfo_count)
			CL_QueueImpulse(pnum, weaponinfo[wwheelsel[pnum]].impulse);
	}
}
//The weapon command autoselects a prioritised weapon like multi-arg impulse does.
//however, it potentially makes the switch only on the next +attack.
void IN_Weapon (void)
{
	int newimp;
	int pnum = CL_TargettedSplit(false);
	int mode, best, i;

	preselectedweapons[pnum] = 0;
	for (i = 1; i < Cmd_Argc() && i <= countof(preselectedweapon[pnum]); i++)
	{
		best = IN_NameToWeaponIdx(Cmd_Argv(i));
		if (best >= 0)
			best = weaponinfo[best].impulse;	//a known weapon
		else
			best = atoi(Cmd_Argv(i));	//fall back
		if (best > 0)
			preselectedweapon[pnum][preselectedweapons[pnum]++] = best;
	}

	best = IN_BestWeapon_Pre(pnum);
	if (best)
	{
		newimp = best;
		if (cl_weaponforgetorder.ival)
		{	//make sure the +attack sticks with the selected weapon.
			preselectedweapon[pnum][0] = best;
			preselectedweapons[pnum] = 1;
		}
	}
	else
		return;	//no new weapon...

	mode = cl_weaponpreselect.ival;
	if (mode == 3)
		mode = (cl.deathmatch==1)?1:0;
	else if (mode == 4)
		mode = (cl.deathmatch==1)?2:0;

	if (mode == 1)
		return;	//don't change yet.
	if (mode == 2 && !(in_attack.state[pnum]&3))
		return;	//2 changes instantly only when already firing.

	CL_QueueImpulse(pnum, newimp);
}

//+fire 8 7 [keycode]
//does impulse 8 or 7 (according to held weapons) along with a +attack
void IN_FireDown(void)
{
	int pnum = CL_TargettedSplit(false);
	int k;
	int impulse;

	impulse = Cmd_Argc()-1;
	k = atoi(Cmd_Argv(impulse));
	if (k >= 32)
		impulse--;	//scancode, don't treat that arg as a weapon number
	else
		k = -1;

	impulse = IN_BestWeapon_Args(pnum, 1, impulse);
	if (impulse)
		CL_QueueImpulse(pnum, impulse);
	else
		IN_DoPostSelect();

	KeyDown_Scan(&in_attack, NULL, k);
}
static void IN_DoWeaponHide(void)
{
	if (cl_weaponhide.ival && !(cl_weaponhide.ival==2 && cl.deathmatch==1))
	{
		int impulse, best = 0;
		int pnum = CL_TargettedSplit(false);
		char tok[64];
		char *l = cl_weaponhide_preference.string;
		if (!strcmp(l, "0")) l = "2"; //for compat with ezquake's cl_weaponhide_axe cvar.
		while(l && *l)
		{
			l = COM_ParseOut(l, tok, sizeof(tok));
			if (IN_HaveWeapon_Name(pnum, tok, &impulse))
				best = impulse;
		}
		if (best)
		{	//looks like we're switching away
			CL_QueueImpulse(pnum, best);
		}
	}
}
//-fire should trigger an impulse 1 or something.
void IN_FireUp(void)
{
	int k;
	int impulse;

	//any args are used in the +fire version and linger through to the -fire.
	//the only useful one is the keynum.

	impulse = Cmd_Argc()-1;
	k = atoi(Cmd_Argv(impulse));
	if (k >= 32)
		impulse--;	//scancode, don't treat that arg as a weapon number
	else
		k = -1;

	if (KeyUp_Scan(&in_attack, k))
		IN_DoWeaponHide();
}

qboolean IN_WeaponWheelIsShown(void)
{
	if (!(in_wwheel.state[0]&1) || !weaponinfo_count)
		return false;
	return true;
}
qboolean IN_WeaponWheelAccumulate(int pnum, float x, float y, float threshhold) //either mouse or controller
{
	if (!(in_wwheel.state[pnum]&1) || !weaponinfo_count)
		return false;

	if (x*x+y*y > threshhold*threshhold)	//protects against deadzones.
	{
		wwheeldir[pnum][0] += x;
		wwheeldir[pnum][1] += y;
	}
	return true;
}
#include "shader.h"
qboolean IN_DrawWeaponWheel(int pnum)
{
	int w;
	float pos[2], centre[2];
	const float radius = 64;
	float d, a;
	shader_t *s;
	if (!(in_wwheel.state[pnum]&1) || !weaponinfo_count)
		return false;
	R2D_ImageColours(1,1,1,1);

	centre[0] = cl.playerview[pnum].gamerect.x + cl.playerview[pnum].gamerect.width/2;
	centre[1] = cl.playerview[pnum].gamerect.y + cl.playerview[pnum].gamerect.height/2;

	d = DotProduct2(wwheeldir[pnum],wwheeldir[pnum]);
	a = 32;
	if (d > a*a && d)
	{
		wwheeldir[pnum][0] *= a/sqrt(d);
		wwheeldir[pnum][1] *= a/sqrt(d);
	}

	a = atan2(wwheeldir[pnum][1], wwheeldir[pnum][0]);
	w = (a/(2*M_PI)+1) * weaponinfo_count + 0.5;
	w = w % weaponinfo_count;

	if (w != wwheelsel[pnum])
	{
		wwheelseltime[pnum] = realtime;
		wwheelsel[pnum] = w;
	}

	s = R2D_SafeCachePic("gfx/weaponwheel.lmp");
	if (R_GetShaderSizes(s, NULL, NULL, false)>0)
		R2D_Image(centre[0]-radius*2, centre[1]-radius*2, radius*4, radius*4, 0, 0, 1, 1, s);
	for (w = 0; w < weaponinfo_count; w++)
	{
		pos[0] = centre[0] + cos((w*2*M_PI) / weaponinfo_count)*radius;
		pos[1] = centre[1] + sin((w*2*M_PI) / weaponinfo_count)*radius;

		if (weaponinfo[w].icons[0])
		{	//draw a shadow
			R2D_ImageColours(0,0,0,1);
			R2D_Image(pos[0]-24+2, pos[1]-16+2, 48, 32, 0, 0, 1, 1, R2D_SafeCachePic(weaponinfo[w].icons[0]));
		}

		//and the real icon (dark if unavailable)
		if (IN_HaveWeapon_Idx(pnum, w))
			R2D_ImageColours(1,1,1,1);
		else
			R2D_ImageColours(0.2,0.2,0.2,1);
		if (w == wwheelsel[pnum])
			d = 1+sin((realtime - wwheelseltime[pnum])*10);	//make it bounce
		else
			d = 0;
		if (cl.playerview[pnum].stats[STAT_ACTIVEWEAPON] == weaponinfo[w].items_val && weaponinfo[w].icons[1])
			R2D_Image(pos[0]-24-d, pos[1]-16-d, 48, 32, 0, 0, 1, 1, R2D_SafeCachePic(weaponinfo[w].icons[1]));
		else if (weaponinfo[w].icons[0])
			R2D_Image(pos[0]-24-d, pos[1]-16-d, 48, 32, 0, 0, 1, 1, R2D_SafeCachePic(weaponinfo[w].icons[0]));
		else
			Draw_FunStringWidth(pos[0]-32-d, pos[1]-4-d, weaponinfo[w].shortname, 64, 2, cl.playerview[pnum].stats[STAT_ACTIVEWEAPON] == weaponinfo[w].items_val);
	}
	R2D_ImageColours(1,1,1,1);

	w = wwheelsel[pnum];
	if (weaponinfo[w].icons[2])
		R2D_Image(centre[0]-12, centre[1]-12, 24, 24, 0, 0, 1, 1, R2D_SafeCachePic(weaponinfo[w].icons[2]));
	Draw_FunStringWidth(centre[0]-32, centre[1] - 28, weaponinfo[w].shortname, 64, 2, false);
	if (weaponinfo[w].ammostat >= 0)
		Draw_FunStringWidth(centre[0]-32, centre[1] + 20, va("%s%d", (cl.playerview[pnum].stats[weaponinfo[w].ammostat]<20)?S_COLOR_RED:"", cl.playerview[pnum].stats[weaponinfo[w].ammostat]), 64, 2, false);

	pos[0] = centre[0] + cos(a)*radius*0.6;
	pos[1] = centre[1] + sin(a)*radius*0.6;
	Draw_FunString(pos[0], pos[1], "X");
	return true;
}
void IN_WWheelDown (void)
{
	int pnum = CL_TargettedSplit(false);
#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(pnum, Cmd_Argv(0)))
		return;
#endif
	if (!(in_wwheel.state[pnum]&1))
	{
		size_t w;
		for (w = 0; w < weaponinfo_count; w++)
		{
			if (cl.playerview[pnum].stats[STAT_ACTIVEWEAPON] == weaponinfo[w].items_val)
			{	//this is our active weapon. start with it highlighted.
				wwheelseltime[pnum] = realtime;
				wwheelsel[pnum] = w;

				wwheeldir[pnum][0] = cos((wwheelsel[pnum]*2*M_PI) / weaponinfo_count)*16;
				wwheeldir[pnum][1] = sin((wwheelsel[pnum]*2*M_PI) / weaponinfo_count)*16;
				break;
			}
		}
	}
	KeyDown(&in_wwheel, NULL);
}
void IN_WWheelUp (void)
{
	int pnum = CL_TargettedSplit(false);
#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(pnum, Cmd_Argv(0)))
		return;
#endif
	if (!KeyUp(&in_wwheel))
		return;
	if (wwheelsel[pnum] < weaponinfo_count)
		CL_QueueImpulse(pnum, weaponinfo[wwheelsel[pnum]].impulse);
}
void IN_IWheelDown (void)
{
}
void IN_IWheelUp (void)
{
}
#else
#define IN_DoPostSelect()
#define IN_DoWeaponHide()
#endif


//q2e compat. too lazy to use the wwheel info. let the gamecode do it the old way.
void IN_WeapNext_f (void)
{
	CL_SendClientCommand(true, "weapnext");
}
void IN_WeapPrev_f (void)
{
	CL_SendClientCommand(true, "weapprev");
}


static void IN_KLookDown (void) {KeyDown(&in_klook, NULL);}
static void IN_KLookUp (void) {KeyUp(&in_klook);}
static void IN_MLookDown (void) {KeyDown(&in_mlook, NULL);}
static void IN_MLookUp (void)
{
	int pnum = CL_TargettedSplit(false);
	KeyUp(&in_mlook);
	if ( !(in_mlook.state[pnum]&1) &&  lookspring.ival)
		V_StartPitchDrift(&cl.playerview[pnum]);
}
static void IN_UpDown(void) {KeyDown(&in_up, &in_down);}
static void IN_UpUp(void) {KeyUp(&in_up);}
static void IN_DownDown(void) {KeyDown(&in_down, &in_up);}
static void IN_DownUp(void) {KeyUp(&in_down);}
static void IN_LeftDown(void) {KeyDown(&in_left, &in_right);}
static void IN_LeftUp(void) {KeyUp(&in_left);}
static void IN_RightDown(void) {KeyDown(&in_right, &in_left);}
static void IN_RightUp(void) {KeyUp(&in_right);}
static void IN_ForwardDown(void) {KeyDown(&in_forward, &in_back);}
static void IN_ForwardUp(void) {KeyUp(&in_forward);}
static void IN_BackDown(void) {KeyDown(&in_back, &in_forward);}
static void IN_BackUp(void) {KeyUp(&in_back);}
static void IN_LookupDown(void) {KeyDown(&in_lookup, &in_lookdown);}
static void IN_LookupUp(void) {KeyUp(&in_lookup);}
static void IN_LookdownDown(void) {KeyDown(&in_lookdown, &in_lookup);}
static void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
static void IN_MoveleftDown(void) {KeyDown(&in_moveleft, &in_moveright);}
static void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
static void IN_MoverightDown(void) {KeyDown(&in_moveright, &in_moveleft);}
static void IN_MoverightUp(void) {KeyUp(&in_moveright);}
static void IN_RollLeftDown(void) {KeyDown(&in_rollleft, &in_rollright);}
static void IN_RollLeftUp(void) {KeyUp(&in_rollleft);}
static void IN_RollRightDown(void) {KeyDown(&in_rollright, &in_rollleft);}
static void IN_RollRightUp(void) {KeyUp(&in_rollright);}

static void IN_SpeedDown(void) {KeyDown(&in_speed, NULL);}
static void IN_SpeedUp(void) {KeyUp(&in_speed);}
static void IN_StrafeDown(void) {KeyDown(&in_strafe, NULL);}
static void IN_StrafeUp(void) {KeyUp(&in_strafe);}

static void IN_AttackDown(void) {IN_DoPostSelect(); KeyDown(&in_attack, NULL);}
static void IN_AttackUp(void) {if (KeyUp(&in_attack)) IN_DoWeaponHide();}

static void IN_UseDown (void) {KeyDown(&in_use, NULL);}
static void IN_UseUp (void) {KeyUp(&in_use);}
static void IN_JumpDown (void)
{
	qboolean up;
	int pnum = CL_TargettedSplit(false);
	playerview_t *pv = &cl.playerview[pnum];


	up = (cls.state == ca_active && cl_smartjump.ival && !prox_inmenu.ival);
	if (!up)
		up = false;
#ifdef Q2CLIENT
	else if (cls.protocol == CP_QUAKE2)
		up = true;	//always smartjump in q2.
#endif
	else if (pv->spectator && pv->cam_state != CAM_FREECAM)
		up = false;	//if we're tracking, don't confuse stuff.
#ifdef QUAKESTATS
	else if (!pv->spectator && pv->stats[STAT_HEALTH] <= 0)
		up = false;	//don't ever 'swim' when dead.
	else if (pv->pmovetype == PM_FLY || pv->pmovetype == PM_6DOF || pv->pmovetype == PM_SPECTATOR || pv->pmovetype == PM_OLD_SPECTATOR)
		up = true;	//fling/spectating
	else if ((pv->pmovetype == PM_NORMAL || pv->pmovetype == PM_WALLWALK) && pv->waterlevel >= 2 && (!cl.teamfortress || !(in_forward.state[pnum] & 1)))
		up = true;	//swimming. TF only (silently) smartjumps when NOT moving.
#endif
	else
		up = false;

	KeyDown((up?&in_up:&in_jump), &in_down);
}
static void IN_JumpUp (void)
{
	if (cl_smartjump.ival)
		KeyUp(&in_up);
	KeyUp(&in_jump);
}

static void IN_ButtonNDown(void) {KeyDown(&in_button[atoi(Cmd_Argv(0)+7)], NULL);}
static void IN_ButtonNUp(void) {KeyUp(&in_button[atoi(Cmd_Argv(0)+7)]);}

float in_rotate;
static void IN_Rotate_f (void) {in_rotate += atoi(Cmd_Argv(1));}


void IN_WriteButtons(vfsfile_t *f, qboolean all)
{
	int s,b;
	struct
	{
		kbutton_t	*button;
		char		*name;
	} buttons [] =
	{
		{&in_mlook,		"mlook"},
		{&in_klook,		"klook"},
		{&in_left,		"left"},
		{&in_right,		"right"},
		{&in_forward,	"forward"},
		{&in_back,		"back"},
		{&in_lookup,	"lookup"},
		{&in_lookdown,	"lookdown"},
		{&in_moveleft,	"moveleft"},
		{&in_moveright,	"moveright"},
		{&in_strafe,	"strafe"},
		{&in_speed,		"speed"},
		{&in_use,		"use"},
		{&in_jump,		"jump"},
		{&in_attack,	"attack"},
		{&in_rollleft,	"rollleft"},
		{&in_rollright,	"rollright"},
		{&in_up,		"up"},
		{&in_down,		"down"},
	};

	s = 0;
	VFS_PRINTF(f, "\n//Player 1 buttons\n");
	for (b = 0; b < countof(buttons); b++)
	{
		if ((buttons[b].button->state[s]&1) && (buttons[b].button->down[s][0]==-1 || buttons[b].button->down[s][1]==-1))
			VFS_PRINTF(f, "+%s\n", buttons[b].name);
		else if (b || all)
			VFS_PRINTF(f, "-%s\n", buttons[b].name);
	}
	for (b = 0; b < countof(in_button); b++)
	{
		if ((in_button[b].state[s]&1) && (in_button[b].down[s][0]==-1 || in_button[b].down[s][1]==-1))
			VFS_PRINTF(f, "+button%i\n", b);
		else
			VFS_PRINTF(f, "-button%i\n", b);
	}
	for (s = 1; s < MAX_SPLITS; s++)
	{
		VFS_PRINTF(f, "\n//Player %i buttons\n", s);
		for (b = 0; b < countof(buttons); b++)
		{
			if ((buttons[b].button->state[s]&1) && (buttons[b].button->down[s][0]==-1 || buttons[b].button->down[s][1]==-1))
				VFS_PRINTF(f, "+p%i %s\n", s, buttons[b].name);
			else if (b || all)
				VFS_PRINTF(f, "-p%i %s\n", s, buttons[b].name);
		}
		for (b = 0; b < countof(in_button); b++)
		{
			if ((in_button[b].state[s]&1) && (in_button[b].down[s][0]==-1 || in_button[b].down[s][1]==-1))
				VFS_PRINTF(f, "+p%i button%i\n", s, b);
			else
				VFS_PRINTF(f, "-p%i button%i\n", s, b);
		}
	}

	//FIXME: save device remappings to config.
}

//This function incorporates Tonik's impulse  8 7 6 5 4 3 2 1 to select the prefered weapon on the basis of having it.
//It also incorporates split screen input as well as impulse buffering
void IN_Impulse (void)
{
	int newimp;
	int pnum = CL_TargettedSplit(false);

	newimp = Q_atoi(Cmd_Argv(1));

#ifdef QUAKESTATS
	if (Cmd_Argc() > 2)
	{
		int best = IN_BestWeapon_Args(pnum, 1, Cmd_Argc() - 1);
		if (best)
			newimp = best;
	}
#endif

	CL_QueueImpulse(pnum, newimp);
}

void IN_Restart (void)
{
	IN_Shutdown();
	IN_ReInit();

	//FIXME: re-assert explicit device re-mappings
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key, int pnum, qboolean noslowstart)
{
	float		val;
	qboolean	impulsedown, impulseup, down;

	noslowstart = noslowstart && cl_fastaccel.ival;
	
	impulsedown = key->state[pnum] & 2;
	impulseup = key->state[pnum] & 4;
	down = key->state[pnum] & 1;
	val = 0;
	
	if (impulsedown && !impulseup)
	{
		if (down)
			val = noslowstart?1.0:0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();
	}
	if (impulseup && !impulsedown)
	{
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame
	}
	if (!impulsedown && !impulseup)
	{
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame
	}
	if (impulsedown && impulseup)
	{
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame
	}

	key->state[pnum] &= 1;		// clear impulses
	
	return val;
}

void CL_ProxyMenuHook(char *command, kbutton_t *key)
{
	if ((key->state[0] & 3) == 3)	//2 is impulse down, 1 is held down
	{
		key->state[0] = 0;		// clear impulses

		Cbuf_AddText(command, RESTRICT_DEFAULT);
	}
}

void CL_ProxyMenuHooks(void)
{
	if (!prox_inmenu.ival)
		return;

	CL_ProxyMenuHook("say proxy:menu down\n", &in_back);
	CL_ProxyMenuHook("say proxy:menu up\n", &in_forward);

	CL_ProxyMenuHook("say proxy:menu left\n", &in_left);
	CL_ProxyMenuHook("say proxy:menu right\n", &in_right);

	CL_ProxyMenuHook("say proxy:menu left\n", &in_moveleft);
	CL_ProxyMenuHook("say proxy:menu right\n", &in_moveright);

	CL_ProxyMenuHook("say proxy:menu use\n", &in_jump);
}


//==========================================================================

cvar_t	cl_upspeed = CVARF("cl_upspeed","400", CVAR_ARCHIVE);
cvar_t	cl_forwardspeed = CVARF("cl_forwardspeed","400", CVAR_ARCHIVE);
cvar_t	cl_backspeed = CVARFD("cl_backspeed","", CVAR_ARCHIVE, "The base speed that you move backwards at. If empty, uses the value of cl_forwardspeed instead.");
cvar_t	cl_sidespeed = CVARF("cl_sidespeed","400", CVAR_ARCHIVE);

cvar_t	cl_movespeedkey = CVAR("cl_movespeedkey","2.0");

cvar_t	cl_yawspeed = CVAR("cl_yawspeed","140");
cvar_t	cl_pitchspeed = CVAR("cl_pitchspeed","150");

cvar_t	cl_anglespeedkey = CVAR("cl_anglespeedkey","1.5");


#define GATHERBIT(bname,bit)	do{if (bname.state[pnum] & 3)	{bits |=   (1u<<(bit));} bname.state[pnum]	&= ~2;}while(0)
#define UNUSEDBUTTON(bnum)		do{if (in_button[bnum].state[pnum] & 3)	{Con_Printf("+button%i is not supported on this protocol\n", bnum); } in_button[bnum].state[pnum]	&= ~3;}while(0)
void CL_GatherButtons (usercmd_t *cmd, int pnum)
{
	unsigned int bits = 0;
	GATHERBIT(in_attack,		0);
#ifdef Q3CLIENT
	if (cls.protocol==CP_QUAKE3)
	{	//quake3's buttons are nice and simple, buttonN -> bit|=(1<<N)
		int i;
		for (i = 0; i < countof(in_button); i++)
		{
			GATHERBIT(in_button[i],		i);
		}
//		bits |= 1<<1;	//rtcw talking
//		bits |= 1<<4;	//rtcw walking
//		bits |= 1<<7;	//rtcw any key
		cmd->buttons = bits;
		return;
	}
#endif
#ifdef Q2CLIENT
	if (cls.protocol==CP_QUAKE2 && cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
	{	//buttons limited to 8 bits.

		//GATHERBIT(in_attack,		0);	//handled above
		GATHERBIT(in_use,			1);
		//GATHERBIT(in_holster,		2);	//urgh
		//GATHERBIT(in_jump,		3);	//also set by +moveup
		//GATHERBIT(in_crouch,		4);	//also set by +movedown
		//GATHERBIT(in_unused,		5);
		//GATHERBIT(in_unused,		6);
		//GATHERBIT(in_any,			7);	//urgh

		//also let +button stuff map to bits.
		GATHERBIT(in_button[0],		0);
		GATHERBIT(in_button[1],		1);
		GATHERBIT(in_button[2],		2);
		GATHERBIT(in_button[3],		3);
		GATHERBIT(in_button[4],		4);
		GATHERBIT(in_button[5],		5);
		GATHERBIT(in_button[6],		6);
		GATHERBIT(in_button[7],		7);

		UNUSEDBUTTON(8);
		UNUSEDBUTTON(9);
		UNUSEDBUTTON(10);
		UNUSEDBUTTON(11);
		UNUSEDBUTTON(12);
		UNUSEDBUTTON(13);
		UNUSEDBUTTON(14);
		UNUSEDBUTTON(15);
		UNUSEDBUTTON(16);
		UNUSEDBUTTON(17);
		UNUSEDBUTTON(18);
		UNUSEDBUTTON(19);
//		UNUSEDBUTTON(20);

		cmd->buttons |= bits;
		return;
	}
#endif

	//quakec's numbered buttons make no sense and have no sane relation to bit numbers
	GATHERBIT(in_button[0],		0);
	UNUSEDBUTTON(1);				//officially, qc's button1 field is unusable (although qw folds button3 over to it)
	GATHERBIT(in_button[2],		1);	GATHERBIT(in_jump,			1);
	GATHERBIT(in_button[3],		2);
	GATHERBIT(in_button[4],		3);
	GATHERBIT(in_button[5],		4);
	GATHERBIT(in_button[6],		5);
	GATHERBIT(in_button[7],		6);
	GATHERBIT(in_button[8],		7);

	//more inconsistencies, as required for dpcompat.
	GATHERBIT(in_use,			(cls.protocol==CP_QUAKEWORLD)?4:8);
	bits |= (Key_Dest_Has(~kdm_game))	?(1u<<9):0;		//'buttonchat'. game is the lowest priority, anything else will take focus away. we consider that to mean 'chat' (although it could be menus).
	bits |= (cursor_active)				?(1u<<10):0;	//'cursor_active'. prydon cursor stuff.
	GATHERBIT(in_button[9],		11);
	GATHERBIT(in_button[10],	12);
	GATHERBIT(in_button[11],	13);
	GATHERBIT(in_button[12],	14);
	GATHERBIT(in_button[13],	15);

	GATHERBIT(in_button[14],	16);
	GATHERBIT(in_button[15],	17);
	GATHERBIT(in_button[16],	18);
	UNUSEDBUTTON(17);
	UNUSEDBUTTON(18);
	UNUSEDBUTTON(19);
//	UNUSEDBUTTON(20);

//NQ protocol:
//bit 30 means input_weapon field is sent. figured out at time of sending.
//bit 31 means input_cursor* fields are sent. figured out at time of sending.
	cmd->buttons |= bits;
}

void CL_ClearPendingCommands(void)
{
	size_t seat, i;
	memset(&cl_pendingcmd, 0, sizeof(cl_pendingcmd));
	for (seat = 0; seat < countof(cl_pendingcmd); seat++)
	{
		for (i=0 ; i<3 ; i++)
			cl_pendingcmd[seat].angles[i] = ((int)(cl.playerview[seat].viewangles[i]*65536.0/360)&65535);
	}
}
/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (int pnum, double frametime)
{
	float	speed, quant;
	float	up, down;
	
	if (in_speed.state[pnum] & 1)
	{
		if (ruleset_allow_frj.ival)
			speed = cl_anglespeedkey.value;
		else
			speed = bound(-2, cl_anglespeedkey.value, 2);
	}
	else
		speed = 1;

	if (in_rotate && pnum==0 && !(cl.fpd & FPD_LIMIT_YAW))
	{
		quant = in_rotate;
		if (!cl_instantrotate.ival)
			quant *= speed*frametime;
		in_rotate -= quant;
		if (r_xflip.ival)
			quant *= -1;
		if (ruleset_allow_frj.ival)
			cl.playerview[pnum].viewanglechange[YAW] += quant;
	}

	if (!(in_strafe.state[pnum] & 1))
	{
		quant = cl_yawspeed.value*speed;
		if ((cl.fpd & FPD_LIMIT_YAW) || !ruleset_allow_frj.ival)
			quant = bound(-900, quant, 900);
		quant *= frametime;
		if (r_xflip.ival)
			quant *= -1;
		cl.playerview[pnum].viewanglechange[YAW] -= quant * CL_KeyState (&in_right, pnum, false);
		cl.playerview[pnum].viewanglechange[YAW] += quant * CL_KeyState (&in_left, pnum, false);
	}
	if (in_klook.state[pnum] & 1)
	{
		V_StopPitchDrift (&cl.playerview[pnum]);
		quant = cl_pitchspeed.value*speed;
		if ((cl.fpd & FPD_LIMIT_PITCH) || !ruleset_allow_frj.ival)
			quant = bound(-700, quant, 700);
		quant *= frametime;
		cl.playerview[pnum].viewanglechange[PITCH] -= quant * CL_KeyState (&in_forward, pnum, false);
		cl.playerview[pnum].viewanglechange[PITCH] += quant * CL_KeyState (&in_back, pnum, false);
	}

	quant = cl_rollspeed.value*speed;
	quant *= frametime;
	cl.playerview[pnum].viewanglechange[ROLL] -= quant * CL_KeyState (&in_rollleft, pnum, false);
	cl.playerview[pnum].viewanglechange[ROLL] += quant * CL_KeyState (&in_rollright, pnum, false);
	
	up = CL_KeyState (&in_lookup, pnum, false);
	down = CL_KeyState(&in_lookdown, pnum, false);

	quant = cl_pitchspeed.value*speed;
	if ((cl.fpd & FPD_LIMIT_PITCH) || !ruleset_allow_frj.ival)
		quant = bound(-700, quant, 700);
	quant *= frametime;
	cl.playerview[pnum].viewanglechange[PITCH] -= quant * up;
	cl.playerview[pnum].viewanglechange[PITCH] += quant * down;

	if (up || down)
		V_StopPitchDrift (&cl.playerview[pnum]);	
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
#ifdef _DIII4A //karin: analog move on Android
extern void IN_Analog(vec3_t moves, const float forwardspeed, const float backspeed, const float sidespeed);
#endif
static void CL_BaseMove (vec3_t moves, int pnum)
{
	float fwdspeed = cl_forwardspeed.value;
	float sidespeed = cl_sidespeed.value;
	float backspeed = (*cl_backspeed.string?cl_backspeed.value:cl_forwardspeed.value);
	float upspeed = (*cl_backspeed.string?cl_backspeed.value:cl_forwardspeed.value);
	float scale = 1;
//
// adjust for speed key
//
#ifdef HEXEN2
	extern qboolean	sbar_hexen2;
	if (sbar_hexen2)
	{	//hexen2 is a bit different. forwardspeed is treated as something of a boolean and we need to be able to cope with the boots-of-speed without forcing it always. not really sure why that's clientside instead of serverside, but oh well. evilness.
		scale = cl.playerview[pnum].statsf[STAT_H2_HASTED];
		if (!scale)
			scale = 1;
		if (((in_speed.state[pnum] & 1) ^ (cl_run.ival || fwdspeed > 200))
			&& scale <= 1)	//don't go super fast with speed boots.
			scale *= cl_movespeedkey.value;
		fwdspeed = backspeed = 200;
		sidespeed = 225;
	}
	else
#endif
	if ((in_speed.state[pnum] & 1) ^ cl_run.ival)
		scale *= cl_movespeedkey.value;

	if (r_xflip.ival)
		sidespeed *= -1;

	moves[0] = 0;
	if (! (in_klook.state[pnum] & 1) )
	{
		moves[0] += (fwdspeed * CL_KeyState (&in_forward, pnum, true) -
					backspeed * CL_KeyState (&in_back, pnum, true));
	}
	moves[1] = sidespeed * (CL_KeyState (&in_moveright, pnum, true) - CL_KeyState (&in_moveleft, pnum, true)) * (in_xflip.ival?-1:1);
	if (in_strafe.state[pnum] & 1)
		moves[1] += sidespeed * (CL_KeyState (&in_right, pnum, true) - CL_KeyState (&in_left, pnum, true)) * (in_xflip.ival?-1:1);
	moves[2] = upspeed * (CL_KeyState (&in_up, pnum, true) - CL_KeyState (&in_down, pnum, true));

#ifdef _DIII4A //karin: analog move on Android
	IN_Analog(moves, fwdspeed, backspeed, sidespeed);
#endif
	moves[0] *= scale;
	moves[1] *= scale;
	moves[2] *= scale;
}

void CL_ClampPitch (int pnum, float frametime)
{
	float mat[16];
	float roll;
	playerview_t *pv = &cl.playerview[pnum];

	if (cl.intermissionmode != IM_NONE)
	{
		memset(pv->viewanglechange, 0, sizeof(pv->viewanglechange));
		return;
	}
 	if (pv->pmovetype == PM_6DOF)
	{
//		vec3_t impact;
//		vec3_t norm;
		float mat2[16];
//		vec3_t cross;
		vec3_t view[4];
//		float dot;
		AngleVectors(pv->viewangles, view[0], view[1], view[2]);
		Matrix4x4_RM_FromVectors(mat, view[0], view[1], view[2], vec3_origin);

		Matrix4_Multiply(Matrix4x4_CM_NewRotation(-pv->viewanglechange[PITCH], 0, 1, 0), mat, mat2);
		Matrix4_Multiply(Matrix4x4_CM_NewRotation(pv->viewanglechange[YAW], 0, 0, 1), mat2, mat);
#if 1
		//roll angles
		Matrix4_Multiply(Matrix4x4_CM_NewRotation(pv->viewanglechange[ROLL], 1, 0, 0), mat, mat2);
#else
		//auto-roll
		Matrix3x4_RM_ToVectors(mat, view[0], view[1], view[2], view[3]);

		VectorMA(pv->simorg, -48, view[2], view[3]);
		if (!TraceLineN(pv->simorg, view[3], impact, norm))
		{
			norm[0] = 0;
			norm[1] = 0;
			norm[2] = 1;
		}

		/*keep the roll relative to the 'ground'*/
		CrossProduct(norm, view[2], cross);
		dot = DotProduct(view[0], cross);
		roll = timestep * 360 * -(dot);
		Matrix4_Multiply(Matrix4x4_CM_NewRotation(roll, 1, 0, 0), mat, mat2);
#endif
		Matrix3x4_RM_ToVectors(mat2, view[0], view[1], view[2], view[3]);
		VectorAngles(view[0], view[2], pv->viewangles, false);
		VectorClear(pv->viewanglechange);

		//fixme: in_vraim stuff
		VectorCopy(pv->viewangles, pv->aimangles);

		return;
	}
#if 1
	if ((pv->gravitydir[2] != -1 || pv->viewangles[2]))
	{
		float surfm[16], invsurfm[16];
		float viewm[16];
		vec3_t view[4];
		vec3_t surf[3];
		vec3_t vang;
		void PerpendicularVector( vec3_t dst, const vec3_t src );

		/*calc current view matrix relative to the surface*/
		AngleVectors(pv->viewangles, view[0], view[1], view[2]);
		VectorNegate(view[1], view[1]);

		/*calculate the surface axis with up from the pmove code and right/forwards relative to the player's directions*/
		if (!pv->gravitydir[0] && !pv->gravitydir[1] && !pv->gravitydir[2])
		{
			VectorSet(surf[2], 0, 0, 1);
		}
		else
		{
			VectorNegate(pv->gravitydir, surf[2]);
		}
		VectorNormalize(surf[2]);
		PerpendicularVector(surf[1], surf[2]);
		VectorNormalize(surf[1]);
		CrossProduct(surf[2], surf[1], surf[0]);
		VectorNegate(surf[0], surf[0]);
		VectorNormalize(surf[0]);
		Matrix4x4_RM_FromVectors(surfm, surf[0], surf[1], surf[2], vec3_origin);
		Matrix3x4_InvertTo4x4_Simple(surfm, invsurfm);

		/*calc current view matrix relative to the surface*/
		Matrix4x4_RM_FromVectors(viewm, view[0], view[1], view[2], vec3_origin);
		Matrix4_Multiply(viewm, invsurfm, mat);
		/*convert that back to angles*/
		Matrix3x4_RM_ToVectors(mat, view[0], view[1], view[2], view[3]);
		VectorAngles(view[0], view[2], vang, false);

		/*edit it*/
		vang[PITCH] += pv->viewanglechange[PITCH];
		vang[YAW] += pv->viewanglechange[YAW];
		if (vang[PITCH] <= -180)
			vang[PITCH] += 360;
		if (vang[PITCH] > 180)
			vang[PITCH] -= 360;
		if (vang[ROLL] >= 180)
			vang[ROLL] -= 360;
		if (vang[ROLL] < -180)
			vang[ROLL] += 360;

		/*keep the player looking relative to their ground (smoothlyish)*/
		if (!vang[ROLL])
		{
			if (!pv->viewanglechange[PITCH] && !pv->viewanglechange[YAW] && !pv->viewanglechange[ROLL])
			{
				VectorCopy(pv->viewangles, pv->aimangles);
				return;
			}
		}
		else
		{
			if (fabs(vang[ROLL]) < frametime*180)
				vang[ROLL] = 0;
			else if (vang[ROLL] > 0)
			{
//				Con_Printf("Roll %f\n", vang[ROLL]);
				vang[ROLL] -= frametime*180;
			}
			else
			{
//				Con_Printf("Roll %f\n", vang[ROLL]);
				vang[ROLL] += frametime*180;
			}
		}
		VectorClear(pv->viewanglechange);
		/*clamp pitch*/
		if (vang[PITCH] > cl.maxpitch)
			vang[PITCH] = cl.maxpitch;
		if (vang[PITCH] < cl.minpitch)
			vang[PITCH] = cl.minpitch;

		/*turn those angles back to a matrix*/
		AngleVectors(vang, view[0], view[1], view[2]);
		VectorNegate(view[1], view[1]);
		Matrix4x4_RM_FromVectors(mat, view[0], view[1], view[2], vec3_origin);
		/*rotate back into world space*/
		Matrix4_Multiply(mat, surfm, viewm);
		/*and figure out the final result*/
		Matrix3x4_RM_ToVectors(viewm, view[0], view[1], view[2], view[3]);
		VectorAngles(view[0], view[2], cl.playerview[pnum].viewangles, false);

		if (pv->viewangles[ROLL] >= 360)
			pv->viewangles[ROLL] -= 360;
		if (pv->viewangles[ROLL] < 0)
			pv->viewangles[ROLL] += 360;
		if (pv->viewangles[PITCH] < -180)
			pv->viewangles[PITCH] += 360;

		//fixme: in_vraim stuff
		VectorCopy(pv->viewangles, pv->aimangles);
		return;
	}
#endif
	pv->viewangles[PITCH] += pv->viewanglechange[PITCH];
	pv->viewangles[YAW] += pv->viewanglechange[YAW];
	pv->viewangles[ROLL] += pv->viewanglechange[ROLL];
	pv->viewangles[YAW] /= 360;
	pv->viewangles[YAW] = pv->viewangles[YAW] - (int)pv->viewangles[YAW];
	pv->viewangles[YAW] *= 360;
	VectorClear(pv->viewanglechange);

	if (in_vraim.ival && (pv->vrdev[VRDEV_HEAD].status&VRSTATUS_ANG))
	{	//overcomplicated code to replace the pitch+roll angles and add to the yaw angle.
#if 0
		matrix3x4 base, head, res;
		vec3_t na = {0, pv->viewangles[YAW], 0};
		vec3_t f,l,u,o;
		Matrix3x4_RM_FromAngles(na, vec3_origin, base[0]);
		for (i=0 ; i<3 ; i++)
			na[i] = SHORT2ANGLE(pv->vrdev[VRDEV_HEAD].angles[i]);
		Matrix3x4_RM_FromAngles(na, pv->vrdev[VRDEV_HEAD].origin, head[0]);
		Matrix3x4_Multiply(head[0], base[0], res[0]);
		Matrix3x4_RM_ToVectors(res[0], f,l,u,o);
		VectorAngles(f,u,pv->aimangles,false);
		for (i=0 ; i<3 ; i++)
			cmd->angles[i] = ANGLE2SHORT(na[i]);
#else
		pv->aimangles[PITCH] = SHORT2ANGLE(pv->vrdev[VRDEV_HEAD].angles[PITCH]);
		pv->aimangles[YAW]   = SHORT2ANGLE(pv->vrdev[VRDEV_HEAD].angles[YAW]) + pv->viewangles[YAW];
		pv->aimangles[ROLL]  = SHORT2ANGLE(pv->vrdev[VRDEV_HEAD].angles[ROLL]);
#endif
	}
	else
		VectorCopy(pv->viewangles, pv->aimangles);

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		float	pitch;
		pitch = SHORT2ANGLE(cl.q2frame.seat[pnum].playerstate.pmove.delta_angles[PITCH]);
		if (pitch > 180)
			pitch -= 360;

		if (pv->viewangles[PITCH] + pitch < -360)
			pv->viewangles[PITCH] += 360; // wrapped
		if (pv->viewangles[PITCH] + pitch > 360)
			pv->viewangles[PITCH] -= 360; // wrapped

		if (pv->viewangles[PITCH] + pitch > cl.maxpitch)
			pv->viewangles[PITCH] = cl.maxpitch - pitch;
		if (pv->viewangles[PITCH] + pitch < cl.minpitch)
			pv->viewangles[PITCH] = cl.minpitch - pitch;
	}
	else
#endif
#ifdef Q3CLIENT
		if (cls.protocol == CP_QUAKE3)	//q3 expects the cgame to do it
	{
			//no-op
	}
	else
#endif
	{
		if (pv->viewangles[PITCH] > cl.maxpitch)
			pv->viewangles[PITCH] = cl.maxpitch;
		if (pv->viewangles[PITCH] < cl.minpitch)
			pv->viewangles[PITCH] = cl.minpitch;

		if (pv->aimangles[PITCH] > cl.maxpitch)
			pv->aimangles[PITCH] = cl.maxpitch;
		if (pv->aimangles[PITCH] < cl.minpitch)
			pv->aimangles[PITCH] = cl.minpitch;
	} 

//	if (cl.viewangles[pnum][ROLL] > 50)
//		cl.viewangles[pnum][ROLL] = 50;
//	if (cl.viewangles[pnum][ROLL] < -50)
//		cl.viewangles[pnum][ROLL] = -50;
	roll = frametime*pv->viewangles[ROLL]*30;
	if ((pv->viewangles[ROLL]-roll < 0) != (pv->viewangles[ROLL]<0))
		pv->viewangles[ROLL] = 0;
	else
		pv->viewangles[ROLL] -= frametime*pv->viewangles[ROLL]*3;
}

/*
==============
CL_FinishMove
==============
*/
static void CL_FinishMove (usercmd_t *cmd, int pnum)
{
	int	i;

	CL_ClampPitch(pnum, 0);

//
// always dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (cl.movesequence <= 2)
	{
		cmd->buttons = 0;
		return;
	}
//
// figure button bits
//

	CL_GatherButtons(cmd, pnum);

	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = (int)(ANGLE2SHORT(cl.playerview[pnum].aimangles[i]))&65535;
	cmd->vr[VRDEV_LEFT] = cl.playerview[pnum].vrdev[VRDEV_LEFT];
	cmd->vr[VRDEV_RIGHT] = cl.playerview[pnum].vrdev[VRDEV_RIGHT];
	cmd->vr[VRDEV_HEAD] = cl.playerview[pnum].vrdev[VRDEV_HEAD];

	if (in_impulsespending[pnum] && !cl.paused)
	{
		cmd->impulse = in_impulse[pnum][(in_nextimpulse[pnum])%IN_IMPULSECACHE];
		in_nextimpulse[pnum]++;
		in_impulsespending[pnum]--;
	}
	else
		cmd->impulse = 0;
}


static void CL_AccumlateInput(int plnum, float frametime/*extra contribution*/, float framemsecs/*total accumulated*/)
{
	usercmd_t *cmd = &cl_pendingcmd[plnum];
	int i;
	static vec3_t mousemovements[MAX_SPLITS];
	vec3_t newmoves;

	float nscale = framemsecs?framemsecs / (framemsecs+cmd->msec):0;
	float oscale = 1 - nscale;
	unsigned int st;

	CL_BaseMove (newmoves, plnum);

	CL_AdjustAngles (plnum, frametime);
	if (!cmd->msec)
		VectorClear(mousemovements[plnum]);
	IN_Move (mousemovements[plnum], newmoves, plnum, frametime);
	CL_ClampPitch(plnum, frametime);

	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ((int)(cl.playerview[plnum].viewangles[i]*65536.0/360)&65535);

	cmd->fservertime = cl.servertime;
	cmd->servertime = cl.time*1000;
#ifdef CSQC_DAT
	cmd->fclienttime = realtime - cl.mapstarttime;
#endif

	cmd->forwardmove = bound(-32768, cmd->forwardmove*oscale + newmoves[0]*nscale + mousemovements[plnum][0], 32767);
	cmd->sidemove = bound(-32768, cmd->sidemove*oscale + newmoves[1]*nscale + mousemovements[plnum][1], 32767);
	cmd->upmove = bound(-32768, cmd->upmove*oscale + newmoves[2]*nscale + mousemovements[plnum][2], 32767);

	if (!cmd->msec && framemsecs)
	{
		CL_GatherButtons(cmd, plnum);	//buttons are from the initial state. don't blend them.

		CL_FinishMove(cmd, plnum);
		Cbuf_Waited();	//its okay to stop waiting now
	}
	cmd->msec = framemsecs;

	if (cl.movesequence >= 1)
	{	//fix up the servertime value to make sure our msecs are actually correct.
		st = cl.outframes[(cl.movesequence-1)&UPDATE_MASK].cmd[plnum].servertime + (cmd->msec);	//round it.
		if (abs((int)st-(int)cmd->servertime) < 50)
		{
			cmd->servertime = st;
			cmd->fservertime = (double)st/1000.0;
		}
	}

	// if we are spectator, try autocam
//	if (cl.spectator)
	Cam_Track(&cl.playerview[plnum], &cl_pendingcmd[plnum]);
	Cam_FinishMove(&cl.playerview[plnum], &cl_pendingcmd[plnum]);
}


static qboolean CLFTE_SendVRCmd (sizebuf_t *buf, unsigned int seats)
{
	//compute the delay between receiving the frame we're acking and when we're sending the new frame
	unsigned int cldelay = (realtime - cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].receivedtime)*10000;	//this is to report actual network latency instead of just reporting our packet rate (framerates may still be a factor).
	unsigned int lost = CL_CalcNet(r_netgraph.value);	//report packetloss
	unsigned int flags = 0;
	unsigned int first = cl.ackedmovesequence+1;	//no point resending that which has already been acked.
	unsigned int last = cl.movesequence+1;			//we want to ignore moveseq itself
	unsigned int frame, seat, count, i;
	const usercmd_t *from, *to;
	qboolean dontdrop = false;
	if (first > last)
		first = last-1;
	if (first < last-(countof(cl.outframes)-2))
		first = last-(countof(cl.outframes)-2);
	if (first < 1)
		first = 1;
	if (last < first)
		count = 0;
	else
		count = last-first;
	if (count > max(1,cl_c2sMaxRedundancy.ival))
		count = max(1,cl_c2sMaxRedundancy.ival);
	if (cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].receivedtime<0)
		cldelay = 0;	//erk?

	MSG_WriteByte (buf, clcfte_move);

#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE)	//nq uses fully separate packet+movement sequences (unlike qw).
		MSG_WriteShort(buf, (last-1)&0xffff);
#endif

	if (seats!=1)
		flags |= VRM_SEATS;
	if (lost)
		flags |= VRM_LOSS;
	if (cldelay)
		flags |= VRM_DELAY;
	if (count!=3)
		flags |= VRM_FRAMES;
	if (cl.numackframes)
		flags |= VRM_ACKS;
	MSG_WriteUInt64 (buf, flags);

	if (flags & VRM_SEATS)
		MSG_WriteUInt64 (buf, seats);
	if (flags & VRM_FRAMES)
		MSG_WriteUInt64 (buf, count);
	if (flags & VRM_LOSS)
		MSG_WriteByte (buf, (qbyte)lost);
	if (flags & VRM_DELAY)
		MSG_WriteByte (buf, bound(0,cldelay,255));	//a byte should always be enough for any framerate above 40, and we don't want peole to be able to lie so easily.
	if (flags & VRM_ACKS)
	{
		MSG_WriteUInt64(buf, cl.numackframes);
		for (i = 0; i < cl.numackframes; i++)
			MSG_WriteLong(buf, cl.ackframes[i]);
		cl.numackframes = 0;
	}

	for (seat = 0; seat < seats; seat++)
	{
		from = &nullcmd;
		for (frame = last-count; frame < last; frame++)
		{
			to = &cl.outframes[frame&UPDATE_MASK].cmd[seat];
			MSGFTE_WriteDeltaUsercmd (buf, cl.playerview[seat].baseangles, from, to);
			if (to->impulse && (int)(last-frame)>=cl_c2sImpulseBackup.ival)
				dontdrop = true;
			from = to;
		}
	}
	return dontdrop;
}


void CL_UpdatePrydonCursor(usercmd_t *from, int pnum)
{
	int hit;
	vec3_t cursor_end;

	vec3_t temp;
	vec3_t cursor_impact_normal;

	cursor_active = true;

	if (!cl_prydoncursor.ival)
	{	//center the cursor
		from->cursor_screen[0] = 0;
		from->cursor_screen[1] = 0;
	}
	else
	{
		from->cursor_screen[0] = mousecursor_x/(vid.width/2.0f) - 1;
		from->cursor_screen[1] = mousecursor_y/(vid.height/2.0f) - 1;
		if (from->cursor_screen[0] < -1)
			from->cursor_screen[0] = -1;
		if (from->cursor_screen[1] < -1)
			from->cursor_screen[1] = -1;

		if (from->cursor_screen[0] > 1)
			from->cursor_screen[0] = 1;
		if (from->cursor_screen[1] > 1)
			from->cursor_screen[1] = 1;
	}

	VectorClear(from->cursor_start);
	temp[0] = (from->cursor_screen[0]+1)/2;
	temp[1] = (-from->cursor_screen[1]+1)/2;
	temp[2] = 1;

	VectorCopy(r_origin, from->cursor_start);
	Matrix4x4_CM_UnProject(temp, cursor_end, cl.playerview[pnum].viewangles, from->cursor_start, r_refdef.fov_x, r_refdef.fov_y);

	CL_SetSolidEntities();
	//don't bother with players, they don't exist in NQ...

	CL_TraceLine(from->cursor_start, cursor_end, from->cursor_impact, cursor_impact_normal, &hit);
	if (hit>0)
		from->cursor_entitynumber = hit;
	else if (hit < 0)
		from->cursor_entitynumber = 0;	//FIXME: ask csqc for the entity's entnum
	else
		from->cursor_entitynumber = 0;

//	P_RunParticleEffect(cursor_impact, vec3_origin, 15, 16);
}

#ifdef NQPROT
void CLNQ_SendMove (usercmd_t *cmd, int pnum, sizebuf_t *buf)
{
	int i;
	unsigned int bits;

	if (cls.demoplayback!=DPB_NONE)
		return;	//err... don't bother... :)
//
// always dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (cl.movesequence <= 2 || cls.state == ca_connected)
	{
		MSG_WriteByte (buf, clc_nop);
		return;
	}

	if (cls.qex)
	{
		MSG_WriteByte (buf, clc_delta);
		MSG_WriteULEB128(buf, cl.movesequence);
	}

	MSG_WriteByte (buf, clc_move);

	if (cls.protocol_nq >= CPNQ_DP7)
	{
		if (!cl_movement.ival)
			MSG_WriteLong(buf, 0);
		else
			MSG_WriteLong(buf, cl.movesequence);
	}
	else if (cls.fteprotocolextensions2 & PEXT2_PREDINFO)
		MSG_WriteShort(buf, cl.movesequence&0xffff);

	MSG_WriteFloat (buf, cmd->fservertime);	// use latest time. because ping reports!

	if (cls.qex)
		MSG_WriteByte(buf, 1);

	for (i=0 ; i<3 ; i++)
	{
		if (cls.protocol_nq == CPNQ_FITZ666 || (cls.proquake_angles_hack && buf->prim.anglesize <= 1))
		{
			//fitz/proquake protocols are always 16bit for this angle and 8bit elsewhere. rmq is always at least 16bit
			//the above logic should satify everything.
			MSG_WriteAngle16 (buf, cl.playerview[pnum].viewangles[i]);
		}
		else
			MSG_WriteAngle (buf, cl.playerview[pnum].viewangles[i]);
	}

	MSG_WriteShort (buf, cmd->forwardmove);
	MSG_WriteShort (buf, cmd->sidemove);
	MSG_WriteShort (buf, cmd->upmove);

	bits = cmd->buttons;
	if (cls.fteprotocolextensions2 & PEXT2_PRYDONCURSOR)
	{
		if (cmd->cursor_screen[0] || cmd->cursor_screen[1] ||
			cmd->cursor_start[0] || cmd->cursor_start[1] || cmd->cursor_start[2] ||
			cmd->cursor_impact[0] || cmd->cursor_impact[1] || cmd->cursor_impact[2] ||
			cmd->cursor_entitynumber)
			bits |= (1u<<31);	//set it if there's actually something to send.
		MSG_WriteLong (buf, bits);
	}
	else if (cls.protocol_nq >= CPNQ_DP6)
	{
		MSG_WriteLong (buf, bits);
		bits |= (1u<<31);	//unconditionally set it (without writing it)
	}
	else
		MSG_WriteByte (buf, cmd->buttons);
	MSG_WriteByte (buf, cmd->impulse);

	if (bits & (1u<<31))
	{
		MSG_WriteShort (buf, cmd->cursor_screen[0] * 32767.0f);
		MSG_WriteShort (buf, cmd->cursor_screen[1] * 32767.0f);
		MSG_WriteFloat (buf, cmd->cursor_start[0]);
		MSG_WriteFloat (buf, cmd->cursor_start[1]);
		MSG_WriteFloat (buf, cmd->cursor_start[2]);
		MSG_WriteFloat (buf, cmd->cursor_impact[0]);
		MSG_WriteFloat (buf, cmd->cursor_impact[1]);
		MSG_WriteFloat (buf, cmd->cursor_impact[2]);
		MSG_WriteEntity (buf, cmd->cursor_entitynumber);
	}
}

void QDECL Name_Callback(struct cvar_s *var, char *oldvalue)
{
	if (cls.state <= ca_connected)
		return;

	if (cls.protocol != CP_NETQUAKE)
		return;

	CL_SendClientCommand(true, "name \"%s\"\n", var->string);
}

void CLNQ_SendCmd(sizebuf_t *buf)
{
	int i;
	int seat;
	usercmd_t *cmd;

	i = cl.movesequence & UPDATE_MASK;
	cl.outframes[i].senttime = realtime;
	cl.outframes[i].latency = -1;
	cl.outframes[i].server_message_num = cl.validsequence;
	cl.outframes[i].cmd_sequence = cl.movesequence;
	cl.outframes[i].sentgametime = cl.movesequence_time;

	for (seat = 0; seat < cl.splitclients; seat++)
	{
		cmd = &cl.outframes[i].cmd[seat];
		*cmd = cl_pendingcmd[seat];
		cmd->fservertime = cl.movesequence_time;
//		cmd->msec = (cl.time - cl.outframes[(i-1)&UPDATE_MASK].sentgametime)*1000;
#ifdef CSQC_DAT
		CSQC_Input_Frame(seat, cmd);
#endif
	}
	CL_ClearPendingCommands();

	//inputs are only sent once we receive an entity.
	if (cls.fteprotocolextensions2 & PEXT2_VRINPUTS)
		CLFTE_SendVRCmd(buf, (cls.signon != 4 || cls.state == ca_connected)?0:cl.splitclients);
	else
	{
		if (cls.signon == 4)
		{
			for (seat = 0; seat < cl.splitclients; seat++)
			{
				// send the unreliable message
	//			if (independantphysics[seat].impulse && !cls.netchan.message.cursize)
	//				CLNQ_SendMove (&cl.outframes[i].cmd[seat], seat, &cls.netchan.message);
	//			else
					CLNQ_SendMove (&cl.outframes[i].cmd[seat], seat, buf);
			}
		}
		else
			MSG_WriteByte (buf, clc_nop);

		for (i = 0; i < cl.numackframes; i++)
		{
			MSG_WriteByte(buf, clcdp_ackframe);
			MSG_WriteLong(buf, cl.ackframes[i]);
		}
		cl.numackframes = 0;
	}
}
#else
void Name_Callback(struct cvar_s *var, char *oldvalue)
{

}
#endif

float CL_FilterTime (double time, float wantfps, float limit, qboolean ignoreserver)	//now returns the extra time not taken in this slot. Note that negative 1 means uncapped.
{
	float fps, fpscap;

	if (cls.timedemo)
		return -1;

	if (cls.protocol == CP_QUAKE3)
		ignoreserver = true;

	/*ignore the server if we're playing demos, sending to the server only as replies, or if its meant to be disabled (netfps depending on where its called from)*/
	if (cls.demoplayback != DPB_NONE || (cls.protocol != CP_QUAKEWORLD && cls.protocol != CP_NETQUAKE) || ignoreserver)
	{
		if (!wantfps)
			return -1;
		fps = max (1.0, wantfps);
	}
	else
	{
		fpscap = cls.maxfps ? max (30.0, cls.maxfps) : 0x7fff;
#ifdef IRCCONNECT
		if (cls.netchan.remote_address.type == NA_IRC)
			fps = bound (0.1, wantfps, fpscap);	//if we're connected via irc, allow a greatly reduced minimum cap
		else
#endif
		if (wantfps < 1)
			fps = fpscap;
		else
			fps = bound (6.7, wantfps, fpscap);	//we actually cap ourselves to 150msecs (1000/7 = 142)
	}

	//its not time yet
	if (ignoreserver)
	{	//don't try to hold to milliseconds.
		if (time < 1000 / fps)
			return 0;
	}
	else
	{
		if (time < ceil(1000 / fps))
			return 0;
	}

	//clamp it if we have over 1.5 frame banked somehow
	if (limit && time - (1000 / fps) > (1000 / fps)*limit)
		return (1000 / fps) * limit;

	//report how much spare time the caller now has
	return time - (1000 / fps);
}

typedef struct clcmdbuf_s {
	struct clcmdbuf_s *next;
	int len;
	qboolean reliable;
	unsigned int seat;
	char command[4];	//this is dynamically allocated, so this is variably sized.
} clcmdbuf_t;
static clcmdbuf_t *clientcmdlist;
void VARGS CL_SendSeatClientCommand(qboolean reliable, unsigned int seat, char *format, ...)
{
	qboolean oldallow;
	va_list		argptr;
	char		string[2048];
	clcmdbuf_t *buf, *prev;

	if (cls.demoplayback && !(cls.demoplayback == DPB_MVD && cls.demoeztv_ext))
		return;	//no point.

	va_start (argptr, format);
	Q_vsnprintfz (string,sizeof(string), format,argptr);
	va_end (argptr);

#ifdef Q3CLIENT
	if (cls.protocol == CP_QUAKE3)
	{
		q3->cl.SendClientCommand("%s", string);
		return;
	}
#endif

	oldallow = CL_AllowIndependantSendCmd(false);

	buf = Z_Malloc(sizeof(*buf)+strlen(string));
	strcpy(buf->command, string);
	buf->len = strlen(buf->command);
	buf->reliable = reliable;
	buf->seat = seat;

	//add to end of the list so that the first of the list is the first to be sent.
	if (!clientcmdlist)
		clientcmdlist = buf;
	else
	{
		for (prev = clientcmdlist; prev->next; prev=prev->next)
			;
		prev->next = buf;
	}

	CL_AllowIndependantSendCmd(oldallow);
}
void VARGS CL_SendClientCommand(qboolean reliable, char *format, ...)
{
	va_list		argptr;
	char		string[2048];

	va_start (argptr, format);
	Q_vsnprintfz (string,sizeof(string), format,argptr);
	va_end (argptr);

	CL_SendSeatClientCommand(reliable, 0, "%s", string);
}

//sometimes a server will quickly restart twice.
//connected clients will then receive TWO 'new' commands - both with the same servercount value.
//the connection process then tries to proceed with two sets of commands until it fails catastrophically.
//by attempting to strip out dupe commands we can usually avoid the issue
//note that FTE servers track progress properly, so this is not an issue for us, but in the interests of compat with mvdsv...
//however, FTE servers can send a little faster, so warnings about this can be awkward.
int CL_RemoveClientCommands(char *command)
{
	clcmdbuf_t *next, *first;
	int removed = 0;
	int len = strlen(command);

	CL_AllowIndependantSendCmd(false);

	if (!clientcmdlist)
		return 0;

	while(!strncmp(clientcmdlist->command, command, len))
	{
		next = clientcmdlist->next;
		Z_Free(clientcmdlist);
		clientcmdlist=next;
		removed++;

		if (!clientcmdlist)
			return removed;
	}
	first = clientcmdlist;
	while(first->next)
	{
		if (!strncmp(first->next->command, command, len))
		{
			next = first->next->next;
			Z_Free(first->next);
			first->next = next;
			removed++;
		}
		else
			first = first->next;
	}

	return removed;
}

void CL_FlushClientCommands(void)
{
	clcmdbuf_t *next;
	CL_AllowIndependantSendCmd(false);

	while(clientcmdlist)
	{
		Con_DPrintf("Flushed command %s\n", clientcmdlist->command);
		next = clientcmdlist->next;
		Z_Free(clientcmdlist);
		clientcmdlist=next;
	}
}

qboolean runningindepphys;
#ifdef MULTITHREAD
void *indeplock;
void *indepthread;

qboolean allowindepphys;
qboolean CL_AllowIndependantSendCmd(qboolean allow)
{
	qboolean ret = allowindepphys;
	if (!runningindepphys)
		return ret;

	if (allowindepphys != allow && runningindepphys)
	{
		if (allow)
			Sys_UnlockMutex(indeplock);
		else
			Sys_LockMutex(indeplock);
		allowindepphys = allow;
	}
	return ret;
}

int CL_IndepPhysicsThread(void *param)
{
	double sleeptime;
	double fps;
	double time, lasttime;
	double spare;
	lasttime = Sys_DoubleTime();
	while(runningindepphys)
	{
		time = Sys_DoubleTime();
		spare = CL_FilterTime((time - lasttime)*1000, cl_netfps.value, 1.5, false);
		if (spare)
		{
			time -= spare/1000.0f;
			Sys_LockMutex(indeplock);
			if (cls.state)
				CL_SendCmd(time - lasttime, false);
			lasttime = time;
			Sys_UnlockMutex(indeplock);
		}

		fps = cl_netfps.value;
		if (fps < 4)
			fps = 4;
		while (fps < 100)
			fps*=2;

		sleeptime = 1/fps;

		Sys_Sleep(sleeptime);
	}
	return 0;
}

void CL_UseIndepPhysics(qboolean allow)
{
	if (runningindepphys == allow)
		return;

	if (allow)
	{	//enable it
		indeplock = Sys_CreateMutex();
		runningindepphys = true;

		indepthread = Sys_CreateThread("indepphys", CL_IndepPhysicsThread, NULL, THREADP_HIGHEST, 8192);
		allowindepphys = true;
	}
	else
	{
		CL_AllowIndependantSendCmd(true);
		//shut it down.
		runningindepphys = false;	//tell thread to exit gracefully
		Sys_WaitOnThread(indepthread);
		indepthread = NULL;
		Sys_DestroyMutex(indeplock);
		indeplock = NULL;
	}
}
#else
qboolean CL_AllowIndependantSendCmd(qboolean allow)
{
	return false;
}
void CL_UseIndepPhysics(qboolean allow)
{
}
#endif

void CL_UpdateSeats(void)
{
	if (!cls.netchan.message.cursize && cl.allocated_client_slots > 1 && cls.state == ca_active && cl.splitclients && (cls.fteprotocolextensions & PEXT_SPLITSCREEN) && cl.worldmodel)
	{
		int targ = bound(1, cl_splitscreen.ival+1, MAX_SPLITS);
		if (cl.splitclients < targ)
		{
			char *ver;
			char buffer[2048];
			char infostr[2048];
			infobuf_t *info = &cls.userinfo[cl.splitclients];

			//some userinfos should always have a value
			if (!*InfoBuf_ValueForKey(info, "name"))	//$name-2
				InfoBuf_SetKey(info, "name", va("%s-%i", InfoBuf_ValueForKey(&cls.userinfo[0], "name"), cl.splitclients+1));
			if (cls.protocol != CP_QUAKE2)
			{
				if (!*InfoBuf_ValueForKey(info, "team"))	//put players on the same team by default. this avoids team damage in coop, and if you're playing on the same computer then you probably want to be on the same team anyway.
					InfoBuf_SetKey(info, "team", InfoBuf_ValueForKey(&cls.userinfo[0], "team"));
				if (!*InfoBuf_ValueForKey(info, "bottomcolor"))	//bottom colour implies team in nq
					InfoBuf_SetKey(info, "bottomcolor", InfoBuf_ValueForKey(&cls.userinfo[0], "bottomcolor"));
				if (!*InfoBuf_ValueForKey(info, "topcolor"))	//should probably pick a random top colour or something
					InfoBuf_SetKey(info, "topcolor", InfoBuf_ValueForKey(&cls.userinfo[0], "topcolor"));
			}
			if (!*InfoBuf_ValueForKey(info, "skin"))	//give players the same skin by default, because we can. q2 cares for teams. qw might as well (its not like anyone actually uses them thanks to enemy-skin forcing).
				InfoBuf_SetKey(info, "skin", InfoBuf_ValueForKey(&cls.userinfo[0], "skin"));
			InfoBuf_SetKey(info, "chat", "");

#ifdef SVNREVISION
			if (strcmp(STRINGIFY(SVNREVISION), "-"))
				ver = va("%s v%i.%02i %s", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR, STRINGIFY(SVNREVISION));
			else
#endif
				ver = va("%s v%i.%02i", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR);
			InfoBuf_SetStarKey(info, "*ver", ver);
			InfoBuf_ToString(info, infostr, sizeof(infostr), NULL, NULL, NULL, &cls.userinfosync, info);

			CL_SendClientCommand(true, "addseat %i %s", cl.splitclients+1, COM_QuotedString(infostr, buffer, sizeof(buffer), false));
		}
		else if (cl.splitclients > targ && targ >= 1)
			CL_SendClientCommand(true, "addseat %i", targ);
	}
}


/*
=================
CL_SendCmd
=================
*/
qboolean CL_WriteDeltas (int plnum, sizebuf_t *buf)
{
	int i;
	usercmd_t *cmd, *oldcmd;
	qboolean dontdrop = false;


	i = (cls.netchan.outgoing_sequence-2) & UPDATE_MASK;
	cmd = &cl.outframes[i].cmd[plnum];
	if (cl_c2sImpulseBackup.ival >= 2)
		dontdrop = dontdrop || cmd->impulse;
	MSGCL_WriteDeltaUsercmd (buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & UPDATE_MASK;
	if (cl_c2sImpulseBackup.ival >= 3)
		dontdrop = dontdrop || cmd->impulse;
	cmd = &cl.outframes[i].cmd[plnum];
	MSGCL_WriteDeltaUsercmd (buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & UPDATE_MASK;
	if (cl_c2sImpulseBackup.ival >= 1)
		dontdrop = dontdrop || cmd->impulse;
	cmd = &cl.outframes[i].cmd[plnum];
	MSGCL_WriteDeltaUsercmd (buf, oldcmd, cmd);

	return dontdrop;
}

#ifdef Q2CLIENT
qboolean CLQ2_SendCmd (sizebuf_t *buf)
{
	int seq_hash;
	qboolean dontdrop = false;
	usercmd_t *cmd;
	int checksumIndex, i;
	int lightlev;
	int seat;

	cl.movesequence = cls.netchan.outgoing_sequence;	//make sure its correct even over map changes.
	seq_hash = cl.movesequence;

	for (seat = 0; seat < cl.splitclients; seat++)
	{
		// send this and the previous cmds in the message, so
		// if the last packet was dropped, it can be recovered
		i = cl.movesequence & UPDATE_MASK;
		cmd = &cl.outframes[i].cmd[seat];

		//q2admin is retarded and kicks you if you get a stall.
		if (cmd->msec > 100)
			cmd->msec = 100;

		if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		{	//checksum byte got switched around to something that doesn't include so much sequence info. not really sure why.
			if (!seat)
			{
				MSG_WriteByte (buf, clcq2_move);
				if (!cl.q2frame.valid || cl_nodelta.ival || (cls.demorecording && !cls.demohadkeyframe))
					MSG_WriteLong (buf, -1);	// no compression
				else
					MSG_WriteLong (buf, cl.q2frame.serverframe);
			}

			checksumIndex = buf->cursize;
			MSG_WriteByte (buf, 0);	//each seat has its own individual checksum for some reason (but no extra clcq2_move - player counts are not dynamic).
		}
		else if (seat)
		{
			//multi-seat still has an extra clc_move per seat
			//but no checksum (pointless when its opensource anyway)
			//no sequence (only seat 0 reports that)
			MSG_WriteByte (buf, clcq2_move);
			checksumIndex = -1;
		}
		else
		{
			MSG_WriteByte (buf, clcq2_move);
			// save the position for a checksum qbyte
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2 || cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
				checksumIndex = -1;
			else
			{
				checksumIndex = buf->cursize;
				MSG_WriteByte (buf, 0);
			}

			if (!cl.q2frame.valid || cl_nodelta.ival || (cls.demorecording && !cls.demohadkeyframe))
				MSG_WriteLong (buf, -1);	// no compression
			else
				MSG_WriteLong (buf, cl.q2frame.serverframe);
		}

		lightlev = R_LightPoint(cl.playerview[seat].simorg);

	//	msecs = msecs - (double)msecstouse;

		i = cls.netchan.outgoing_sequence & UPDATE_MASK;
		cmd = &cl.outframes[i].cmd[seat];
		*cmd = cl_pendingcmd[seat];

		cmd->lightlevel = (lightlev>255)?255:lightlev;

		cl.outframes[i].senttime = realtime;
		cl.outframes[i].latency = -1;

		if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		{
			if (cmd->upmove >= 100)
				cmd->buttons |= 1<<3;	//jump
			else if (cmd->upmove <= -100)
				cmd->buttons |= 1<<4;	//crouch
		}
		else
		{
			if (in_jump.state[seat]&3 && cmd->upmove==0)
				cmd->upmove = 200;
		}
		if (cmd->buttons)
			cmd->buttons |= 128;	//fixme: this isn't really what's meant by the anykey.

	// calculate a checksum over the move commands
		dontdrop |= CL_WriteDeltas(seat, buf);

		if (checksumIndex >= 0)
		{
			buf->data[checksumIndex] = Q2COM_BlockSequenceCRCByte(
				buf->data + checksumIndex + 1, buf->cursize - checksumIndex - 1,
				seq_hash);
		}
	}
	CL_ClearPendingCommands();

	if (cl.sendprespawn || !cls.protocol_q2)
		buf->cursize = 0;	//tastyspleen.net is alergic.
	else
		CL_UpdateSeats();

	return dontdrop;
}
#endif

qboolean CLQW_SendCmd (sizebuf_t *buf, qboolean actuallysend)
{
	int seq_hash;
	qboolean dontdrop = false;
	usercmd_t *cmd;
	int checksumIndex, firstsize, plnum;
	int clientcount, lost;
	int curframe;
	int st = buf->cursize;
	int chatstate;

	cl.movesequence = cls.netchan.outgoing_sequence;	//make sure its correct even over map changes.
	curframe = cl.movesequence & UPDATE_MASK;
	seq_hash = cl.movesequence;

	cl.outframes[curframe].server_message_num = cl.validsequence;
	cl.outframes[curframe].cmd_sequence = cl.movesequence;
	cl.outframes[curframe].senttime = realtime;
	cl.outframes[curframe].latency = -1;

// send this and the previous cmds in the message, so
// if the last packet was dropped, it can be recovered
	clientcount = cl.splitclients;

	if (!clientcount)
		clientcount = 1;

	chatstate = 0;
	if (cl_sendchatstate.ival)
	{
		if (Key_Dest_Has(kdm_message|kdm_console|kdm_cwindows))
			chatstate |= 1;		//chatting
		else if (Key_Dest_Has(~(kdm_game|kdm_centerprint)))
			chatstate |= 2;		//afk. ezquake sends chatting, but neither are really appropriate.
		if (!vid.activeapp || vid.isminimized)
			chatstate |= 2;		//afk.
		//FIXME: flag as afk if no new inputs for a while.
	}
	for (plnum = 0; plnum<clientcount; plnum++)
	{
		if (cl.playerview[plnum].chatstate != chatstate)
		{
			if (chatstate)
				CL_SetInfo(plnum, "chat", va("%i", chatstate));
			else
				CL_SetInfo(plnum, "chat", "");
			cl.playerview[plnum].chatstate = chatstate;
		}

		cmd = &cl.outframes[curframe].cmd[plnum];
		*cmd = cl_pendingcmd[plnum];
		
		cmd->lightlevel = 0;
#ifdef CSQC_DAT
		if (!runningindepphys)
			CSQC_Input_Frame(plnum, cmd);
#endif
	}
	CL_ClearPendingCommands();

	if (cls.fteprotocolextensions2 & PEXT2_VRINPUTS)
		dontdrop = CLFTE_SendVRCmd(buf, clientcount);
	else
	{
		cmd = &cl.outframes[curframe].cmd[0];
		if (cmd->cursor_screen[0] || cmd->cursor_screen[1] || cmd->cursor_entitynumber ||
			cmd->cursor_start[0] || cmd->cursor_start[1] || cmd->cursor_start[2] ||
			cmd->cursor_impact[0] || cmd->cursor_impact[1] || cmd->cursor_impact[2])
		{
			MSG_WriteByte (buf, clcfte_prydoncursor);
			MSG_WriteShort(buf, cmd->cursor_screen[0] * 32767.0f);
			MSG_WriteShort(buf, cmd->cursor_screen[1] * 32767.0f);
			MSG_WriteFloat(buf, cmd->cursor_start[0]);
			MSG_WriteFloat(buf, cmd->cursor_start[1]);
			MSG_WriteFloat(buf, cmd->cursor_start[2]);
			MSG_WriteFloat(buf, cmd->cursor_impact[0]);
			MSG_WriteFloat(buf, cmd->cursor_impact[1]);
			MSG_WriteFloat(buf, cmd->cursor_impact[2]);
			MSG_WriteEntity(buf, cmd->cursor_entitynumber);
		}

		MSG_WriteByte (buf, clc_move);

		// save the position for a checksum qbyte
		checksumIndex = buf->cursize;
		MSG_WriteByte (buf, 0);

		// write our lossage percentage
		lost = CL_CalcNet(r_netgraph.value);
		MSG_WriteByte (buf, (qbyte)lost);

		firstsize=0;
		for (plnum = 0; plnum<clientcount; plnum++)
		{
			cmd = &cl.outframes[curframe].cmd[plnum];

			if (plnum)
				MSG_WriteByte (buf, clc_move);

			dontdrop = CL_WriteDeltas(plnum, buf) || dontdrop;

			if (!firstsize)
				firstsize = buf->cursize;
		}

	// calculate a checksum over the move commands

		buf->data[checksumIndex] = COM_BlockSequenceCRCByte(
			buf->data + checksumIndex + 1, firstsize - checksumIndex - 1,
			seq_hash);
	}

	// request delta compression of entities
	if (cls.netchan.outgoing_sequence - cl.validsequence >= UPDATE_BACKUP-1)
		cl.validsequence = 0;

	//delta_sequence is the _expected_ previous sequences, so is set before it arrives.
	if (cl.validsequence && !cl_nodelta.ival && cls.state == ca_active)// && !cls.demorecording)
	{
		MSG_WriteByte (buf, clc_delta);
//		Con_Printf("%i\n", cl.validsequence);
		MSG_WriteByte (buf, cl.validsequence&255);
	}

	if (cl.sendprespawn || !actuallysend)
		buf->cursize = st;	//don't send movement commands while we're still supposedly downloading. mvdsv does not like that.
	else
		CL_UpdateSeats();

	return dontdrop;
}

static void CL_SendUserinfoUpdate(void)
{
	const char *key = cls.userinfosync.keys[0].name;
	infobuf_t *info = cls.userinfosync.keys[0].context;
	size_t bloboffset = cls.userinfosync.keys[0].syncpos;
	unsigned int seat = info - cls.userinfo;
	size_t blobsize;
	const char *blobdata = InfoBuf_BlobForKey(info, key, &blobsize, NULL);
	size_t sendsize = blobsize - bloboffset;

	const char *s;
	qboolean final = true;
	char enckey[2048];
	char encval[2048];

#ifdef Q3CLIENT
	if (cls.protocol == CP_QUAKE3)
	{	//q3 sends it all in one go
		char userinfo[2048];
		InfoSync_Strip(&cls.userinfosync, info);	//can't track this stuff. all or nothing.
		if (info == &cls.userinfo[0])
		{
			InfoBuf_ToString(info, userinfo, sizeof(userinfo), NULL, NULL, NULL, NULL, NULL);
			q3->cl.SendClientCommand("userinfo \"%s\"", userinfo);
		}
		return;
	}
#endif
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2 && !cls.fteprotocolextensions)
	{
		char userinfo[2048];
		InfoSync_Strip(&cls.userinfosync, info);	//can't track this stuff. all or nothing.

		if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		{
			extern size_t Q2EX_UserInfoToString(char *infostring, size_t maxsize, const char **ignore, int seats);
			Q2EX_UserInfoToString(userinfo, sizeof(userinfo), NULL, cl.splitclients);

			MSG_WriteByte (&cls.netchan.message, clcq2_userinfo);
			MSG_WriteString (&cls.netchan.message, userinfo+(*userinfo=='\\'?1:0));
		}
		else if (info == &cls.userinfo[0])
		{
			InfoBuf_ToString(info, userinfo, sizeof(userinfo), NULL, NULL, NULL, NULL, NULL);

			MSG_WriteByte (&cls.netchan.message, clcq2_userinfo);
			MSG_WriteString (&cls.netchan.message, userinfo);
		}
		return;
	}
#endif

	if (seat < max(1,cl.splitclients))
	{
		if (sendsize > 1023)
		{
			final = false;
			sendsize = 1023;	//should be a multiple of 3
		}

		if (!InfoBuf_EncodeString(key, strlen(key), enckey, sizeof(enckey)) ||
			!InfoBuf_EncodeString(blobdata+bloboffset, sendsize, encval, sizeof(encval)))
		{	//some buffer wasn't big enough... shouldn't happen.
			InfoSync_Remove(&cls.userinfosync, 0);
			return;
		}

		if (final && !bloboffset && *encval != '\xff' && *encval != '\xff')
		{	//vanilla-compatible info.
			s = va("setinfo \"%s\" \"%s\"", enckey, encval);
		}
		else if (cls.fteprotocolextensions2 & PEXT2_INFOBLOBS)
		{	//only flood servers that actually support it.
			if (final)
				s = va("setinfo \"%s\" \"%s\" %u", enckey, encval, (unsigned int)bloboffset);
			else
				s = va("setinfo \"%s\" \"%s\" %u+", enckey, encval, (unsigned int)bloboffset);
		}
		else
		{	//server doesn't support it, just ignore the key
			InfoSync_Remove(&cls.userinfosync, 0);
			return;
		}
		if (seat && (cls.fteprotocolextensions&PEXT_SPLITSCREEN))
		{
			MSG_WriteByte (&cls.netchan.message, (cls.protocol == CP_QUAKE2)?clcq2_stringcmd_seat:clcfte_stringcmd_seat);
			MSG_WriteByte (&cls.netchan.message, seat);
		}
		else
		{
			MSG_WriteByte (&cls.netchan.message, (cls.protocol == CP_QUAKE2)?clcq2_stringcmd:clc_stringcmd);
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
				MSG_WriteByte (&cls.netchan.message, 1+seat);
		}
		MSG_WriteString (&cls.netchan.message, s);
	}

	if (bloboffset+sendsize == blobsize)
		InfoSync_Remove(&cls.userinfosync, 0);
	else
		cls.userinfosync.keys[0].syncpos += sendsize;
}

void CL_SendCmd (double frametime, qboolean mainloop)
{
	sizebuf_t	buf;
	qbyte		data[MAX_DATAGRAM*16];
	int			i, plnum;
	usercmd_t	*cmd;
	float wantfps;
	int fullsend;	//-1: send for sequence, with no usercmd. 0: update input frame, but don't send anything. 1: time for a new usercmd

	static float	pps_balance = 0;
	static int	dropcount = 0;
	static double msecs;
	static double msecsround;
	qboolean	dontdrop=false;
	float usetime;		//how many msecs we can use for the new frame
	int msecstouse;		//usetime truncated to network precision (how much we'll actually eat)
	float framemsecs;	//how long we're saying the input frame should be (differs from realtime with nq as we want to send frames reguarly, but note this might end up with funny-duration frames).
	qboolean xonoticworkaround;

	clcmdbuf_t *next;

	if (runningindepphys)
	{
		double curtime;
		static double lasttime;
		curtime = Sys_DoubleTime();
		frametime = curtime - lasttime;
		lasttime = curtime;
	}

	CL_ProxyMenuHooks();

	if (cls.demoplayback != DPB_NONE || cls.state <= ca_demostart)
	{
		cursor_active = false;
		if (!cls.state || cls.demoplayback == DPB_MVD)
		{
			extern cvar_t cl_splitscreen;
			cl.ackedmovesequence = cl.movesequence;
			i = cl.movesequence & UPDATE_MASK;
			cl.movesequence++;
			cl.outframes[i].server_message_num = cl.validsequence;
			cl.outframes[i].cmd_sequence = cl.movesequence;
			cl.outframes[i].senttime = realtime;		// we haven't gotten a reply yet
//			cl.outframes[i].receivedtime = -1;		// we haven't gotten a reply yet

			if (cl.splitclients > cl_splitscreen.ival+1)
			{
				cl.splitclients = cl_splitscreen.ival+1;
				if (cl.splitclients < 1)
					cl.splitclients = 1;
			}
			for (plnum = 0; plnum < cl.splitclients; plnum++)
			{
				playerview_t *pv = &cl.playerview[plnum];
				cmd = &cl.outframes[i].cmd[plnum];

				CL_AccumlateInput(plnum, frametime, frametime*1000);
				*cmd = cl_pendingcmd[plnum];
				memset(&cl_pendingcmd[plnum], 0, sizeof(*cmd));	//reset the pending for the next frame.

#ifdef CSQC_DAT
				CSQC_Input_Frame(plnum, cmd);
#endif

				if (cls.state == ca_active)
				{
					player_state_t *from, *to;
					from = &cl.inframes[cl.ackedmovesequence & UPDATE_MASK].playerstate[pv->playernum];
					to = &cl.inframes[cl.movesequence & UPDATE_MASK].playerstate[pv->playernum];
					CL_PredictUsercmd(pv->playernum, pv->viewentity, from, to, &cl.outframes[cl.ackedmovesequence & UPDATE_MASK].cmd[plnum]);
				}
			}

			while (clientcmdlist)
			{
				next = clientcmdlist->next;
				CL_Demo_ClientCommand(clientcmdlist->command);
				Con_DLPrintf(2, "Sending stringcmd %s\n", clientcmdlist->command);
				Z_Free(clientcmdlist);
				clientcmdlist = next;
			}

			cls.netchan.outgoing_sequence = cl.movesequence;
		}

		IN_Move (NULL, NULL, 0, frametime);

		Cbuf_Waited();	//its okay to stop waiting now
		return; // sendcmds come from the demo
	}

	memset(&buf, 0, sizeof(buf));
	buf.maxsize = sizeof(data);
	buf.cursize = 0;
	buf.data = data;
	buf.prim = cls.netchan.message.prim;

	xonoticworkaround = cls.protocol == CP_NETQUAKE && CPNQ_IS_DP && cl.time && !cl.paused;
	if (xonoticworkaround)
	{
		if (cl.movesequence_time > cl.time + 0.5)
			cl.movesequence_time = cl.time + 0.5;	//shouldn't really happen
		if (cl.movesequence_time < cl.time - 0.5)
			cl.movesequence_time = cl.time - 0.5;	//shouldn't really happen
		framemsecs = (cl.time - cl.movesequence_time)*1000;

		wantfps = cl_netfps.value;
		usetime = CL_FilterTime(framemsecs, wantfps, 5, false);
		if (usetime > 0)
		{
			usetime = framemsecs - usetime;
			fullsend = true;
		}
		else
		{
			usetime = framemsecs - usetime;
			fullsend = false;
		}
		msecstouse = usetime;
		framemsecs = msecstouse;
		msecs = 0;
	}
	else
	{
		msecs += frametime*1000;

	//	Con_Printf("%f\n", msecs);

		wantfps = cl_netfps.value;
		fullsend = true;

		msecstouse = 0;

	#ifndef CLIENTONLY
		if (sv.state && cls.state != ca_active)
		{	//HACK: if we're also the server, spam like a crazy person until we're on the server, for faster apparent load times.
			fullsend = -1;	//send no movement command.
			msecstouse = usetime = msecs;
		}
		else 
	#endif
		{
			// while we're not playing send a slow keepalive fullsend to stop mvdsv from screwing up
			if (cls.state < ca_active && !cls.download)
			{
				#ifdef IRCCONNECT	//don't spam irc.
				if (cls.netchan.remote_address.type == NA_IRC)
					wantfps = 0.5;
				else
				#endif
					wantfps = 12.5;
			}
			if (!runningindepphys && (cl_netfps.value > 0 || !fullsend))
			{
				float spare;
				spare = CL_FilterTime(msecs, wantfps, (/*cls.protocol == CP_NETQUAKE*/0?0:1.5), false);
				usetime = msecsround + (msecs - spare);
				msecstouse = (int)usetime;
				if (!spare)
					fullsend = false;
				else
				{
					msecsround = usetime - msecstouse;
					msecs = spare + msecstouse;
				}
			}
			else
			{
				usetime = msecsround + msecs;
				msecstouse = (int)usetime;
				msecsround = usetime - msecstouse;
			}
		}

		if (msecstouse > 200) // cap at 200 to avoid servers splitting movement more than four times
			msecstouse = 200;

		// align msecstouse to avoid servers wasting our msecs
		if (msecstouse > 100)
			msecstouse &= ~3; // align to 4
		else if (msecstouse > 50)
			msecstouse &= ~1; // align to 2

		if (msecstouse <= 0)	//FIXME
			fullsend = false;
		if (usetime <= 0)
			return;	//infinite frame times = weirdness.

		framemsecs = msecstouse;

		if (cls.protocol == CP_NETQUAKE)
			framemsecs = 1000*(cl.time - cl.movesequence_time);
	}

#ifdef HLCLIENT
	if (!CLHL_BuildUserInput(msecstouse, &cl_pendingcmd[0]))
#endif
	for (plnum = 0; plnum < (cl.splitclients?cl.splitclients:1); plnum++)
		CL_AccumlateInput(plnum, frametime, framemsecs);

	//the main loop isn't allowed to send
	if (runningindepphys && mainloop)
		return;

//	if (skipcmd)
//		return;

	if (!fullsend)
		return; // when we're actually playing we try to match netfps exactly to avoid gameplay problems

//	if (msecstouse > 127)
//		Con_Printf("%i\n", msecstouse, msecs);

	//HACK: 1000/77 = 12.98. nudge it just under so we never appear to be using 83fps at 77fps (which can trip cheat detection in mods that expect 72 fps when many servers are configured for 77)
	//so lets just never use 12.
	if (fullsend && cls.maxfps == 77)
		for (plnum = 0; plnum < (cl.splitclients?cl.splitclients:1); plnum++)
			if (cl_pendingcmd[plnum].msec > 12.9 && cl_pendingcmd[plnum].msec < 13)
				cl_pendingcmd[plnum].msec = 13;

#ifdef NQPROT
	if (cls.protocol != CP_NETQUAKE || cls.netchan.nqreliable_allowed)
#endif
	{
		CL_SendDownloadReq(&buf);

		//only start spamming userinfo blobs once we receive the initial serverinfo.
		while (cls.userinfosync.numkeys && cls.netchan.message.cursize < 512 && (cl.haveserverinfo || cls.protocol == CP_QUAKE2 || cls.protocol == CP_QUAKE3))
			CL_SendUserinfoUpdate();

		while (clientcmdlist)
		{
			next = clientcmdlist->next;
			if (clientcmdlist->reliable)
			{
				if (cls.netchan.message.cursize + 2+strlen(clientcmdlist->command)+100 > cls.netchan.message.maxsize)
					break;
				if (!strncmp(clientcmdlist->command, "spawn", 5) && cls.userinfosync.numkeys && cl.haveserverinfo)
					break;	//HACK: don't send the spawn until all pending userinfos have been flushed.
				if (cls.protocol==CP_QUAKE2 && cls.protocol_q2==PROTOCOL_VERSION_Q2EX)
				{
					MSG_WriteByte (&cls.netchan.message, clcq2_stringcmd);
					MSG_WriteByte (&cls.netchan.message, clientcmdlist->seat+1);
				}
				else if (clientcmdlist->seat && (cls.fteprotocolextensions&PEXT_SPLITSCREEN))
				{
					MSG_WriteByte (&cls.netchan.message, clcfte_stringcmd_seat);
					MSG_WriteByte (&cls.netchan.message, clientcmdlist->seat);
				}
				else
					MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
				MSG_WriteString (&cls.netchan.message, clientcmdlist->command);
			}
			else
			{
				if (buf.cursize + 2+strlen(clientcmdlist->command)+100 <= buf.maxsize)
				{
					if (clientcmdlist->seat && (cls.fteprotocolextensions&PEXT_SPLITSCREEN))
					{
						MSG_WriteByte (&cls.netchan.message, clcfte_stringcmd_seat);
						MSG_WriteByte (&cls.netchan.message, clientcmdlist->seat);
					}
					else
						MSG_WriteByte (&buf, clc_stringcmd);
					MSG_WriteString (&buf, clientcmdlist->command);
				}
			}
			Con_DLPrintf(2, "Sending stringcmd %s\n", clientcmdlist->command);
			Z_Free(clientcmdlist);
			clientcmdlist = next;
		}
	}

	// if we're not doing clc_moves and etc, don't continue unless we wrote something previous
	// or we have something on the reliable buffer (or we're loopback and don't care about flooding)
	if (!fullsend && cls.netchan.remote_address.type != NA_LOOPBACK && buf.cursize < 1 && cls.netchan.message.cursize < 1)
		return;

	if (fullsend)
	{
		if (!cls.state)
		{
			msecs -= (double)msecstouse;
			return;
		}
		cursor_active = false;

		for (plnum = 0; plnum < cl.splitclients; plnum++)
		{
			cmd = &cl_pendingcmd[plnum];
			if (((cls.fteprotocolextensions2 & PEXT2_PRYDONCURSOR)||(cls.protocol == CP_NETQUAKE && cls.protocol_nq >= CPNQ_DP6)) && 
				(*cl_prydoncursor.string && cl_prydoncursor.ival >= 0) && cls.state == ca_active)
				CL_UpdatePrydonCursor(cmd, plnum);
			else
			{
				Vector2Clear(cmd->cursor_screen);
				VectorClear(cmd->cursor_start);
				VectorClear(cmd->cursor_impact);
				cmd->cursor_entitynumber = 0;
			}
		}

		if (xonoticworkaround)
			cl.movesequence_time += msecstouse/1000.0;
		else
			cl.movesequence_time = cl.time;
		switch (cls.protocol)
		{
#ifdef NQPROT
		case CP_NETQUAKE:
			msecs -= (double)msecstouse;
			CLNQ_SendCmd (&buf);
			dontdrop = true;
			break;
#endif
		case CP_QUAKEWORLD:
			msecs -= (double)msecstouse;
			dontdrop = CLQW_SendCmd (&buf, fullsend == true);
			break;
#ifdef Q2CLIENT
		case CP_QUAKE2:
			msecs -= (double)msecstouse;
			dontdrop = CLQ2_SendCmd (&buf);
			break;
#endif
#ifdef Q3CLIENT
		case CP_QUAKE3:
			msecs -= (double)msecstouse;
			i = cl.movesequence&UPDATE_MASK;
			memcpy(cl.outframes[i].cmd, cl_pendingcmd, sizeof(usercmd_t)*bound(1, cl.splitclients, MAX_SPLITS));
			cl.outframes[i].cmd_sequence = cl.movesequence++;
			q3->cl.SendCmd(cls.sockets, cl.outframes[i].cmd, cl.movesequence, cl.time);
			cls.netchan.outgoing_sequence = cl.movesequence;
			CL_ClearPendingCommands();

			//don't bank too much, because that results in banking speedcheats
			if (msecs > 200)
				msecs = 200;
			return; // Q3 does it's own thing
#endif
		default:
			Host_EndGame("Invalid protocol in CL_SendCmd: %i", cls.protocol);
			return;
		}

		if (cls.demorecording)
			CL_WriteDemoCmd(&cl.outframes[cl.movesequence & UPDATE_MASK].cmd[0]);

//		Con_DPrintf("generated sequence %i\n", cl.movesequence);
		cl.movesequence++;

		//clear enough of the pending command for the next frame.
		for (plnum = 0; plnum < cl.splitclients; plnum++)
		{
			cl_pendingcmd[plnum].sequence = cl.movesequence;
			cl_pendingcmd[plnum].msec = 0;
			cl_pendingcmd[plnum].impulse = 0;
//			cl_pendingcmd[plnum].buttons = 0;
		}
	}

#ifdef IRCCONNECT
	if (cls.netchan.remote_address.type == NA_IRC)
	{
		if (dropcount >= 2)
		{
			dropcount = 0;
		}
		else
		{
			// don't count this message when calculating PL
			cl.outframes[cls.netchan.outgoing_sequence&UPDATE_MASK].latency = -3;
			// drop this message
			cls.netchan.outgoing_sequence++;
			dropcount++;
			return;
		}
	}
	else
#endif
	//shamelessly stolen from fuhquake
		if (cl_c2spps.ival>0)
	{
		pps_balance += frametime;
		// never drop more than 2 messages in a row -- that'll cause PL
		// and don't drop if one of the last two movemessages have an impulse
		if (pps_balance > 0 || dropcount >= 2 || dontdrop)
		{
			float	pps;
			pps = cl_c2spps.ival;
			if (pps < 10) pps = 10;
			if (pps > 72) pps = 72;
			pps_balance -= 1 / pps;
			// bound pps_balance. FIXME: is there a better way?
			if (pps_balance > 0.1) pps_balance = 0.1;
			if (pps_balance < -0.1) pps_balance = -0.1;
			dropcount = 0;
		}
		else
		{
			// don't count this message when calculating PL
			cl.outframes[(cl.movesequence-1) & UPDATE_MASK].latency = -3;
			// drop this message
			cls.netchan.outgoing_sequence++;
			dropcount++;
			return;
		}
	}
	else
	{
		pps_balance = 0;
		dropcount = 0;
	}

#ifdef VOICECHAT
	if (cls.protocol == CP_QUAKE2)
		S_Voip_Transmit(clcq2_voicechat, &buf);
	else
		S_Voip_Transmit(clcfte_voicechat, &buf);
#endif

//
// deliver the message
//
	cls.netchan.dupe = cl_c2sdupe.ival;
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data, 2500);

	//don't bank too much, because that results in banking speedcheats
	if (msecs > 200)
		msecs = 200;

	if (cls.netchan.fatal_error)
	{
		cls.netchan.fatal_error = false;
		cls.netchan.message.overflowed = false;
		cls.netchan.message.cursize = 0;
	}
}

void CL_SendCvar_f (void)
{
	cvar_t *var;
	char *val;
	char *name = Cmd_Argv(1);

	var = Cvar_FindVar(name);
	if (!var)
		val = "";
	else if (var->flags & CVAR_NOUNSAFEEXPAND)
		val = "";
	else
		val = var->string;
	CL_SendSeatClientCommand(true, CL_TargettedSplit(false), "sentcvar %s \"%s\"", name, val);
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	static char pcmd[MAX_SPLITS][3][6];
	unsigned int sp, i;
#define inputnetworkcvargroup "client networking options"
	cl.splitclients = 1;

	Cmd_AddCommand("rotate", IN_Rotate_f);
	Cmd_AddCommand("in_restart", IN_Restart);
	Cmd_AddCommand("sendcvar", CL_SendCvar_f);

	Cvar_Register (&cl_fastaccel, inputnetworkcvargroup);
	Cvar_Register (&in_xflip, inputnetworkcvargroup);
	Cvar_Register (&in_vraim, inputnetworkcvargroup);
	Cvar_Register (&cl_nodelta, inputnetworkcvargroup);

	Cvar_Register (&prox_inmenu, inputnetworkcvargroup);

	Cvar_Register (&cl_c2sdupe, inputnetworkcvargroup);
	Cvar_Register (&cl_c2sImpulseBackup, inputnetworkcvargroup);
	Cvar_Register (&cl_c2sMaxRedundancy, inputnetworkcvargroup);
	Cvar_Register (&cl_c2spps, inputnetworkcvargroup);
	Cvar_Register (&cl_queueimpulses, inputnetworkcvargroup);
	Cvar_Register (&cl_netfps, inputnetworkcvargroup);
	Cvar_Register (&cl_run, inputnetworkcvargroup);
	Cvar_Register (&cl_iDrive, inputnetworkcvargroup);

#ifdef NQPROT
	Cvar_Register (&cl_movement, inputnetworkcvargroup);
#endif
	Cvar_Register (&cl_sendchatstate, inputnetworkcvargroup);
	Cvar_Register (&cl_smartjump, inputnetworkcvargroup);

	Cvar_Register (&cl_prydoncursor, inputnetworkcvargroup);
	Cvar_Register (&cl_instantrotate, inputnetworkcvargroup);
	Cvar_Register (&cl_forceseat, inputnetworkcvargroup);

	for (sp = 0; sp < MAX_SPLITS; sp++)
	{
		Q_snprintfz(pcmd[sp][0], sizeof(pcmd[sp][0]), "p%i", sp+1);
		Q_snprintfz(pcmd[sp][1], sizeof(pcmd[sp][1]), "+p%i", sp+1);
		Q_snprintfz(pcmd[sp][2], sizeof(pcmd[sp][2]), "-p%i", sp+1);
		Cmd_AddCommand (pcmd[sp][0],	CL_Split_f);
		Cmd_AddCommand (pcmd[sp][1],	CL_Split_f);
		Cmd_AddCommand (pcmd[sp][2],	CL_Split_f);

/*default mlook to pressed, (on android we split the two sides of the screen)*/
		in_mlook.state[sp] = 1;
	}

	/*then alternative arged ones*/
	Cmd_AddCommand ("p",			CL_SplitA_f);
	Cmd_AddCommand ("+p",			CL_SplitA_f);
	Cmd_AddCommand ("-p",			CL_SplitA_f);
	
	Cmd_AddCommand ("+moveup",		IN_UpDown);
	Cmd_AddCommand ("-moveup",		IN_UpUp);
	Cmd_AddCommand ("+movedown",	IN_DownDown);
	Cmd_AddCommand ("-movedown",	IN_DownUp);
	Cmd_AddCommand ("+left",		IN_LeftDown);
	Cmd_AddCommand ("-left",		IN_LeftUp);
	Cmd_AddCommand ("+right",		IN_RightDown);
	Cmd_AddCommand ("-right",		IN_RightUp);
	Cmd_AddCommand ("+forward",		IN_ForwardDown);
	Cmd_AddCommand ("-forward",		IN_ForwardUp);
	Cmd_AddCommand ("+back",		IN_BackDown);
	Cmd_AddCommand ("-back",		IN_BackUp);
	Cmd_AddCommand ("+lookup",		IN_LookupDown);
	Cmd_AddCommand ("-lookup",		IN_LookupUp);
	Cmd_AddCommand ("+lookdown",	IN_LookdownDown);
	Cmd_AddCommand ("-lookdown",	IN_LookdownUp);
	Cmd_AddCommand ("+strafe",		IN_StrafeDown);
	Cmd_AddCommand ("-strafe",		IN_StrafeUp);
	Cmd_AddCommand ("+moveleft",	IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft",	IN_MoveleftUp);
	Cmd_AddCommand ("+moveright",	IN_MoverightDown);
	Cmd_AddCommand ("-moveright",	IN_MoverightUp);
	Cmd_AddCommand ("+rollleft",	IN_RollLeftDown);
	Cmd_AddCommand ("-rollleft",	IN_RollLeftUp);
	Cmd_AddCommand ("+rollright",	IN_RollRightDown);
	Cmd_AddCommand ("-rollright",	IN_RollRightUp);
	Cmd_AddCommand ("+speed",		IN_SpeedDown);
	Cmd_AddCommand ("-speed",		IN_SpeedUp);
	Cmd_AddCommand ("+attack",		IN_AttackDown);
	Cmd_AddCommand ("-attack",		IN_AttackUp);
	Cmd_AddCommand ("+use",			IN_UseDown);
	Cmd_AddCommand ("-use",			IN_UseUp);
	Cmd_AddCommand ("+jump",		IN_JumpDown);
	Cmd_AddCommand ("-jump",		IN_JumpUp);
	Cmd_AddCommandD("impulse",		IN_Impulse, "Sends an impulse number to the server (read: weapon change).");
	Cmd_AddCommand ("+klook",		IN_KLookDown);
	Cmd_AddCommand ("-klook",		IN_KLookUp);
	Cmd_AddCommand ("+mlook",		IN_MLookDown);
	Cmd_AddCommand ("-mlook",		IN_MLookUp);

#ifdef QUAKESTATS
	Cmd_AddCommand ("+weaponwheel",		IN_WWheelDown);
	Cmd_AddCommand ("-weaponwheel",		IN_WWheelUp);
	Cmd_AddCommandD ("register_bestweapon",		IN_RegisterWeapon_f, "Normally set via a mod's default.cfg file");
	Cmd_AddCommandD ("bestweapon",		IN_Impulse, "Works like 'impulse', for compat with other engines.");
	Cvar_Register (&cl_weaponhide, inputnetworkcvargroup);
	Cvar_Register (&cl_weaponhide_preference, inputnetworkcvargroup);
	Cvar_Register (&cl_weaponpreselect, inputnetworkcvargroup);
	Cvar_Register (&cl_weaponforgetorder, inputnetworkcvargroup);
	Cvar_Register (&r_viewpreselgun, inputnetworkcvargroup);
#endif

	for (i = 0; i < countof(in_button); i++)
	{
		static char bcmd[countof(in_button)][2][10];
		Q_snprintfz(bcmd[i][0], sizeof(bcmd[sp][0]), "+button%i", i);
		Q_snprintfz(bcmd[i][1], sizeof(bcmd[sp][1]), "-button%i", i);
		Cmd_AddCommandD(bcmd[i][0],		IN_ButtonNDown, "This auxilliary command has mod-specific behaviour (often none).");
		Cmd_AddCommand (bcmd[i][1],		IN_ButtonNUp);
	}
}
