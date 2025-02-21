//file for builtin implementations relevent to all VMs.

#include "quakedef.h"

#if !defined(CLIENTONLY) || defined(CSQC_DAT) || defined(MENU_DAT)

#include "pr_common.h"
#ifdef SQL
#include "sv_sql.h"
#endif
#include "fs.h"

#include <ctype.h>

#define VMUTF8 utf8_enable.ival
#define VMUTF8MARKUP false

static char *cvargroup_progs = "Progs variables";

cvar_t utf8_enable = CVARD("utf8_enable", "0", "When 1, changes the qc builtins to act upon codepoints instead of bytes. Do not use unless com_parseutf8 is also set.");
cvar_t sv_gameplayfix_linknonsolid = CVARD("sv_gameplayfix_nolinknonsolid", "1", "When 0, setorigin et al will not link the entity into the collision nodes (which is faster, especially if you have a lot of non-solid entities. When 1, allows entities to freely switch between .solid values (except for SOLID_BSP) without relinking. A lot of DP mods assume a value of 1 and will bug out otherwise, while 0 will restore a bugs present in various mods.");
cvar_t sv_gameplayfix_blowupfallenzombies = CVARD("sv_gameplayfix_blowupfallenzombies", "0", "Allow findradius to find non-solid entities. This may break certain mods. It is better for mods to use FL_FINDABLE_NONSOLID instead.");
cvar_t sv_gameplayfix_findradiusdistancetobox = CVARD("sv_gameplayfix_findradiusdistancetobox", "0", "When 1, findradius checks to the nearest part of the entity instead of only its origin, making it find slightly more entities.");
cvar_t sv_gameplayfix_droptofloorstartsolid = CVARD("sv_gameplayfix_droptofloorstartsolid", "0", "When droptofloor fails, this causes a second attemp, but with traceline instead.");
cvar_t dpcompat_findradiusarealinks = CVARD("dpcompat_findradiusarealinks", "0", "Use the world collision info to accelerate findradius instead of looping through every single entity. May actually be slower for large radiuses, or fail to find entities which have not been linked properly with setorigin.");
#ifdef HAVE_LEGACY
cvar_t dpcompat_strcat_limit = CVARD("dpcompat_strcat_limit", "", "When set, cripples strcat (and related function) string lengths to the value specified.\nSet to 16383 to replicate DP's limit, otherwise leave as 0 to avoid limits.");
#endif
cvar_t pr_autocreatecvars = CVARD("pr_autocreatecvars", "1", "Implicitly create any cvars that don't exist when read.");
cvar_t pr_droptofloorunits = CVARD("pr_droptofloorunits", "256", "Distance that droptofloor is allowed to drop to be considered successul.");
cvar_t pr_brokenfloatconvert = CVAR("pr_brokenfloatconvert", "0");
cvar_t	pr_fixbrokenqccarrays = CVARFD("pr_fixbrokenqccarrays", "0", CVAR_MAPLATCH, "As part of its nq/qw/h2/csqc support, FTE remaps QC fields to match an internal order. This is a faster way to handle extended fields. However, some QCCs are buggy and don't report all field defs.\n0: do nothing. QCC must be well behaved.\n1: Duplicate engine fields, remap the ones we can to known offsets. This is sufficient for QCCX/FrikQCC mods that use hardcoded or even occasional calculated offsets (fixes ktpro).\n2: Scan the mod for field accessing instructions, and assume those are the fields (and that they don't alias non-fields). This can be used to work around gmqcc's WTFs (fixes xonotic).");
cvar_t pr_tempstringcount = CVARD("pr_tempstringcount", "", "Obsolete. Set to 16 if you want to recycle+reuse the same 16 tempstring references and break lots of mods.");
cvar_t pr_tempstringsize = CVARD("pr_tempstringsize", "4096", "Obsolete");
#ifdef MULTITHREAD
cvar_t pr_gc_threaded = CVARD("pr_gc_threaded", "1", "Says whether to use a separate thread for tempstring garbage collections. This avoids main-thread stalls but at the expense of more memory usage.");
#else
cvar_t pr_gc_threaded = CVARFD("pr_gc_threaded", "0", CVAR_NOSET|CVAR_NOSAVE, "Says whether to use a separate thread for tempstring garbage collections. This avoids main-thread stalls but at the expense of more memory usage.");
#endif
cvar_t	pr_sourcedir = CVARD("pr_sourcedir", "src", "Subdirectory where your qc source is located. Used by the internal compiler and qc debugging functionality.");
cvar_t pr_enable_uriget = CVARD("pr_enable_uriget", "1", "Allows gamecode to make direct http requests");
cvar_t pr_enable_profiling = CVARD("pr_enable_profiling", "0", "Enables profiling support. Will run more slowly. Change the map and then use the profile_ssqc/profile_csqc commands to see the results.");
#ifdef HAVE_CLIENT
cvar_t pr_precachepic_slow = CVARD("pr_precachepic_slow", "0", "Legacy setting. Should be set to 0 where supported.");
#endif
int tokenizeqc(const char *str, qboolean dpfuckage);

void PF_buf_shutdown(pubprogfuncs_t *prinst);

void skel_info_f(void);
void skel_generateragdoll_f(void);
void *PR_PointerToNative_Resize(pubprogfuncs_t *inst, pint_t ptr, size_t offset, size_t datasize);			//dangerous version
void *PR_PointerToNative_MoInvalidate(pubprogfuncs_t *inst, pint_t ptr, size_t offset, size_t datasize);	//safer faily version.

#ifdef __SSE2__
#include "xmmintrin.h"
#endif

void PF_Common_RegisterCvars(void)
{
#ifndef QUAKETC
#ifdef __SSE2__
	//disable FTZ and DAZ, in case some compiler left them on...
	unsigned int mxcsr = _mm_getcsr();
	if (mxcsr & 0x8040)
	{
		if (COM_CheckParm("-nodaz"))
		{
			Con_DPrintf("Disabling DAZ. This may have performance implications.\n");
			_mm_setcsr(mxcsr & ~(0x8040));
		}
		else
			Con_DPrintf(CON_WARNING "denormalised floats are disabled. Use -nodaz to re-enable if mods malfunction\n");
	}
#else
	volatile union
	{
		int i;
		float f;
	} a, b;
	a.i = 1;
	b.i = 1;
	if (!(a.f && b.f))
		Con_Printf(CON_WARNING "denormalised floats are disabled. Some mods might may malfunction\n");
#endif
#endif


	Cvar_Register (&sv_gameplayfix_blowupfallenzombies, cvargroup_progs);
	Cvar_Register (&sv_gameplayfix_findradiusdistancetobox, cvargroup_progs);
	Cvar_Register (&sv_gameplayfix_linknonsolid, cvargroup_progs);
	Cvar_Register (&sv_gameplayfix_droptofloorstartsolid, cvargroup_progs);
	Cvar_Register (&dpcompat_findradiusarealinks, cvargroup_progs);
#ifdef HAVE_LEGACY
	Cvar_Register (&dpcompat_strcat_limit, cvargroup_progs);
#endif
	Cvar_Register (&pr_droptofloorunits, cvargroup_progs);
	Cvar_Register (&pr_brokenfloatconvert, cvargroup_progs);
	Cvar_Register (&pr_tempstringcount, cvargroup_progs);
	Cvar_Register (&pr_tempstringsize, cvargroup_progs);
	Cvar_Register (&pr_gc_threaded, cvargroup_progs);
#ifdef WEBCLIENT
	Cvar_Register (&pr_enable_uriget, cvargroup_progs);
#endif
	Cvar_Register (&pr_enable_profiling, cvargroup_progs);
	Cvar_Register (&pr_sourcedir, cvargroup_progs);
	Cvar_Register (&pr_fixbrokenqccarrays, cvargroup_progs);
	Cvar_Register (&utf8_enable, cvargroup_progs);
	Cvar_Register (&pr_autocreatecvars, cvargroup_progs);
#ifdef HAVE_CLIENT
	Cvar_Register (&pr_precachepic_slow, cvargroup_progs);
#endif

#ifdef RAGDOLL
	Cmd_AddCommand("skel_info", skel_info_f);
	Cmd_AddCommand("skel_generateragdoll", skel_generateragdoll_f);
#endif

	WPhys_Init();

#ifdef ENGINE_ROUTING
	PR_Route_Init();
#endif
}

qofs_t PR_ReadBytesString(char *str)
{
	//use doubles, so we can cope with eg "5.3mb" or much larger values
	double d = strtod(str, &str);
	if (d < 0)
	{
#if (defined(_WIN64) && !defined(WINRT)) || (defined(__linux__)&&defined(__LP64__))
		return 0x80000000;	//use of virtual address space rather than physical memory means we can just go crazy and use the max of 2gb.
#elif defined(FTE_TARGET_WEB)
		return 8*1024*1024;
#else
		return 32*1024*1024;
#endif
	}
	if (*str == 'g')
		d *= 1024*1024*1024;
	if (*str == 'm')
		d *= 1024*1024;
	if (*str == 'k')
		d *= 1024;
	return d;
}

//just prints out a warning with stack trace. so I can throttle spammy stack traces.
static void PF_Warningf(pubprogfuncs_t *prinst, const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string)-1, fmt, argptr);
	va_end (argptr);

	if (developer.ival)
		PR_StackTrace(prinst, false);
	Con_Printf("%s", string);
}

const char *PF_VarString (pubprogfuncs_t *prinst, int	first, struct globalvars_s *pr_globals)
{
#define VARSTRINGLEN 65536+8
	int		i;
	static char buffer[2][VARSTRINGLEN];
	static int bufnum;
	const char *s;
	char *out;

	if (prinst->callargc - first == 1)
		return PR_GetStringOfs(prinst, OFS_PARM0+first*3);	//no need to copy/etc.

	out = buffer[(bufnum++)&1];

	out[0] = 0;
	for (i=first ; i<prinst->callargc ; i++)
	{
//		if (G_INT(OFS_PARM0+i*3) < 0 || G_INT(OFS_PARM0+i*3) >= 1024*1024);
//			break;

		s = PR_GetStringOfs(prinst, OFS_PARM0+i*3);
		if (s)
		{
			if (strlen(out)+strlen(s)+1 >= VARSTRINGLEN)
				Con_DPrintf("VarString (builtin call ending with strings) exceeded maximum string length of %i chars", VARSTRINGLEN);

			Q_strncatz (out, s, VARSTRINGLEN);
		}
	}
	return out;
}

static int debuggerresume;
static int debuggerresumeline;
extern int isPlugin;	//if 2, we were invoked by a debugger, and we need to give it debug locations (and it'll feed us continue/steps/breakpoints)
#ifndef SERVERONLY
static int debuggerstacky;
#endif

#if defined(_WIN32) && !defined(FTE_SDL) && !defined(_XBOX)
	#include <windows.h>
	void INS_UpdateGrabs(int fullscreen, int activeapp);
#else
	#include <unistd.h>
#endif

int QCLibEditor(pubprogfuncs_t *prinst, const char *filename, int *line, int *statement, int firststatement, char *error, pbool fatal);
void QCLoadBreakpoints(const char *vmname, const char *progsname)
{	//this asks the gui to reapply any active breakpoints and waits for them so that any spawn functions can be breakpointed properly.
	extern int				isPlugin;
	if (isPlugin >= 2)
	{
#ifdef SERVERONLY
		SV_GetConsoleCommands();
#else
		Sys_SendKeyEvents();
#endif
		debuggerresume = -1;
		fprintf(stdout, "qcreloaded \"%s\" \"%s\"\n", vmname, progsname);
		fflush(stdout);
#if defined(_WIN32) && !defined(FTE_SDL)
#ifndef SERVERONLY
		INS_UpdateGrabs(false, false);
#endif
#endif
		while(debuggerresume == -1 && !wantquit)
		{
#if defined(_WIN32) && !defined(FTE_SDL)
			Sleep(10);
#else
			usleep(10*1000);
#endif
#ifdef SERVERONLY
			SV_GetConsoleCommands();
#else
			Sys_SendKeyEvents();
#endif
		}
	}
}
extern cvar_t pr_sourcedir;
pubprogfuncs_t *debuggerinstance;
const char *debuggerfile;
size_t debuggerwnd;

qboolean QCExternalDebuggerCommand(char *text)
{
#if defined(CSQC_DAT) && !defined(SERVERONLY)
	extern world_t csqc_world;
#endif
#if defined(MENU_DAT) && !defined(SERVERONLY)
	extern world_t menu_world;
#endif

	if (!isPlugin)
		return false;
	if ((!strncmp(text, "qcstep", 6) && (text[6] == 0 || text[6] == ' ')) || (!strncmp(text, "qcresume", 8) && (text[8] == 0 || text[8] == ' ')))
	{
//		int l;
		if (text[2] == 's')
		{
			text += 6;
			while(*text==' ' || *text=='\t')
				text++;
			if (!strncmp(text, "out", 3))
				debuggerresume = DEBUG_TRACE_OUT;
			else if (!strncmp(text, "over", 3))
				debuggerresume = DEBUG_TRACE_OVER;
			else
				debuggerresume = DEBUG_TRACE_INTO;
//			l = atoi(text+7);
		}
		else
		{
//			l = atoi(text+9);
			debuggerresume = DEBUG_TRACE_OFF;
		}
//		if (l)
//			debuggerresumeline = l;
	}
	else if (!strncmp(text, "qcjump ", 7))
	{
		char file[MAX_QPATH];
		char linebuf[32];
		text += 7;
		text = COM_ParseOut(text, file, sizeof(file));
		text = COM_ParseOut(text, linebuf, sizeof(linebuf));

		if (debuggerinstance && debuggerfile && !Q_strcasecmp(file, debuggerfile))
		{
			debuggerresumeline = atoi(linebuf);
			debuggerresume = DEBUG_TRACE_NORESUME;	//'resume' from the debugger only to break again, so we know the new line number (if they tried setting the line to a blank one)
		}
	}
	else if (!strncmp(text, "debuggerwnd ", 11))
	{
		//send focus to this window when debugging
		debuggerwnd = strtoul(text+12, NULL, 0);
	}
	else if (!strncmp(text, "qcinspect ", 10))
	{
		//called on mouse-over events in the gui
		char *variable;
//		char *function;
		char resultbuffer[8192], tmpbuffer[8192];
		char *vmnames[4] = {"cur: ", "ssqc: ", "csqc: ", "menu: "};
		char *values[4] = {NULL, NULL, NULL, NULL};
		int i;
		Cmd_TokenizeString(text, false, false);
		variable = Cmd_Argv(1);
//		function = Cmd_Argv(2);

		
		//togglebreakpoint just finds the first statement (via the function table for file names) with the specified line number, and sets some unused high bit that causes it to be an invalid opcode.
		if (debuggerinstance)
		{
			if (debuggerinstance->EvaluateDebugString)
				values[0] = debuggerinstance->EvaluateDebugString(debuggerinstance, variable);
		}
		else
		{
#ifndef CLIENTONLY
			if (sv.world.progs && sv.world.progs->EvaluateDebugString)
				values[1] = sv.world.progs->EvaluateDebugString(sv.world.progs, variable);
#endif
#ifndef SERVERONLY
#ifdef CSQC_DAT
			if (csqc_world.progs && csqc_world.progs->EvaluateDebugString)
				values[2] = csqc_world.progs->EvaluateDebugString(csqc_world.progs, variable);
#endif
#ifdef MENU_DAT
			if (menu_world.progs && menu_world.progs->EvaluateDebugString)
				values[3] = menu_world.progs->EvaluateDebugString(menu_world.progs, variable);
#endif
#endif
		}

		for (i = 0, *resultbuffer = 0; i < 4; i++)
		{
			if (!values[i])
				continue;
			if (!strcmp(values[i], "(unable to evaluate)"))
				continue;
			if (*resultbuffer || !debuggerinstance)
				Q_strncatz(resultbuffer, "\n", sizeof(resultbuffer));
			if (!debuggerinstance)
			{
				Q_strncatz(resultbuffer, vmnames[i], sizeof(resultbuffer));
			}
			Q_strncatz(resultbuffer, COM_QuotedString(values[i], tmpbuffer, sizeof(tmpbuffer), true), sizeof(resultbuffer));
		}

		printf("qcvalue \"%s\" %s\n", variable, COM_QuotedString(resultbuffer, tmpbuffer, sizeof(tmpbuffer), false));
		fflush(stdout);
	}
	else if (!strncmp(text, "qcreload", 8))
	{
#if defined(MENU_DAT) && !defined(SERVERONLY)
		Cbuf_AddText("menu_restart\n", RESTRICT_LOCAL);
#endif
#ifndef CLIENTONLY
		if (sv.state)
			Cbuf_AddText("restart\n", RESTRICT_LOCAL);
#endif
//		Host_EndGame("Reloading QC");
		debuggerresume = DEBUG_TRACE_ABORTERROR;
	}
	else if (!strncmp(text, "qcbreakpoint ", 13))
	{
		int mode;
		char *filename;
		int line;
		Cmd_TokenizeString(text, false, false);
		mode = strtoul(Cmd_Argv(1), NULL, 0);
		filename = Cmd_Argv(2);
		line = strtoul(Cmd_Argv(3), NULL, 0);
		//togglebreakpoint just finds the first statement (via the function table for file names) with the specified line number, and sets some unused high bit that causes it to be an invalid opcode.
#ifndef SERVERONLY
#ifdef CSQC_DAT
		if (csqc_world.progs && csqc_world.progs->ToggleBreak)
			csqc_world.progs->ToggleBreak(csqc_world.progs, filename, line, mode);
#endif
#ifdef MENU_DAT
		if (menu_world.progs && menu_world.progs->ToggleBreak)
			menu_world.progs->ToggleBreak(menu_world.progs, filename, line, mode);
#endif
#endif
#ifndef CLIENTONLY
		if (sv.world.progs && sv.world.progs->ToggleBreak)
			sv.world.progs->ToggleBreak(sv.world.progs, filename, line, mode);
#endif
	}
	else
		return false;
	return true;
}

int QDECL QCEditor (pubprogfuncs_t *prinst, const char *filename, int *line, int *statement, int firststatement, char *reason, pbool fatal)
{
//#if defined(_WIN32) && !defined(FTE_SDL) && !defined(_XBOX)
	if (isPlugin >= 2)
	{
		if (wantquit)
			return DEBUG_TRACE_ABORTERROR;
		if (!*filename || !line || !*line)	//don't try editing an empty line, it won't work
		{
			if (!*filename)
				Con_Printf("Unable to debug, please disable optimisations\n");
			else
				Con_Printf("Unable to debug, please provide line number info\n");
			if (fatal)
				return DEBUG_TRACE_ABORTERROR;
			return DEBUG_TRACE_OFF;
		}
#ifdef SERVERONLY
		SV_GetConsoleCommands();
#else
		Sys_SendKeyEvents();
#endif
		debuggerresume = -1;
		debuggerresumeline = *line;
#if defined(_WIN32) && !defined(FTE_SDL)
		if (debuggerwnd)
			SetForegroundWindow((HWND)debuggerwnd);
#endif
		if (reason)
		{
			char tmpbuffer[8192];
			printf("qcfault \"%s\":%i %s\n", filename, *line, COM_QuotedString(reason, tmpbuffer, sizeof(tmpbuffer), false));
		}
		else
			printf("qcstep \"%s\":%i\n", filename, *line);
		fflush(stdout);
		debuggerinstance = prinst;
		debuggerfile = filename;
#ifdef SERVERONLY
		if (reason)
		{
			printf("Debugger triggered at \"%s\":%i, %s\n", filename, *line, reason);
			PR_StackTrace(prinst, 1);
		}
		while(debuggerresume == -1 && !wantquit)
		{
#if defined(_WIN32) && !defined(FTE_SDL)
			Sleep(10);
#else
			usleep(10*1000);
#endif
			SV_GetConsoleCommands();
		}
#else
#if defined(_WIN32) && !defined(FTE_SDL)
		INS_UpdateGrabs(false, false);
#endif
		if (reason)
			Con_Footerf(NULL, false, "^bDebugging: %s", reason);
		else
			Con_Footerf(NULL, false, "^bDebugging");
		while(debuggerresume == -1 && !wantquit)
		{
#if defined(_WIN32) && !defined(FTE_SDL)
			Sleep(10);
#else
			usleep(10*1000);
#endif
			Sys_SendKeyEvents();

			if (qrenderer)
			{
				//FIXME: display a stack trace and locals instead
				R2D_ImageColours(0.1, 0, 0, 1);
				R2D_FillBlock(0, 0, vid.width, vid.height);
				Con_DrawConsole(vid.height/2, true);	//draw console at half-height
				debuggerstacky = vid.height/2;
				if (debuggerstacky)
					PR_StackTrace(prinst, 2);
				debuggerstacky = 0;
				if (R2D_Flush)
					R2D_Flush();
				VID_SwapBuffers();
			}
		}
		Con_Footerf(NULL, false, "");
#endif
		*line = debuggerresumeline;
		debuggerinstance = NULL;
		debuggerfile = NULL;
		if (wantquit)
			return DEBUG_TRACE_ABORTERROR;
		return debuggerresume;
	}
//#endif

#ifdef TEXTEDITOR
	return QCLibEditor(prinst, filename, line, statement, firststatement, reason, fatal);
#else
	if (fatal)
		return DEBUG_TRACE_ABORTERROR;
	return DEBUG_TRACE_OFF;	//get lost
#endif
}

//tag warnings/errors for easier debugging.
int PR_Print (qboolean dev, const char *msg)
{
	char		file[MAX_OSPATH];
	int			line = -1;
	char		*ls, *ms, *nl;

	while (*msg)
	{
		nl = strchr(msg, '\n');
		if (nl)
			*nl = 0;
		*file = 0;

		/*when we're debugging, stack dumps should appear directly on-screen instead of being shoved on the console*/
#ifndef SERVERONLY
		if (debuggerstacky)
		{
			Draw_FunString(0, debuggerstacky, msg);
			debuggerstacky += 8;
			if (nl)
				msg = nl+1;
			else
				break;
			continue;
		}
#endif

		ls = strchr(msg, ':');
		if (ls)
		{
			ms = strchr(ls+1, ':');
			if (ms)
			{
				*ms = '\0';
				if (!strchr(msg, ' ') && !strchr(msg, '\t') && !strchr(msg, '\r') && (ls - msg) < sizeof(file)-1)
				{
					memcpy(file, msg, ls - msg);
					file[ls-msg] = 0;
					line = strtoul(ls+1, NULL, 0);
				}
				*ms = ':';
			}
		}

		if (dev)
		{
			if (*file)
				Con_DPrintf ("^[%s\\edit\\%s:%i^]", msg, file, line);
			else
				Con_DPrintf ("%s", msg);
		}
		else
		{
			if (*file)
				Con_Printf ("^[%s\\edit\\%s:%i^]", msg, file, line);
			else
				Con_Printf ("%s", msg);
		}

		if (nl)
		{
			if (dev)
				Con_DPrintf ("\n");
			else
				Con_Printf ("\n");
			msg = nl+1;
		}
		else
			break;
	}
	return 0;
}

int PR_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg), fmt,argptr);
	va_end (argptr);

	return PR_Print(false, msg);
}
int PR_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg), fmt,argptr);
	va_end (argptr);

	return PR_Print(true, msg);
}

#define MAX_TEMPSTRS	((int)pr_tempstringcount.value)
#define MAXTEMPBUFFERLEN	((int)pr_tempstringsize.value)
string_t PR_TempString(pubprogfuncs_t *prinst, const char *str)
{
	char *tmp;
	if (!prinst->user.tempstringbase)
		return prinst->TempString(prinst, str);

	if (!str || !*str)
		return 0;

	if (prinst->user.tempstringnum == MAX_TEMPSTRS)
		prinst->user.tempstringnum = 0;
	tmp = prinst->user.tempstringbase + (prinst->user.tempstringnum++)*MAXTEMPBUFFERLEN;

	Q_strncpyz(tmp, str, MAXTEMPBUFFERLEN);
	return tmp - prinst->stringtable;
}

void PF_InitTempStrings(pubprogfuncs_t *prinst)
{
	if (pr_tempstringcount.value > 0 && pr_tempstringcount.value < 2)
		pr_tempstringcount.value = 2;
	if (pr_tempstringsize.value < 256)
		pr_tempstringsize.value = 256;
	pr_tempstringcount.flags |= CVAR_NOSET;
	pr_tempstringsize.flags |= CVAR_NOSET;

	if (pr_tempstringcount.value >= 2)
		prinst->user.tempstringbase = prinst->AddString(prinst, "", MAXTEMPBUFFERLEN*MAX_TEMPSTRS, false);
	else
		prinst->user.tempstringbase = 0;
	prinst->user.tempstringnum = 0;
}

//#define	RETURN_EDICT(pf, e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(pf, e))
#define	RETURN_SSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_SetString(prinst, s))	//static - exe will not change it.
#define	RETURN_TSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_TempString(prinst, s))	//temp (static but cycle buffers)
#define	RETURN_CSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_SetString(prinst, s))	//semi-permanant. (hash tables?)
#define	RETURN_PSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_NewString(prinst, s, 0))	//permanant



void VARGS PR_BIError(pubprogfuncs_t *progfuncs, char *format, ...)
{
	va_list		argptr;
	static char		string[2048];

	va_start (argptr, format);
	vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	if (developer.value || !progfuncs)
	{
		struct globalvars_s *pr_globals = PR_globals(progfuncs, PR_CURRENT);
		PR_RunWarning(progfuncs, CON_ERROR"%s\n", string);
		G_INT(OFS_RETURN)=0;	//just in case it was a float and should be an ent...
		G_INT(OFS_RETURN+1)=0;
		G_INT(OFS_RETURN+2)=0;
	}
	else
	{
		PR_StackTrace(progfuncs, false);
//		PR_AbortStack(progfuncs);
		progfuncs->parms->Abort (CON_ERROR"%s", string);
	}
}


pbool QDECL QC_WriteFile(const char *name, void *data, int len)
{
	char buffer[256];
	Q_snprintfz(buffer, sizeof(buffer), "%s", name);
	COM_WriteFile(buffer, FS_GAMEONLY, data, len);
	return true;
}

//a little loop so we can keep track of used mem
void *VARGS PR_CB_Malloc(int size)
{
	return BZ_Malloc(size);//Z_TagMalloc (size, 100);
}
void VARGS PR_CB_Free(void *mem)
{
	BZ_Free(mem);
}

////////////////////////////////////////////////////
//JSON stuff.
typedef struct qcjson_s
{
	int type;
	string_t name;
	union
	{
		struct
		{
			int childptr;
			unsigned int count;	//for arrays and objects.
		};
		double num;			//for number types.
		string_t strofs;	//for strings.
	} u;
} qcjson_t;
static qcjson_t json_null = {json_type_null};	//dummy safe node that we poke on bad inputs.
#define JSONFromQC(qcptr) ((unsigned int)qcptr >= prinst->stringtablesize-sizeof(qcjson_t))?PR_BIError (prinst, "PR_JSONFromQC: bad pointer"),&json_null:qcptr?(qcjson_t*)((char*)prinst->stringtable + qcptr):&json_null
#define RETURN_JSON(r) G_INT(OFS_RETURN) = (const char*)r - prinst->stringtable

#define JSON_PERSIST_STRINGDATA	//slower but safer
static void PR_JSON_Count(json_t *r, size_t *nodes, size_t *strings)	//counts the nodes and bytes for string data.
{
	*nodes+=1;
	if (*r->name)
		*strings += strlen(r->name)+1;
	safeswitch(r->type)
	{
	case json_type_object:
	case json_type_array:
		for(r = r->child; r; r = r->sibling)
			PR_JSON_Count(r, nodes, strings);
		break;
	case json_type_string:
#ifndef JSON_PERSIST_STRINGDATA
		*strings += JSON_ReadBody(r, NULL, 0)+1;
		break;
#endif
	case json_type_true:
	case json_type_false:
	case json_type_null:
	case json_type_number:
	safedefault:
		break;
	}
}
static void PR_JSON_Linearise(pubprogfuncs_t *prinst, json_t *r, qcjson_t *out, qcjson_t **nodes, char **strings)
{
	json_t *c;
	size_t children = 0;
	out->type = r->type;

	//give it a name
	if (*r->name)
	{
		out->name = *strings - prinst->stringtable;
		memcpy(*strings, r->name, strlen(r->name)+1);
		*strings += strlen(r->name)+1;
	}

	//make sure its values are valid.
	safeswitch(out->type)
	{
	case json_type_string:
#ifdef JSON_PERSIST_STRINGDATA
		{	//allocate a tempstring for each, so that they last beyond node destruction.
			size_t sz = JSON_ReadBody(r, NULL, 0);
			char *tmp = alloca(sz+1);
			JSON_ReadBody(r, tmp, sz+1);
			out->u.strofs = PR_TempString(prinst, tmp);
		}
#else
		out->u.strofs = *strings - prinst->stringtable;
		JSON_ReadBody(r, *strings, prinst->stringtablesize - out->u.strofs-1);
		*strings += strlen(*strings)+1;
#endif
		break;
	case json_type_object:
	case json_type_array:
		out->u.childptr = (char*)*nodes - prinst->stringtable;
		for(c = r->child; c; c = c->sibling)
			children++;
		out->u.count = children;
		out = *nodes;
		*nodes+=children;
		for(c = r->child; c; c = c->sibling, out++)
			PR_JSON_Linearise(prinst, c, out, nodes, strings);
		break;
	case json_type_false:
	case json_type_null:
		out->u.num = false;
		break;
	case json_type_true:
		out->u.num = true;
		break;
	case json_type_number:
	safedefault:
		out->u.num = JSON_ReadFloat(r, 0);
		break;
	}
}
void QCBUILTIN PF_json_parse(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	json_t *inroot = JSON_Parse(PR_GetStringOfs(prinst, OFS_PARM0));
	qcjson_t *outroot, *outnodes;
	char *outstrings;
	size_t n = 0, s = 0;
	if (inroot)
	{
		//linearise the json nodes for easy access (just use qc pointers instead of needing lots of separate handles)
		PR_JSON_Count(inroot, &n, &s);
		outnodes = prinst->AddressableAlloc(prinst, sizeof(*outroot)*n + s);
		outstrings = (char*)(outnodes + n);

		outroot = outnodes++;
		PR_JSON_Linearise(prinst, inroot, outroot, &outnodes, &outstrings);

		JSON_Destroy(inroot);	//our input string becomes irrelevant at this point

		RETURN_JSON(outroot);
	}
	else
		G_INT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_json_get_value_type(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	G_INT(OFS_RETURN) = handle->type;
}
void QCBUILTIN PF_json_get_name(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	G_INT(OFS_RETURN) = handle->name;
}
void QCBUILTIN PF_json_get_integer(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	safeswitch (handle->type)
	{
	case json_type_number:
	case json_type_true:
	case json_type_false:
		G_INT(OFS_RETURN) = handle->u.num;
		break;
	case json_type_string:
		G_INT(OFS_RETURN) = atoi(PR_GetString(prinst, handle->u.strofs));
		break;
	case json_type_object:
	case json_type_array:
	case json_type_null:
	safedefault:
		G_INT(OFS_RETURN) = 0;
		break;
	}
}
void QCBUILTIN PF_json_get_float(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	safeswitch (handle->type)
	{
	case json_type_number:
	case json_type_true:
	case json_type_false:
		G_FLOAT(OFS_RETURN) = handle->u.num;
		break;
	case json_type_string:
		G_FLOAT(OFS_RETURN) = atof(PR_GetString(prinst, handle->u.strofs));
		break;
	case json_type_object:
	case json_type_array:
	case json_type_null:
	safedefault:
		G_FLOAT(OFS_RETURN) = 0;
		break;
	}
}
void QCBUILTIN PF_json_get_string(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	if (handle->type != json_type_string)
		G_INT(OFS_RETURN) = 0;
	else
		G_INT(OFS_RETURN) = handle->u.strofs;
}
void QCBUILTIN PF_json_get_child_at_index(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	size_t idx = G_INT(OFS_PARM1);
	G_INT(OFS_RETURN) = 0;	//assume the worst
	switch(handle->type)
	{
	case json_type_array:
	case json_type_object:
		if (idx < handle->u.count)
			G_INT(OFS_RETURN) = handle->u.childptr + idx*sizeof(*handle);	//don't really need to validate this here.
		break;
	default:
		break;
	}
}
void QCBUILTIN PF_json_get_length(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *handle = JSONFromQC(G_INT(OFS_PARM0));
	switch (handle->type)
	{
	case json_type_object:
	case json_type_array:
		G_INT(OFS_RETURN) = handle->u.count;
		break;
	default:
		G_INT(OFS_RETURN) = 0;
		break;
	}
}
void QCBUILTIN PF_json_find_object_child(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcjson_t *parent = JSONFromQC(G_INT(OFS_PARM0));
	const char *childname = PR_GetStringOfs(prinst, OFS_PARM1);
	unsigned int idx;
	G_INT(OFS_RETURN) = 0;	//assume the worst
	switch (parent->type)
	{
	case json_type_object:
	case json_type_array:
		for (idx = 0; idx < parent->u.count; idx++)
		{
			qcjson_t *childnode = JSONFromQC(parent->u.childptr + idx*sizeof(*childnode));
			if (!strcmp(childname, PR_GetString(prinst, childnode->name)))
			{
				RETURN_JSON(childnode);
				break;
			}
		}
		break;
	default:
		break;
	}
}

#ifdef FTE_TARGET_WEB
#include <emscripten.h>
//FIXME: make sure the module is signed/'local'/trusted
void QCBUILTIN PF_js_run_script(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *jscript = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *ret;
	ret = emscripten_run_script_string(jscript);
	if (ret)
		G_INT(OFS_RETURN) = PR_TempString(prinst, ret);
	else
		G_INT(OFS_RETURN) = 0;
}
#endif


////////////////////////////////////////////////////
//model functions
//DP_QC_GETSURFACE
static void PF_BuildSurfaceMesh(model_t *model, unsigned int surfnum)
{
	//this function might be called on dedicated servers.
#ifdef Q1BSPS
	void ModQ1_Batches_BuildQ1Q2Poly(model_t *mod, msurface_t *surf, builddata_t *cookie);
	if (model->fromgame == fg_quake || model->fromgame == fg_halflife)
		ModQ1_Batches_BuildQ1Q2Poly(model, &model->surfaces[surfnum], NULL);
#endif
	//fixme: q3...
}
// #434 float(entity e, float s) getsurfacenumpoints (DP_QC_GETSURFACE)
void QCBUILTIN PF_getsurfacenumpoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int surfnum;
	model_t *model;
	wedict_t *ent;
	world_t *w = prinst->parms->user;

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);

	model = w->Get_CModel(w, ent->v->modelindex);

	if (!model || model->type != mod_brush || surfnum >= model->nummodelsurfaces)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		surfnum += model->firstmodelsurface;
		if (!model->surfaces[surfnum].mesh)
			PF_BuildSurfaceMesh(model, surfnum);
		if (model->surfaces[surfnum].mesh)
			G_FLOAT(OFS_RETURN) = model->surfaces[surfnum].mesh->numvertexes;
		else
			G_FLOAT(OFS_RETURN) = 0;	//not loaded properly.
	}
}
// #435 vector(entity e, float s, float n) getsurfacepoint (DP_QC_GETSURFACE)
void QCBUILTIN PF_getsurfacepoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int surfnum, pointnum;
	model_t *model;
	wedict_t *ent;
	world_t *w = prinst->parms->user;

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);
	pointnum = G_FLOAT(OFS_PARM2);

	model = w->Get_CModel(w, ent->v->modelindex);

	if (!model || model->type != mod_brush || surfnum >= model->nummodelsurfaces)
	{
		G_FLOAT(OFS_RETURN+0) = 0;
		G_FLOAT(OFS_RETURN+1) = 0;
		G_FLOAT(OFS_RETURN+2) = 0;
	}
	else
	{
		surfnum += model->firstmodelsurface;

		G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->xyz_array[pointnum][0];
		G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->xyz_array[pointnum][1];
		G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->xyz_array[pointnum][2];
	}
}
// #436 vector(entity e, float s) getsurfacenormal (DP_QC_GETSURFACE)
void QCBUILTIN PF_getsurfacenormal(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int surfnum;
	model_t *model;
	wedict_t *ent;
	world_t *w = prinst->parms->user;

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);

	model = w->Get_CModel(w, ent->v->modelindex);

	if (!model || model->type != mod_brush || surfnum >= model->nummodelsurfaces || !model->surfaces[model->firstmodelsurface+surfnum].plane)
	{	//non-planar surfs don't always have a single plane... return nothing instead of breaking.
		G_FLOAT(OFS_RETURN+0) = 0;
		G_FLOAT(OFS_RETURN+1) = 0;
		G_FLOAT(OFS_RETURN+2) = 0;
	}
	else
	{
		surfnum += model->firstmodelsurface;

		G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].plane->normal[0];
		G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].plane->normal[1];
		G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].plane->normal[2];
		if (model->surfaces[surfnum].flags & SURF_PLANEBACK)
			VectorInverse(G_VECTOR(OFS_RETURN));
	}
}
// #437 string(entity e, float s) getsurfacetexture (DP_QC_GETSURFACE)
void QCBUILTIN PF_getsurfacetexture(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	model_t *model;
	wedict_t *ent;
	msurface_t *surf;
	int surfnum;
	world_t *w = prinst->parms->user;

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);
	model = w->Get_CModel(w, ent->v->modelindex);

	G_INT(OFS_RETURN) = 0;
	if (!model || model->type != mod_brush)
		return;

	if (surfnum >= model->nummodelsurfaces)
		return;
	if (surfnum < 0)
	{
		surfnum = -1-surfnum;
		if ((unsigned)surfnum >= (unsigned)model->numtextures)
			return;	//nope, outta range.
		if (!model->textures[surfnum])
			return;	//some maps are just broken.
		G_INT(OFS_RETURN) = PR_TempString(prinst, model->textures[surfnum]->name);
	}
	else
	{
		surfnum += model->firstmodelsurface;
		surf = &model->surfaces[surfnum];
		G_INT(OFS_RETURN) = PR_TempString(prinst, surf->texinfo->texture->name);
	}
}
#define TriangleNormal(a,b,c,n) ( \
	(n)[0] = ((a)[1] - (b)[1]) * ((c)[2] - (b)[2]) - ((a)[2] - (b)[2]) * ((c)[1] - (b)[1]), \
	(n)[1] = ((a)[2] - (b)[2]) * ((c)[0] - (b)[0]) - ((a)[0] - (b)[0]) * ((c)[2] - (b)[2]), \
	(n)[2] = ((a)[0] - (b)[0]) * ((c)[1] - (b)[1]) - ((a)[1] - (b)[1]) * ((c)[0] - (b)[0]) \
	)
static float getsurface_clippointpoly(model_t *model, msurface_t *surf, pvec3_t point, pvec3_t bestcpoint, float bestdist)
{
	int e, edge;
	vec3_t edgedir, edgenormal, cpoint, temp;
	mvertex_t *v1, *v2;
	float dist = DotProduct(point, surf->plane->normal) - surf->plane->dist;
	//don't care about SURF_PLANEBACK, the maths works out the same.

	if (dist*dist < bestdist)
	{	//within a specific range
		//make sure it's within the poly
		VectorMA(point, dist, surf->plane->normal, cpoint);
		for (e = surf->firstedge+surf->numedges; e > surf->firstedge; edge++)
		{
			edge = model->surfedges[--e];
			if (edge < 0)
			{
				v1 = &model->vertexes[model->edges[-edge].v[0]];
				v2 = &model->vertexes[model->edges[-edge].v[1]];
			}
			else
			{
				v2 = &model->vertexes[model->edges[edge].v[0]];
				v1 = &model->vertexes[model->edges[edge].v[1]];
			}
			
			VectorSubtract(v1->position, v2->position, edgedir);
			CrossProduct(edgedir, surf->plane->normal, edgenormal);
			if (!(surf->flags & SURF_PLANEBACK))
			{
				VectorNegate(edgenormal, edgenormal);
			}
			VectorNormalize(edgenormal);

			dist = DotProduct(v1->position, edgenormal) - DotProduct(cpoint, edgenormal);
			if (dist < 0)
				VectorMA(cpoint, dist, edgenormal, cpoint);
		}

		VectorSubtract(cpoint, point, temp);
		dist = DotProduct(temp, temp);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(cpoint, bestcpoint);
		}
	}
	return bestdist;
}
static float getsurface_clippointtri(model_t *model, msurface_t *surf, pvec3_t point, pvec3_t bestcpoint, float bestdist)
{
	int j;
	mesh_t *mesh = surf->mesh;
	vec3_t trinorm, edgedir, edgenormal, temp, cpoint;
	float dist;
	int e;
	float *v1, *v2;
	if (!mesh)
	{
		PF_BuildSurfaceMesh(model, surf - model->surfaces);
		mesh = surf->mesh;
		if (!mesh)
			return 0;
	}
	for (j = 0; j < mesh->numindexes; j+=3)
	{
		//calculate the distance from the plane
		TriangleNormal(mesh->xyz_array[mesh->indexes[j+2]], mesh->xyz_array[mesh->indexes[j+1]], mesh->xyz_array[mesh->indexes[j+0]], trinorm);
		if (!trinorm[0] && !trinorm[1] && !trinorm[2])
			continue;
		VectorNormalize(trinorm);
		dist = DotProduct(point, trinorm) - DotProduct(mesh->xyz_array[mesh->indexes[j+0]], trinorm);
		if (dist*dist < bestdist)
		{
			//set cpoint to be the point on the plane
			VectorMA(point, -dist, trinorm, cpoint);

			//clip to each edge of the triangle
			for (e = 0; e < 3; e++)
			{
				v1 = mesh->xyz_array[mesh->indexes[j+e]];
				v2 = mesh->xyz_array[mesh->indexes[j+((e+1)%3)]];

				VectorSubtract(v1, v2, edgedir);
				CrossProduct(edgedir, trinorm, edgenormal);
				VectorNormalize(edgenormal);

				dist = DotProduct(cpoint, edgenormal) - DotProduct(v1, edgenormal);
				if (dist < 0)
					VectorMA(cpoint, -dist, edgenormal, cpoint);
			}

			//if the point is closer, we win.
			VectorSubtract(cpoint, point, temp);
			dist = DotProduct(temp, temp);
			if (dist < bestdist)
			{
				bestdist = dist;
				VectorCopy(cpoint, bestcpoint);
			}
		}
	}
	return bestdist;
}
msurface_t *Mod_GetSurfaceNearPoint(model_t *model, pvec3_t point)
{
	msurface_t *surf;
	int i;

	pvec3_t cpoint = {0,0,0};
	float bestdist = FLT_MAX, dist;
	msurface_t *bestsurf = NULL;

	if (model->fromgame == fg_quake || model->fromgame == fg_quake2)
	{
		//all polies, we can skip parts. special case.
		surf = model->surfaces + model->firstmodelsurface;
		for (i = 0; i < model->nummodelsurfaces; i++, surf++)
		{
			dist = getsurface_clippointpoly(model, surf, point, cpoint, bestdist);
			if (dist < bestdist)
			{
				bestdist = dist;
				bestsurf = surf;
			}
		}
	}
	else
	{
		//if performance is needed, I suppose we could try walking bsp nodes a bit
		surf = model->surfaces + model->firstmodelsurface;
		for (i = 0; i < model->nummodelsurfaces; i++, surf++)
		{
			dist = getsurface_clippointtri(model, surf, point, cpoint, bestdist);
			if (dist < bestdist)
			{
				bestdist = dist;
				bestsurf = surf;
			}
		}
	}
	return bestsurf;
}
// #438 float(entity e, vector p) getsurfacenearpoint (DP_QC_GETSURFACE)
void QCBUILTIN PF_getsurfacenearpoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t *ent = G_WEDICT(prinst, OFS_PARM0);
	pvec_t *point = G_VECTOR(OFS_PARM1);

	world_t *w = prinst->parms->user;
	model_t *model = w->Get_CModel(w, ent->v->modelindex);
	msurface_t *surf;

	G_FLOAT(OFS_RETURN) = -1;
	if (model && model->type == mod_brush)
	{
		surf = Mod_GetSurfaceNearPoint(model, point);
		if (surf)
			G_FLOAT(OFS_RETURN) = surf - (model->surfaces + model->firstmodelsurface);
	}
}

// #439 vector(entity e, float s, vector p) getsurfaceclippedpoint (DP_QC_GETSURFACE)
void QCBUILTIN PF_getsurfaceclippedpoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	model_t *model;
	wedict_t *ent;
	msurface_t *surf;
	float *point;
	unsigned int surfnum;

	world_t *w = prinst->parms->user;
	pvec_t *result = G_VECTOR(OFS_RETURN);

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);
	point = G_VECTOR(OFS_PARM2);

	VectorCopy(point, result);

	model = w->Get_CModel(w, ent->v->modelindex);

	if (!model || model->type != mod_brush)
		return;
	if (surfnum >= model->nummodelsurfaces)
		return;

	if (model->fromgame == fg_quake)
	{
		//all polies, we can skip parts. special case.
		surf = model->surfaces + model->firstmodelsurface + surfnum;
		getsurface_clippointpoly(model, surf, point, result, FLT_MAX);
	}
	else
	{
		//if performance is needed, I suppose we could try walking bsp nodes a bit
		surf = model->surfaces + model->firstmodelsurface + surfnum;
		getsurface_clippointtri(model, surf, point, result, FLT_MAX);
	}
}

// #628 float(entity e, float s) getsurfacenumtriangles
void QCBUILTIN PF_getsurfacenumtriangles(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int surfnum;
	model_t *model;
	wedict_t *ent;
	world_t *w = prinst->parms->user;

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);

	model = w->Get_CModel(w, ent->v->modelindex);

	if (!model || model->type != mod_brush || surfnum >= model->nummodelsurfaces)
	{
		G_FLOAT(OFS_RETURN) = 0;
	}
	else
	{
		surfnum += model->firstmodelsurface;
		if (!model->surfaces[surfnum].mesh)
			PF_BuildSurfaceMesh(model, surfnum);
		if (model->surfaces[surfnum].mesh)
			G_FLOAT(OFS_RETURN) = model->surfaces[surfnum].mesh->numindexes/3;
		else
			G_FLOAT(OFS_RETURN) = 0;
	}
}
// #629 float(entity e, float s) getsurfacetriangle
void QCBUILTIN PF_getsurfacetriangle(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int surfnum, firstidx;
	model_t *model;
	wedict_t *ent;
	world_t *w = prinst->parms->user;

	ent = G_WEDICT(prinst, OFS_PARM0);
	surfnum = G_FLOAT(OFS_PARM1);
	firstidx = G_FLOAT(OFS_PARM2)*3;

	model = w->Get_CModel(w, ent->v->modelindex);

	if (model && model->type == mod_brush && surfnum < model->nummodelsurfaces)
	{
		surfnum += model->firstmodelsurface;

		if (!model->surfaces[surfnum].mesh)
			PF_BuildSurfaceMesh(model, surfnum);
		if (model->surfaces[surfnum].mesh)
		if (firstidx+2 < model->surfaces[surfnum].mesh->numindexes)
		{
			G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->indexes[firstidx+0];
			G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->indexes[firstidx+1];
			G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->indexes[firstidx+2];
			return;
		}
	}

	G_FLOAT(OFS_RETURN+0) = 0;
	G_FLOAT(OFS_RETURN+1) = 0;
	G_FLOAT(OFS_RETURN+2) = 0;
}

//vector(entity e, float s, float n, float a) getsurfacepointattribute
void QCBUILTIN PF_getsurfacepointattribute(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t *ent = G_WEDICT(prinst, OFS_PARM0);
	unsigned int surfnum = G_FLOAT(OFS_PARM1);
	unsigned int pointnum = G_FLOAT(OFS_PARM2);
	unsigned int attribute = G_FLOAT(OFS_PARM3);
	world_t *w = prinst->parms->user;
	model_t *model = w->Get_CModel(w, ent->v->modelindex);
	G_FLOAT(OFS_RETURN+0) = 0;
	G_FLOAT(OFS_RETURN+1) = 0;
	G_FLOAT(OFS_RETURN+2) = 0;
	if (model && model->type == mod_brush && surfnum < model->nummodelsurfaces)
	{
		surfnum += model->firstmodelsurface;
		if (!model->surfaces[surfnum].mesh)
			PF_BuildSurfaceMesh(model, surfnum);
		if (model->surfaces[surfnum].mesh)
		if (pointnum < model->surfaces[surfnum].mesh->numvertexes)
		{
			switch(attribute)
			{
			case 0:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->xyz_array[pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->xyz_array[pointnum][1];
				G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->xyz_array[pointnum][2];
				break;
			case 1:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->snormals_array[pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->snormals_array[pointnum][1];
				G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->snormals_array[pointnum][2];
				break;
			case 2:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->tnormals_array[pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->tnormals_array[pointnum][1];
				G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->tnormals_array[pointnum][2];
				break;
			case 3:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->normals_array[pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->normals_array[pointnum][1];
				G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->normals_array[pointnum][2];
				break;
			case 4:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->st_array[pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->st_array[pointnum][1];
				G_FLOAT(OFS_RETURN+2) = 0;
				break;
			case 5:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->lmst_array[0][pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->lmst_array[0][pointnum][1];
				G_FLOAT(OFS_RETURN+2) = 0;
				break;
			case 6:
				G_FLOAT(OFS_RETURN+0) = model->surfaces[surfnum].mesh->colors4f_array[0][pointnum][0];
				G_FLOAT(OFS_RETURN+1) = model->surfaces[surfnum].mesh->colors4f_array[0][pointnum][1];
				G_FLOAT(OFS_RETURN+2) = model->surfaces[surfnum].mesh->colors4f_array[0][pointnum][2];
				//no way to return alpha here.
				break;
			}
		}
	}
}

//#240 float(vector viewpos, entity viewee) checkpvs (FTE_QC_CHECKPVS)
//note: this requires a correctly setorigined entity.
void QCBUILTIN PF_checkpvs(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;
	model_t *worldmodel = world->worldmodel;
	VM_VECTORARG(viewpos, OFS_PARM0);
	wedict_t *ent = G_WEDICT(prinst, OFS_PARM1);
	int cluster;
	int qcpvsarea[2];
	qbyte *pvs;

	if (!worldmodel || worldmodel->loadstate != MLS_LOADED)
		G_FLOAT(OFS_RETURN) = false;
	else if (!worldmodel->funcs.FatPVS)
		G_FLOAT(OFS_RETURN) = true;
	else
	{
		qcpvsarea[0] = 1;
		cluster = worldmodel->funcs.ClusterForPoint(worldmodel, viewpos, &qcpvsarea[1]);
		pvs = worldmodel->funcs.ClusterPVS(worldmodel, cluster, NULL, PVM_FAST);
		G_FLOAT(OFS_RETURN) = worldmodel->funcs.EdictInFatPVS(worldmodel, &ent->pvsinfo, pvs, qcpvsarea);
	}
}

void QCBUILTIN PF_setattachment(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t *e = G_WEDICT(prinst, OFS_PARM0);
	wedict_t *tagentity = G_WEDICT(prinst, OFS_PARM1);
	const char *tagname = PR_GetStringOfs(prinst, OFS_PARM2);
	world_t *world = prinst->parms->user;

	model_t *model;

	int tagidx;

	tagidx = 0;

	if (tagentity != world->edicts && tagname && tagname[0])
	{
		model = world->Get_CModel(world, tagentity->v->modelindex);
		if (model)
		{
			if (model && model->loadstate == MLS_LOADING)
				COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
			tagidx = Mod_TagNumForName(model, tagname, 0);
			if (tagidx == 0)
				Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i (model \"%s\") but could not find it\n", NUM_FOR_EDICT(prinst, e), NUM_FOR_EDICT(prinst, tagentity), tagname, tagname, NUM_FOR_EDICT(prinst, tagentity), model->name);
		}
		else
			Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): Couldn't load model\n", NUM_FOR_EDICT(prinst, e), NUM_FOR_EDICT(prinst, tagentity), tagname);
	}

	e->xv->tag_entity = EDICT_TO_PROG(prinst, tagentity);
	e->xv->tag_index = tagidx;
}

#ifndef TERRAIN
void QCBUILTIN PF_terrain_edit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = false;
}
#endif

//end model functions
////////////////////////////////////////////////////

void QCBUILTIN PF_touchtriggers(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t *ent = ((prinst->callargc>0)?G_WEDICT(prinst, OFS_PARM0):PROG_TO_WEDICT(prinst, *w->g.self));
	if (prinst->callargc > 1)
	{
		if (ent->readonly)
		{
			Con_Printf("setorigin on readonly entity %i\n", ent->entnum);
			return;
		}
		VectorCopy(G_VECTOR(OFS_PARM1), ent->v->origin);
	}
	World_LinkEdict (w, ent, true);
}


////////////////////////////////////////////////////
//Finding

//entity(string field, float match) findchainflags = #450
//chained search for float reference fields
void QCBUILTIN PF_findchainflags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	unsigned int ff, cf;
	int s;
	wedict_t	*ent, *chain;

	chain = (wedict_t *) *prinst->parms->edicts;

	ff = G_INT(OFS_PARM0)+prinst->fieldadjust;
	s = G_FLOAT(OFS_PARM1);
	if (prinst->callargc > 2)
		cf = G_INT(OFS_PARM2)+prinst->fieldadjust;
	else
		cf = &((comentvars_t*)NULL)->chain - (pint_t*)NULL;
	if (ff >= prinst->activefieldslots || cf >= prinst->activefieldslots)
	{
		PR_BIError (prinst, "PF_FindChain: bad field reference");
		return;
	}

	for (i = 1; i < *prinst->parms->num_edicts; i++)
	{
		ent = WEDICT_NUM_PB(prinst, i);
		if (ED_ISFREE(ent))
			continue;
		if (!((int)((pvec_t *)ent->v)[ff] & s))
			continue;

		((pint_t*)ent->v)[cf] = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, chain);
}

//entity(string field, float match) findchainfloat = #403
//chained search for float, int, and entity reference fields
void QCBUILTIN PF_findchainfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	unsigned int ff, cf;
	float s;
	wedict_t	*ent, *chain;

	chain = (wedict_t *) *prinst->parms->edicts;

	ff = G_INT(OFS_PARM0)+prinst->fieldadjust;
	s = G_FLOAT(OFS_PARM1);
	if (prinst->callargc > 2)
		cf = G_INT(OFS_PARM2)+prinst->fieldadjust;
	else
		cf = &((comentvars_t*)NULL)->chain - (pint_t*)NULL;
	if (ff >= prinst->activefieldslots || cf >= prinst->activefieldslots)
	{
		PR_BIError (prinst, "PF_FindChain: bad field reference");
		return;
	}

	for (i = 1; i < *prinst->parms->num_edicts; i++)
	{
		ent = WEDICT_NUM_PB(prinst, i);
		if (ED_ISFREE(ent))
			continue;
		if (((pvec_t *)ent->v)[ff] != s)
			continue;

		((pint_t*)ent->v)[cf] = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, chain);
}

//entity(string field, string match) findchain = #402
//chained search for strings in entity fields
void QCBUILTIN PF_findchain (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	unsigned int ff, cf;
	const char *s;
	string_t t;
	wedict_t *ent, *chain;
	
	chain = (wedict_t *) *prinst->parms->edicts;

	ff = G_INT(OFS_PARM0)+prinst->fieldadjust;
	s = PR_GetStringOfs(prinst, OFS_PARM1);
	if (prinst->callargc > 2)
		cf = G_INT(OFS_PARM2)+prinst->fieldadjust;
	else
		cf = &((comentvars_t*)NULL)->chain - (int*)NULL;
	if (ff >= prinst->activefieldslots || cf >= prinst->activefieldslots)
	{
		PR_BIError (prinst, "PF_FindChain: bad field reference");
		return;
	}

	for (i = 1; i < *prinst->parms->num_edicts; i++)
	{
		ent = WEDICT_NUM_PB(prinst, i);
		if (ED_ISFREE(ent))
			continue;
		t = *(string_t *)&((float*)ent->v)[ff];
		if (!t)
			continue;
		if (strcmp(PR_GetString(prinst, t), s))
			continue;

		((int*)ent->v)[cf] = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, chain);
}

//EXTENSION: DP_QC_FINDFLAGS
//entity(entity start, float fld, float match) findflags = #449
void QCBUILTIN PF_FindFlags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int e;
	unsigned int f;
	int s;
	wedict_t *ed;

	e = G_EDICTNUM(prinst, OFS_PARM0);
	f = G_INT(OFS_PARM1)+prinst->fieldadjust;
	if (f >= prinst->activefieldslots)
	{
		PR_BIError (prinst, "PF_FindFlags: bad field reference");
		return;
	}
	s = G_FLOAT(OFS_PARM2);

	for (e++; e < *prinst->parms->num_edicts; e++)
	{
		ed = WEDICT_NUM_PB(prinst, e);
		if (ED_ISFREE(ed))
			continue;
		if ((int)((float *)ed->v)[f] & s)
		{
			RETURN_EDICT(prinst, ed);
			return;
		}
	}

	RETURN_EDICT(prinst, *prinst->parms->edicts);
}

//entity(entity start, float fld, float match) findfloat = #98
void QCBUILTIN PF_FindFloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int e;
	unsigned int f;
	int s;
	wedict_t *ed;

#ifdef HAVE_LEGACY
	if (prinst->callargc != 3)	//I can hate mvdsv if I want to.
	{
		PR_BIError(prinst, "PF_FindFloat (#98): callargc != 3\nDid you mean to set pr_imitatemvdsv to 1?");
		return;
	}
#endif

	e = G_EDICTNUM(prinst, OFS_PARM0);
	f = G_INT(OFS_PARM1)+prinst->fieldadjust;
	if (f >= prinst->activefieldslots)
	{
		PR_BIError (prinst, "PF_FindFloat: bad field reference");
		return;
	}
	s = G_INT(OFS_PARM2);

	for (e++; e < *prinst->parms->num_edicts; e++)
	{
		ed = WEDICT_NUM_PB(prinst, e);
		if (ED_ISFREE(ed))
			continue;
		if (((int *)ed->v)[f] == s)
		{
			RETURN_EDICT(prinst, ed);
			return;
		}
	}

	RETURN_EDICT(prinst, *prinst->parms->edicts);
}

// entity (entity start, .string field, string match) find = #5;
void QCBUILTIN PF_FindString (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int		e;
	unsigned int		f;
	const char	*s;
	string_t t;
	wedict_t	*ed;

	e = G_EDICTNUM(prinst, OFS_PARM0);
	f = G_INT(OFS_PARM1)+prinst->fieldadjust;
	if (f >= prinst->activefieldslots)
	{
		PR_BIError (prinst, "PF_FindString: bad field reference");
		return;
	}
	s = PR_GetStringOfs(prinst, OFS_PARM2);

	if (!s || !*s)
	{	//checking for empty (and by extension null)
		//looking for an empty string is a bloody stupid thing to do, and basically always a bug, but existing code exists. either way it warrents a warning.
		if (developer.ival)
			PR_RunWarning(prinst, "PF_FindString: empty string\n");
		for (e++ ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			t = ((string_t *)ed->v)[f];
			if (!t || !*PR_GetString(prinst, t))
			{
				RETURN_EDICT(prinst, ed);
				return;
			}
		}
	}
	else
	{	//should be safe to assume that null is empty and thus never a match. speed
		for (e++ ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			t = ((string_t *)ed->v)[f];
			if (!t)
				continue;
			if (!strcmp(PR_GetString(prinst, t),s))
			{
				RETURN_EDICT(prinst, ed);
				return;
			}
		}
	}

	RETURN_EDICT(prinst, *prinst->parms->edicts);
}

#ifdef QCGC
void QCBUILTIN PF_FindList (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int		e;
	unsigned int f = G_INT(OFS_PARM0)+prinst->fieldadjust;
	wedict_t	*ed;
	etype_t type = G_INT(OFS_PARM2);

	int *list = alloca(sizeof(*list)*w->num_edicts);	//guess at a max
	int *retlist;
	unsigned found = 0;

	if (type <= ev_double)
	{
		extern const unsigned int type_size[];
		if (f < 0 || f+type_size[type] > prinst->activefieldslots)
		{	//invalid field.
			G_INT(OFS_PARM3) = G_INT(OFS_RETURN) = 0;
			return;
		}
	}
	else
	{	//unsupported field type.
		G_INT(OFS_PARM3) = G_INT(OFS_RETURN) = 0;
		return;
	}

	if (type == ev_string)
	{
		const char	*s = PR_GetStringOfs(prinst, OFS_PARM1);
		string_t t;
		if (!s)
			s = ""; /* o.O */
		for (e=1 ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed) || ed->readonly)
				continue;
			t = ((string_t *)ed->v)[f];
			if (!t)
				continue;
			if (!strcmp(PR_GetString(prinst, t),s))
				list[found++] = EDICT_TO_PROG(prinst, ed);
		}
	}
	else if (type == ev_float)
	{	//handling -0 properly requires care
		pvec_t s = G_FLOAT(OFS_PARM1);
		for (e=1 ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			if (((pvec_t*)ed->v)[f] == s)
				list[found++] = EDICT_TO_PROG(prinst, ed);
		}
	}
	else if (type == ev_double)
	{	//handling 64bit -0 properly requires care
		double s = G_DOUBLE(OFS_PARM1);
		for (e=1 ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			if (*(pdouble_t*)((pvec_t*)ed->v+f) == s)
				list[found++] = EDICT_TO_PROG(prinst, ed);
		}
	}
	else if (type == ev_int64 || type == ev_uint64)
	{	//handling -0 properly requires care
		pint64_t s = G_INT64(OFS_PARM1);
		for (e=1 ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			if (*(pint64_t*)((pint_t*)ed->v+f) == s)
				list[found++] = EDICT_TO_PROG(prinst, ed);
		}
	}
	else if (type == ev_vector)
	{	//big types...
		pvec_t *s = G_VECTOR(OFS_PARM1);
		for (e=1 ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			if (((pvec_t*)ed->v)[f+0] == s[0]&&
				((pvec_t*)ed->v)[f+1] == s[1]&&
				((pvec_t*)ed->v)[f+2] == s[2])
				list[found++] = EDICT_TO_PROG(prinst, ed);
		}
	}
	else
	{	//generic references and other stuff that can just be treated as ints
		pint_t s = G_INT(OFS_PARM1);
		for (e=1 ; e < *prinst->parms->num_edicts ; e++)
		{
			ed = WEDICT_NUM_PB(prinst, e);
			if (ED_ISFREE(ed))
				continue;
			if (((pint_t*)ed->v)[f] == s)
				list[found++] = EDICT_TO_PROG(prinst, ed);
		}
	}

	G_INT(OFS_PARM3) = found;
	G_INT(OFS_RETURN) = prinst->AllocTempString(prinst, (char**)&retlist, (found+1)*sizeof(*retlist));
	memcpy(retlist, list, found*sizeof(*retlist));
	retlist[found] = 0;
}
#endif

//Finding
////////////////////////////////////////////////////
//Cvars

cvar_t *PF_Cvar_FindOrGet(const char *var_name)
{
	const char *def;
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if (!var && pr_autocreatecvars.ival)
	{
		//this little chunk is so cvars dp creates are created with meaningful values
		def = "";
		if (!strcmp(var_name, "sv_maxairspeed"))
			def = "30";
		else if (!strcmp(var_name, "sv_jumpvelocity"))
			def = "270";
		else
			def = "";

		var = Cvar_Get(var_name, def, 0, "Implicit QC variables");
		if (var)
			Con_DPrintf("^3Created QC Cvar %s\n", var_name);
		else
			Con_Printf(CON_ERROR"Unable to create QC Cvar %s\n", var_name);
	}
	return var;
}

//string(string cvarname) cvar_string
void QCBUILTIN PF_cvar_string (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str = PR_GetStringOfs(prinst, OFS_PARM0);
	cvar_t *cv = PF_Cvar_FindOrGet(str);
	if (cv && !(cv->flags & CVAR_NOUNSAFEEXPAND))
	{
		if(cv->latched_string)
			RETURN_TSTRING(cv->latched_string);
		else
			RETURN_TSTRING(cv->string);
	}
	else
		G_INT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_cvars_haveunsaved (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = Cvar_UnsavedArchive();
}

//string(string cvarname) cvar_defstring
void QCBUILTIN PF_cvar_defstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str = PR_GetStringOfs(prinst, OFS_PARM0);
	cvar_t *cv = PF_Cvar_FindOrGet(str);
	if (cv && !(cv->flags & CVAR_NOUNSAFEEXPAND))
		RETURN_TSTRING(cv->defaultstr);
	else
		G_INT(OFS_RETURN) = 0;
}

//string(string cvarname) cvar_description
void QCBUILTIN PF_cvar_description (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str = PR_GetStringOfs(prinst, OFS_PARM0);
	cvar_t *cv = PF_Cvar_FindOrGet(str);
	if (cv && !(cv->flags & CVAR_NOUNSAFEEXPAND))
	{
		if (cv->description)
			RETURN_TSTRING(localtext(cv->description));
		else
			G_INT(OFS_RETURN) = 0;
	}
	else
		G_INT(OFS_RETURN) = 0;
}

//float(string name) cvar_type
void QCBUILTIN PF_cvar_type (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str = PR_GetStringOfs(prinst, OFS_PARM0);
	int ret = 0;
	cvar_t *v = Cvar_FindVar(str);	//this builtin MUST NOT create cvars implicitly, otherwise there would be no way to test if it exists.
	if (v)
	{
		ret |= CVAR_TYPEFLAG_EXISTS;
		if(v->flags & CVAR_ARCHIVE)
			ret |= CVAR_TYPEFLAG_SAVED;
		if(v->flags & (CVAR_NOTFROMSERVER|CVAR_NOUNSAFEEXPAND))
			ret |= CVAR_TYPEFLAG_PRIVATE;
		if(!(v->flags & CVAR_USERCREATED))
			ret |= CVAR_TYPEFLAG_ENGINE;
		if (v->description)
			ret |= CVAR_TYPEFLAG_HASDESCRIPTION;
		if (v->flags & (CVAR_NOSET|CVAR_RENDEREROVERRIDE))
			ret |= CVAR_TYPEFLAG_READONLY;
	}
	G_FLOAT(OFS_RETURN) = ret;
}

//void(string cvarname, string newvalue) cvar
void QCBUILTIN PF_cvar_set (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*var_name, *val;
	cvar_t *var;

	var_name = PR_GetStringOfs(prinst, OFS_PARM0);
	val = PR_GetStringOfs(prinst, OFS_PARM1);

	var = PF_Cvar_FindOrGet(var_name);
	if (!var || (var->flags & CVAR_NOTFROMSERVER))
		return;
	Cvar_Set (var, val);
}

void QCBUILTIN PF_cvar_setlatch (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*var_name, *val;
	cvar_t *var;

	var_name = PR_GetStringOfs(prinst, OFS_PARM0);
	val = PR_GetStringOfs(prinst, OFS_PARM1);

	var = PF_Cvar_FindOrGet(var_name);
	if (!var || (var->flags & CVAR_NOTFROMSERVER))
		return;
	Cvar_LockFromServer(var, val);
}

void QCBUILTIN PF_cvar_setf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*var_name;
	float	val;
	cvar_t *var;

	var_name = PR_GetStringOfs(prinst, OFS_PARM0);
	val = G_FLOAT(OFS_PARM1);

	var = PF_Cvar_FindOrGet(var_name);
	if (!var || (var->flags & CVAR_NOTFROMSERVER))
		return;
	Cvar_SetValue (var, val);
}

//float(string name, string value) registercvar
void QCBUILTIN PF_registercvar (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *name = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *value = (prinst->callargc>2)?PR_GetStringOfs(prinst, OFS_PARM1):"";
	int dpflags = (prinst->callargc>2)?G_FLOAT(OFS_PARM2):0;
	int realflags = 0;
	name = PR_GetStringOfs(prinst, OFS_PARM0);

	if (dpflags)
	{	//this is a DP extension, so uses DP's internal cvar flags.
		//which is stupid when cvar_type reports a different set of flags.
		//if (dpflags & (1<<0)) dpflags &= ~(1<<0), blocked from realflags |= avialable only to ;
		if (dpflags & (1<<4)) dpflags &= ~(1<<4), realflags |= CVAR_CHEAT;
		if (dpflags & (1<<5)) dpflags &= ~(1<<5), realflags |= CVAR_ARCHIVE;
		if (dpflags & (1<<8)) dpflags &= ~(1<<8), realflags |= CVAR_SERVERINFO;
		if (dpflags & (1<<9)) dpflags &= ~(1<<9), realflags |= CVAR_USERINFO;
		if (dpflags & (1<<11)) dpflags &= ~(1<<11), realflags |= CVAR_NOUNSAFEEXPAND;
		if (dpflags)
			Con_Printf(CON_WARNING"WARNING: Unknown flags passed to registercvar(\"%s\", \"%s\", %x)\n", name, value, dpflags);
	}

	if (Cvar_FindVar(name))
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
	// archive?
		if (Cvar_Get(name, value, CVAR_USERCREATED|realflags, "QC created vars"))
			G_FLOAT(OFS_RETURN) = 1;
		else
			G_FLOAT(OFS_RETURN) = 0;
	}
}

//Cvars
////////////////////////////////////////////////////
//memory stuff
void QCBUILTIN PF_memalloc (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int size = G_INT(OFS_PARM0);
	void *ptr;
	if (!size)
		size = 1; //return something free can free.
	if (size <= 0 || size > 0x01000000)
		ptr = NULL;	//don't let them abuse things too much. values that are too large might overflow.
	else
		ptr = prinst->AddressableAlloc(prinst, size);
	if (ptr)
	{
		memset(ptr, 0, size);
		G_INT(OFS_RETURN) = (char*)ptr - prinst->stringtable;
	}
	else
	{
		G_INT(OFS_RETURN) = 0;
		PR_BIError(prinst, "PF_memalloc: failure (size %i)\n", size);
	}
}
void QCBUILTIN PF_memrealloc (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	void *oldcptr = prinst->stringtable + G_INT(OFS_PARM0);
	int size = G_INT(OFS_PARM1);
	void *ptr;
	if (!size)
		size = 1; //return something free can free.
	if (size <= 0 || size > 0x01000000)
		ptr = NULL;	//don't let them abuse things too much. values that are too large might overflow.
	else
		ptr = prinst->AddressableRealloc(prinst, oldcptr, size);
	if (ptr)
		G_INT(OFS_RETURN) = (char*)ptr - prinst->stringtable;
	else
	{
		G_INT(OFS_RETURN) = 0;
		PR_BIError(prinst, "PF_memrealloc: failure (size %i)\n", size);
	}
}
void QCBUILTIN PF_memfree (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int qcptr = G_INT(OFS_PARM0);
	void *cptr = prinst->stringtable + G_INT(OFS_PARM0);
	if (qcptr)
		prinst->AddressableFree(prinst, cptr);
}
void QCBUILTIN PF_memcmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int qcdst = G_INT(OFS_PARM0);
	int qcsrc = G_INT(OFS_PARM1);
	int size = G_INT(OFS_PARM2);
	int srcoffset = (prinst->callargc>3)?G_INT(OFS_PARM3):0;
	int dstoffset = (prinst->callargc>4)?G_INT(OFS_PARM4):0;
	G_INT(OFS_RETURN) = 0;
	if (size < 0)
		PR_BIError(prinst, "PF_memcmp: invalid size\n");
	else if (size)
	{
		void *dst = PR_PointerToNative_MoInvalidate(prinst, qcdst, dstoffset, size);
		void *src = PR_PointerToNative_MoInvalidate(prinst, qcsrc, srcoffset, size);
		if (!dst)
			PR_BIError(prinst, "PF_memcmp: invalid dest\n");
		else if (!src)
			PR_BIError(prinst, "PF_memcmp: invalid source\n");
		else
			G_INT(OFS_RETURN) = memcmp(dst, src, size);
	}
}
void QCBUILTIN PF_memcpy (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int qcdst = G_INT(OFS_PARM0);
	int qcsrc = G_INT(OFS_PARM1);
	int size = G_INT(OFS_PARM2);
	int srcoffset = (prinst->callargc>3)?G_INT(OFS_PARM3):0;
	int dstoffset = (prinst->callargc>4)?G_INT(OFS_PARM4):0;
	if (size < 0)
		PR_BIError(prinst, "PF_memcpy: invalid size %#x\n", size);
	else if (size)
	{
		void *dst = PR_PointerToNative_Resize(prinst, qcdst, dstoffset, size);
		void *src = PR_PointerToNative_MoInvalidate(prinst, qcsrc, srcoffset, size);
		if (!dst)
			PR_BIError(prinst, "PF_memcpy: invalid dest (%#x[%#x...%#x]\n", qcdst, dstoffset, dstoffset+size-1);
		else if (!src)
			PR_BIError(prinst, "PF_memcpy: invalid source (%#x[%#x...%#x]\n", qcsrc, srcoffset, srcoffset+size-1);
		else
			memmove(dst, src, size);
	}
}

void QCBUILTIN PF_memfill8 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int dst = G_INT(OFS_PARM0);
	int val = G_INT(OFS_PARM1);
	int size = G_INT(OFS_PARM2);
	int dstoffset = (prinst->callargc>3)?G_INT(OFS_PARM3):0;

	void *ptr = PR_PointerToNative_Resize(prinst, dst, dstoffset, size);
	if (ptr)
		memset(ptr, val, size);
	else
		PR_BIError(prinst, "PF_memfill8: invalid dest\n");
}

void QCBUILTIN PF_memptradd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//convienience function. needed for: ptr = &ptr[5]; or ptr += 5;
	int dst = G_INT(OFS_PARM0);
	int ofs = G_FLOAT(OFS_PARM1);
	if (ofs != G_FLOAT(OFS_PARM1))
		PR_BIError(prinst, "PF_memptradd: non-integer offset\n");
	if (ofs & 3)
		PR_BIError(prinst, "PF_memptradd: offset is not 32-bit aligned.\n");	//allows for other implementations to provide this with eg pointers expressed as 16.16 with 18-bit segments/allocations.
	//cannot work with tempstrings/createbuffer
	if (((unsigned int)ofs & 0x80000000) && ofs!=0)
		PR_BIError(prinst, "PF_memptradd: special pointers cannot be offset.\n");

	G_INT(OFS_RETURN) = dst + ofs;
}

void QCBUILTIN PF_memstrsize(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//explicitly returns bytes, not chars.
	G_FLOAT(OFS_RETURN) = strlen(PR_GetStringOfs(prinst, OFS_PARM0));
}

//memory stuff
////////////////////////////////////////////////////
//hash table stuff
typedef struct
{
	pubprogfuncs_t *prinst;
	etype_t defaulttype;
	hashtable_t tab;
	void *bucketmem;
} pf_hashtab_t;
typedef struct 
{
	bucket_t	buck;
	char		*name;
	etype_t		type;
	union
	{
		vec3_t	data;
		char	*stringdata;
	};
} pf_hashentry_t;
#define HASH_REPLACE	256
#define HASH_ADD		512
#define FIRSTTABLE 1
static pf_hashtab_t *pf_hashtab;
static size_t pf_hash_maxtables;
static pf_hashtab_t pf_peristanthashtab;	//persists over map changes.
//static pf_hashtab_t pf_reverthashtab;		//pf_peristanthashtab as it was at map start, for map restarts.
static pf_hashtab_t *PF_hash_findtab(pubprogfuncs_t *prinst, int idx)
{
	idx -= FIRSTTABLE;
	if (idx >= 0 && (unsigned)idx < pf_hash_maxtables && pf_hashtab[idx].prinst)
		return &pf_hashtab[idx];
	else if (idx == 0-FIRSTTABLE)
	{
		if (!pf_peristanthashtab.tab.numbuckets)
		{
			int numbuckets = 256;
			pf_peristanthashtab.defaulttype = ev_string;
			pf_peristanthashtab.prinst = NULL;
			pf_peristanthashtab.bucketmem = Z_Malloc(Hash_BytesForBuckets(numbuckets));
			Hash_InitTable(&pf_peristanthashtab.tab, numbuckets, pf_peristanthashtab.bucketmem);
		}
		return &pf_peristanthashtab;
	}
	else
		PR_BIError(prinst, "PF_hash_findtab: invalid hash table\n");
	return NULL;
}

void QCBUILTIN PF_hash_getkey (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	pf_hashtab_t *tab = PF_hash_findtab(prinst, G_FLOAT(OFS_PARM0));
	int idx = G_FLOAT(OFS_PARM1);
	pf_hashentry_t *ent = NULL;
	G_INT(OFS_RETURN) = 0;
	if (tab)
	{
		ent = Hash_GetIdx(&tab->tab, idx);
		if (ent)
			RETURN_TSTRING(ent->name);
	}
}
void QCBUILTIN PF_hash_delete (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	pf_hashtab_t *tab = PF_hash_findtab(prinst, G_FLOAT(OFS_PARM0));
	const char *name = PR_GetStringOfs(prinst, OFS_PARM1);
	pf_hashentry_t *ent = NULL;
	memset(G_VECTOR(OFS_RETURN), 0, sizeof(vec3_t));
	if (tab)
	{
		ent = Hash_Get(&tab->tab, name);
		if (ent)
		{
			if (ent->type == ev_string)
			{
				G_INT(OFS_RETURN+2) = 0;
				G_INT(OFS_RETURN+1) = 0;
				RETURN_TSTRING(ent->stringdata);
			}
			else
				memcpy(G_VECTOR(OFS_RETURN), ent->data, sizeof(vec3_t));
			Hash_RemoveData(&tab->tab, name, ent);
			BZ_Free(ent);
		}
	}
}
void QCBUILTIN PF_hash_get (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	pf_hashtab_t *tab = PF_hash_findtab(prinst, G_FLOAT(OFS_PARM0));
	const char *name = PR_GetStringOfs(prinst, OFS_PARM1);
	void *dflt = (prinst->callargc>2)?G_VECTOR(OFS_PARM2):vec3_origin;
	int type = (prinst->callargc>3)?G_FLOAT(OFS_PARM3):0;
	int index = (prinst->callargc>4)?G_FLOAT(OFS_PARM4):0;
	pf_hashentry_t *ent = NULL;
	if (tab)
	{
		ent = Hash_Get(&tab->tab, name);
		//skip ones that are the wrong type.
		while (type != 0 && ent && ent->type != type)
			ent = Hash_GetNext(&tab->tab, name, ent);
		//and ones that are not the match that we're after.
		while(index-->0 && ent)
		{
			ent = Hash_GetNext(&tab->tab, name, ent);
			while (type != 0 && ent && ent->type != type)
				ent = Hash_GetNext(&tab->tab, name, ent);
		}
		if (ent)
		{
			if (ent->type == ev_string)
			{
				G_INT(OFS_RETURN+2) = 0;
				G_INT(OFS_RETURN+1) = 0;
				RETURN_TSTRING(ent->stringdata);
			}
			else
				memcpy(G_VECTOR(OFS_RETURN), ent->data, sizeof(vec3_t));
			return;
		}
	}
	memcpy(G_VECTOR(OFS_RETURN), dflt, sizeof(vec3_t));
}
void QCBUILTIN PF_hash_getcb (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
/*
	pf_hashtab_t *tab = PF_hash_findtab(prinst, G_FLOAT(OFS_PARM0));
	func_t callback = G_FUNCTION(OFS_PARM1);
	char *name = PR_GetStringOfs(prinst, OFS_PARM2);
	pf_hashentry_t *ent = NULL;
	if (tab >= 0 && tab < MAX_QC_HASHTABLES && pf_hashtab[tab].valid)
		ent = Hash_Get(&pf_hashtab[tab].tab, name);
	else
		PR_BIError(prinst, "PF_hash_getcb: invalid hash table\n");
	if (ent)
		memcpy(G_VECTOR(OFS_RETURN), ent->data, sizeof(vec3_t));
	else
		memcpy(G_VECTOR(OFS_RETURN), G_VECTOR(OFS_PARM2), sizeof(vec3_t));
*/
}

void QCBUILTIN PF_hash_add (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	pf_hashtab_t *tab = PF_hash_findtab(prinst, G_FLOAT(OFS_PARM0));
	const char *name = PR_GetStringOfs(prinst, OFS_PARM1);
	void *data = G_VECTOR(OFS_PARM2);
	int flags = (prinst->callargc>3)?G_FLOAT(OFS_PARM3):0;
	int type = flags & 0xff;
	pf_hashentry_t *ent = NULL;
	if (tab && *name)	//our hash tables can't cope with empty keys.
	{
		if (!type)
			type = tab->defaulttype;
		if (!(flags & HASH_ADD) || (flags & HASH_REPLACE))
		{
			ent = Hash_Get(&tab->tab, name);
			if (ent)
			{
				Hash_RemoveData(&tab->tab, name, ent);
				BZ_Free(ent);
			}
		}

		if (type == ev_string)
		{	//strings copy their value out.
			const char *value = PR_GetStringOfs(prinst, OFS_PARM2);
			int nlen = strlen(name);
			int vlen = strlen(value);
			ent = BZ_Malloc(sizeof(*ent) + nlen+1 + vlen+1);
			ent->name = (char*)(ent+1);
			ent->type = ev_string;
			ent->stringdata = ent->name+(nlen+1);
			memcpy(ent->name, name, nlen);
			ent->name[nlen] = 0;
			memcpy(ent->stringdata, value, vlen+1);
			Hash_Add(&tab->tab, ent->name, ent, &ent->buck);
		}
		else
		{
			int nlen = strlen(name);
			ent = BZ_Malloc(sizeof(*ent) + nlen + 1);
			ent->name = (char*)(ent+1);
			ent->type = type;
			memcpy(ent->name, name, nlen);
			ent->name[nlen] = 0;
			memcpy(ent->data, data, sizeof(vec3_t));
			Hash_Add(&tab->tab, ent->name, ent, &ent->buck);
		}
	}
}
static void PF_hash_destroytab_enum(void *ctx, void *ent)
{
	BZ_Free(ent);
}
void QCBUILTIN PF_hash_destroytab (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	pf_hashtab_t *tab = PF_hash_findtab(prinst, G_FLOAT(OFS_PARM0));
	if (tab && tab->prinst == prinst)
	{
		tab->prinst = NULL;
		Hash_Enumerate(&tab->tab, PF_hash_destroytab_enum, NULL);
		Z_Free(tab->bucketmem);
	}
}
void PF_Hash_DestroyAll(pubprogfuncs_t *prinst)
{
	qboolean freed = true;
	size_t idx;
	for (idx = 0; idx < pf_hash_maxtables; idx++)
	{
		if (pf_hashtab[idx].prinst == prinst)
		{
			pf_hashtab[idx].prinst = NULL;
			Hash_Enumerate(&pf_hashtab[idx].tab, PF_hash_destroytab_enum, NULL);
			Z_Free(pf_hashtab[idx].bucketmem);
		}
		else if (pf_hashtab[idx].prinst)
			freed = false;
	}

	if (freed && pf_hashtab)
	{
		pf_hash_maxtables = 0;
		Z_Free(pf_hashtab);
		pf_hashtab	= NULL;
	}
}
void QCBUILTIN PF_hash_createtab (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//FIXME: these need to be managed by the qcvm for garbage collection
	int i;
	int numbuckets = G_FLOAT(OFS_PARM0);
//	qboolean dupestrings = (prinst->callargc>1)?G_FLOAT(OFS_PARM1):false;
	etype_t type = (prinst->callargc>1)?G_FLOAT(OFS_PARM1):ev_vector;
	if (!type)
		type = ev_vector;
	if (numbuckets < 4)
		numbuckets = 64;

	for (i = 0; i < pf_hash_maxtables; i++)
	{	//look for an empty slot
		if (!pf_hashtab[i].prinst)
			break;
	}
	if (i == pf_hash_maxtables)
	{	//all slots taken, expand list
		if (!ZF_ReallocElements((void**)&pf_hashtab, &pf_hash_maxtables, pf_hash_maxtables+64, sizeof(*pf_hashtab)))
		{
			G_FLOAT(OFS_RETURN) = 0;
			return;
		}
	}

	//fill it in
	pf_hashtab[i].defaulttype = type;
	pf_hashtab[i].prinst = prinst;
	pf_hashtab[i].bucketmem = Z_Malloc(Hash_BytesForBuckets(numbuckets));
	Hash_InitTable(&pf_hashtab[i].tab, numbuckets, pf_hashtab[i].bucketmem);
	G_FLOAT(OFS_RETURN) = i + FIRSTTABLE;
}

static void PF_hash_savetab(void *ctx, void *data)
{
	char tmp[8192];
	pf_hashentry_t *ent = data;
	if (ent->type == ev_string)
		VFS_PRINTF (ctx, "\t%i \"%s\" %s\n", ent->type, ent->name, COM_QuotedString(ent->stringdata, tmp, sizeof(tmp), false));
	else if (ent->type == ev_vector)
		VFS_PRINTF (ctx, "\t%i \"%s\" %f %f %f\n", ent->type, ent->name, ent->data[0], ent->data[1], ent->data[2]);
	else if (ent->type == ev_float)
		VFS_PRINTF (ctx, "\t%i \"%s\" %f\n", ent->type, ent->name, *(float*)ent->data);
	else
		VFS_PRINTF (ctx, "\t%i \"%s\" %#x\n", ent->type, ent->name, *(int*)ent->data);
}
static void PR_hash_savegame(vfsfile_t *f, pubprogfuncs_t *prinst, qboolean binary)	//write the persistant table to a saved game.
{
	unsigned int tab;
	char *tmp = NULL;

	for (tab = 0; tab < pf_hash_maxtables; tab++)
	{
		if (pf_hashtab[tab].prinst == prinst)// && (pf_hashtab[tab].flags & BUFFLAG_SAVED))
		{
			VFS_PRINTF (f, "hashtable %u %i %u\n", tab+FIRSTTABLE, pf_hashtab[tab].defaulttype, (unsigned int)pf_hashtab[tab].tab.numbuckets);
			VFS_PRINTF (f, "{\n");
			Hash_Enumerate(&pf_hashtab[tab].tab, PF_hash_savetab, f);
			VFS_PRINTF (f, "}\n");
		}
	}
	free(tmp);
}
static const char *PR_hash_loadgame(pubprogfuncs_t *prinst, const char *l)
{
	char name[8192];
	char token[65536];
	int tabno;
	int nlen, vlen;
	etype_t hashtype;
	size_t buffersize;
	com_tokentype_t tt;
	pf_hashtab_t *tab;
	pf_hashentry_t *ent;
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	tabno = atoi(token)-FIRSTTABLE;
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	hashtype = atoi(token);
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	buffersize = atoi(token);

	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_LINEENDING)return NULL;
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_PUNCTUATION)return NULL;
	if (strcmp(token, "{"))	return NULL;

	if (tabno < 0 || tabno >= 1<<16)
		return NULL;
	if (tabno >= pf_hash_maxtables)
		Z_ReallocElements((void**)&pf_hashtab, &pf_hash_maxtables, tabno+1, sizeof(*pf_hashtab));

	tab = &pf_hashtab[tabno];
	if (tab->prinst)
	{
		tab->prinst = NULL;
		Hash_Enumerate(&tab->tab, PF_hash_destroytab_enum, NULL);
		Z_Free(tab->bucketmem);
		tab->bucketmem = NULL;
	}
	tab->prinst = prinst;
	tab->defaulttype = hashtype;
	tab->bucketmem = Z_Malloc(Hash_BytesForBuckets(buffersize));
	Hash_InitTable(&tab->tab, buffersize, tab->bucketmem);

	for(;;)
	{
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);
		if (tt == TTP_LINEENDING)
			continue;
		if (tt == TTP_PUNCTUATION && !strcmp(token, "}"))
			break;
		if (tt != TTP_RAWTOKEN)
			break;
		hashtype = atoi(token);
		l = COM_ParseTokenOut(l, NULL, name, sizeof(name), &tt);if (tt != TTP_STRING)return NULL;

		nlen = strlen(name);
		if (hashtype == ev_string)
		{
			l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return NULL;
			vlen = strlen(token);
			l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_LINEENDING)return NULL;

			ent = BZ_Malloc(sizeof(*ent) + nlen+1 + vlen+1);
			ent->name = (char*)(ent+1);
			ent->type = hashtype;
			ent->stringdata = ent->name+(nlen+1);
			memcpy(ent->name, name, nlen);
			ent->name[nlen] = 0;
			memcpy(ent->stringdata, token, vlen+1);
			Hash_Add(&tab->tab, ent->name, ent, &ent->buck);
		}
		else
		{
			vec3_t data = {0,0,0};
			if (hashtype == ev_vector)
			{
				l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
				data[0] = atof(token);
				l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
				data[1] = atof(token);
				l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
				data[2] = atof(token);
			}
			else if (hashtype == ev_float)
			{
				l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
				*data = atof(token);
			}
			else	//treat it as an ev_int
			{
				l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
				*(int*)data = atoi(token);
			}
			l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_LINEENDING)return NULL;

			ent = BZ_Malloc(sizeof(*ent) + nlen + 1);
			ent->name = (char*)(ent+1);
			ent->type = hashtype;
			memcpy(ent->name, name, nlen);
			ent->name[nlen] = 0;
			memcpy(ent->data, data, sizeof(vec3_t));
			Hash_Add(&tab->tab, ent->name, ent, &ent->buck);
		}
	}
	return l;
}
void pf_hash_preserve(void)	//map changed, make sure it can be reset properly.
{
}
void pf_hash_purge(void)	//restart command was used. revert to the state at the start of the map.
{
}

//hash table stuff
////////////////////////////////////////////////////
//File access

#define MAX_QC_FILES 256

#define FIRST_QC_FILE_INDEX 1000

typedef struct {
	char name[256];
	vfsfile_t *file;
	char *data;
	size_t bufferlen;
	size_t len;
	size_t ofs;
	pubprogfuncs_t *prinst;
	long accessmode;
} pf_fopen_files_t;
static pf_fopen_files_t pf_fopen_files[MAX_QC_FILES];

static qboolean QC_PathRequiresSandbox(const char *name)
{
	//if its a user config, ban any fallback locations so that csqc can't read passwords or whatever.
	/* (sorry about the windows paths, it avoids compiler warnings... -Werror sucks)
	bad:
		*.cfg
		configs\*.cfg
	allowed:
		particles\*.cfg (required for particle editor type stuff)
		huds\*.cfg  (shouldn't have any passwords. yay editing)
		models\*.*  (xonotic is evil)
		sound\*.*  (xonotic is evil)
	*/
	if ((!strchr(name, '/') || !strnicmp(name, "configs/", 8))	//
		&& !stricmp(COM_GetFileExtension(name, NULL), ".cfg"))
		return true;
	return false;
}

//returns false if the file is denied.
//fallbackread can be NULL, if the qc is not allowed to read that (original) file at all.
qboolean QC_FixFileName(const char *name, const char **result, const char **fallbackread)
{
	if (!strncmp(name, "file:", 5))
	{
		*result = name;
		*fallbackread = NULL;
		return true;
	}

	if (!*name ||	//blank names are bad
		strchr(name, ':') ||	//dos/win absolute path, ntfs ADS, amiga drives. reject them all.
		strchr(name, '\\') ||	//windows-only paths.
		*name == '/' ||	//absolute path was given - reject
		strstr(name, ".."))	//someone tried to be clever.
	{
		return false;
	}

	if (!strncmp(name, "data/", 5))
	{
		*fallbackread = NULL;	//don't be weird.
		*result = name;	//already has a data/ prefix.
	}
	else if (COM_CheckParm("-unsafefopen") && !QC_PathRequiresSandbox(name))
	{
		*fallbackread = va("data/%s", name);	//in case the mod was distributed with a data/ subdir.
		*result = name;
	}
	else
	{
		*fallbackread = QC_PathRequiresSandbox(name)?NULL:name;
		*result = va("data/%s", name);

	}
	return true;
}

void QCBUILTIN PF_fopen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *name = PR_GetStringOfs(prinst, OFS_PARM0);
	int fmode = (prinst->callargc>1)?G_FLOAT(OFS_PARM1):-1;
	int fsize = (prinst->callargc>2)?G_FLOAT(OFS_PARM2):0;
	const char *fallbackread;
	int i;
	size_t insize;

	Con_DPrintf("qcfopen(\"%s\", %i) called\n", name, fmode);

	for (i = 0; i < MAX_QC_FILES; i++)
		if (!pf_fopen_files[i].prinst)
			break;

	G_FLOAT(OFS_RETURN) = -1; //assume an error
	if (i == MAX_QC_FILES)	//too many already open
	{
		Con_Printf("qcfopen(\"%s\"): too many files open\n", name);
		return;
	}

	if (fmode < 0 && (!strncmp(name, "tcp://", 6) || !strncmp(name, "tls://", 6) || !strncmp(name, "ws:", 3) || !strncmp(name, "wss:", 4)))
	{
		extern cvar_t pr_enable_uriget;
#if defined(CSQC_DAT) && !defined(SERVERONLY)
		extern world_t csqc_world;
		if (prinst == csqc_world.progs)	//menuqc will refuse to load from untrusted sources. ssqc is the admin's choice. csqc is fundamentally untrusted though.
			return;
#endif
		if (!pr_enable_uriget.ival)	//same cvar to block http requests.
			return;

		Q_strncpyz(pf_fopen_files[i].name, name, sizeof(pf_fopen_files[i].name));
		pf_fopen_files[i].accessmode = FRIK_FILE_STREAM;
		pf_fopen_files[i].bufferlen = 0;
		pf_fopen_files[i].data = NULL;
		pf_fopen_files[i].len = 0;
		pf_fopen_files[i].ofs = 0;
		pf_fopen_files[i].prinst = prinst;

		pf_fopen_files[i].file = FS_OpenTCP(name, 0, true);
		if (pf_fopen_files[i].file)
			G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
		else
		{
			G_FLOAT(OFS_RETURN) = -1;
			memset(&pf_fopen_files[i], 0, sizeof(pf_fopen_files[i]));
		}
		return;
	}

	if (!QC_FixFileName(name, &name, &fallbackread))
	{
		Con_Printf("qcfopen(\"%s\"): Access denied\n", name);
		return;
	}

	Q_strncpyz(pf_fopen_files[i].name, name, sizeof(pf_fopen_files[i].name));

	pf_fopen_files[i].accessmode = fmode;
	switch (fmode)
	{
	case FRIK_FILE_MMAP_READ:
	case FRIK_FILE_MMAP_RW:
		{
			vfsfile_t *f = FS_OpenVFS(pf_fopen_files[i].name, "rb", FS_GAME);
			if (!f && fallbackread)
				f = FS_OpenVFS(fallbackread, "rb", FS_GAME);
			if (f)
			{
				pf_fopen_files[i].bufferlen = pf_fopen_files[i].len = VFS_GETLEN(f);
				if (pf_fopen_files[i].bufferlen < fsize)
					pf_fopen_files[i].bufferlen = fsize;
				pf_fopen_files[i].data = PR_AddressableAlloc(prinst, pf_fopen_files[i].bufferlen);
				VFS_READ(f, pf_fopen_files[i].data, pf_fopen_files[i].len);
				VFS_CLOSE(f);
			}
			else if (fmode == FRIK_FILE_MMAP_RW)
			{
				pf_fopen_files[i].bufferlen = fsize;
				pf_fopen_files[i].data = PR_AddressableAlloc(prinst, pf_fopen_files[i].bufferlen);
			}
			else
			{
				pf_fopen_files[i].bufferlen = 0;
				pf_fopen_files[i].data = NULL;
			}

			if (!pf_fopen_files[i].data)
				break;

			pf_fopen_files[i].len = pf_fopen_files[i].bufferlen;
			pf_fopen_files[i].ofs = 0;
			G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
			pf_fopen_files[i].prinst = prinst;
		}
		break;
	case FRIK_FILE_READ:
	case FRIK_FILE_READ_DELAY:
		{
			pf_fopen_files[i].accessmode = FRIK_FILE_READ_DELAY;
			pf_fopen_files[i].file = FS_OpenVFS(pf_fopen_files[i].name, "rb", FS_GAME);
			if (!pf_fopen_files[i].file && fallbackread)
			{
				Q_strncpyz(pf_fopen_files[i].name, fallbackread, sizeof(pf_fopen_files[i].name));
				pf_fopen_files[i].file = FS_OpenVFS(pf_fopen_files[i].name, "rb", FS_GAME);
			}
			pf_fopen_files[i].ofs = 0;
			if (pf_fopen_files[i].file)
			{
				pf_fopen_files[i].len = VFS_GETLEN(pf_fopen_files[i].file);

				G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
				pf_fopen_files[i].prinst = prinst;
			}
		}
		break;

//	case FRIK_FILE_READ:	//read
	case FRIK_FILE_READNL:	//read whole file
		fsize = FS_LoadFile(pf_fopen_files[i].name, (void**)&pf_fopen_files[i].data);
		if (!pf_fopen_files[i].data && fallbackread)
		{
			Q_strncpyz(pf_fopen_files[i].name, fallbackread, sizeof(pf_fopen_files[i].name));
			fsize = FS_LoadFile(pf_fopen_files[i].name, (void**)&pf_fopen_files[i].data);
		}

		if (pf_fopen_files[i].data)
		{
			G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
			pf_fopen_files[i].prinst = prinst;
		}

		pf_fopen_files[i].bufferlen = pf_fopen_files[i].len = fsize;
		pf_fopen_files[i].ofs = 0;
		break;
	case FRIK_FILE_APPEND:	//append
		pf_fopen_files[i].data = FS_LoadMallocFile(pf_fopen_files[i].name, &insize);
		pf_fopen_files[i].ofs = pf_fopen_files[i].bufferlen = pf_fopen_files[i].len = insize;
		if (pf_fopen_files[i].data)
		{
			G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
			pf_fopen_files[i].prinst = prinst;
			break;
		}
		//file didn't exist - fall through
	case FRIK_FILE_WRITE:	//write
		pf_fopen_files[i].bufferlen = 8192;
		pf_fopen_files[i].data = BZ_Malloc(pf_fopen_files[i].bufferlen);
		pf_fopen_files[i].len = 0;
		pf_fopen_files[i].ofs = 0;
		G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
		pf_fopen_files[i].prinst = prinst;
		break;
	case FRIK_FILE_INVALID:
		pf_fopen_files[i].bufferlen = 0;
		pf_fopen_files[i].data = "";
		pf_fopen_files[i].len = 0;
		pf_fopen_files[i].ofs = 0;
		G_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
		pf_fopen_files[i].prinst = prinst;
		break;
	default: //bad
		G_FLOAT(OFS_RETURN) = -1;
		break;
	}
}

//non-builtin function used by saved game code.
int PR_QCFile_From_VFS (pubprogfuncs_t *prinst, const char *name, vfsfile_t *f, qboolean write)
{
	int i;

	for (i = 0; i < MAX_QC_FILES; i++)
		if (!pf_fopen_files[i].prinst)
			break;
	if (i == MAX_QC_FILES)	//too many already open
		return -1;

	pf_fopen_files[i].accessmode = write?FRIK_FILE_STREAM:FRIK_FILE_READ_DELAY;

	Q_strncpyz(pf_fopen_files[i].name, name, sizeof(pf_fopen_files[i].name));
	pf_fopen_files[i].file = f;

	pf_fopen_files[i].ofs = VFS_TELL(pf_fopen_files[i].file);
	if (pf_fopen_files[i].file)
	{
		pf_fopen_files[i].len = VFS_GETLEN(pf_fopen_files[i].file);

		pf_fopen_files[i].prinst = prinst;
		return i + FIRST_QC_FILE_INDEX;
	}
	else
		return -1;
}
int PR_QCFile_From_Buffer (pubprogfuncs_t *prinst, const char *name, void *buffer, size_t ofs, size_t len)
{
	int i;

	for (i = 0; i < MAX_QC_FILES; i++)
		if (!pf_fopen_files[i].prinst)
			break;
	if (i == MAX_QC_FILES)	//too many already open
		return -1;

	pf_fopen_files[i].accessmode = FRIK_FILE_READ;

	Q_strncpyz(pf_fopen_files[i].name, name, sizeof(pf_fopen_files[i].name));
	pf_fopen_files[i].file = NULL;
	pf_fopen_files[i].data = (void*)buffer;
	pf_fopen_files[i].ofs = ofs;
	pf_fopen_files[i].len = len;

	pf_fopen_files[i].prinst = prinst;
	return i + FIRST_QC_FILE_INDEX;
}

//internal function used by search_begin
static int PF_fopen_search (pubprogfuncs_t *prinst, const char *name, flocation_t *loc)
{
	int i;

	Con_DPrintf("qcfopen(\"%s\") called\n", name);

	for (i = 0; i < MAX_QC_FILES; i++)
		if (!pf_fopen_files[i].prinst)
			break;

	if (i == MAX_QC_FILES)	//too many already open
	{
		Con_Printf("qcfopen(\"%s\"): too many files open\n", name);
		return -1;
	}

	if (QC_PathRequiresSandbox(name))
	{	//we're ignoring the data/ dir so using only the fallback, but still blocking it if its a nasty path.
		Con_Printf("qcfopen(\"%s\"): Access denied\n", name);
		return -1;
	}

	pf_fopen_files[i].accessmode = FRIK_FILE_READ_DELAY;

	Q_strncpyz(pf_fopen_files[i].name, name, sizeof(pf_fopen_files[i].name));
	if (loc->search->handle)
		pf_fopen_files[i].file = FS_OpenReadLocation(name, loc);
	else
		pf_fopen_files[i].file = FS_OpenVFS(loc->rawname, "rb", FS_ROOT);

	pf_fopen_files[i].ofs = 0;
	if (pf_fopen_files[i].file)
	{
		pf_fopen_files[i].len = VFS_GETLEN(pf_fopen_files[i].file);

		pf_fopen_files[i].prinst = prinst;
		return i + FIRST_QC_FILE_INDEX;
	}
	else
		return -1;
}

void PF_fclose_i (int fnum)
{
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		Con_Printf("PF_fclose: File out of range\n");
		return;	//out of range
	}

	if (!pf_fopen_files[fnum].prinst)
	{
		Con_Printf("PF_fclose: File is not open\n");
		return;	//not open
	}

	switch(pf_fopen_files[fnum].accessmode)
	{
	case FRIK_FILE_MMAP_RW:
		COM_WriteFile(pf_fopen_files[fnum].name, FS_GAMEONLY, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		/*fall through*/
	case FRIK_FILE_MMAP_READ:
		pf_fopen_files[fnum].prinst->AddressableFree(pf_fopen_files[fnum].prinst, pf_fopen_files[fnum].data);
		break;

	case FRIK_FILE_STREAM:
	case FRIK_FILE_READ_DELAY:
		VFS_CLOSE(pf_fopen_files[fnum].file);
		break;

	case FRIK_FILE_READ:
	case FRIK_FILE_READNL:
		BZ_Free(pf_fopen_files[fnum].data);
		break;
	case FRIK_FILE_APPEND:
	case FRIK_FILE_WRITE:
		COM_WriteFile(pf_fopen_files[fnum].name, FS_GAMEONLY, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		BZ_Free(pf_fopen_files[fnum].data);
		break;
	case FRIK_FILE_INVALID:
		break;
	}
	pf_fopen_files[fnum].file = NULL;
	pf_fopen_files[fnum].data = NULL;
	pf_fopen_files[fnum].prinst = NULL;
}

void QCBUILTIN PF_fclose (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0)-FIRST_QC_FILE_INDEX;

	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PF_Warningf(prinst, "PF_fclose: File out of range (%g)\n", G_FLOAT(OFS_PARM0));
		return;	//out of range
	}

	if (!pf_fopen_files[fnum].prinst)
	{
		Con_Printf("PF_fclose: File is not open\n");
		return;	//not open
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PF_Warningf(prinst, "PF_fclose: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	PF_fclose_i(fnum);
}

void QCBUILTIN PF_fgets (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char c, *s, *o, *max, *eof;
	int fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	char pr_string_temp[4096];

	*pr_string_temp = '\0';
	G_INT(OFS_RETURN) = 0;	//EOF
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PF_Warningf(prinst, "PF_fgets: File out of range (%g)\n", G_FLOAT(OFS_PARM0));
		return;	//out of range
	}

	if (!pf_fopen_files[fnum].prinst)
	{
		PF_Warningf(prinst, "PF_fgets: File is not open\n");
		return;	//not open
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PF_Warningf(prinst, "PF_fgets: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_STREAM)
	{
		if (VFS_GETS(pf_fopen_files[fnum].file, pr_string_temp, sizeof(pr_string_temp)))
			RETURN_TSTRING(pr_string_temp);
		else
			G_INT(OFS_RETURN) = 0;	//EOF
		return;
	}
	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_READ_DELAY)
	{	//on first read, convert into a regular file.
		pf_fopen_files[fnum].accessmode = FRIK_FILE_READ;
		pf_fopen_files[fnum].data = BZ_Malloc(pf_fopen_files[fnum].len+1);
		pf_fopen_files[fnum].data[pf_fopen_files[fnum].len] = 0;
		pf_fopen_files[fnum].len = pf_fopen_files[fnum].bufferlen = VFS_READ(pf_fopen_files[fnum].file, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		VFS_CLOSE(pf_fopen_files[fnum].file);
		pf_fopen_files[fnum].file = NULL;
	}

	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_MMAP_READ || pf_fopen_files[fnum].accessmode == FRIK_FILE_MMAP_RW)
	{
		G_INT(OFS_RETURN) = PR_SetString(prinst, pf_fopen_files[fnum].data);
		return;
	}
	
	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_READNL)
	{
		if (pf_fopen_files[fnum].ofs >= pf_fopen_files[fnum].len)
			G_INT(OFS_RETURN) = 0;	//EOF
		else
			RETURN_TSTRING(pf_fopen_files[fnum].data);
	}
	else
	{
		//read up to the next \n, ignoring any \rs.
		o = pr_string_temp;
		max = o + sizeof(pr_string_temp)-1;
		s = pf_fopen_files[fnum].data+pf_fopen_files[fnum].ofs;
		eof = pf_fopen_files[fnum].data+pf_fopen_files[fnum].len;
		
		if (s >= eof)
		{
			G_INT(OFS_RETURN) = 0;	//EOF
			return;
		}

		while(s < eof)
		{
			c = *s++;
			if (c == '\n' && pf_fopen_files[fnum].accessmode != FRIK_FILE_READNL)
				break;
			if (c == '\r' && pf_fopen_files[fnum].accessmode != FRIK_FILE_READNL)
				continue;

			if (c == 0)
			{	//modified utf-8, woo. but don't double-encode other chars.
				if (o+1 >= max)
					break;
				*o++ = 0xc0;
				*o++ = 0x80;
			}
			else
			{
				if (o == max)
					break;
				*o++ = c;
			}
		}
		*o = '\0';

		pf_fopen_files[fnum].ofs = s - pf_fopen_files[fnum].data;

		RETURN_TSTRING(pr_string_temp);
	}
}

//ensures that the buffer is at least big enough.
static qboolean PF_fresizebuffer_internal (pf_fopen_files_t *f, size_t newlen)
{
	switch(f->accessmode)
	{
	default:
		return false;
	case FRIK_FILE_MMAP_RW:
		//mmap cannot change the buffer size/allocation.
		if (newlen > f->bufferlen)
			return false;
		break;
	case FRIK_FILE_APPEND:
	case FRIK_FILE_WRITE:
		if (f->bufferlen < newlen)
		{
			char *newbuf;
			size_t newbuflen = max(newlen, newlen*2+1024);
			newbuf = BZF_Malloc(newbuflen);
			if (!newbuf)
				return false;
			memcpy(newbuf, f->data, f->bufferlen);
			memset(newbuf+f->bufferlen, 0, newbuflen - f->bufferlen);
			BZ_Free(f->data);
			f->data = newbuf;
			f->bufferlen = newbuflen;
		}
		break;
	}
	return true;
}
static int PF_fwrite_internal (pubprogfuncs_t *prinst, int fnum, const char *msg, size_t len)
{
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PF_Warningf(prinst, "PF_fwrite: File out of range\n");
		return 0;	//out of range
	}

	if (!pf_fopen_files[fnum].prinst)
	{
		PF_Warningf(prinst, "PF_fwrite: File is not open\n");
		return 0;	//not open
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PF_Warningf(prinst, "PF_fwrite: File is from wrong instance\n");
		return 0;	//this just isn't ours.
	}

	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_STREAM)
		return VFS_WRITE(pf_fopen_files[fnum].file, msg, len);

	if (pf_fopen_files[fnum].ofs + len < pf_fopen_files[fnum].ofs)
	{
		PF_Warningf(prinst, "PF_fwrite: size overflow\n");
		return 0;
	}

	switch(pf_fopen_files[fnum].accessmode)
	{
	default:
		return 0;
	case FRIK_FILE_APPEND:
	case FRIK_FILE_WRITE:
	case FRIK_FILE_MMAP_RW:
		PF_fresizebuffer_internal(&pf_fopen_files[fnum], pf_fopen_files[fnum].ofs+len);
		len = min(len, pf_fopen_files[fnum].bufferlen-pf_fopen_files[fnum].ofs);

		memcpy(pf_fopen_files[fnum].data + pf_fopen_files[fnum].ofs, msg, len);
		if (pf_fopen_files[fnum].len < pf_fopen_files[fnum].ofs + len)
			pf_fopen_files[fnum].len = pf_fopen_files[fnum].ofs + len;
		pf_fopen_files[fnum].ofs+=len;
		return len;
	}
}

static int PF_fread_internal (pubprogfuncs_t *prinst, int fnum, char *buf, size_t len)
{
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PF_Warningf(prinst, "PF_fread: File out of range\n");
		return 0;	//out of range
	}

	if (!pf_fopen_files[fnum].prinst)
	{
		PF_Warningf(prinst, "PF_fread: File is not open\n");
		return 0;	//not open
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PF_Warningf(prinst, "PF_fread: File is from wrong instance\n");
		return 0;	//this just isn't ours.
	}

	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_STREAM)
		return VFS_READ(pf_fopen_files[fnum].file, buf, len);
	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_READ_DELAY)
	{	//on first read, convert into a regular file.
		pf_fopen_files[fnum].accessmode = FRIK_FILE_READ;
		pf_fopen_files[fnum].data = BZ_Malloc(pf_fopen_files[fnum].len+1);
		pf_fopen_files[fnum].data[pf_fopen_files[fnum].len] = 0;
		pf_fopen_files[fnum].len = pf_fopen_files[fnum].bufferlen = VFS_READ(pf_fopen_files[fnum].file, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		VFS_CLOSE(pf_fopen_files[fnum].file);
		pf_fopen_files[fnum].file = NULL;
	}

	switch(pf_fopen_files[fnum].accessmode)
	{
	default:
		PF_Warningf(prinst, "PF_fread: File not opened for reading\n");
		return 0;
	case FRIK_FILE_READ:
		if (pf_fopen_files[fnum].ofs + len > pf_fopen_files[fnum].len)
			len = pf_fopen_files[fnum].len - pf_fopen_files[fnum].ofs;

		memcpy(buf, pf_fopen_files[fnum].data + pf_fopen_files[fnum].ofs, len);
		pf_fopen_files[fnum].ofs+=len;
		return len;
	}
}

void QCBUILTIN PF_fputs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	const char *msg = PF_VarString(prinst, 1, pr_globals);
	int len = strlen(msg);

	PF_fwrite_internal (prinst, fnum, msg, len);
}

void QCBUILTIN PF_fwrite (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	int qcptr = G_INT(OFS_PARM1);
	int size = G_INT(OFS_PARM2);
	int srcoffset = (prinst->callargc>3)?G_INT(OFS_PARM3):0;
	const void *ptr = PR_PointerToNative_MoInvalidate(prinst, qcptr, srcoffset, size);
	if (!ptr)
	{
		PR_BIError(prinst, "PF_fwrite: invalid ptr / size\n");
		return;
	}

	G_INT(OFS_RETURN) = PF_fwrite_internal (prinst, fnum, ptr, size);
}
void QCBUILTIN PF_fread (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	int qcptr = G_INT(OFS_PARM1);
	int size = G_INT(OFS_PARM2);
	int dstoffset = (prinst->callargc>3)?G_INT(OFS_PARM3):0;
	void *ptr = PR_PointerToNative_Resize(prinst, qcptr, dstoffset, size);
	if (!ptr)
	{
		PR_BIError(prinst, "PF_fread: invalid ptr / size\n");
		return;
	}

	G_INT(OFS_RETURN) = PF_fread_internal (prinst, fnum, ptr, size);
}
void QCBUILTIN PF_fseek64 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	G_INT64(OFS_RETURN) = -1;
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PF_Warningf(prinst, "PF_fseek: File out of range\n");
		return;	//out of range
	}
	if (!pf_fopen_files[fnum].prinst)
	{
		PF_Warningf(prinst, "PF_fseek: File is not open\n");
		return;	//not open
	}
	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PF_Warningf(prinst, "PF_fseek: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_READ_DELAY)
	{	//on first read, convert into a regular file.
		pf_fopen_files[fnum].accessmode = FRIK_FILE_READ;
		pf_fopen_files[fnum].data = BZ_Malloc(pf_fopen_files[fnum].len+1);
		pf_fopen_files[fnum].data[pf_fopen_files[fnum].len] = 0;
		pf_fopen_files[fnum].len = pf_fopen_files[fnum].bufferlen = VFS_READ(pf_fopen_files[fnum].file, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		VFS_CLOSE(pf_fopen_files[fnum].file);
		pf_fopen_files[fnum].file = NULL;
	}

	if (pf_fopen_files[fnum].file)
	{
		G_INT64(OFS_RETURN) = VFS_TELL(pf_fopen_files[fnum].file);
		if (prinst->callargc>1 && G_INT64(OFS_PARM1) >= 0)
			VFS_SEEK(pf_fopen_files[fnum].file, G_INT64(OFS_PARM1));
	}
	else
	{
		G_INT64(OFS_RETURN) = pf_fopen_files[fnum].ofs;
		if (prinst->callargc>1 && G_INT64(OFS_PARM1) >= 0)
		{
			pf_fopen_files[fnum].ofs = G_INT64(OFS_PARM1);
		}
	}
}
void QCBUILTIN PF_fseek32 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT64(OFS_PARM1) = G_INT(OFS_PARM1);
	PF_fseek64(prinst, pr_globals);
	G_INT(OFS_RETURN) = G_INT64(OFS_RETURN);
}
void QCBUILTIN PF_fsize64 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	G_INT64(OFS_RETURN) = -1;
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PF_Warningf(prinst, "PF_fsize: File out of range\n");
		return;	//out of range
	}
	if (!pf_fopen_files[fnum].prinst)
	{
		PF_Warningf(prinst, "PF_fsize: File is not open\n");
		return;	//not open
	}
	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PF_Warningf(prinst, "PF_fsize: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	if (pf_fopen_files[fnum].accessmode == FRIK_FILE_READ_DELAY)
	{	//on first read, convert into a regular file.
		pf_fopen_files[fnum].accessmode = FRIK_FILE_READ;
		pf_fopen_files[fnum].data = BZ_Malloc(pf_fopen_files[fnum].len+1);
		pf_fopen_files[fnum].data[pf_fopen_files[fnum].len] = 0;
		pf_fopen_files[fnum].len = pf_fopen_files[fnum].bufferlen = VFS_READ(pf_fopen_files[fnum].file, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		VFS_CLOSE(pf_fopen_files[fnum].file);
		pf_fopen_files[fnum].file = NULL;
	}

	if (pf_fopen_files[fnum].file)
	{
		G_INT64(OFS_RETURN) = VFS_GETLEN(pf_fopen_files[fnum].file);
		if (prinst->callargc>1 && G_INT64(OFS_PARM1) >= 0)
			PF_Warningf(prinst, "PF_fsize: truncation/extension is not supported for stream file types\n");
	}
	else
	{
		G_INT64(OFS_RETURN) = pf_fopen_files[fnum].len;
		if (prinst->callargc>1 && G_INT64(OFS_PARM1) >= 0)
		{
			size_t newlen = G_INT64(OFS_PARM1);
			PF_fresizebuffer_internal(&pf_fopen_files[fnum], newlen);
			pf_fopen_files[fnum].len = min(pf_fopen_files[fnum].bufferlen, newlen);
		}
	}
}
void QCBUILTIN PF_fsize32 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT64(OFS_PARM1) = G_INT(OFS_PARM1);
	PF_fsize64(prinst, pr_globals);
	G_INT(OFS_RETURN) = G_INT64(OFS_RETURN);
}

void PF_fcloseall (pubprogfuncs_t *prinst)
{
	qboolean write;
	int i;
	for (i = 0; i < MAX_QC_FILES; i++)
	{
		if (pf_fopen_files[i].prinst != prinst)
			continue;

		switch(pf_fopen_files[i].accessmode)
		{
		case FRIK_FILE_STREAM:
		case FRIK_FILE_APPEND:
		case FRIK_FILE_WRITE:
		case FRIK_FILE_MMAP_RW:
			write = true;
			break;
		default:
			write = false;
			break;
		}
		if (developer.ival || write)
			Con_Printf("qc file %s was still open\n", pf_fopen_files[i].name);
		PF_fclose_i(i);
	}
	tokenizeqc("", false);
	PF_buf_shutdown(prinst);	//might as well put this here
}

void QCBUILTIN PF_fcopy (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *srcname = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *dstname = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *fallbackread, *fallbackwrite;
	vfsfile_t *src, *dst;
	char buffer[65536];
	int sz;
	G_FLOAT(OFS_RETURN) = -1;
	if (QC_FixFileName(srcname, &srcname, &fallbackread))
	{
		if (QC_FixFileName(dstname, &dstname, &fallbackwrite))
		{
			src = FS_OpenVFS(srcname, "rb", FS_GAME);
			if (!src)
				src = FS_OpenVFS(fallbackread, "rb", FS_GAME);
			if (src)
			{
				dst = FS_OpenVFS(srcname, "wbp", FS_GAMEONLY);	//lets mark it as persistent. this is probably profile data after all.
				if (dst)
				{
					while ((sz = VFS_READ(src, buffer, sizeof(buffer)))>0)
					{
						if (sz != VFS_WRITE(dst, buffer, sz))
							G_FLOAT(OFS_RETURN) = -3;	//weird errors...
					}
					G_FLOAT(OFS_RETURN) = 0;	//success...
					VFS_CLOSE(dst);
				}
				else
					G_FLOAT(OFS_RETURN) = -2;	//output failure
				VFS_CLOSE(src);
			}
		}
	}
}
void QCBUILTIN PF_frename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *srcname = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *dstname = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *fallbackread, *fallbackwrite; //not actually used, but present because QC_FixFileName wants a fallback
	G_FLOAT(OFS_RETURN) = -1; //some kind of dodgy path problem
	if (QC_FixFileName(srcname, &srcname, &fallbackread))
		if (QC_FixFileName(dstname, &dstname, &fallbackwrite))
		{
			if (FS_Rename(srcname, dstname, FS_GAMEONLY))
				G_FLOAT(OFS_RETURN) = 0;
			else
				G_FLOAT(OFS_RETURN) = -5; //random, but whatever
		}
}
void QCBUILTIN PF_fremove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *fname = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *fallbackread; //not actually used, but present because QC_FixFileName wants a fallback
	G_FLOAT(OFS_RETURN) = -1; //some kind of dodgy path problem
	if (QC_FixFileName(fname, &fname, &fallbackread))
	{
		if (FS_Remove(fname, FS_GAMEONLY))
			G_FLOAT(OFS_RETURN) = 0;
		else
			G_FLOAT(OFS_RETURN) = -5; //random, but whatever
	}
}
void QCBUILTIN PF_fexists (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *srcname = PR_GetStringOfs(prinst, OFS_PARM0);
	flocation_t loc;

	int depth = FS_FLocateFile(srcname, FSLF_DEPTH_INEXPLICIT, &loc);

	if (depth == 1)
		G_FLOAT(OFS_RETURN) = true;		//exists and should be in the writable path.
	else
		G_FLOAT(OFS_RETURN) = false;	//doesn't exist / not writable / etc, should match wrath.
}
void QCBUILTIN PF_rmtree (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *fname = PR_GetStringOfs(prinst, OFS_PARM0);
	Con_Printf("rmtree(\"%s\"): rmtree is not implemented at this time\n", fname);

	/*flocation_t loc;
	G_FLOAT(OFS_RETURN) = -1; //error
	if (FS_FLocateFile(fname, FSLF_IGNORELINKS|FSLF_DONTREFERENCE, &loc))	//find the right gamedir for it...
	{
		fname = va("%s/", fname);	//its meant to be a directory, make sure that's explicit
		if (FS_RemoveTree(loc.search->handle, fname))
			G_FLOAT(OFS_RETURN) = 0;
	}*/
}

//DP_QC_WHICHPACK
void QCBUILTIN PF_whichpack (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *srcname = PR_GetStringOfs(prinst, OFS_PARM0);
	unsigned int flags = prinst->callargc>1?G_FLOAT(OFS_PARM1):WP_REFERENCE;
	flocation_t loc;

	if (FS_FLocateFile(srcname, FSLF_IFFOUND, &loc))
	{
		srcname = FS_WhichPackForLocation(&loc, flags);
		if (srcname == NULL)
			srcname = "";
		RETURN_TSTRING(srcname);
	}
	else
	{
		G_INT(OFS_RETURN) = 0;	//null/empty
	}

}


enum
{
//	QCSEARCH_INSENSITIVE = 1u<<0,	//for dp, we're always insensitive you prick.
	QCSEARCH_FULLPACKAGE = 1u<<1,	//package names include gamedir prefix etc.
	QCSEARCH_ALLOWDUPES  = 1u<<2,	//don't filter out dupes, allowing entries hidden by later packages to be shown.
	QCSEARCH_FORCESEARCH = 1u<<3,	//force the search to succeed even if the gamedir/package is not active.
	QCSEARCH_MULTISEARCH = 1u<<4,	//to avoid possible string manipulation exploits?
	QCSEARCH_NAMESORT    = 1u<<5,	//sort results by filename, instead of by filesystem priority/randomness
};
searchpathfuncs_t *COM_EnumerateFilesPackage (char *matches, const char *package, unsigned int flags, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t*), void *parm);
typedef struct prvmsearch_s {
	pubprogfuncs_t *fromprogs;	//share across menu/server

	searchpath_t searchinfo;

	int entries;
	struct prvmsearchentry_s
	{
		char *name;
		qofs_t size;
		time_t mtime;
		searchpathfuncs_t *package;
	} *entry;
	char *pattern;
	unsigned int flags;
	unsigned int fsflags;
} prvmsearch_t;
static prvmsearch_t *pr_searches;	//realloced to extend
static size_t numpr_searches;

void search_close (pubprogfuncs_t *prinst, int handle)
{
	int i;
	prvmsearch_t *s;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "search_close: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	for (i = 0; i < s->entries; i++)
		BZ_Free(s->entry[i].name);
	Z_Free(s->pattern);
	BZ_Free(s->entry);
	if (s->searchinfo.handle)
		s->searchinfo.handle->ClosePath(s->searchinfo.handle);
	memset(s, 0, sizeof(*s));
}
//a progs was closed... hunt down it's searches, and warn about any searches left open.
void search_close_progs(pubprogfuncs_t *prinst, qboolean complain)
{
	int i, j;
	prvmsearch_t *s;
	qboolean stillactive = false;

	for (j = 0; j < numpr_searches; j++)
	{
		s = &pr_searches[j];
		if (s->fromprogs == prinst)
		{	//close it down.

			if (complain)
				Con_DPrintf("Warning: Progs search was still active (pattern: %s)\n", s->pattern);

			for (i = 0; i < s->entries; i++)
				BZ_Free(s->entry[i].name);
			Z_Free(s->pattern);
			BZ_Free(s->entry);
			memset(s, 0, sizeof(*s));
		}
		else if (s->fromprogs)
			stillactive = true;
	}

	if (!stillactive)
	{	//none left, we might as well release the memory.
		BZ_Free(pr_searches);
		pr_searches = NULL;
		numpr_searches = 0;
	}
}

static int QDECL search_name_sort(const void *av, const void *bv)
{
	const struct prvmsearchentry_s *a = av, *b = bv;
	int ret = strcmp(a->name, b->name);
	//FIXME: if equal sort by original order!
	return ret;
}

static int QDECL search_enumerate(const char *name, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	prvmsearch_t *s = parm;

	size_t i;
	if (!(s->flags & QCSEARCH_ALLOWDUPES))
	{
		for (i = 0; i < s->entries; i++)
		{
			if (!Q_strcasecmp(name, s->entry[i].name))
				return true;	//already in the list, apparently. try to avoid dupes.
		}
	}

	s->entry = BZ_Realloc(s->entry, ((s->entries+64)&~63) * sizeof(*s->entry));
	s->entry[s->entries].name = BZ_Malloc(strlen(name)+1);
	strcpy(s->entry[s->entries].name, name);
	s->entry[s->entries].size = fsize;
	s->entry[s->entries].mtime = mtime;
	s->entry[s->entries].package = spath;

	s->entries++;
	return true;
}

//float	search_begin(string pattern, float flags, float quiet) = #74;
void QCBUILTIN PF_search_begin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//< 0 for error, >= 0 for handle.
	//error includes bad search patterns, but not no files
	const char *pattern = PR_GetStringOfs(prinst, OFS_PARM0);
	unsigned int flags = G_FLOAT(OFS_PARM1);
//	qboolean quiet = G_FLOAT(OFS_PARM2);	//fte is not noisy
	const char *package = (prinst->callargc>3)?PR_GetStringOfs(prinst, OFS_PARM3):NULL;
	prvmsearch_t *s;
	size_t j;

	if (!*pattern || (*pattern == '.' && pattern[1] == '.') || *pattern == '/' || *pattern == '\\' || (!(flags&QCSEARCH_MULTISEARCH)&&strchr(pattern, ':')))
	{
		PF_Warningf(prinst, "PF_search_begin: bad search pattern \"%s\"\n", pattern);
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	for (j = 0; j < numpr_searches; j++)
		if (!pr_searches[j].fromprogs)
			break;
	if (j == numpr_searches)
	{
		if (!ZF_ReallocElements((void**)&pr_searches, &numpr_searches, j+1, sizeof(*s)))
		{	//o.O
			G_FLOAT(OFS_RETURN) = -1;
			return;
		}
	}
	s = &pr_searches[j];

	s->pattern = Z_StrDup(pattern);
	s->fromprogs = prinst;
	s->flags = flags;
	s->fsflags = 0;
	if (flags&QCSEARCH_FULLPACKAGE)
		s->fsflags |= WP_FULLPATH;
	if (flags&QCSEARCH_FORCESEARCH)
		s->fsflags |= WP_FORCE;

	Q_strncpyz(s->searchinfo.purepath, package?package:"", sizeof(s->searchinfo.purepath));
	s->searchinfo.handle = COM_EnumerateFilesPackage(s->pattern, package?s->searchinfo.purepath:NULL, s->fsflags, search_enumerate, s);

	if (flags&QCSEARCH_NAMESORT)
		qsort(s->entry, s->entries, sizeof(*s->entry), search_name_sort);

	G_FLOAT(OFS_RETURN) = j;
}
//void	search_end(float handle) = #75;
void QCBUILTIN PF_search_end (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	search_close(prinst, handle);
}
//float	search_getsize(float handle) = #76;
void QCBUILTIN PF_search_getsize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	prvmsearch_t *s;
	G_FLOAT(OFS_RETURN) = 0;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "PF_search_getsize: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	G_FLOAT(OFS_RETURN) = s->entries;
}
//string	search_getfilename(float handle, float num) = #77;
void QCBUILTIN PF_search_getfilename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	int num = G_FLOAT(OFS_PARM1);
	prvmsearch_t *s;
	G_INT(OFS_RETURN) = 0;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "PF_search_getfilename: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	if (num < 0 || num >= s->entries)
		return;
	RETURN_TSTRING(s->entry[num].name);
}
void QCBUILTIN PF_search_getfilesize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	int num = G_FLOAT(OFS_PARM1);
	prvmsearch_t *s;
	G_INT(OFS_RETURN) = 0;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "PF_search_getfilesize: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	if (num < 0 || num >= s->entries)
		return;
	G_FLOAT(OFS_RETURN) = s->entry[num].size;
}
void QCBUILTIN PF_search_getfilemtime (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	int num = G_FLOAT(OFS_PARM1);
	prvmsearch_t *s;
	char timestr[128];
	G_INT(OFS_RETURN) = 0;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "PF_search_getfilemtime: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	if (num < 0 || num >= s->entries)
		return;
	if (s->entry[num].mtime != 0)	//return null/empty if the time isn't set/known.
	{
		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&s->entry[num].mtime));
		RETURN_TSTRING(timestr);
	}
}
static qboolean PF_search_getloc(flocation_t *loc, prvmsearch_t *s, int num)
{
	const char *fname = s->entry[num].name;
	if (s->searchinfo.handle)	//we were only searching a single package...
	{	//fail if its a directory, or a (pk3)symlink that we'd have to resolve.
		loc->search = &s->searchinfo;
		return loc->search->handle->FindFile(loc->search->handle, loc, fname, NULL) == FF_FOUND;
	}
	else if (!s->entry[num].package)
	{
		loc->search = &s->searchinfo;

		Q_snprintfz(loc->rawname, sizeof(loc->rawname), "%s/%s", s->searchinfo.purepath, fname);
		return true;
	}
	else
		return FS_GetLocationForPackageHandle(loc, s->entry[num].package, fname);
}
void QCBUILTIN PF_search_getpackagename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	int num = G_FLOAT(OFS_PARM1);
	prvmsearch_t *s;
	flocation_t loc;
	const char *pkgname;
	G_INT(OFS_RETURN) = 0;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "PF_search_getpackagename: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	if (num < 0 || num >= s->entries)
		return;
	if (PF_search_getloc(&loc, s, num))
	{
		pkgname = FS_WhichPackForLocation(&loc, s->fsflags);
		if (pkgname)
			RETURN_TSTRING(pkgname);
	}
}
void QCBUILTIN PF_search_fopen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	int num = G_FLOAT(OFS_PARM1);
	prvmsearch_t *s;
	flocation_t loc;
	G_FLOAT(OFS_RETURN) = -1;

	if (handle < 0 || handle >= numpr_searches || pr_searches[handle].fromprogs != prinst)
	{
		PF_Warningf(prinst, "PF_search_getpackagename: Invalid search handle %i\n", handle);
		return;
	}
	s = &pr_searches[handle];

	if (num < 0 || num >= s->entries)
		return;
	if (PF_search_getloc(&loc, s, num))
		G_FLOAT(OFS_RETURN) = PF_fopen_search (prinst, s->entry[num].name, &loc);
}

//closes filesystem type stuff for when a progs has stopped needing it.
void PR_fclose_progs (pubprogfuncs_t *prinst)
{
	PF_fcloseall(prinst);
	search_close_progs(prinst, true);
	PF_Hash_DestroyAll(prinst);
}

//File access
////////////////////////////////////////////////////
//reflection

//float	isfunction(string function_name)
void QCBUILTIN PF_isfunction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*name = PR_GetStringOfs(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = !!PR_FindFunction(prinst, name, PR_ANY);
}

//void	callfunction(...)
void QCBUILTIN PF_callfunction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*name;
	func_t f;
	if (prinst->callargc < 1)
		PR_BIError(prinst, "callfunction needs at least one argument\n");
	name = PR_GetStringOfs(prinst, OFS_PARM0+(prinst->callargc-1)*3);
	prinst->callargc -= 1;
	f = PR_FindFunction(prinst, name, PR_ANY);
	if (f)
		PR_ExecuteProgram(prinst, f);
}

//void	loadfromfile(string file)
void QCBUILTIN PF_loadfromfile (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*filename = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *file = COM_LoadTempFile(filename, 0, NULL);

	size_t size;

	if (!file)
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	while(prinst->restoreent(prinst, file, &size, NULL))
	{
		file += size;
	}

	G_FLOAT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_writetofile(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = G_FLOAT(OFS_PARM0)-FIRST_QC_FILE_INDEX;
	void *ed = G_EDICT(prinst, OFS_PARM1);

	char buffer[65536];
	char *entstr;
	size_t buflen;

	buflen = 0;
	entstr = prinst->saveent(prinst, buffer, &buflen, sizeof(buffer), ed);	//will save just one entities vars
	if (entstr)
	{
		PF_fwrite_internal (prinst, fnum, entstr, buflen);
	}
}

//read (multiple) {entity data} into new entities, with no real indication of how much was read.
void QCBUILTIN PF_loadfromdata (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*file = PR_GetStringOfs(prinst, OFS_PARM0);

	size_t size;

	if (!*file)
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	while(prinst->restoreent(prinst, file, &size, NULL))
	{
		file += size;
	}

	G_FLOAT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_generateentitydata(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	void *ed = G_EDICT(prinst, OFS_PARM0);

	char buffer[65536];
	char *entstr;
	size_t buflen;

	buflen = 0;
	entstr = prinst->saveent(prinst, buffer, &buflen, sizeof(buffer), ed);

	if (entstr)
		RETURN_TSTRING(entstr);
	else
		G_INT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_parseentitydata(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	void	*ed = G_EDICT(prinst, OFS_PARM0);
	const char	*file = PR_GetStringOfs(prinst, OFS_PARM1);
	int offset = (prinst->callargc>=3)?G_FLOAT(OFS_PARM2):0;

	size_t size;

	if (offset < 0)
		offset = 0;
	else if (offset)
	{
		int boffset = offset;
		if (VMUTF8)
			boffset = unicode_byteofsfromcharofs(file, offset, VMUTF8MARKUP);
		else
		{
			int l = strlen(file);
			if (boffset > l)
				boffset = l;
		}
		file += boffset;
	}

	if (!*file)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	if (!prinst->restoreent(prinst, file, &size, ed))
	{
		if (prinst->callargc<3)
			PF_Warningf(prinst, "parseentitydata: missing opening data\n");
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	else if (prinst->callargc<3)
	{
		file += size;
		while(*file < ' ' && *file)
			file++;
		if (*file)
			PF_Warningf(prinst, "parseentitydata: too much data\n");
	}

	if (VMUTF8)
		size = unicode_charofsfrombyteofs(file, size, VMUTF8MARKUP);

	G_FLOAT(OFS_RETURN) = offset + size;
}
//reflection
////////////////////////////////////////////////////
//Entities

void QCBUILTIN PF_WasFreed (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t	*ent;
	ent = G_WEDICT(prinst, OFS_PARM0);
	if (!ent)
		PR_BIError(prinst, "PF_WasFreed: invalid entity");
	G_FLOAT(OFS_RETURN) = ED_ISFREE(ent);
}

void QCBUILTIN PF_num_for_edict (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t	*ent;
	ent = G_WEDICT(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = ent->entnum;
}

void QCBUILTIN PF_edict_for_num(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	unsigned int num = G_FLOAT(OFS_PARM0);
	if (num >= w->num_edicts)
		RETURN_EDICT(prinst, w->edicts);
	else
	{
		G_INT(OFS_RETURN) = num;	//just directly store it. if its not spawned yet we'll need to catch that elsewhere anyway.
		if (!G_WEDICT(prinst, OFS_RETURN))
			RETURN_EDICT(prinst, w->edicts);	//hoi! it wasn't valid!
	}
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void QCBUILTIN PF_findradius (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	extern cvar_t sv_gameplayfix_blowupfallenzombies;
	extern cvar_t dpcompat_findradiusarealinks;
	wedict_t	*ent, *chain;
	float	rad;
	pvec_t	*org;
	pvec3_t	eorg;
	int		i, j;
	int f;

	chain = w->edicts;

	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	if (prinst->callargc > 2)
		f = G_INT(OFS_PARM2)+prinst->fieldadjust;
	else
		f = &((comentvars_t*)NULL)->chain - (pint_t*)NULL;

	if (dpcompat_findradiusarealinks.ival)
	{
		static wedict_t *nearent[32768];
		vec3_t mins, maxs;
		int numents;

		mins[0] = org[0] - rad;
		mins[1] = org[1] - rad;
		mins[2] = org[2] - rad;
		maxs[0] = org[0] + rad;
		maxs[1] = org[1] + rad;
		maxs[2] = org[2] + rad;

		numents = World_AreaEdicts(w, mins, maxs, nearent, countof(nearent), AREA_ALL);
		rad = rad*rad;
		for (i=0 ; i<numents ; i++)
		{
			ent = nearent[i];
			if (ent->v->solid == SOLID_NOT && (!((pint_t)ent->v->flags & FL_FINDABLE_NONSOLID)) && !sv_gameplayfix_blowupfallenzombies.ival)
				continue;
			if (sv_gameplayfix_findradiusdistancetobox.ival)
			{
				for (j=0 ; j<3 ; j++)
				{
					eorg[j] = org[j] - ent->v->origin[j];
					eorg[j] -= bound(ent->v->mins[j], eorg[j], ent->v->maxs[j]);
				}
			}
			else
			{
				for (j=0 ; j<3 ; j++)
					eorg[j] = org[j] - (ent->v->origin[j] + (ent->v->mins[j] + ent->v->maxs[j])*0.5);
			}
			if (DotProduct(eorg,eorg) > rad)
				continue;

			((pint_t*)ent->v)[f] = EDICT_TO_PROG(prinst, chain);
			chain = ent;
		}
	}
	else
	{
		rad = rad*rad;
		for (i=1 ; i<w->num_edicts ; i++)
		{
			ent = WEDICT_NUM_PB(prinst, i);
			if (ED_ISFREE(ent))
				continue;
			if (ent->v->solid == SOLID_NOT && (!((pint_t)ent->v->flags & FL_FINDABLE_NONSOLID)) && !sv_gameplayfix_blowupfallenzombies.value)
				continue;
			if (sv_gameplayfix_findradiusdistancetobox.ival)
			{
				for (j=0 ; j<3 ; j++)
				{
					eorg[j] = org[j] - ent->v->origin[j];
					eorg[j] -= bound(ent->v->mins[j], eorg[j], ent->v->maxs[j]);
				}
			}
			else
			{
				for (j=0 ; j<3 ; j++)
					eorg[j] = org[j] - (ent->v->origin[j] + (ent->v->mins[j] + ent->v->maxs[j])*0.5);
			}
			if (DotProduct(eorg,eorg) > rad)
				continue;

			((pint_t*)ent->v)[f] = EDICT_TO_PROG(prinst, chain);
			chain = ent;
		}
	}

	RETURN_EDICT(prinst, chain);
}

#ifdef QCGC
void QCBUILTIN PF_findradius_list (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t	*ent;
	pvec_t	rad;
	float	*org;
	pvec3_t	eorg;
	int		i, j;
	wedict_t **nearent;
	vec3_t mins, maxs;
	int		numents, count = 0;
	int *temp;

	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	//find out how many ents there are within the box specified.
	mins[0] = org[0] - rad;
	mins[1] = org[1] - rad;
	mins[2] = org[2] - rad;
	maxs[0] = org[0] + rad;
	maxs[1] = org[1] + rad;
	maxs[2] = org[2] + rad;
	nearent = alloca(sizeof(wedict_t)*w->num_edicts);	//guess at a max
	numents = World_AreaEdicts(w, mins, maxs, nearent, w->num_edicts, AREA_ALL);

	//allocate space for a result (overestimating slightly still)
	G_INT(OFS_RETURN) = prinst->AllocTempString(prinst, (char**)&temp, (1+numents)*sizeof(*temp));

	rad = rad*rad;
	for (i=0 ; i<numents ; i++)
	{
		ent = nearent[i];
		if (ent->v->solid == SOLID_NOT && !((pint_t)ent->v->flags & FL_FINDABLE_NONSOLID))
			continue;
		for (j=0 ; j<3 ; j++)
		{
			eorg[j] = org[j] - ent->v->origin[j];
			eorg[j] -= bound(ent->v->mins[j], org[j], ent->v->maxs[j]);
		}
		if (DotProduct(eorg,eorg) > rad)
			continue;

		temp[count++] = EDICT_TO_PROG(prinst, ent);
	}

	temp[count] = 0;

	G_INT(OFS_PARM2) = count;
}
#endif

//entity nextent(entity)
void QCBUILTIN PF_nextent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int		i;
	wedict_t	*ent;

	i = G_EDICTNUM(prinst, OFS_PARM0);
	while (1)
	{
		i++;
		if (i == *prinst->parms->num_edicts)
		{
			RETURN_EDICT(prinst, *prinst->parms->edicts);
			return;
		}
		ent = WEDICT_NUM_PB(prinst, i);
		if (!ED_ISFREE(ent))
		{
			RETURN_EDICT(prinst, ent);
			return;
		}
	}
}

//entity() spawn
void QCBUILTIN PF_Spawn (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	struct edict_s	*ed;
	ed = ED_Alloc(prinst, false, 0);
	pr_globals = PR_globals(prinst, PR_CURRENT);
	RETURN_EDICT(prinst, ed);
}

void QCBUILTIN PF_spawn_object (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int size = G_INT(OFS_PARM0);
	struct edict_s	*ed;
	if (prinst->callargc > 1)
	{
		int idx = G_INT(OFS_PARM1);
		ed = EDICT_NUM_UB(prinst, idx);
		G_FLOAT(OFS_PARM2) = (ed && ed->ereftype == ER_ENTITY);
		ed = prinst->EntAllocIndex(prinst, idx, true, size);
	}
	else
		ed = ED_Alloc(prinst, true, size);
	pr_globals = PR_globals(prinst, PR_CURRENT);
	RETURN_EDICT(prinst, ed);
}

void QCBUILTIN PF_respawnedict (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int idx = G_FLOAT(OFS_PARM0);
	struct edict_s	*ed = EDICT_NUM_UB(prinst, idx);
	G_FLOAT(OFS_PARM1) = (ed && ed->ereftype == ER_ENTITY);
	ed = prinst->EntAllocIndex(prinst, idx, false, 0);
	pr_globals = PR_globals(prinst, PR_CURRENT);
	RETURN_EDICT(prinst, ed);
}

//EXTENSION: DP_QC_COPYENTITY

//void(entity from, entity to) copyentity = #400
//copies data from one entity to another
void QCBUILTIN PF_copyentity (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t *in, *out;

	in = G_WEDICT(prinst, OFS_PARM0);
	if (prinst->callargc <= 1)
		out = (wedict_t*)ED_Alloc(prinst, false, 0);
	else
		out = G_WEDICT(prinst, OFS_PARM1);

	if (ED_ISFREE(in))
		PR_BIError(prinst, "PF_copyentity: source is free");
	if (!out || ED_ISFREE(out))
		PR_BIError(prinst, "PF_copyentity: destination is free");
	if (out->readonly)
		PR_BIError(prinst, "PF_copyentity: destination is read-only");
	if (out->fieldsize != in->fieldsize)
		PR_BIError(prinst, "PF_copyentity: different object types");

	memcpy(out->v, in->v, out->fieldsize);
	World_LinkEdict(w, out, false);

	RETURN_EDICT(prinst, out);
}

void QCBUILTIN PF_entityprotection (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t *e = G_WEDICT(prinst, OFS_PARM0);
	int prot = G_FLOAT(OFS_PARM1);

	if (ED_ISFREE(e))
		PR_BIError(prinst, "PF_entityprotection: entity is free");

	G_FLOAT(OFS_RETURN) = prot;
	if (prot < 0 || prot > 1)
		return;
	e->readonly = prot;
}

//Entities
////////////////////////////////////////////////////
//String functions

//PF_dprint
void QCBUILTIN PF_dprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_DPrintf ("%s",PF_VarString(prinst, 0, pr_globals));
}

//PF_print
void QCBUILTIN PF_print (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf ("%s",PF_VarString(prinst, 0, pr_globals));
}

//FTE_STRINGS
//C style strncasecmp (compare first n characters - case insensitive)
//C style strcasecmp (case insensitive string compare)
void QCBUILTIN PF_strncasecmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *a = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *b = PR_GetStringOfs(prinst, OFS_PARM1);

	if (prinst->callargc > 2)
	{
		int len = G_FLOAT(OFS_PARM2);
		int aofs = prinst->callargc>3?G_FLOAT(OFS_PARM3):0;
		int bofs = prinst->callargc>4?G_FLOAT(OFS_PARM4):0;

		if (VMUTF8)
		{
			aofs = aofs?unicode_byteofsfromcharofs(a, aofs, VMUTF8MARKUP):0;
			bofs = bofs?unicode_byteofsfromcharofs(b, bofs, VMUTF8MARKUP):0;
			len = max(unicode_byteofsfromcharofs(a+aofs, len, VMUTF8MARKUP), unicode_byteofsfromcharofs(b+bofs, len, VMUTF8MARKUP));
		}
		else
		{
			if (aofs < 0 || (aofs && aofs > strlen(a)))
				aofs = strlen(a);
			if (bofs < 0 || (bofs && bofs > strlen(b)))
				bofs = strlen(b);
		}

		G_FLOAT(OFS_RETURN) = Q_strncasecmp(a+aofs, b+bofs, len);
	}
	else
		G_FLOAT(OFS_RETURN) = Q_strcasecmp(a, b);
}

//FTE_STRINGS
//C style strncmp (compare first n characters - case sensitive. Note that there is no strcmp provided)
void QCBUILTIN PF_strncmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *a = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *b = PR_GetStringOfs(prinst, OFS_PARM1);

	if (prinst->callargc > 2)
	{
		int len = G_FLOAT(OFS_PARM2);
		int aofs = prinst->callargc>3?G_FLOAT(OFS_PARM3):0;
		int bofs = prinst->callargc>4?G_FLOAT(OFS_PARM4):0;

		if (VMUTF8)
		{
			aofs = aofs?unicode_byteofsfromcharofs(a, aofs, VMUTF8MARKUP):0;
			bofs = bofs?unicode_byteofsfromcharofs(b, bofs, VMUTF8MARKUP):0;
			len = max(unicode_byteofsfromcharofs(a+aofs, len, VMUTF8MARKUP), unicode_byteofsfromcharofs(b+bofs, len, VMUTF8MARKUP));
		}
		else
		{
			if (aofs < 0 || (aofs && aofs > strlen(a)))
				aofs = strlen(a);
			if (bofs < 0 || (bofs && bofs > strlen(b)))
				bofs = strlen(b);
		}

		G_FLOAT(OFS_RETURN) = Q_strncmp(a + aofs, b, len);
	}
	else
		G_FLOAT(OFS_RETURN) = Q_strcmp(a, b);
}

//uses qw style \key\value strings
void QCBUILTIN PF_infoget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *info = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *key = PR_GetStringOfs(prinst, OFS_PARM1);

	key = Info_ValueForKey(info, key);

	RETURN_TSTRING(key);
}

//uses qw style \key\value strings
void QCBUILTIN PF_infoadd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *info = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *key = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *value = PF_VarString(prinst, 2, pr_globals);
	char temp[8192];

	Q_strncpyz(temp, info, MAXTEMPBUFFERLEN);

	Info_SetValueForStarKey(temp, key, value, MAXTEMPBUFFERLEN);

	RETURN_TSTRING(temp);
}

//string(float pad, string str1, ...) strpad
void QCBUILTIN PF_strpad (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char destbuf[4096];
	char *dest = destbuf;
	int pad = G_FLOAT(OFS_PARM0);
	const char *src = PF_VarString(prinst, 1, pr_globals);

	//UTF-8-FIXME: pad is chars not bytes...

	if (pad < 0)
	{	//pad left
		pad = -pad - strlen(src);
		if (pad>=sizeof(destbuf))
			pad = sizeof(destbuf)-1;
		if (pad < 0)
			pad = 0;

		Q_strncpyz(dest+pad, src, sizeof(destbuf)-pad);
		while(pad)
		{
			dest[--pad] = ' ';
		}
	}
	else
	{	//pad right
		if (pad>=sizeof(destbuf))
			pad = sizeof(destbuf)-1;
		pad -= strlen(src);
		if (pad < 0)
			pad = 0;

		Q_strncpyz(dest, src, sizeof(destbuf));
		dest+=strlen(dest);

		while(pad-->0)
			*dest++ = ' ';
		*dest = '\0';
	}

	RETURN_TSTRING(destbuf);
}

void QCBUILTIN PF_strtrim (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *str = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *end;
	char *news;
	
	//figure out the new start
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
		str++;

	//figure out the new end.
	end = str + strlen(str);
	while(end > str && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r'))
		end--;

	//copy that substring into a tempstring.
	((int *)pr_globals)[OFS_RETURN] = prinst->AllocTempString(prinst, &news, end - str + 1);
	memcpy(news, str, end-str);
	news[end-str] = 0;
}

//part of PF_strconv
static int chrconv_number(int i, int base, int conv)
{
	i -= base;
	switch (conv)
	{
	default:
	case 5:
	case 6:
	case 0:
		break;
	case 1:
		base = '0';
		break;
	case 2:
		base = '0'+128;
		break;
	case 3:
		base = '0'-30;
		break;
	case 4:
		base = '0'+128-30;
		break;
	}
	return i + base;
}
//part of PF_strconv
static int chrconv_punct(int i, int base, int conv)
{
	i -= base;
	switch (conv)
	{
	default:
	case 0:
		break;
	case 1:
		base = 0;
		break;
	case 2:
		base = 128;
		break;
	}
	return i + base;
}
//part of PF_strconv
static int chrchar_alpha(int i, int basec, int baset, int convc, int convt, int charnum)
{
	//convert case and colour seperatly...

	i -= baset + basec;
	switch (convt)
	{
	default:
	case 0:
		break;
	case 1:
		baset = 0;
		break;
	case 2:
		baset = 128;
		break;

	case 5:
	case 6:
		baset = 128*((charnum&1) == (convt-5));
		break;
	}

	switch (convc)
	{
	default:
	case 0:
		break;
	case 1:
		basec = 'a';
		break;
	case 2:
		basec = 'A';
		break;
	}
	return i + basec + baset;
}
//FTE_STRINGS
//bulk convert a string. change case or colouring.
void QCBUILTIN PF_strconv (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int ccase = G_FLOAT(OFS_PARM0);		//0 same, 1 lower, 2 upper
	int redalpha = G_FLOAT(OFS_PARM1);	//0 same, 1 white, 2 red,  5 alternate, 6 alternate-alternate
	int rednum = G_FLOAT(OFS_PARM2);	//0 same, 1 white, 2 red, 3 redspecial, 4 whitespecial, 5 alternate, 6 alternate-alternate
	const unsigned char *string = PF_VarString(prinst, 3, pr_globals);
	int len = strlen(string);
	int i;
	unsigned char resbuf[8192];
	unsigned char *result = resbuf;

	//UTF-8-FIXME: cope with utf+^U etc

	if (len >= MAXTEMPBUFFERLEN)
		len = MAXTEMPBUFFERLEN-1;

	for (i = 0; i < len; i++, string++, result++)	//should this be done backwards?
	{
		if (*string >= '0' && *string <= '9')	//normal numbers...
			*result = chrconv_number(*string, '0', rednum);
		else if (*string >= '0'+128 && *string <= '9'+128)
			*result = chrconv_number(*string, '0'+128, rednum);
		else if (*string >= '0'+128-30 && *string <= '9'+128-30)
			*result = chrconv_number(*string, '0'+128-30, rednum);
		else if (*string >= '0'-30 && *string <= '9'-30)
			*result = chrconv_number(*string, '0'-30, rednum);

		else if (*string >= 'a' && *string <= 'z')	//normal numbers...
			*result = chrchar_alpha(*string, 'a', 0, ccase, redalpha, i);
		else if (*string >= 'A' && *string <= 'Z')	//normal numbers...
			*result = chrchar_alpha(*string, 'A', 0, ccase, redalpha, i);
		else if (*string >= 'a'+128 && *string <= 'z'+128)	//normal numbers...
			*result = chrchar_alpha(*string, 'a', 128, ccase, redalpha, i);
		else if (*string >= 'A'+128 && *string <= 'Z'+128)	//normal numbers...
			*result = chrchar_alpha(*string, 'A', 128, ccase, redalpha, i);

		else if ((*string & 127) < 16 || !redalpha)	//special chars..
			*result = *string;
		else if (*string < 128)
			*result = chrconv_punct(*string, 0, redalpha);
		else
			*result = chrconv_punct(*string, 128, redalpha);
	}
	*result = '\0';

	RETURN_TSTRING(((char*)resbuf));
}

//FTE_STRINGS
//returns a string containing one character per parameter (up to the qc max params of 8).
void QCBUILTIN PF_chr2str (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int ch;
	int i;
	char string[128], *s = string;
	for (i = 0; i < prinst->callargc; i++)
	{
		ch = G_FLOAT(OFS_PARM0 + i*3);
		if (VMUTF8 || ch > 0xff)
			s += unicode_encode(s, ch, (string+sizeof(string)-1)-s, VMUTF8MARKUP||(ch>0xff));
		else
			*s++ = G_FLOAT(OFS_PARM0 + i*3);
	}
	*s++ = '\0';
	RETURN_TSTRING(string);
}

//FTE_STRINGS
//returns character at position X
void QCBUILTIN PF_str2chr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int err;
	const char *next;
	const char *instr = PR_GetStringOfs(prinst, OFS_PARM0);
	int ofs = (prinst->callargc>1)?G_FLOAT(OFS_PARM1):0;

	if (VMUTF8)
	{
		if (ofs < 0)
			ofs = unicode_charcount(instr, 1<<30, VMUTF8MARKUP)+ofs;
		ofs = unicode_byteofsfromcharofs(instr, ofs, VMUTF8MARKUP);
	}
	else
	{
		if (ofs < 0)
			ofs = strlen(instr)+ofs;
	}

	if (ofs && (ofs < 0 || ofs > strlen(instr)))
		G_FLOAT(OFS_RETURN) = '\0';
	else
	{
		if (VMUTF8)
			G_FLOAT(OFS_RETURN) = unicode_decode(&err, instr+ofs, &next, VMUTF8MARKUP);
		else
			G_FLOAT(OFS_RETURN) = (unsigned char)instr[ofs];
	}
}

//FTE_STRINGS
//strstr, without generating a new string. Use in conjunction with FRIK_FILE's substring for more similar strstr.
void QCBUILTIN PF_strstrofs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *instr = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *match = PR_GetStringOfs(prinst, OFS_PARM1);

	int firstofs = (prinst->callargc>2)?G_FLOAT(OFS_PARM2):0;

	if (VMUTF8)
		firstofs = unicode_byteofsfromcharofs(instr, firstofs, VMUTF8MARKUP);

	if (firstofs && (firstofs < 0 || firstofs > strlen(instr)))
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	match = strstr(instr+firstofs, match);
	if (!match)
		G_FLOAT(OFS_RETURN) = -1;
	else
		G_FLOAT(OFS_RETURN) = VMUTF8?unicode_charofsfrombyteofs(instr, match-instr, VMUTF8MARKUP):(match - instr);
}

//float(string input) stof
void QCBUILTIN PF_stof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*s;

	s = PR_GetStringOfs(prinst, OFS_PARM0);

	G_FLOAT(OFS_RETURN) = atof(s);
}

//tstring(float input) ftos
void QCBUILTIN PF_ftos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	v;
	char pr_string_temp[64];
	v = G_FLOAT(OFS_PARM0);

	if (v == (int)v)
		sprintf (pr_string_temp, "%d",(int)v);
	else if (pr_brokenfloatconvert.value)
		sprintf (pr_string_temp, "%5.1f",v);
	else
		Q_ftoa (pr_string_temp, v);
	RETURN_TSTRING(pr_string_temp);
}

void QCBUILTIN PF_ftou (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_UINT(OFS_RETURN) = G_FLOAT(OFS_PARM0);
}
void QCBUILTIN PF_utof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (prinst->callargc > 1)
	{
		unsigned int value = G_UINT(OFS_PARM0);
		unsigned int shift = G_FLOAT(OFS_PARM1);
		unsigned int count = G_FLOAT(OFS_PARM2);
		value >>= shift;
		if (count != 32)
			value &= ((1u<<count)-1u);
		G_FLOAT(OFS_RETURN) = value;
	}
	else
		G_FLOAT(OFS_RETURN) = G_UINT(OFS_PARM0);
}

void QCBUILTIN PF_ftoi (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT(OFS_RETURN) = G_FLOAT(OFS_PARM0);
}
void QCBUILTIN PF_itof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (prinst->callargc > 1)
	{
		unsigned int value = G_INT(OFS_PARM0);
		unsigned int shift = G_FLOAT(OFS_PARM1);
		unsigned int count = G_FLOAT(OFS_PARM2);
		value >>= shift;
		if (count != 32)
			value &= ((1u<<count)-1u);
		G_FLOAT(OFS_RETURN) = value;
	}
	else
		G_FLOAT(OFS_RETURN) = G_INT(OFS_PARM0);
}

//tstring(integer input) itos
void QCBUILTIN PF_itos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int	v;
	char pr_string_temp[64];
	v = G_INT(OFS_PARM0);

	sprintf (pr_string_temp, "%d",v);
	RETURN_TSTRING(pr_string_temp);
}

//int(string input) stoi
void QCBUILTIN PF_stoi (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *input = PR_GetStringOfs(prinst, OFS_PARM0);

	G_INT(OFS_RETURN) = atoi(input);
}

//tstring(integer input) htos
void QCBUILTIN PF_htos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int	v;
	char pr_string_temp[64];
	v = G_INT(OFS_PARM0);

	sprintf (pr_string_temp, "%08x",v);
	RETURN_TSTRING(pr_string_temp);
}

//int(string input) stoh
void QCBUILTIN PF_stoh (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *input = PR_GetStringOfs(prinst, OFS_PARM0);

	G_INT(OFS_RETURN) = strtoul(input, NULL, 16);
}

//vector(string s) stov = #117
//returns vector value from a string
void QCBUILTIN PF_stov (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	const char *s;
	float *out;

	s = PF_VarString(prinst, 0, pr_globals);
	out = G_VECTOR(OFS_RETURN);
	out[0] = out[1] = out[2] = 0;

	if (*s == '\'')
		s++;

	for (i = 0; i < 3; i++)
	{
		while (*s == ' ' || *s == '\t')
			s++;
		out[i] = atof (s);
		if (!out[i] && *s != '-' && *s != '+' && (*s < '0' || *s > '9'))
			break; // not a number
		while (*s && *s != ' ' && *s !='\t' && *s != '\'')
			s++;
		if (*s == '\'')
			break;
	}
}

//tstring(vector input) vtos
void QCBUILTIN PF_vtos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char pr_string_temp[64];
	//sprintf (pr_string_temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	sprintf (pr_string_temp, "'%f %f %f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	RETURN_TSTRING(pr_string_temp);
}

void QCBUILTIN PF_Logarithm (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//log2(v) = ln(v)/ln(2)
	double r;
	r = log(G_FLOAT(OFS_PARM0));
	if (prinst->callargc > 1)
		r /= log(G_FLOAT(OFS_PARM1));
	G_FLOAT(OFS_RETURN) = r;
}


void QCBUILTIN PF_strunzone(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef QCGC
	//gc frees everything for us.
	//FIXME: explicitly free it anyway, to save running the gc quite so often.
#else
	prinst->AddressableFree(prinst, prinst->stringtable + G_INT(OFS_PARM0));
#endif
}

void QCBUILTIN PF_strzone(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)	//frik_file
{
#ifdef QCGC
	//just allocate a tempstring instead, because we can.
	PF_strcat(prinst, pr_globals);
#else
	char *buf;
	int len = 0;
	const char *s[8];
	int l[8];
	int i;
	for (i = 0; i < prinst->callargc; i++)
	{
		s[i] = PR_GetStringOfs(prinst, OFS_PARM0+i*3);
		l[i] = strlen(s[i]);
		len += l[i];
	}
	len++; /*for the null*/

	buf = prinst->AddressableAlloc(prinst, len);
	if (!buf)
	{
		G_INT(OFS_RETURN) = 0;
		return;
	}
	G_INT(OFS_RETURN) = (char*)buf - prinst->stringtable;
	
	len = 0;
	for (i = 0; i < prinst->callargc; i++)
	{
		memcpy(buf, s[i], l[i]);
		buf += l[i];
	}
	*buf = '\0';
#endif
}

#ifdef QCGC
void QCBUILTIN PF_createbuffer (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int len = G_INT(OFS_PARM0);
	char *buf;
	if (len <= 0)
		G_INT(OFS_RETURN) = 0;
	else
	{
		len++;
		G_INT(OFS_RETURN) = prinst->AllocTempString(prinst, &buf, len);
		memset(buf, 0, len);
	}
}
#endif

//string(string str1, string str2, str3, etc) strcat
void QCBUILTIN PF_strcat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *buf;
	size_t len = 0;
	const char *s[8];
	int l[8];
	int i;
	for (i = 0; i < prinst->callargc; i++)
	{
		s[i] = PR_GetStringOfs(prinst, OFS_PARM0+i*3);
		l[i] = strlen(s[i]);
		len += l[i];

#ifdef HAVE_LEGACY
		if (dpcompat_strcat_limit.ival && len > dpcompat_strcat_limit.ival)
		{
			l[i]-= len-dpcompat_strcat_limit.ival;
			len -= len-dpcompat_strcat_limit.ival;
		}
#endif
	}
	len++; /*for the null*/
	((int *)pr_globals)[OFS_RETURN] = prinst->AllocTempString(prinst, &buf, len);
	if (buf)
	{
		for (i = 0; i < prinst->callargc; i++)
		{
			memcpy(buf, s[i], l[i]);
			buf += l[i];
		}
		*buf = '\0';
	}
}

//returns a section of a string as a tempstring
void QCBUILTIN PF_substring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int start, length, slen;
	const char *s;
	char *string;

	s = PR_GetStringOfs(prinst, OFS_PARM0);
	start = G_FLOAT(OFS_PARM1);
	length = G_FLOAT(OFS_PARM2);

	//UTF-8-FIXME: start+length are chars not bytes...

	if (VMUTF8)
		slen = unicode_charcount(s, 1<<30, VMUTF8MARKUP);
	else
		slen = strlen(s);

	if (start < 0)
		start = slen+start;
	if (length < 0)
		length = slen-start+(length+1);
	if (start < 0)
	{
	//	length += start;
		start = 0;
	}

	if (start >= slen || length<=0)
	{
		RETURN_TSTRING("");
		return;
	}

	slen -= start;
	if (length > slen)
		length = slen;

	if (VMUTF8)
	{
		start = unicode_byteofsfromcharofs(s, start, VMUTF8MARKUP);
		length = unicode_byteofsfromcharofs(s+start, length, VMUTF8MARKUP);
	}
	s += start;

	((int *)pr_globals)[OFS_RETURN] = prinst->AllocTempString(prinst, &string, length+1);

	memcpy(string, s, length);
	string[length] = '\0';
}

void QCBUILTIN PF_strlen(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (VMUTF8)
		G_FLOAT(OFS_RETURN) = unicode_charcount(PR_GetStringOfs(prinst, OFS_PARM0), 1<<30, VMUTF8MARKUP);
	else
		G_FLOAT(OFS_RETURN) = strlen(PR_GetStringOfs(prinst, OFS_PARM0));
}

//float(string input, string token) instr
void QCBUILTIN PF_instr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *sub;
	const char *s1;
	const char *s2;

	s1 = PR_GetStringOfs(prinst, OFS_PARM0);
	s2 = PF_VarString(prinst, 1, pr_globals);

	if (!s1 || !s2)
	{
		PR_BIError(prinst, "Null string in \"instr\"\n");
		return;
	}

	sub = strstr(s1, s2);

	if (sub == NULL)
		G_INT(OFS_RETURN) = 0;
	else
		RETURN_SSTRING(sub);	//last as long as the original string
}

void QCBUILTIN PF_strreplace (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char resultbuf[4096];
	char *result = resultbuf;
	const char *search = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *replace = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *subject = PR_GetStringOfs(prinst, OFS_PARM2);
	int searchlen = strlen(search);
	int replacelen = strlen(replace);

	if (searchlen)
	{
		while (*subject && result < resultbuf + sizeof(resultbuf) - replacelen - 2)
		{
			if (!strncmp(subject, search, searchlen))
			{
				subject += searchlen;
				memcpy(result, replace, replacelen);
				result += replacelen;
			}
			else
				*result++ = *subject++;
		}
		*result = 0;
		RETURN_TSTRING(resultbuf);
	}
	else
		RETURN_TSTRING(subject);
}
void QCBUILTIN PF_strireplace (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char resultbuf[4096];
	char *result = resultbuf;
	const char *search = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *replace = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *subject = PR_GetStringOfs(prinst, OFS_PARM2);
	int searchlen = strlen(search);
	int replacelen = strlen(replace);

	if (searchlen)
	{
		while (*subject && result < resultbuf + sizeof(resultbuf) - replacelen - 2)
		{
			//UTF-8-FIXME: case insensitivity is awkward...
			if (!strnicmp(subject, search, searchlen))
			{
				subject += searchlen;
				memcpy(result, replace, replacelen);
				result += replacelen;
			}
			else
				*result++ = *subject++;
		}
		*result = 0;
		RETURN_TSTRING(resultbuf);
	}
	else
		RETURN_TSTRING(subject);
}

//string(entity ent) etos = #65
void QCBUILTIN PF_etos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char s[64];
	snprintf (s, sizeof(s), "entity %i", G_EDICTNUM(prinst, OFS_PARM0));
	RETURN_TSTRING(s);
}

//DP_QC_STRINGCOLORFUNCTIONS
// #476 float(string s) strlennocol - returns how many characters are in a string, minus color codes
void QCBUILTIN PF_strlennocol (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *in = PR_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];
	conchar_t flagged[8192];
	unsigned int len = 0;
	COM_ParseFunString(CON_WHITEMASK, in, flagged, sizeof(flagged), false);
	COM_DeFunString(flagged, NULL, result, sizeof(result), true, false);

	for (len = 0; result[len]; len++)
		;
	G_FLOAT(OFS_RETURN) = len;
}

//DP_QC_STRINGCOLORFUNCTIONS
// string (string s) strdecolorize - returns the passed in string with color codes stripped
void QCBUILTIN PF_strdecolorize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *in = PR_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];
	conchar_t flagged[8192];
	COM_ParseFunString(CON_WHITEMASK, in, flagged, sizeof(flagged), false);
	COM_DeFunString(flagged, NULL, result, sizeof(result), true, false);

	RETURN_TSTRING(result);
}

//DP_QC_STRING_CASE_FUNCTIONS
void QCBUILTIN PF_strtolower (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *in = PR_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];

	unicode_strtolower(in, result, sizeof(result), VMUTF8MARKUP);

	RETURN_TSTRING(result);
}

//DP_QC_STRING_CASE_FUNCTIONS
void QCBUILTIN PF_strtoupper (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *in = PR_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];

	unicode_strtoupper(in, result, sizeof(result), VMUTF8MARKUP);

	RETURN_TSTRING(result);
}

//DP_QC_STRFTIME
void QCBUILTIN PF_strftime (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *in = PF_VarString(prinst, 1, pr_globals);
	char result[8192];

	time_t ctime;
	struct tm *tm;

	ctime = time(NULL);

	if (G_FLOAT(OFS_PARM0))
		tm = localtime(&ctime);
	else
		tm = gmtime(&ctime);

	//msvc sucks.
	if (!strcmp(in, "%R"))
		in = "%H:%M";
	else if (!strcmp(in, "%F"))
		in = "%Y-%m-%d";

	strftime(result, sizeof(result), in, tm);

	RETURN_TSTRING(result);
}

//String functions
////////////////////////////////////////////////////
//515's String functions

struct strbuf {
	pubprogfuncs_t *prinst;
	char **strings;
	size_t used;
	size_t allocated;
	int flags;
};

#define BUFFLAG_SAVED 1
#define BUFSTRBASE 1	//officially these are 0-based (ie: use negatives for not-a-buffer), but fte biases it to catch qc bugs.
struct strbuf	*strbuflist;
size_t			strbufmax;

static void PR_buf_savegame(vfsfile_t *f, pubprogfuncs_t *prinst, qboolean binary)
{
	unsigned int i, bufno;
	char *tmp = NULL;
	size_t tmpsize = 0, ns;

	for (bufno = 0; bufno < strbufmax; bufno++)
	{
		if (strbuflist[bufno].prinst == prinst && (strbuflist[bufno].flags & BUFFLAG_SAVED))
		{
			VFS_PRINTF (f, "buffer %u %i %i %u\n", bufno+BUFSTRBASE, strbuflist[bufno].flags, ev_string, (unsigned int)strbuflist[bufno].used);
			VFS_PRINTF (f, "{\n");
			for (i = 0; i < strbuflist[bufno].used; i++)
				if (strbuflist[bufno].strings[i])
				{
					ns = strlen(strbuflist[bufno].strings[i])*2 + 4;
					if (ns > tmpsize)
						Z_ReallocElements((void**)&tmp, &tmpsize, ns, sizeof(char));
					VFS_PRINTF (f, "%u %s\n", i, COM_QuotedString(strbuflist[bufno].strings[i], tmp, tmpsize, false));
				}
			VFS_PRINTF (f, "}\n");
		}
	}
	free(tmp);
}
static const char *PR_buf_loadgame(pubprogfuncs_t *prinst, const char *l)
{
	char token[65536];
	int bufno;
	unsigned int flags;
	size_t buffersize, index;
	com_tokentype_t tt;
	struct strbuf *buf;
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	bufno = atoi(token)-BUFSTRBASE;
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	flags = atoi(token);
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	if (atoi(token) != ev_string) return NULL;	//we only support string buffers right now. FIXME: if the token was "string" then create an empty strbuf.
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_RAWTOKEN)return NULL;
	buffersize = atoi(token);

	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_LINEENDING)return NULL;
	l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_PUNCTUATION)return NULL;
	if (strcmp(token, "{"))	return NULL;

	if (bufno < 0 || bufno >= 1<<16)
		return NULL;
	if (bufno >= strbufmax)
		Z_ReallocElements((void**)&strbuflist, &strbufmax, bufno+1, sizeof(*strbuflist));

	buf = &strbuflist[bufno];
	if (buf->prinst)
	{	//already alive... wipe it.
		for (index = 0; index < buf->used; index++)
			Z_Free(buf->strings[index]);
		Z_Free(buf->strings);

		buf->strings = NULL;
		buf->used = 0;
		buf->allocated = 0;
	}
	buf->prinst = prinst;
	buf->flags = flags;

	if (buffersize)
	{	//preallocate the buffer to match what it used to be.
		if (buffersize < 1<<20)
		{
			Z_ReallocElements((void**)&buf->strings, &buf->allocated, buffersize, sizeof(*buf->strings));
			buf->used = buf->allocated;
		}
	}

	for(;;)
	{
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);
		if (tt == TTP_LINEENDING)
			continue;
		if (tt == TTP_PUNCTUATION && !strcmp(token, "}"))
			break;
		if (tt != TTP_RAWTOKEN)
			break;
		index = atoi(token);
		l = COM_ParseTokenOut(l, NULL, token, sizeof(token), &tt);if (tt != TTP_STRING)return NULL;

		if (index < 0 || index >= buf->allocated)
			continue;	//some sort of error.

		if (buf->strings[index])	//shouldn't really happen, but just in case we're given bad input...
			Z_Free(buf->strings[index]);
		buf->strings[index] = Z_Malloc(strlen(token)+1);
		strcpy(buf->strings[index], token);

		if (index >= buf->used)
			buf->used = index+1;
	}
	return l;
}

void PF_buf_shutdown(pubprogfuncs_t *prinst)
{
	size_t i, bufno;

	for (bufno = 0; bufno < strbufmax; bufno++)
	{
		if (strbuflist[bufno].prinst == prinst)
		{
			for (i = 0; i < strbuflist[bufno].used; i++)
				Z_Free(strbuflist[bufno].strings[i]);
			Z_Free(strbuflist[bufno].strings);

			strbuflist[bufno].strings = NULL;
			strbuflist[bufno].used = 0;
			strbuflist[bufno].allocated = 0;

			strbuflist[bufno].prinst = NULL;
		}
	}
	while (strbufmax>0 && !strbuflist[strbufmax-1].prinst)
		strbufmax--;
	if (!strbufmax)
	{
		Z_Free(strbuflist);
		strbuflist = NULL;
	}
}

// #440 float() buf_create (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_buf_create  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t i;

	const char *type = ((prinst->callargc>0)?PR_GetStringOfs(prinst, OFS_PARM0):"string");
	unsigned int flags = ((prinst->callargc>1)?G_FLOAT(OFS_PARM1):BUFFLAG_SAVED);
	flags &= BUFFLAG_SAVED;

	if (!Q_strcasecmp(type, "string"))
		;
	else
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	//flags&1 == saved. apparently.

	for (i = 0; i < strbufmax; i++)
	{
		if (!strbuflist[i].prinst)
			break;
	}
	if (i == strbufmax)
	{
		if (!ZF_ReallocElements((void**)&strbuflist, &strbufmax, strbufmax+1, sizeof(*strbuflist)))
		{
			G_FLOAT(OFS_RETURN) = -1;
			return;
		}
	}
	strbuflist[i].prinst = prinst;
	strbuflist[i].used = 0;
	strbuflist[i].allocated = 0;
	strbuflist[i].strings = NULL;
	strbuflist[i].flags = flags;
	G_FLOAT(OFS_RETURN) = i+BUFSTRBASE;
}
// #441 void(float bufhandle) buf_del (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_buf_del  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t i;
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	for (i = 0; i < strbuflist[bufno].used; i++)
		Z_Free(strbuflist[bufno].strings[i]);
	Z_Free(strbuflist[bufno].strings);

	strbuflist[bufno].strings = NULL;
	strbuflist[bufno].used = 0;
	strbuflist[bufno].allocated = 0;

	strbuflist[bufno].prinst = NULL;
}
// #442 float(float bufhandle) buf_getsize (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_buf_getsize  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	G_FLOAT(OFS_RETURN) = strbuflist[bufno].used;
}
// #443 void(float bufhandle_from, float bufhandle_to) buf_copy (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_buf_copy  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t buffrom = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	size_t bufto = G_FLOAT(OFS_PARM1)-BUFSTRBASE;
	size_t i;

	if (bufto == buffrom)	//err...
		return;
	if (buffrom >= strbufmax)
		return;
	if (strbuflist[buffrom].prinst != prinst)
		return;
	if (bufto >= strbufmax)
		return;
	if (strbuflist[bufto].prinst != prinst)
		return;

	//obliterate any and all existing data.
	for (i = 0; i < strbuflist[bufto].used; i++)
		Z_Free(strbuflist[bufto].strings[i]);
	Z_Free(strbuflist[bufto].strings);

	//copy new data over.
	strbuflist[bufto].used = strbuflist[bufto].allocated = strbuflist[buffrom].used;
	strbuflist[bufto].strings = BZ_Malloc(strbuflist[buffrom].used * sizeof(char*));
	for (i = 0; i < strbuflist[buffrom].used; i++)
		strbuflist[bufto].strings[i] = strbuflist[buffrom].strings[i]?Z_StrDup(strbuflist[buffrom].strings[i]):NULL;
}
static int PF_buf_sort_sortprefixlen;
static int QDECL PF_buf_sort_ascending_prefix(const void *a, const void *b)
{
	return strncmp(*(char**)a, *(char**)b, PF_buf_sort_sortprefixlen);
}
static int QDECL PF_buf_sort_descending_prefix(const void *b, const void *a)
{
	return strncmp(*(char**)a, *(char**)b, PF_buf_sort_sortprefixlen);
}
// #444 void(float bufhandle, float sortprefixlen, float backward) buf_sort (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_buf_sort  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	int sortprefixlen = G_FLOAT(OFS_PARM1);
	qboolean backwards = G_FLOAT(OFS_PARM2);
	int s,d;
	char **strings;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (sortprefixlen <= 0)
		sortprefixlen = 0x7fffffff;

	//take out the nulls first, to avoid weird/crashy sorting
	for (s = 0, d = 0, strings = strbuflist[bufno].strings; s < strbuflist[bufno].used; )
	{
		if (!strings[s])
		{
			s++;
			continue;
		}
		strings[d++] = strings[s++];
	}
	strbuflist[bufno].used = d;

	//no nulls now, sort it.
	PF_buf_sort_sortprefixlen = sortprefixlen;	//eww, a global. burn in hell.
	if (backwards)	//z first
		qsort(strings, strbuflist[bufno].used, sizeof(char*), PF_buf_sort_descending_prefix);
	else	//a first
		qsort(strings, strbuflist[bufno].used, sizeof(char*), PF_buf_sort_ascending_prefix);
}
// #445 string(float bufhandle, string glue) buf_implode (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_buf_implode  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	const char *glue = PR_GetStringOfs(prinst, OFS_PARM1);
	unsigned int gluelen = strlen(glue);
	size_t retlen, l, i;
	char **strings;
	char *ret;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	//count neededlength
	strings = strbuflist[bufno].strings;
	for (i = 0, retlen = 0; i < strbuflist[bufno].used; i++)
	{
		if (strings[i])
		{
			if (retlen)
				retlen += gluelen;
			retlen += strlen(strings[i]);
		}
	}

	//generate the output
	ret = malloc(retlen+1);
	for (i = 0, retlen = 0; i < strbuflist[bufno].used; i++)
	{
		if (strings[i])
		{
			if (retlen)
			{
				memcpy(ret+retlen, glue, gluelen);
				retlen += gluelen;
			}
			l = strlen(strings[i]);
			memcpy(ret+retlen, strings[i], l);
			retlen += l;
		}
	}

	//add the null and return
	ret[retlen] = 0;
	RETURN_TSTRING(ret);
	free(ret);
}
// #446 string(float bufhandle, float string_index) bufstr_get (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_bufstr_get  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	size_t index = G_FLOAT(OFS_PARM1);

	if ((unsigned int)bufno >= strbufmax)
	{
		G_INT(OFS_RETURN) = 0;
		return;
	}
	if (strbuflist[bufno].prinst != prinst)
	{
		G_INT(OFS_RETURN) = 0;
		return;
	}

	if (index >= strbuflist[bufno].used)
	{
		G_INT(OFS_RETURN) = 0;
		return;
	}

	RETURN_TSTRING(strbuflist[bufno].strings[index]);
}
// #447 void(float bufhandle, float string_index, string str) bufstr_set (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_bufstr_set  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	size_t index = G_FLOAT(OFS_PARM1);
	const char *string = PR_GetStringOfs(prinst, OFS_PARM2);
	size_t oldcount;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (index >= strbuflist[bufno].allocated)
	{
		if (index > 1024*1024)
		{
			PR_RunWarning(prinst, "index outside sanity range\n");
			return;
		}
		oldcount = strbuflist[bufno].allocated;
		strbuflist[bufno].allocated = (index + 256);
		strbuflist[bufno].strings = BZ_Realloc(strbuflist[bufno].strings, strbuflist[bufno].allocated*sizeof(char*));
		memset(strbuflist[bufno].strings+oldcount, 0, (strbuflist[bufno].allocated - oldcount) * sizeof(char*));
	}
	if (strbuflist[bufno].strings[index])
		Z_Free(strbuflist[bufno].strings[index]);
	strbuflist[bufno].strings[index] = Z_Malloc(strlen(string)+1);
	strcpy(strbuflist[bufno].strings[index], string);

	if (index >= strbuflist[bufno].used)
		strbuflist[bufno].used = index+1;
}

static size_t PF_bufstr_add_internal(int bufno, const char *string, int appendonend)
{
	size_t index;
	if (appendonend)
	{
		//add on end
		index = strbuflist[bufno].used;
	}
	else
	{
		//find a hole
		for (index = 0; index < strbuflist[bufno].used; index++)
			if (!strbuflist[bufno].strings[index])
				break;
	}

	//expand it if needed
	if (index >= strbuflist[bufno].allocated)
	{
		int oldcount;
		oldcount = strbuflist[bufno].allocated;
		strbuflist[bufno].allocated = (index + 256);
		strbuflist[bufno].strings = BZ_Realloc(strbuflist[bufno].strings, strbuflist[bufno].allocated*sizeof(char*));
		memset(strbuflist[bufno].strings+oldcount, 0, (strbuflist[bufno].allocated - oldcount) * sizeof(char*));
	}

	//add in the new string.
	if (strbuflist[bufno].strings[index])
		Z_Free(strbuflist[bufno].strings[index]);
	strbuflist[bufno].strings[index] = Z_Malloc(strlen(string)+1);
	strcpy(strbuflist[bufno].strings[index], string);

	if (index >= strbuflist[bufno].used)
		strbuflist[bufno].used = index+1;

	return index;
}

// #448 float(float bufhandle, string str, float order) bufstr_add (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_bufstr_add  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	const char *string = PR_GetStringOfs(prinst, OFS_PARM1);
	int order = G_FLOAT(OFS_PARM2);

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	G_FLOAT(OFS_RETURN) = PF_bufstr_add_internal(bufno, string, order);
}
// #449 void(float bufhandle, float string_index) bufstr_free (DP_QC_STRINGBUFFERS)
void QCBUILTIN PF_bufstr_free  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	size_t index = G_FLOAT(OFS_PARM1);

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (index >= strbuflist[bufno].used)
		return;	//not valid anyway.

	if (strbuflist[bufno].strings[index])
		Z_Free(strbuflist[bufno].strings[index]);
	strbuflist[bufno].strings[index] = NULL;
}

static int QDECL PF_buf_sort_ascending(const void *a, const void *b)
{
	return strcmp(*(char**)a, *(char**)b);
}
void QCBUILTIN PF_buf_cvarlist  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	const char *pattern = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *antipattern = PR_GetStringOfs(prinst, OFS_PARM2);
	int i;
	cvar_group_t	*grp;
	cvar_t	*var;
	extern cvar_group_t *cvar_groups;
	int plen = strlen(pattern), alen = strlen(antipattern);
	qboolean pwc = strchr(pattern, '*')||strchr(pattern, '?'),
			 awc = strchr(antipattern, '*')||strchr(antipattern, '?');

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	//obliterate any and all existing data.
	for (i = 0; i < strbuflist[bufno].used; i++)
		Z_Free(strbuflist[bufno].strings[i]);
	Z_Free(strbuflist[bufno].strings);
	strbuflist[bufno].strings = NULL;
	strbuflist[bufno].used = strbuflist[bufno].allocated = 0;

	//ignore name2, no point listing it twice.
	for (grp=cvar_groups ; grp ; grp=grp->next)
		for (var=grp->cvars ; var ; var=var->next)
		{
			if (plen && (pwc?!wildcmp(pattern, var->name):strncmp(var->name, pattern, plen)))
				continue;
			if (alen && (awc?wildcmp(antipattern, var->name):!strncmp(var->name, antipattern, alen)))
				continue;

			PF_bufstr_add_internal(bufno, var->name, true);
		}

	qsort(strbuflist[bufno].strings, strbuflist[bufno].used, sizeof(char*), PF_buf_sort_ascending);
}

enum matchmethod_e
{
	MATCH_AUTO=0,
	MATCH_EXACT=1,
	MATCH_LEFT=2,
	MATCH_RIGHT=3,
	MATCH_MIDDLE=4,
	MATCH_PATTERN=5,
};
static qboolean domatch(const char *str, const char *pattern, enum matchmethod_e method)
{
	switch(method)
	{
	case MATCH_EXACT:
		return !strcmp(str, pattern);
	case MATCH_LEFT:
		return !strncmp(str, pattern, strlen(pattern));
	case MATCH_RIGHT:
		{
			size_t slen = strlen(str);
			size_t plen = strlen(pattern);
			if (plen > slen)
				return false;
			return !strcmp(str + slen-plen, pattern);
		}
	case MATCH_MIDDLE:
		return !!strstr(str, pattern);
	case MATCH_AUTO:	//just treat as MATCH_PATTERN. we could optimise it a bit, but mneh
	case MATCH_PATTERN:
	default:
		return wildcmp(pattern, str);
	}
}
void QCBUILTIN PF_bufstr_find  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bufno = G_FLOAT(OFS_PARM0)-BUFSTRBASE;
	const char *pattern = PR_GetStringOfs(prinst, OFS_PARM1);
	enum matchmethod_e matchmethod = G_FLOAT(OFS_PARM2);
	int idx = (prinst->callargc > 3)?G_FLOAT(OFS_PARM3):0;
	int step = (prinst->callargc > 4)?G_FLOAT(OFS_PARM4):1;
	const char *s;

	G_FLOAT(OFS_RETURN) = -1;	//assume the worst

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (idx < 0 || step <= 0)
		return;
	for (; idx < strbuflist[bufno].used; idx += step)
	{
		s = strbuflist[bufno].strings[idx];
		if (!s) continue;
		if (domatch(s, pattern, matchmethod))
		{
			G_FLOAT(OFS_RETURN) = idx;
			break;
		}
	}
}

//directly reads a file into a stringbuffer
void QCBUILTIN PF_buf_loadfile  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *fname = PR_GetStringOfs(prinst, OFS_PARM0);
	size_t bufno = G_FLOAT(OFS_PARM1)-BUFSTRBASE;
	vfsfile_t *file;
	char line[8192];
	const char *fallback;

	G_FLOAT(OFS_RETURN) = 0;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (!QC_FixFileName(fname, &fname, &fallback))
	{
		Con_Printf("qcfopen: Access denied: %s\n", fname);
		return;
	}

	file = FS_OpenVFS(fname, "rb", FS_GAME);
	if (!file && fallback)
		file = FS_OpenVFS(fallback, "rb", FS_GAME);
	if (!file)
		return;

	while(VFS_GETS(file, line, sizeof(line)))
	{
		PF_bufstr_add_internal(bufno, line, true);
	}
	VFS_CLOSE(file);

	G_FLOAT(OFS_RETURN) = 1;
}

void QCBUILTIN PF_buf_writefile  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t fnum = G_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	size_t bufno = G_FLOAT(OFS_PARM1)-BUFSTRBASE;
	char **strings;
	int idx, midx;

	G_FLOAT(OFS_RETURN) = 0;

	if (bufno >= strbufmax)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (fnum >= MAX_QC_FILES)
		return;
	if (pf_fopen_files[fnum].prinst != prinst)
		return;

	if (prinst->callargc >= 3)
		idx = G_FLOAT(OFS_PARM2);
	else
		idx = 0;
	if (prinst->callargc >= 4)
		midx = idx + G_FLOAT(OFS_PARM3);
	else
		midx = strbuflist[bufno].used - idx;
	idx = bound(0, idx, (int)strbuflist[bufno].used);
	midx = min(midx, (int)strbuflist[bufno].used);
	for(strings = strbuflist[bufno].strings; idx < midx; idx++)
	{
		if (strings[idx])
		{
			PF_fwrite_internal (prinst, fnum, strings[idx], strlen(strings[idx]));
			PF_fwrite_internal (prinst, fnum, "\n", 1);
		}
	}
	G_FLOAT(OFS_RETURN) = 1;
}

//515's String functions
////////////////////////////////////////////////////

//float(float caseinsensitive, string s, ...) crc16 = #494 (DP_QC_CRC16)
void QCBUILTIN PF_crc16 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int insens = G_FLOAT(OFS_PARM0);
	const char *str = PF_VarString(prinst, 1, pr_globals);
	int len = strlen(str);

	if (insens)
		G_FLOAT(OFS_RETURN) = CalcHashInt(&hash_crc16_lower, str, len);
	else
		G_FLOAT(OFS_RETURN) = CalcHashInt(&hash_crc16, str, len);
}

static void QCBUILTIN PF_digest_internal (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals, const char *hashtype, const void *str, size_t len)
{
	int digestsize, i;
	unsigned char digest[64];
	unsigned char hexdig[sizeof(digest)*2+1];

	if (0)
		;
	else if (!strcmp(hashtype, "MD4"))
		digestsize = CalcHash(&hash_md4, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "MD5"))
		digestsize = CalcHash(&hash_md5, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "SHA1"))
		digestsize = CalcHash(&hash_sha1, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "SHA2-224") || !strcmp(hashtype, "SHA224"))
		digestsize = CalcHash(&hash_sha2_224, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "SHA2-256") || !strcmp(hashtype, "SHA256"))
		digestsize = CalcHash(&hash_sha2_256, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "SHA2-384") || !strcmp(hashtype, "SHA384"))
		digestsize = CalcHash(&hash_sha2_384, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "SHA2-512") || !strcmp(hashtype, "SHA512"))
		digestsize = CalcHash(&hash_sha2_512, digest, sizeof(digest), str, len);
	else if (!strcmp(hashtype, "CRC16"))
		digestsize = CalcHash(&hash_crc16, digest, sizeof(digest), str, len);
	else
		digestsize = 0;

	if (digestsize)
	{
		for (i = 0; i < digestsize; i++)
		{
			const char *hex = "0123456789abcdef";
			hexdig[i*2+0] = hex[digest[i]>>4];
			hexdig[i*2+1] = hex[digest[i]&0xf];
		}
		hexdig[i*2] = 0;
		RETURN_TSTRING(hexdig);
	}
	else
		G_INT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_digest_hex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *hashtype = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *str = PF_VarString(prinst, 1, pr_globals);
	PF_digest_internal(prinst, pr_globals, hashtype, str, strlen(str));
}

void QCBUILTIN PF_digest_ptr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *hashtype = PR_GetStringOfs(prinst, OFS_PARM0);
	int qcptr = G_INT(OFS_PARM1);
	int size = G_INT(OFS_PARM2);
	int offset = (prinst->callargc>3)?G_INT(OFS_PARM3):0;
	const void *input = PR_PointerToNative_MoInvalidate(prinst, qcptr, offset, size);	//make sure the input is a valid qc pointer.
	if (!input)
	{
		PR_BIError(prinst, "PF_digest_ptr: invalid pointer\n");
		G_INT(OFS_RETURN) = 0;
		return;
	}
	PF_digest_internal(prinst, pr_globals, hashtype, input, size);
}

void QCBUILTIN PF_base64encode (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t bytes = G_INT(OFS_PARM1);										//input blob size.
	const void *input = PR_GetReadQCPtr(prinst, G_INT(OFS_PARM0), bytes);	//make sure the input is a valid qc pointer.
	size_t chars = ((bytes+2)/3)*4+1;										//size required
	char *temp;
	G_INT(OFS_RETURN) = prinst->AllocTempString(prinst, (char**)&temp, chars);
	Base64_EncodeBlock(input, bytes, temp, chars);							//spew out the string.
}
void QCBUILTIN PF_base64decode (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const unsigned char *s = PR_GetStringOfs(prinst, OFS_PARM0);	//grab the input string
	size_t bytes = Base64_DecodeBlock(s, NULL, NULL, 0);			//figure out how long the output needs to be
	qbyte *ptr = prinst->AddressableAlloc(prinst, bytes);			//grab some qc memory
	bytes = Base64_DecodeBlock(s, NULL, ptr, bytes);				//decode it.
	ptr[bytes] = 0;													//make sure its null terminated or whatever.

	G_INT(OFS_RETURN) = (char*)ptr - prinst->stringtable;			//let the qc know where it is.
	G_INT(OFS_PARM1) = bytes;										//let the qc know how many bytes were actually read.
}

// #510 string(string in) uri_escape = #510;
void QCBUILTIN PF_uri_escape  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	static const char *hex = "0123456789ABCDEF";

	unsigned char result[8192];
	unsigned char *o = result;
	const unsigned char *s = PR_GetStringOfs(prinst, OFS_PARM0);
	*result = 0;
	while (*s && o < result+sizeof(result)-4)
	{
		//unreserved chars according to RFC3986
		if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9')
				|| *s == '.' || *s == '-' || *s == '_' || *s == '~')
			*o++ = *s++;
		else
		{
			*o++ = '%';
			*o++ = hex[*s>>4];
			*o++ = hex[*s&0xf];
			s++;
		}
	}
	*o = 0;
	RETURN_TSTRING(result);
}

// #511 string(string in) uri_unescape = #511;
void QCBUILTIN PF_uri_unescape  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned char *s = (unsigned char*)PR_GetStringOfs(prinst, OFS_PARM0);
	unsigned char resultbuf[8192];
	unsigned char *i, *o;
	unsigned char hex;
	i = s; o = resultbuf;
	while (*i && o < resultbuf+sizeof(resultbuf)-2)
	{
		if (*i == '%')
		{
			hex = 0;
			if (i[1] >= 'A' && i[1] <= 'F')
				hex += i[1]-'A'+10;
			else if (i[1] >= 'a' && i[1] <= 'f')
				hex += i[1]-'a'+10;
			else if (i[1] >= '0' && i[1] <= '9')
				hex += i[1]-'0';
			else
			{
				*o++ = *i++;
				continue;
			}
			hex <<= 4;
			if (i[2] >= 'A' && i[2] <= 'F')
				hex += i[2]-'A'+10;
			else if (i[2] >= 'a' && i[2] <= 'f')
				hex += i[2]-'a'+10;
			else if (i[2] >= '0' && i[2] <= '9')
				hex += i[2]-'0';
			else
			{
				*o++ = *i++;
				continue;
			}
			*o++ = hex;
			i += 3;
		}
		else
			*o++ = *i++;
	}
	*o = 0;
	RETURN_TSTRING(resultbuf);
}

#ifdef WEBCLIENT
typedef struct
{
	world_t *w;
	float id;
	int selfnum;
	int spawncount;

	int response;
	vfsfile_t *file;
} urigetcbctx_t;
static void PR_uri_get_callback2(int iarg, void *data)
{
	urigetcbctx_t *ctx = data;
	world_t *w = ctx->w;
	pubprogfuncs_t *prinst = w->progs;
	float id = ctx->id;
	int selfnum = ctx->selfnum;
	int replycode = ctx->response;
	func_t func;

	//make sure the progs vm is still active and okay.
	if (prinst && ctx->spawncount == w->spawncount)
	{
		func = PR_FindFunction(prinst, "URI_Get_Callback", PR_ANY);
		if (!func)
			Con_Printf("URI_Get_Callback missing\n");
		else if (func)
		{
			int len;
			char *buffer;
			struct globalvars_s *pr_globals = PR_globals(prinst, PR_CURRENT);
			int oldself = *w->g.self;
			*w->g.self = selfnum;
			G_FLOAT(OFS_PARM0) = id;
			G_FLOAT(OFS_PARM1) = (replycode!=200)?replycode:0;	//for compat with DP, we change any 200s to 0.
			G_INT(OFS_PARM2) = 0;
			G_INT(OFS_PARM3) = 0;

			if (ctx->file)
			{
				len = VFS_GETLEN(ctx->file);
				G_INT(OFS_PARM2) = prinst->AllocTempString(prinst, &buffer, len+1);
				len = VFS_READ(ctx->file, buffer, len);
				if (len < 0)
					len = 0;
				buffer[len] = 0;
				G_INT(OFS_PARM3) = len;
			}

			PR_ExecuteProgram(prinst, func);
			*w->g.self = oldself;
		}
	}
	if (ctx->file)
		VFS_CLOSE(ctx->file);
}
static void PR_uri_get_callback(struct dl_download *dl)
{	//we might be sitting on a setmodel etc call (or really anywhere). don't call qc while the qc is already busy.
	urigetcbctx_t ctx;

	ctx.w = dl->user_ctx;
	ctx.id = dl->user_float;
	ctx.selfnum = dl->user_num;
	ctx.spawncount = dl->user_sequence;
	ctx.response = dl->replycode;
	ctx.file = dl->file;
	dl->file = NULL;	//steal the file pointer, so the download code can't close it before we can read it.

	//now post it to the timer queue, it'll get processed within a frame, ish, without screwing up any other qc state.
	Cmd_AddTimer(0, PR_uri_get_callback2, 0, &ctx, sizeof(ctx));
}
#endif

// uri_get() gets content from an URL and calls a callback "uri_get_callback" with it set as string; an unique ID of the transfer is returned
// returns 1 on success, and then calls the callback with the ID, 0 or the HTTP status code, and the received data in a string
//float(string uril, float id) uri_get = #513;
void QCBUILTIN PF_uri_get  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifndef WEBCLIENT
	Con_Printf("uri_get is not implemented in this build\n");
	G_FLOAT(OFS_RETURN) = 0;
#else
	world_t *w = prinst->parms->user;
	struct dl_download *dl = NULL;

	const unsigned char *url = PR_GetStringOfs(prinst, OFS_PARM0);
	float id = G_FLOAT(OFS_PARM1);
	const char *mimetype = (prinst->callargc >= 3)?PR_GetStringOfs(prinst, OFS_PARM2):"";
	const char *dataorsep = (prinst->callargc >= 4)?PR_GetStringOfs(prinst, OFS_PARM3):"";
	int strbufid = (prinst->callargc >= 5)?G_FLOAT(OFS_PARM4):0;
	//float cryptokeyidx = (prinst->callargc >= 5)?G_FLOAT(OFS_PARM5):0;	//DP feature, not supported in FTE. adds a X-D0-Blind-ID-Detached-Signature header signing [postdata\0]querystring

	if (!pr_enable_uriget.ival)
	{
		Con_Printf("%s: blocking \"%s\"\n", pr_enable_uriget.name, url);
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	if (HTTP_CL_GetActiveDownloads() > 32)
	{
		//don't spam. I don't like it when you spam.
		Con_Printf("PF_uri_get(\"%s\",%g): too many pending downloads\n", url, id);
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	if (*mimetype)
	{
		Con_DPrintf("PF_uri_post(%s,%g)\n", url, id);
		if (strbufid)
		{
			size_t bufno = strbufid-BUFSTRBASE;
			if (bufno < strbufmax && strbuflist[bufno].prinst == prinst)
			{
				const char *glue = dataorsep;
				unsigned int gluelen = strlen(dataorsep);
				size_t datalen, l, i;
				char **strings;
				char *data;

				//count neededlength
				strings = strbuflist[bufno].strings;
				for (i = 0, datalen = 0; i < strbuflist[bufno].used; i++)
				{
					if (strings[i])
					{
						if (datalen)
							datalen += gluelen;
						datalen += strlen(strings[i]);
					}
				}

				//concat it, with dataorsep separating each element
				data = malloc(datalen+1);
				for (i = 0, datalen = 0; i < strbuflist[bufno].used; i++)
				{
					if (strings[i])
					{
						if (datalen)
						{
							memcpy(data+datalen, glue, gluelen);
							datalen += gluelen;
						}
						l = strlen(strings[i]);
						memcpy(data+datalen, strings[i], l);
						datalen += l;
					}
				}

				//add the null and send it
				data[datalen] = 0;
				dl = HTTP_CL_Put(url, mimetype, data, strlen(data), PR_uri_get_callback);
				free(data);
			}
		}
		else
		{
			//simple data post.
			dl = HTTP_CL_Put(url, mimetype, dataorsep, strlen(dataorsep), PR_uri_get_callback);
		}
	}
	else
	{
		Con_DPrintf("PF_uri_get(%s,%g)\n", url, id);
		dl = HTTP_CL_Get(url, NULL, PR_uri_get_callback);
	}
	if (dl)
	{
		dl->user_ctx = w;
		dl->user_float = id;
		dl->user_num = *w->g.self;
		dl->user_sequence = w->spawncount;
		dl->isquery = true;

#ifdef MULTITHREAD
		DL_CreateThread(dl, NULL, NULL);
#endif
		G_FLOAT(OFS_RETURN) = 1;
	}
	else
		G_FLOAT(OFS_RETURN) = 0;
#endif
}
void QCBUILTIN PF_netaddress_resolve(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *address = PR_GetStringOfs(prinst, OFS_PARM0);
	int defaultport = (prinst->callargc > 1)?G_FLOAT(OFS_PARM1):0;
	netadr_t adr;
	char result[256];
	if (NET_StringToAdr(address, defaultport, &adr))
		RETURN_TSTRING(NET_AdrToString (result, sizeof(result), &adr));
	else
		RETURN_TSTRING("");
}

////////////////////////////////////////////////////
//Console functions

static struct {
	char *token;
	unsigned int start;
	unsigned int end;
} *qctoken;
static unsigned int qctoken_count;
static unsigned int qctoken_max;

void tokenize_flush(void)
{
	while(qctoken_count > 0)
	{
		qctoken_count--;
		free(qctoken[qctoken_count].token);
	}

	qctoken_count = 0;
	free(qctoken);
	qctoken = NULL;
	qctoken_max = 0;
}

void QCBUILTIN PF_ArgC  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)				//85			//float() argc;
{
	G_FLOAT(OFS_RETURN) = qctoken_count;
}

int tokenizeqc(const char *str, qboolean dpfuckage)
{
	const char *start = str;
	while(qctoken_count > 0)
	{
		qctoken_count--;
		free(qctoken[qctoken_count].token);
	}
	for (qctoken_count = 0; ; )
	{
		if (qctoken_count == qctoken_max)
		{
			void *n = realloc(qctoken, sizeof(*qctoken)*(qctoken_max+8));
			if (!n)
				break;
			qctoken_max+=8;
			qctoken = n;
		}
		/*skip whitespace here so the token's start is accurate*/
		while (*str && *(unsigned char*)str <= ' ')
			str++;

		if (!*str)
			break;

		qctoken[qctoken_count].start = str - start;
		str = COM_StringParse (str, com_token, sizeof(com_token), false, dpfuckage);
		if (!str)
			break;

		qctoken[qctoken_count].token = strdup(com_token);

		qctoken[qctoken_count].end = str - start;
		qctoken_count++;
	}
	return qctoken_count;
}

/*KRIMZON_SV_PARSECLIENTCOMMAND added these two - note that for compatibility with DP, this tokenize builtin is veeery vauge and doesn't match the console*/
void QCBUILTIN PF_Tokenize  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)			//84			//void(string str) tokanize;
{
	G_FLOAT(OFS_RETURN) = tokenizeqc(PR_GetStringOfs(prinst, OFS_PARM0), true);
}

void QCBUILTIN PF_tokenize_console  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)			//84			//void(string str) tokanize;
{
	G_FLOAT(OFS_RETURN) = tokenizeqc(PR_GetStringOfs(prinst, OFS_PARM0), false);
}

void QCBUILTIN PF_tokenizebyseparator  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *str = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *sep[7];
	int seplen[7];
	int seps = 0, s;
	const char *start = str;
	int tlen;
	int found;

	while (seps < prinst->callargc - 1 && seps < 7)
	{
		sep[seps] = PR_GetStringOfs(prinst, OFS_PARM1 + seps*3);
		seplen[seps] = strlen(sep[seps]);
		seps++;
	}

	while(qctoken_count > 0)
	{
		qctoken_count--;
		free(qctoken[qctoken_count].token);
	}

	if (qctoken_count == qctoken_max)
	{
		void *n = realloc(qctoken, sizeof(*qctoken)*(qctoken_max+8));
		if (!n)
		{
			G_FLOAT(OFS_RETURN) = qctoken_count;
			return;
		}
		qctoken_max+=8;
		qctoken = n;
	}

	qctoken[qctoken_count].start = 0;
	if (*str)
	for(;;)
	{
		found = false;

		/*see if its a separator*/
		if (!*str)
		{
			qctoken[qctoken_count].end = str - start;
			found = -1;
		}
		else
		{
			for (s = 0; s < seps; s++)
			{
				if (!strncmp(str, sep[s], seplen[s]))
				{
					qctoken[qctoken_count].end = str - start;
					str += seplen[s];
					found = true;
					break;
				}
			}
		}
		/*it was, split it out*/
		if (found)
		{
			tlen = qctoken[qctoken_count].end - qctoken[qctoken_count].start;
			qctoken[qctoken_count].token = malloc(tlen + 1);
			memcpy(qctoken[qctoken_count].token, start + qctoken[qctoken_count].start, tlen);
			qctoken[qctoken_count].token[tlen] = 0;

			qctoken_count++;
			if (qctoken_count == qctoken_max)
			{
				void *n = realloc(qctoken, sizeof(*qctoken)*(qctoken_max+8));
				if (!n)
					break;
				qctoken_max+=8;
				qctoken = n;
			}

			if (found==-1)
				break;
			qctoken[qctoken_count].start = str - start;
		}
		else
			str++;
	}
	G_FLOAT(OFS_RETURN) = qctoken_count;
}

void QCBUILTIN PF_argv_start_index  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int idx = G_FLOAT(OFS_PARM0);

	/*negative indexes are relative to the end*/
	if (idx < 0)
		idx += qctoken_count;	

	if ((unsigned int)idx >= qctoken_count)
		G_FLOAT(OFS_RETURN) = -1;
	else
		G_FLOAT(OFS_RETURN) = qctoken[idx].start;
}

void QCBUILTIN PF_argv_end_index  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int idx = G_FLOAT(OFS_PARM0);

	/*negative indexes are relative to the end*/
	if (idx < 0)
		idx += qctoken_count;	

	if ((unsigned int)idx >= qctoken_count)
		G_FLOAT(OFS_RETURN) = -1;
	else
		G_FLOAT(OFS_RETURN) = qctoken[idx].end;
}

void QCBUILTIN PF_ArgV  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)				//86			//string(float num) argv;
{
	int idx = G_FLOAT(OFS_PARM0);

	/*negative indexes are relative to the end*/
	if (idx < 0)
		idx += qctoken_count;

	if ((unsigned int)idx >= qctoken_count)
		G_INT(OFS_RETURN) = 0;
	else
		RETURN_TSTRING(qctoken[idx].token);
}

void QCBUILTIN PF_argescape(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[8192];
	const char *str = PR_GetStringOfs(prinst, OFS_PARM0);
	RETURN_TSTRING(COM_QuotedString(str, temp, sizeof(temp), false));
}

//Console functions
////////////////////////////////////////////////////
//Maths functions

void QCBUILTIN PF_random (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//don't return 1 (it would break array[random()*array.length];
	//don't return 0 either, it would break the self.nextthink = time+random()*foo; lines in walkmonster_start, resulting rarely in statue-monsters.
	G_FLOAT(OFS_RETURN) = (rand ()&0x7fff) / ((float)0x08000) + (0.5/0x08000);

	if (prinst->callargc)
	{
		if (prinst->callargc == 1)
			G_FLOAT(OFS_RETURN) *= G_FLOAT(OFS_PARM0);
		else
			G_FLOAT(OFS_RETURN) = G_FLOAT(OFS_PARM0) + G_FLOAT(OFS_RETURN)*(G_FLOAT(OFS_PARM1)-G_FLOAT(OFS_PARM0));
	}
}

//float(float number, float quantity) bitshift = #218;
void QCBUILTIN PF_bitshift(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bitmask;
	int shift;

	bitmask = G_FLOAT(OFS_PARM0);
	shift = G_FLOAT(OFS_PARM1);

	if (shift < 0)
		bitmask >>= -shift;
	else
		bitmask <<= shift;

	G_FLOAT(OFS_RETURN) = bitmask;
}

//float(float a, floats) min = #94
void QCBUILTIN PF_min (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	float f;

	if (prinst->callargc == 2)
	{
		G_FLOAT(OFS_RETURN) = min(G_FLOAT(OFS_PARM0), G_FLOAT(OFS_PARM1));
	}
	else if (prinst->callargc >= 3)
	{
		f = G_FLOAT(OFS_PARM0);
		for (i = 1; i < prinst->callargc; i++)
		{
			if (G_FLOAT((OFS_PARM0 + i * 3)) < f)
				f = G_FLOAT((OFS_PARM0 + i * 3));
		}
		G_FLOAT(OFS_RETURN) = f;
	}
	else
		PR_BIError(prinst, "PF_min: must supply at least 2 floats\n");
}

//float(float a, floats) max = #95
void QCBUILTIN PF_max (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	float f;

	if (prinst->callargc == 2)
	{
		G_FLOAT(OFS_RETURN) = max(G_FLOAT(OFS_PARM0), G_FLOAT(OFS_PARM1));
	}
	else if (prinst->callargc >= 3)
	{
		f = G_FLOAT(OFS_PARM0);
		for (i = 1; i < prinst->callargc; i++) {
			if (G_FLOAT((OFS_PARM0 + i * 3)) > f)
				f = G_FLOAT((OFS_PARM0 + i * 3));
		}
		G_FLOAT(OFS_RETURN) = f;
	}
	else
	{
		PR_BIError(prinst, "PF_min: must supply at least 2 floats\n");
	}
}

//float(float minimum, float val, float maximum) bound = #96
void QCBUILTIN PF_bound (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (G_FLOAT(OFS_PARM1) > G_FLOAT(OFS_PARM2))
		G_FLOAT(OFS_RETURN) = G_FLOAT(OFS_PARM2);
	else if (G_FLOAT(OFS_PARM1) < G_FLOAT(OFS_PARM0))
		G_FLOAT(OFS_RETURN) = G_FLOAT(OFS_PARM0);
	else
		G_FLOAT(OFS_RETURN) = G_FLOAT(OFS_PARM1);
}

void QCBUILTIN PF_mod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float a = G_FLOAT(OFS_PARM0);
	float n = G_FLOAT(OFS_PARM1);

	if (n == 0)
	{
		PR_RunWarning(prinst, "mod by zero\n");
		G_FLOAT(OFS_RETURN) = 0;
	}
	else
	{
		//because QC is inherantly floaty, lets use floats.
		G_FLOAT(OFS_RETURN) = a - (n * (int)(a/n));
//		G_FLOAT(OFS_RETURN) = a % n;
	}
}

void QCBUILTIN PF_Sin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = sin(G_FLOAT(OFS_PARM0));
}
void QCBUILTIN PF_Cos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = cos(G_FLOAT(OFS_PARM0));
}
void QCBUILTIN PF_Sqrt (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}
void QCBUILTIN PF_pow (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = pow(G_FLOAT(OFS_PARM0), G_FLOAT(OFS_PARM1));
}
void QCBUILTIN PF_asin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = asin(G_FLOAT(OFS_PARM0));
}
void QCBUILTIN PF_acos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = acos(G_FLOAT(OFS_PARM0));
}
void QCBUILTIN PF_atan (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = atan(G_FLOAT(OFS_PARM0));
}
void QCBUILTIN PF_atan2 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = atan2(G_FLOAT(OFS_PARM0), G_FLOAT(OFS_PARM1));
}
void QCBUILTIN PF_tan (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = tan(G_FLOAT(OFS_PARM0));
}

void QCBUILTIN PF_fabs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = fabs(v);
}

void QCBUILTIN PF_rint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	f;
	f = G_FLOAT(OFS_PARM0);
	if (f > 0)
		G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	else
		G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}

void QCBUILTIN PF_floor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}

void QCBUILTIN PF_ceil (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}

void QCBUILTIN PF_anglemod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float v = G_FLOAT(OFS_PARM0);

	while (v >= 360)
		v = v - 360;
	while (v < 0)
		v = v + 360;

	G_FLOAT(OFS_RETURN) = v;
}
void QCBUILTIN PF_anglesub (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float v = G_FLOAT(OFS_PARM0) - G_FLOAT(OFS_PARM1);

	while (v > 180)
		v = v - 360;
	while (v < -180)
		v = v + 360;

	G_FLOAT(OFS_RETURN) = v;
}

//void(vector dir) vectorvectors
//Writes new values for v_forward, v_up, and v_right based on the given forward vector
void QCBUILTIN PF_vectorvectors (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;

	VectorCopy(G_VECTOR(OFS_PARM0), world->g.v_forward);
	VectorNormalize(world->g.v_forward);
	VectorVectors(world->g.v_forward, world->g.v_right, world->g.v_up);
}

void QCBUILTIN PF_crossproduct (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	VM_VECTORARG(v0, OFS_PARM0);
	VM_VECTORARG(v1, OFS_PARM1);
	CrossProduct(v0, v1, G_VECTOR(OFS_RETURN));
}

//Maths functions
////////////////////////////////////////////////////

#ifdef HAVE_LEGACY
unsigned int FTEToDPContents(unsigned int contents)
{
	unsigned int r = 0;
	if (contents & FTECONTENTS_SOLID)
		r |= DPCONTENTS_SOLID;
	if (contents & FTECONTENTS_WATER)
		r |= DPCONTENTS_WATER;
	if (contents & FTECONTENTS_SLIME)
		r |= DPCONTENTS_SLIME;
	if (contents & FTECONTENTS_LAVA)
		r |= DPCONTENTS_LAVA;
	if (contents & FTECONTENTS_SKY)
		r |= DPCONTENTS_SKY;
	if (contents & FTECONTENTS_BODY)
		r |= DPCONTENTS_BODY;
	if (contents & FTECONTENTS_CORPSE)
		r |= DPCONTENTS_CORPSE;
	if (contents & Q3CONTENTS_NODROP)
		r |= DPCONTENTS_NODROP;
	if (contents & FTECONTENTS_PLAYERCLIP)
		r |= DPCONTENTS_PLAYERCLIP;
	if (contents & FTECONTENTS_MONSTERCLIP)
		r |= DPCONTENTS_MONSTERCLIP;
	if (contents & Q3CONTENTS_DONOTENTER)
		r |= DPCONTENTS_DONOTENTER;
	if (contents & Q3CONTENTS_BOTCLIP)
		r |= DPCONTENTS_BOTCLIP;
//	if (contents & FTECONTENTS_OPAQUE)
//		r |= DPCONTENTS_OPAQUE;
	return r;
}
#endif

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void QCBUILTIN PF_droptofloor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	extern cvar_t pr_droptofloorunits;
	world_t *world = prinst->parms->user;
	wedict_t	*ent;
	vec3_t		end;
	vec3_t		start;
	trace_t		trace;
	const pvec_t *gravitydir;

	ent = PROG_TO_WEDICT(prinst, *world->g.self);

	if (ent->xv->gravitydir[2] || ent->xv->gravitydir[1] || ent->xv->gravitydir[0])
		gravitydir = ent->xv->gravitydir;
	else
		gravitydir = world->g.defaultgravitydir;

	VectorCopy (ent->v->origin, end);
	if (pr_droptofloorunits.value > 0)
		VectorMA(end, pr_droptofloorunits.value, gravitydir, end);
	else
		VectorMA(end, 256, gravitydir, end);

	VectorCopy (ent->v->origin, start);
	trace = World_Move (world, start, ent->v->mins, ent->v->maxs, end, MOVE_NORMAL, ent);

	if (trace.allsolid && sv_gameplayfix_droptofloorstartsolid.ival && gravitydir[2] == -1)
	{
		//try again but with a traceline, something is better than nothing.
		vec3_t offset;
		VectorAvg(ent->v->maxs, ent->v->mins, offset);
		offset[2] = ent->v->mins[2];
		VectorAdd(start, offset, start);
		VectorAdd(end, offset, end);
		trace = World_Move (world, start, vec3_origin, vec3_origin, end, MOVE_NORMAL, ent);
		if (trace.fraction < 1)
		{
			VectorSubtract (trace.endpos, offset, ent->v->origin);
			World_LinkEdict (world, ent, false);
			ent->v->flags = (int)ent->v->flags | FL_ONGROUND;
			ent->v->groundentity = EDICT_TO_PROG(prinst, trace.ent);
			G_FLOAT(OFS_RETURN) = 1;
		}
	}
	else if (trace.fraction == 1 || trace.allsolid)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		VectorCopy (trace.endpos, ent->v->origin);
		World_LinkEdict (world, ent, false);
		ent->v->flags = (int)ent->v->flags | FL_ONGROUND;
		ent->v->groundentity = EDICT_TO_PROG(prinst, trace.ent);
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
=============
PF_checkbottom
=============
*/
void QCBUILTIN PF_checkbottom (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;
	wedict_t	*ent;
	vec3_t		gravup;

	ent = G_WEDICT(prinst, OFS_PARM0);

	if (ent->xv->gravitydir[0] || ent->xv->gravitydir[1] || ent->xv->gravitydir[2])
		VectorNegate(ent->xv->gravitydir, gravup);
	else
		VectorSet(gravup, 0, 0, 1);

	G_FLOAT(OFS_RETURN) = World_CheckBottom (world, ent, gravup);
}

////////////////////////////////////////////////////
//Vector functions

//vector() randomvec = #91
void QCBUILTIN PF_randomvector (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	vec3_t temp;
	do
	{
		temp[0] = (rand() & 32767) * (2.0 / 32767.0) - 1.0;
		temp[1] = (rand() & 32767) * (2.0 / 32767.0) - 1.0;
		temp[2] = (rand() & 32767) * (2.0 / 32767.0) - 1.0;
	} while (DotProduct(temp, temp) >= 1);
	VectorCopy (temp, G_VECTOR(OFS_RETURN));
}


/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C

FIXME: add gravitydir support
==============
*/
float World_changeyaw (wedict_t *ent);
void QCBUILTIN PF_changeyaw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t		*ent;
//	float		ideal, current, move, speed;

	ent = PROG_TO_WEDICT(prinst, *w->g.self);

	World_changeyaw(ent);
}

//void() changepitch = #63;
//FIXME: support gravitydir
void QCBUILTIN PF_changepitch (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t		*ent;
	float		ideal, current, move, speed;

	ent = PROG_TO_WEDICT(prinst, *w->g.self);
	current = anglemod( ent->v->angles[0] );
	ideal = ent->xv->idealpitch;
	speed = ent->xv->pitch_speed;

	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->v->angles[0] = anglemod (current + move);
}

//float vectoyaw(vector, optional entity reference)
void QCBUILTIN PF_vectoyaw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1;
	float	x, y;
	float	yaw;

	value1 = G_VECTOR(OFS_PARM0);

	if (prinst->callargc >= 2)
	{
		vec3_t axis[3];
		World_GetEntGravityAxis(G_WEDICT(prinst, OFS_PARM1), axis);
		x = DotProduct(value1, axis[0]);
		y = DotProduct(value1, axis[1]);
	}
	else
	{
		x = value1[0];
		y = value1[1];
	}

	if (y == 0 && x == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(y, x) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	G_FLOAT(OFS_RETURN) = yaw;
}

//float(vector) vlen
void QCBUILTIN PF_vlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1 = G_VECTOR(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = sqrt(DotProduct(value1, value1));
}
//float(vector) vhlen
void QCBUILTIN PF_vhlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1 = G_VECTOR(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = sqrt(DotProduct2(value1, value1));
}

//vector vectoangles(vector)
void QCBUILTIN PF_vectoangles (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1, *up;

	value1 = G_VECTOR(OFS_PARM0);
	if (prinst->callargc >= 2)
		up = G_VECTOR(OFS_PARM1);
	else
		up = NULL;

	VectorAngles(value1, up, G_VECTOR(OFS_RETURN), true);
}

//vector normalize(vector)
void QCBUILTIN PF_normalize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1;
	vec3_t	newvalue;
	float	newf;

	value1 = G_VECTOR(OFS_PARM0);

	newf = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	newf = sqrt(newf);

	if (newf == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		newf = 1/newf;
		newvalue[0] = value1[0] * newf;
		newvalue[1] = value1[1] * newf;
		newvalue[2] = value1[2] * newf;
	}

	VectorCopy (newvalue, G_VECTOR(OFS_RETURN));
}

void QCBUILTIN PF_rotatevectorsbyangles (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;

	float *ang = G_VECTOR(OFS_PARM0);
	vec3_t src[3], trans[3], res[3];
	AngleVectorsMesh(ang, trans[0], trans[1], trans[2]);
	VectorInverse(trans[1]);

	VectorCopy(w->g.v_forward, src[0]);
	VectorNegate(w->g.v_right, src[1]);
	VectorCopy(w->g.v_up, src[2]);

	R_ConcatRotations(trans, src, res);

	VectorCopy(res[0], w->g.v_forward);
	VectorNegate(res[1], w->g.v_right);
	VectorCopy(res[2], w->g.v_up);
}

void QCBUILTIN PF_rotatevectorsbymatrix (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	vec3_t base[3], trans[3], res[3];

	VectorCopy(G_VECTOR(OFS_PARM0), base[0]);
	VectorNegate(G_VECTOR(OFS_PARM1), base[1]);
	VectorCopy(G_VECTOR(OFS_PARM2), base[2]);

	VectorCopy(w->g.v_forward, trans[0]);
	VectorNegate(w->g.v_right, trans[1]);
	VectorCopy(w->g.v_up, trans[2]);

	R_ConcatRotations(trans, base, res);

	VectorCopy(res[0], w->g.v_forward);
	VectorNegate(res[1], w->g.v_right);
	VectorCopy(res[2], w->g.v_up);
}

//Vector functions
////////////////////////////////////////////////////
//Progs internals

qcstate_t *PR_CreateThread(pubprogfuncs_t *prinst, float retval, float resumetime, qboolean wait)
{
	world_t *world = prinst->parms->user;
	qcstate_t *state;
	edict_t *ed;

	state = prinst->parms->memalloc(sizeof(qcstate_t));
	state->next = world->qcthreads;
	world->qcthreads = state;
	state->resumetime = resumetime;

	if (prinst->edicttable_length)
	{
		ed = PROG_TO_EDICT(prinst, world->g.self?*world->g.self:0);
		state->self = NUM_FOR_EDICT(prinst, ed);
		state->selfid = (prinst==svprogfuncs&&ed->ereftype==ER_ENTITY)?ed->xv->uniquespawnid:0;
		ed = PROG_TO_EDICT(prinst, world->g.self?*world->g.self:0);
		state->other = NUM_FOR_EDICT(prinst, ed);
		state->otherid = (prinst==svprogfuncs&&ed->ereftype==ER_ENTITY)?ed->xv->uniquespawnid:0;
	}
	else	//allows us to call this during init().
		state->self = state->other = state->selfid = state->otherid = 0;
	state->thread = prinst->Fork(prinst);
	state->waiting = wait;
	state->returnval = retval;
	return state;
}
void PR_RunThreads(world_t *world)
{
	struct globalvars_s *pr_globals;
	edict_t *ed;

	qcstate_t *state = world->qcthreads, *next;
	world->qcthreads = NULL;
	while(state)
	{
		pubprogfuncs_t *prinst = world->progs;
		next = state->next;

		if (state->resumetime > (world->g.time?*world->g.time:0) || state->waiting)
		{	//not time yet, reform original list.
			state->next = world->qcthreads;
			world->qcthreads = state;
		}
		else
		{	//call it and forget it ever happened. The Sleep biltin will recreate if needed.
			pr_globals = PR_globals(prinst, PR_CURRENT);

			if (world->g.self)
			{
				//restore the thread's self variable, if applicable.
				ed = PROG_TO_EDICT(prinst, state->self);
				if ((prinst==svprogfuncs?ed->xv->uniquespawnid:0) != state->selfid)
					ed = prinst->edicttable[0];
				*world->g.self = EDICT_TO_PROG(prinst, ed);
			}

			if (world->g.other)
			{
				//restore the thread's other variable, if applicable
				ed = PROG_TO_EDICT(prinst, state->other);
				if ((prinst==svprogfuncs?ed->xv->uniquespawnid:0) != state->otherid)
					ed = prinst->edicttable[0];
				*world->g.other = EDICT_TO_PROG(prinst, ed);
			}

			G_FLOAT(OFS_RETURN) = state->returnval;	//return value of fork or sleep

			prinst->RunThread(prinst, state->thread);
			prinst->parms->memfree(state->thread);
			prinst->parms->memfree(state);
		}

		state = next;
	}
}
void PR_ClearThreads(pubprogfuncs_t *prinst)
{
	world_t *world;
	qcstate_t *state, *next;
	if (!prinst)
		return; //shoo!
	world = prinst->parms->user;
	state = world->qcthreads;
	world->qcthreads = NULL;
	while(state)
	{
		next = state->next;

		//free the memory.
		prinst->parms->memfree(state->thread);
		prinst->parms->memfree(state);

		state = next;
	}
}
void QCBUILTIN PF_Abort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	prinst->AbortStack(prinst);
}
void QCBUILTIN PF_Sleep(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;
	float sleeptime;

	sleeptime = G_FLOAT(OFS_PARM0);

	PR_CreateThread(prinst, 1, (world->g.time?*world->g.time:0) + sleeptime, false);

	prinst->AbortStack(prinst);
}
void QCBUILTIN PF_Fork(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;
	float sleeptime;

	if (svprogfuncs->callargc >= 1)
		sleeptime = G_FLOAT(OFS_PARM0);
	else
		sleeptime = 0;

	PR_CreateThread(prinst, 1, (world->g.time?*world->g.time:0) + sleeptime, false);

//	PRSV_RunThreads();

	G_FLOAT(OFS_RETURN) = 0;
}

//this func calls a function in another progs
//it works in the same way as the above func, except that it calls by reference to a function, as opposed to by it's name
//used for entity function variables - not actually needed anymore
void QCBUILTIN PF_externrefcall (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
//	int progsnum;
	func_t f;
	int i;
//	progsnum = G_PROG(OFS_PARM0);
	f = G_INT(OFS_PARM1);

	for (i = OFS_PARM0; i < OFS_PARM5; i+=3)
		VectorCopy(G_VECTOR(i+(2*3)), G_VECTOR(i));

	PR_ExecuteProgram(prinst, f);
}

void QCBUILTIN PF_externset (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)	//set a value in another progs
{
	int n = G_PROG(OFS_PARM0);
	eval_t *v = (eval_t*)&G_INT(OFS_PARM1);
	const char *varname = PF_VarString(prinst, 2, pr_globals);
	eval_t *var;
	etype_t t = ev_void;

	var = PR_FindGlobal(prinst, varname, n, &t);

	if (var)
	{
		if (t == ev_vector)
			VectorCopy(v->_vector, var->_vector);
		else if (t == ev_int64 || t == ev_uint64 || t == ev_double)
			var->i64 = v->i64;
		else
			var->_int = v->_int;
	}
}

void QCBUILTIN PF_externvalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)	//return a value in another progs
{
	int n = G_PROG(OFS_PARM0);
	const char *varname = PF_VarString(prinst, 1, pr_globals);
	eval_t *var;

	if (*varname == '&')
	{
		//return its address instead of its value, for pointer use.
		var = prinst->FindGlobal(prinst, varname+1, n, NULL);
		if (var)
			G_INT(OFS_RETURN) = (char*)var - prinst->stringtable;
		else
			G_INT(OFS_RETURN) = 0;
		G_INT(OFS_RETURN+1) = 0;
		G_INT(OFS_RETURN+2) = 0;
	}
	else
	{
		var = prinst->FindGlobal(prinst, varname, n, NULL);

		if (var)
		{
			G_INT(OFS_RETURN+0) = ((int*)&var->_int)[0];
			G_INT(OFS_RETURN+1) = ((int*)&var->_int)[1];
			G_INT(OFS_RETURN+2) = ((int*)&var->_int)[2];
		}
		else
		{
			n = prinst->FindFunction(prinst, varname, n);
			G_INT(OFS_RETURN) = n;
			G_INT(OFS_RETURN+1) = 0;
			G_INT(OFS_RETURN+2) = 0;
		}
	}
}

void QCBUILTIN PF_externcall (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)	//this func calls a function in another progs (by name)
{
	int progsnum;
	const char *funcname;
	int i;
	string_t failedst = G_INT(OFS_PARM1);
	func_t f;

	progsnum = G_PROG(OFS_PARM0);
	funcname = PR_GetStringOfs(prinst, OFS_PARM1);

	f = PR_FindFunction(prinst, funcname, progsnum);
	if (f)
	{
		for (i = OFS_PARM0; i < OFS_PARM5; i+=3)
			VectorCopy(G_VECTOR(i+(2*3)), G_VECTOR(i));

		PR_ExecuteProgram(prinst, f);
	}
	else
	{
		f = PR_FindFunction(prinst, "MissingFunc", progsnum);
		if (!f)
		{
			PR_BIError(prinst, "Couldn't find function %s", funcname);
			return;
		}

		for (i = OFS_PARM0; i < OFS_PARM6; i+=3)
			VectorCopy(G_VECTOR(i+(1*3)), G_VECTOR(i));
		G_INT(OFS_PARM0) = failedst;

		PR_ExecuteProgram(prinst, f);
	}
}

void QCBUILTIN PF_setwatchpoint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *desc = PR_GetStringOfs(prinst, OFS_PARM0);
	int type = G_FLOAT(OFS_PARM1);
	int qcptr = G_INT(OFS_PARM2);
	char variable[64];

	if (type == ev_float)
		Q_snprintfz(variable,sizeof(variable), "*(float*)%#x", qcptr);
	else if (type == ev_string)
		Q_snprintfz(variable,sizeof(variable), "*(string*)%#x", qcptr);
	else
		Q_snprintfz(variable,sizeof(variable), "*(int*)%#x", qcptr);

	if (prinst->SetWatchPoint(prinst, desc, variable))
		Con_DPrintf("Watchpoint set\n");
	else
		Con_Printf("Watchpoint failure\n");
}

void QCBUILTIN PF_traceon (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	prinst->debug_trace = DEBUG_TRACE_INTO;
}

void QCBUILTIN PF_traceoff (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	prinst->debug_trace = DEBUG_TRACE_OFF;
}
void QCBUILTIN PF_coredump (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t size = 1024*1024*8;
	char *buffer = BZ_Malloc(size);
	prinst->save_ents(prinst, buffer, &size, size, 3);
	COM_WriteFile("core.txt", FS_GAMEONLY, buffer, size);
	BZ_Free(buffer);
}
void QCBUILTIN PF_eprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t max = 1024*1024;
	size_t size = 0;
	char *buffer = BZ_Malloc(max);
	char *buf;
	buf = prinst->saveent(prinst, buffer, &size, max, (struct edict_s*)G_WEDICT(prinst, OFS_PARM0));
	Con_Printf("Entity %i:\n%s\n", G_EDICTNUM(prinst, OFS_PARM0), buf);
	BZ_Free(buffer);
}

void QCBUILTIN PF_break (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	PR_RunWarning (prinst, "break statement");
}

void QCBUILTIN PF_error (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*s;

	s = PF_VarString(prinst, 0, pr_globals);
/*	Con_Printf ("======SERVER ERROR in %s:\n%s\n", PR_GetString(pr_xfunction->s_name) ,s);
	ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print (ed);
*/

	PR_StackTrace(prinst, false);

	Con_Printf("%s\n", s);

	if (developer.value)
	{
//		SV_Error ("Program error: %s", s);
		PF_break(prinst, pr_globals);
		prinst->debug_trace = DEBUG_TRACE_INTO;
	}
	else
	{
		PR_AbortStack(prinst);
		PR_BIError (prinst, "Program error: %s", s);
	}
}

//Progs internals
////////////////////////////////////////////////////
//System

//Sends text over to the client's execution buffer
void QCBUILTIN PF_localcmd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str;

	str = PF_VarString(prinst, 0, pr_globals);
	if (developer.ival >= 2)
	{
		PR_StackTrace(prinst, false);
		Con_Printf("localcmd: %s\n", str);
	}
	if (!strcmp(str, "host_framerate 0\n"))
		Cbuf_AddText ("sv_mintic 0\n", RESTRICT_INSECURE);	//hmm... do this better...
	else
		Cbuf_AddText (str, RESTRICT_INSECURE);
}

void QCBUILTIN PF_gettimed (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//usng doubles is for longer uptimes rather than extra precision. we artificially limit precision to reduce spectre sidebands, though I'm not sure how useful it'd be.
	int timer = (prinst->callargc > 0)?G_INT(OFS_PARM0):0;
	switch(timer)
	{
	default:
	case 0:		//cached time at start of frame
		G_DOUBLE(OFS_RETURN) = realtime;
		break;
	case 1:		//actual time, ish. we round to milliseconds to reduce spectre exposure
		G_DOUBLE(OFS_RETURN) = (qint64_t)Sys_Milliseconds()/1000.0;
		break;
	//case 2:	//highres.. looks like time into the frame
	//case 3:	//uptime
	//case 4:	//cd track
#ifndef SERVERONLY
	case 5:		//sim time
		G_DOUBLE(OFS_RETURN) = cl.time;
		break;
#endif
	}
}
void QCBUILTIN PF_gettimef (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT(OFS_PARM0) = G_FLOAT(OFS_PARM0);
	PF_gettimed (prinst, pr_globals);
	G_FLOAT(OFS_RETURN) = G_DOUBLE(OFS_RETURN);
}

void QCBUILTIN PF_calltimeofday (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	date_t date;
	func_t f;

	f = PR_FindFunction(prinst, "timeofday", PR_ANY);
	if (f)
	{
		COM_TimeOfDay(&date);

		G_FLOAT(OFS_PARM0) = (float)date.sec;
		G_FLOAT(OFS_PARM1) = (float)date.min;
		G_FLOAT(OFS_PARM2) = (float)date.hour;
		G_FLOAT(OFS_PARM3) = (float)date.day;
		G_FLOAT(OFS_PARM4) = (float)date.mon;
		G_FLOAT(OFS_PARM5) = (float)date.year;
		G_INT(OFS_PARM6) = (int)PR_TempString(prinst, date.str);

		PR_ExecuteProgram(prinst, f);
	}
}

void QCBUILTIN PF_sprintf_internal (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals, const char *s, int firstarg, char *outbuf, int outbuflen)
{
	const char *s0;
	char *o = outbuf, *end = outbuf + outbuflen, *err;
	int width, precision, thisarg, flags;
	char formatbuf[16];
	char *f;
	int argpos = firstarg;
	int isfloat, is64bit;
	static int dummyivec[3] = {0, 0, 0};
	static float dummyvec[3] = {0, 0, 0};

#define PRINTF_ALTERNATE 1
#define PRINTF_ZEROPAD 2
#define PRINTF_LEFT 4
#define PRINTF_SPACEPOSITIVE 8
#define PRINTF_SIGNPOSITIVE 16

	formatbuf[0] = '%';

#define GETARG_FLOAT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_FLOAT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_DOUBLE(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_DOUBLE(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_VECTOR(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_VECTOR(OFS_PARM0 + 3 * (a))) : dummyvec)
#define GETARG_INT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_INT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_INT64(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_INT64(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_UINT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_UINT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_UINT64(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_UINT64(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_INTVECTOR(a) (((a)>=firstarg && (a)<prinst->callargc) ? ((int*) G_VECTOR(OFS_PARM0 + 3 * (a))) : dummyivec)
#define GETARG_STRING(a) (((a)>=firstarg && (a)<prinst->callargc) ? (PR_GetStringOfs(prinst, OFS_PARM0 + 3 * (a))) : "")

#define GETARG_SNUMERIC(t, a) (is64bit?(isfloat ? (t) GETARG_DOUBLE(a) : (t) GETARG_INT64 (a)):(isfloat ? (t) GETARG_FLOAT(a) : (t) GETARG_INT (a)))
#define GETARG_UNUMERIC(t, a) (is64bit?(isfloat ? (t) GETARG_DOUBLE(a) : (t) GETARG_UINT64(a)):(isfloat ? (t) GETARG_FLOAT(a) : (t) GETARG_UINT(a)))

	for(;;)
	{
		s0 = s;
		switch(*s)
		{
			case 0:
				goto finished;
			case '%':
				++s;

				if(*s == '%')
					goto verbatim;

				// complete directive format:
				// %3$*1$.*2$ld
				
				width = -1;
				precision = -1;
				thisarg = -1;
				flags = 0;
				isfloat = -1;
				is64bit = 0;

				// is number following?
				if(*s >= '0' && *s <= '9')
				{
					width = strtol(s, &err, 10);
					if(!err)
					{
						Con_Printf("PF_sprintf: bad format string: %s\n", s0);
						goto finished;
					}
					if(*err == '$')
					{
						thisarg = width + (firstarg-1);
						width = -1;
						s = err + 1;
					}
					else
					{
						if(*s == '0')
						{
							flags |= PRINTF_ZEROPAD;
							if(width == 0)
								width = -1; // it was just a flag
						}
						s = err;
					}
				}

				if(width < 0)
				{
					for(;;)
					{
						switch(*s)
						{
							case '#': flags |= PRINTF_ALTERNATE; break;
							case '0': flags |= PRINTF_ZEROPAD; break;
							case '-': flags |= PRINTF_LEFT; break;
							case ' ': flags |= PRINTF_SPACEPOSITIVE; break;
							case '+': flags |= PRINTF_SIGNPOSITIVE; break;
							default:
								goto noflags;
						}
						++s;
					}
noflags:
					if(*s == '*')
					{
						++s;
						if(*s >= '0' && *s <= '9')
						{
							width = strtol(s, &err, 10);
							if(!err || *err != '$')
							{
								Con_Printf("PF_sprintf: invalid format string: %s\n", s0);
								goto finished;
							}
							s = err + 1;
						}
						else
							width = argpos++;
						width = GETARG_FLOAT(width);
						if(width < 0)
						{
							flags |= PRINTF_LEFT;
							width = -width;
						}
					}
					else if(*s >= '0' && *s <= '9')
					{
						width = strtol(s, &err, 10);
						if(!err)
						{
							Con_Printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
						}
						s = err;
						if(width < 0)
						{
							flags |= PRINTF_LEFT;
							width = -width;
						}
					}
					// otherwise width stays -1
				}

				if(*s == '.')
				{
					++s;
					if(*s == '*')
					{
						++s;
						if(*s >= '0' && *s <= '9')
						{
							precision = strtol(s, &err, 10);
							if(!err || *err != '$')
							{
								Con_Printf("PF_sprintf: invalid format string: %s\n", s0);
								goto finished;
							}
							s = err + 1;
						}
						else
							precision = argpos++;
						precision = GETARG_FLOAT(precision);
					}
					else if(*s >= '0' && *s <= '9')
					{
						precision = strtol(s, &err, 10);
						if(!err)
						{
							Con_Printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
						}
						s = err;
					}
					else
					{
						Con_Printf("PF_sprintf: invalid format string: %s\n", s0);
						goto finished;
					}
				}

				for(;;)
				{
					switch(*s)
					{
						/*	note: this is technically a dp extension, so for our inputs:
							dp defined %lx for entity numbers (it not having actual ints)
							it later defined %llx for Actual ints, not that it makes a difference in DP.
							This leaves us unable to use C's 'l' for our int64 type (aka long), nor 'll' either.
							So lean on BSD's 'q'.

							for our outputs, %llx is standard for int64 with MS violating c99 and requiring %I64x instead.
						*/
						case 'h': isfloat = 1; break;	//short in C, interpreted as float here. doubled for char.
						case 'l': isfloat = 0; break;	//long, twice for long long... we interpret as int32.
						case 'L': isfloat = 0; break;	//'long double'
						case 'q': is64bit = 1; break;	//BSD synonym for long long. or more specifically int64 and NOT int128.
						case 'j': break;	//intmax_t
						//case 'Z':	//synonym for 'z'. do not use
						case 'z': break;	//size_t
						case 't': break;	//ptrdiff_t

						default:
							goto nolength;
					}
					++s;
				}
nolength:

				// now s points to the final directive char and is no longer changed
				if (*s == 'p' || *s == 'P')
				{
					//%p is slightly different from %x.
					//always 8-bytes wide with 0 padding, always ints.
					flags |= PRINTF_ZEROPAD;
					if (width < 0) width = 8;
					if (isfloat < 0) isfloat = 0;
				}
				else if (*s == 'i')
				{
					//%i defaults to ints, not floats.
					if(isfloat < 0) isfloat = 0;
				}

				//assume floats, not ints.
				if(isfloat < 0)
					isfloat = 1;

				if(thisarg < 0)
					thisarg = argpos++;

				if(o < end - 1)
				{
					f = &formatbuf[1];
					if(*s != 's' && *s != 'c' && *s != 'S')
						if(flags & PRINTF_ALTERNATE) *f++ = '#';
					if(flags & PRINTF_ZEROPAD) *f++ = '0';
					if(flags & PRINTF_LEFT) *f++ = '-';
					if(flags & PRINTF_SPACEPOSITIVE) *f++ = ' ';
					if(flags & PRINTF_SIGNPOSITIVE) *f++ = '+';
					*f++ = '*';
					if(precision >= 0)
					{
						*f++ = '.';
						*f++ = '*';
					}

					switch(*s)
					{
						case 'd': case 'i': case 'I':
						case 'o': case 'u': case 'x': case 'X': case 'p': case 'P':
#ifdef _WIN32				//not c99
							*f++ = 'I';
							*f++ = '6';
							*f++ = '4';
#else						//c99
							*f++ = 'l';
							if (sizeof(long) == 4)
								*f++ = 'l';	//go for long long instead
#endif
							break;
					}


					if (*s == 'p')
						*f++ = 'x';
					else if (*s == 'P')
						*f++ = 'X';
					else if (*s == 'S')
						*f++ = 's';
					else
						*f++ = *s;
					*f++ = 0;

					if(width < 0) // not set
						width = 0;

					switch(*s)
					{
						case 'd': case 'i': case 'I':
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, formatbuf, width, GETARG_SNUMERIC(qint64_t, thisarg));
							else
								Q_snprintfz(o, end - o, formatbuf, width, precision, GETARG_SNUMERIC(qint64_t, thisarg));
							o += strlen(o);
							break;
						case 'o': case 'u': case 'x': case 'X': case 'p': case 'P':
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, formatbuf, width, GETARG_UNUMERIC(quint64_t, thisarg));
							else
								Q_snprintfz(o, end - o, formatbuf, width, precision, GETARG_UNUMERIC(quint64_t, thisarg));
							o += strlen(o);
							break;
						case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, formatbuf, width, GETARG_SNUMERIC(double, thisarg));
							else
								Q_snprintfz(o, end - o, formatbuf, width, precision, GETARG_SNUMERIC(double, thisarg));
							o += strlen(o);
							break;
						case 'v': case 'V':
							f[-2] += 'g' - 'v';
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, va("%s %s %s", /* NESTED SPRINTF IS NESTED */ formatbuf, formatbuf, formatbuf),
									width, (isfloat ? (double) GETARG_VECTOR(thisarg)[0] : (double) GETARG_INTVECTOR(thisarg)[0]),
									width, (isfloat ? (double) GETARG_VECTOR(thisarg)[1] : (double) GETARG_INTVECTOR(thisarg)[1]),
									width, (isfloat ? (double) GETARG_VECTOR(thisarg)[2] : (double) GETARG_INTVECTOR(thisarg)[2])
								);
							else
								Q_snprintfz(o, end - o, va("%s %s %s", /* NESTED SPRINTF IS NESTED */ formatbuf, formatbuf, formatbuf),
									width, precision, (isfloat ? (double) GETARG_VECTOR(thisarg)[0] : (double) GETARG_INTVECTOR(thisarg)[0]),
									width, precision, (isfloat ? (double) GETARG_VECTOR(thisarg)[1] : (double) GETARG_INTVECTOR(thisarg)[1]),
									width, precision, (isfloat ? (double) GETARG_VECTOR(thisarg)[2] : (double) GETARG_INTVECTOR(thisarg)[2])
								);
							o += strlen(o);
							break;
						case 'c':
							if((flags & PRINTF_ALTERNATE) || !VMUTF8)
							{	//precision+width are in bytes
								if(precision < 0) // not set
									Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg)));
								else
									Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg)));
								o += strlen(o);
							}
							else
							{	//precision+width are in chars
								unsigned int c = (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg));
								char buf[16];
								c = unicode_encode(buf, c, sizeof(buf), VMUTF8MARKUP);
								buf[c] = 0;
								if(precision < 0) // not set
									precision = end - o - 1;
								unicode_strpad(o, end - o, buf, (flags & PRINTF_LEFT) != 0, width, precision, VMUTF8MARKUP);
								o += strlen(o);
							}
							break;
						case 'S':
							{
								const char *quotedarg = GETARG_STRING(thisarg);
								char quotedbuf[65536];	//FIXME: no idea how big this actually needs to be.
								quotedarg = COM_QuotedString(quotedarg, quotedbuf, sizeof(quotedbuf), false);
								if((flags & PRINTF_ALTERNATE) || !VMUTF8)
								{	//precision+width are in bytes
									if(precision < 0) // not set
										Q_snprintfz(o, end - o, formatbuf, width, quotedarg);
									else
										Q_snprintfz(o, end - o, formatbuf, width, precision, quotedarg);
									o += strlen(o);
								}
								else
								{	//precision+width are in chars
									if(precision < 0) // not set
										precision = end - o - 1;
									unicode_strpad(o, end - o, quotedarg, (flags & PRINTF_LEFT) != 0, width, precision, VMUTF8MARKUP);
									o += strlen(o);
								}
							}
							break;
						case 's':	
							if((flags & PRINTF_ALTERNATE) || !VMUTF8)
							{	//precision+width are in bytes
								if(precision < 0) // not set
									Q_snprintfz(o, end - o, formatbuf, width, GETARG_STRING(thisarg));
								else
									Q_snprintfz(o, end - o, formatbuf, width, precision, GETARG_STRING(thisarg));
								o += strlen(o);
							}
							else
							{	//precision+width are in chars
								if(precision < 0) // not set
									precision = end - o - 1;
								unicode_strpad(o, end - o, GETARG_STRING(thisarg), (flags & PRINTF_LEFT) != 0, width, precision, VMUTF8MARKUP);
								o += strlen(o);
							}
							break;
						default:
							Con_Printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
					}
				}
				++s;
				break;
			default:
verbatim:
				if(o < end - 1)
					*o++ = *s;
				s++;
				break;
		}
	}
finished:
	*o = 0;
}

void QCBUILTIN PF_sprintf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char outbuf[65536];	//FIXME: no idea how big this actually needs to be.
	PF_sprintf_internal(prinst, pr_globals, PR_GetStringOfs(prinst, OFS_PARM0), 1, outbuf, sizeof(outbuf));
	RETURN_TSTRING(outbuf);
}

//float()
void QCBUILTIN PF_numentityfields (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int count = 0;
	prinst->FieldInfo(prinst, &count);
	G_FLOAT(OFS_RETURN) = count;
}
void QCBUILTIN PF_findentityfield (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *fieldname = PR_GetStringOfs(prinst, OFS_PARM0);
	unsigned int count = 0, fidx;
	fdef_t *fdef;
	fdef = prinst->FieldInfo(prinst, &count);
	G_FLOAT(OFS_RETURN) = 0;
	for (fidx = 0; fidx < count; fidx++)
	{
		if (!strcmp(fdef[fidx].name, fieldname))
		{
			G_FLOAT(OFS_RETURN) = fidx;
			break;
		}
	}
}
void QCBUILTIN PF_entityfieldref (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int fidx = G_FLOAT(OFS_PARM0);
	unsigned int count = 0;
	fdef_t *fdef;
	fdef = prinst->FieldInfo(prinst, &count);
	G_INT(OFS_RETURN) = 0;
	if (fidx < count)
	{
		G_INT(OFS_RETURN) = fdef[fidx].ofs;
	}
}
//string(float fieldnum)
void QCBUILTIN PF_entityfieldname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int fidx = G_FLOAT(OFS_PARM0);
	unsigned int count = 0;
	fdef_t *fdef;
	fdef = prinst->FieldInfo(prinst, &count);
	if (fidx < count)
	{
		RETURN_TSTRING(fdef[fidx].name);
	}
	else
		G_INT(OFS_RETURN) = 0;
}
//float(float fieldnum)
void QCBUILTIN PF_entityfieldtype (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int fidx = G_FLOAT(OFS_PARM0);
	unsigned int count = 0;
	fdef_t *fdef = prinst->FieldInfo(prinst, &count);
	if (fidx < count)
	{
		G_FLOAT(OFS_RETURN) = fdef[fidx].type;
	}
	else
		G_FLOAT(OFS_RETURN) = 0;
}
//string(float fieldnum, entity ent)
void QCBUILTIN PF_getentityfieldstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int fidx = G_FLOAT(OFS_PARM0);
	wedict_t *ent = (wedict_t *)G_EDICT(prinst, OFS_PARM1);
	eval_t *eval;
	unsigned int count = 0;
	fdef_t *fdef = prinst->FieldInfo(prinst, &count);
	G_INT(OFS_RETURN) = 0;
	if (fidx < count)
	{
#if !defined(CLIENTONLY) && defined(HAVE_LEGACY)
		qboolean isserver = (prinst == sv.world.progs);
#endif
		eval = (eval_t *)&((pvec_t *)ent->v)[fdef[fidx].ofs];
#ifdef HAVE_LEGACY	//extra code to be lazy so that xonotic doesn't go crazy and spam the fuck out of e
		if ((fdef->type & 0xff) == ev_vector)
		{
			if (eval->_vector[0]==0&&eval->_vector[1]==0&&eval->_vector[2]==0)
				return;
		}
#ifndef CLIENTONLY
#ifdef HEXEN2
		else if (isserver && (pvec_t*)eval == &((edict_t*)ent)->xv->drawflags && eval->_float == 96)
			return;
#endif
		else if (isserver && (pvec_t*)eval == &((edict_t*)ent)->xv->uniquespawnid)
			return;
#endif
		else if (((pvec_t*)eval == &ent->xv->dimension_solid ||
				  (pvec_t*)eval == &ent->xv->dimension_hit
#ifndef CLIENTONLY
				  || (isserver && ((pvec_t*)eval == &((edict_t*)ent)->xv->dimension_see
				  || (pvec_t*)eval == &((edict_t*)ent)->xv->dimension_seen))
#endif
				  )  && eval->_float == 255)
			return;
		else
		{
			if (!eval->_int)
				return;
		}
#endif
		RETURN_TSTRING(prinst->UglyValueString(prinst, fdef[fidx].type, eval));
	}
}
//float(float fieldnum, entity ent, string s)
void QCBUILTIN PF_putentityfieldstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int fidx = G_FLOAT(OFS_PARM0);
	wedict_t *ent = (wedict_t *)G_EDICT(prinst, OFS_PARM1);
	const char *str = PR_GetStringOfs(prinst, OFS_PARM2);
	eval_t *eval;
	unsigned int count = 0;
	fdef_t *fdef = prinst->FieldInfo(prinst, &count);
	if (fidx < count)
	{
		eval = (eval_t *)&((float *)ent->v)[fdef[fidx].ofs];
		G_FLOAT(OFS_RETURN) = prinst->ParseEval(prinst, eval, fdef[fidx].type, str);
	}
	else
		G_FLOAT(OFS_RETURN) = 0;
}

//must match ordering in Cmd_ExecuteString.
void QCBUILTIN PF_checkcommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *str = PR_GetStringOfs(prinst, OFS_PARM0);
	//functions, aliases, cvars. in that order.
	if (Cmd_Exists(str))
	{
		G_FLOAT(OFS_RETURN) = 1;
		return;
	}
	if (Cmd_AliasExist(str, RESTRICT_INSECURE))
	{
		G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	if (Cvar_FindVar(str))
	{
		G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	G_FLOAT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_physics_supported(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef USERBE
	world_t *world = prinst->parms->user;
	if (prinst->callargc)
	{
		if (G_FLOAT(OFS_PARM0) && !world->rbe)
			World_RBE_Start(world);
		else if (!G_FLOAT(OFS_PARM0) && world->rbe)
			World_RBE_Shutdown(world);
	}
	if (world->rbe)
		G_FLOAT(OFS_RETURN) = 1;
	else
#endif
		G_FLOAT(OFS_RETURN) = 0;
}
#ifdef USERBE
void QCBUILTIN PF_physics_enable(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t*e			= G_WEDICT(prinst, OFS_PARM0);
	int		isenable	= G_FLOAT(OFS_PARM1);
	world_t *world = prinst->parms->user;
	rbecommandqueue_t cmd;
	
	cmd.command = isenable?RBECMD_ENABLE:RBECMD_DISABLE;
	cmd.edict = e;

	if (world->rbe)
		world->rbe->PushCommand(world, &cmd);
}
void QCBUILTIN PF_physics_addforce(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t*e				= G_WEDICT(prinst, OFS_PARM0);
	float	*force			= G_VECTOR(OFS_PARM1);
	float	*impactpos		= G_VECTOR(OFS_PARM2);	//world coord of impact.
	world_t *world = prinst->parms->user;
	rbecommandqueue_t cmd;

	cmd.command = RBECMD_FORCE;
	cmd.edict = e;
	VectorCopy(force, cmd.v1);
	VectorCopy(impactpos, cmd.v2);

	if (world->rbe)
		world->rbe->PushCommand(world, &cmd);
}
void QCBUILTIN PF_physics_addtorque(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	wedict_t*e				= G_WEDICT(prinst, OFS_PARM0);
	float	*torque			= G_VECTOR(OFS_PARM1);
	world_t *world = prinst->parms->user;
	rbecommandqueue_t cmd;

	cmd.command = RBECMD_TORQUE;
	cmd.edict = e;
	VectorCopy(torque, cmd.v1);

	if (world->rbe)
		world->rbe->PushCommand(world, &cmd);
}
#endif

/*
=============
PF_pushmove
=============
*/
void QCBUILTIN PF_pushmove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;
	wedict_t *ent;
	float *move;
	float *amove;
	
	ent = G_WEDICT(prinst, OFS_PARM0);
	move = G_VECTOR(OFS_PARM1);
	amove = G_VECTOR(OFS_PARM2);

	G_FLOAT(OFS_RETURN) = WPhys_Push(world, ent, move, amove);
}

void PR_Common_Shutdown(pubprogfuncs_t *progs, qboolean errored)
{
	PR_ClearThreads(progs);
#if defined(SKELETALOBJECTS) || defined(RAGDOLLS)
	skel_reset(progs->parms->user);
#endif
	PR_fclose_progs(progs);
	search_close_progs(progs, !errored);
#ifdef ENGINE_ROUTING
	PR_Route_Shutdown (progs->parms->user);
#endif
#ifdef TEXTEDITOR
	Editor_ProgsKilled(progs);
#endif
#ifdef SQL
	SQL_KillServers(progs);
#endif
	tokenize_flush();
}

qboolean PR_Common_LoadGame(pubprogfuncs_t *prinst, char *command, const char **file)
{
	const char *l = *file;
	if (!strcmp(command, "buffer"))
	{
		l = PR_buf_loadgame(prinst, l);
		if (!l)
			return false;
	}
	else if (!strcmp(command, "hashtable"))
	{
		l = PR_hash_loadgame(prinst, l);
		if (!l)
			return false;
	}
	else
		return false;
	*file = l;
	return true;
}
void PR_Common_SaveGame(vfsfile_t *f, pubprogfuncs_t *prinst, qboolean binary)
{
	PR_buf_savegame(f, prinst, binary);
	PR_hash_savegame(f, prinst, binary);
}


void *PR_GetWriteQCPtr(pubprogfuncs_t *prinst, int qcptr, int qcsize)
{
//	void *r;
	if (qcsize < 0)
		return NULL;
	if (!qcptr)
		return NULL;
	if (qcptr >= 0 && qcptr <= prinst->stringtablemaxsize)
	{
		if (qcptr + qcsize <= prinst->stringtablemaxsize)
			return prinst->stringtable+qcptr;	//its in bounds
	}
	/*else
	{
		r = PR_GetString(prinst, qcptr);
		if (qcsize < strlen(r))
			return r;
	}*/
	return NULL;
}
const void *PR_GetReadQCPtr(pubprogfuncs_t *prinst, int qcptr, int qcsize)
{
	const char *r;
	if (qcsize < 0)
		return NULL;
	if (!qcptr)
		return NULL;
	if (qcptr >= 0 && qcptr <= prinst->stringtablemaxsize)
	{
		if (qcptr + qcsize <= prinst->stringtablemaxsize)
			return prinst->stringtable+qcptr;	//its in bounds
	}
	else
	{
		r = PR_GetString(prinst, qcptr);
		if (qcsize < strlen(r))
			return r;
	}
	return NULL;
}


#define DEF_SAVEGLOBAL (1u<<15)
static void PR_AutoCvarApply(pubprogfuncs_t *prinst, eval_t *val, etype_t type, cvar_t *var)
{
	switch(type & ~DEF_SAVEGLOBAL)
	{
	case ev_float:
		val->_float = var->value;
		break;
	case ev_integer:
		val->_int = var->ival;
		break;
	case ev_string:
#ifdef QCGC
		if (*var->string)
			val->_int = PR_NewString(prinst, var->string);
		else
			val->_int = 0;
#else
		if (val->_int)
			prinst->RemoveProgsString(prinst, val->_int);
		if (*var->string)
			val->_int = PR_SetString(prinst, var->string);
		else
			val->_int = 0;
#endif
		break;
	case ev_vector:
		{
			char res[128];
			char *vs = var->string;
			vs = COM_ParseOut(vs, res, sizeof(res));
			val->_vector[0] = atof(res);
			vs = COM_ParseOut(vs, res, sizeof(res));
			val->_vector[1] = atof(res);
			vs = COM_ParseOut(vs, res, sizeof(res));
			val->_vector[2] = atof(res);
		}
		break;
	}
}
/*called when a var has changed*/
void PR_AutoCvar(pubprogfuncs_t *prinst, cvar_t *var)
{
	char *gname;
	eval_t *val;
	etype_t type;
	int n, p;

	if (var->flags & CVAR_NOUNSAFEEXPAND)
		return;

	for (n = 0; n < 2; n++)
	{
		gname = n?var->name2:var->name;
		if (!gname)
			continue;
		gname = va("autocvar_%s", gname);
		
		for (p = 0; p < prinst->numprogs; p++)
		{
			val = PR_FindGlobal(prinst, gname, p, &type);
			if (val)
				PR_AutoCvarApply(prinst, val, type, var);
		}
	}
}

static void PDECL PR_FoundAutoCvarGlobal(pubprogfuncs_t *progfuncs, char *name, eval_t *val, etype_t type, void *ctx)
{
	cvar_t *var;
	const char *vals;
	int nlen;
	name += 9; //autocvar_
	
	switch(type & ~DEF_SAVEGLOBAL)
	{
	case ev_float:
		//ignore individual vector componants. let the vector itself do all the work.
		nlen = strlen(name);
		if(nlen >= 2 && name[nlen-2] == '_' && (name[nlen-1] == 'x' || name[nlen-1] == 'y' || name[nlen-1] == 'z'))
			return;

		vals = va("%g", val->_float);
		break;
	case ev_integer:
		vals = va("%i", val->_int);
		break;
	case ev_vector:
		vals = va("%g %g %g", val->_vector[0], val->_vector[1], val->_vector[2]);
		break;
	case ev_string:
		vals = PR_GetString(progfuncs, val->string);
		val->_int = 0;
		break;
	default:
		return;
	}
	var = Cvar_Get(name, vals, 0, "autocvars");
	if (!var || (var->flags & CVAR_NOUNSAFEEXPAND))
		return;

	var->flags |= CVAR_TELLGAMECODE;

	PR_AutoCvarApply(progfuncs, val, type, var);
}

void PDECL PR_FoundDoTranslateGlobal(pubprogfuncs_t *progfuncs, char *name, eval_t *val, etype_t type, void *ctx)
{
	const char *olds;
	const char *news;
	if ((type & ~DEF_SAVEGLOBAL) != ev_string)
		return;

	olds = PR_GetString(progfuncs, val->string);
	news = PO_GetText(ctx, olds);
	if (news != olds)
		val->string = PR_NewString(progfuncs, news);
}

//called after each progs is loaded
void PR_ProgsAdded(pubprogfuncs_t *prinst, int newprogs, const char *modulename)
{
	vfsfile_t *f = NULL, *f2 = NULL;
	char lang[64], *h;
	extern cvar_t language;
	if (!prinst || newprogs < 0)
		return;

	Q_strncpyz(lang, language.string, sizeof(lang));
	while ((h = strchr(lang, '-')))
		*h = '_';
	for(;;)
	{
		if (!f)
			f = FS_OpenVFS(va("%s.%s.po", modulename, *lang?lang:"default"), "rb", FS_GAME);
		if (!f2)
			f2 = FS_OpenVFS(va("common.%s.po", *lang?lang:"default"), "rb", FS_GAME);
		if (f && f2)
			break;
		if (!*lang)
			break;
		h = strchr(lang, '_');
		if (h)
			*h = 0;
		else if (*lang)
			*lang = 0;
		else
			break;
	}

	if (f || f2)
	{
		void *pofile = PO_Create();
		PO_Merge(pofile, f);
		PO_Merge(pofile, f2);
		prinst->FindPrefixGlobals (prinst, newprogs, "dotranslate_", PR_FoundDoTranslateGlobal, pofile);
		PO_Close(pofile);
	}
	prinst->FindPrefixGlobals (prinst, newprogs, "autocvar_", PR_FoundAutoCvarGlobal, NULL);

	QCLoadBreakpoints("", modulename);
}

//rpd provides its own version of many of the earlier protocols, so stuff might work even without those.
static qboolean check_pext_rpd				(extcheck_t *extcheck, unsigned int pext1) {return (extcheck->pext1 & pext1) || (extcheck->pext2&PEXT2_REPLACEMENTDELTAS);}

static qboolean check_pext_setview			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_SETVIEW);}
static qboolean check_pext_scale			(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_SCALE);}
static qboolean check_pext_lightstylecol	(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_LIGHTSTYLECOL);}
static qboolean check_pext_trans			(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_TRANS);}
static qboolean check_pext_view2			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_VIEW2_);}
//static qboolean check_pext_scale			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_BULLETENS);}
//static qboolean check_pext_senttimings		(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_ACCURATETIMINGS);}
//static qboolean check_pext_sounddbl			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_SOUNDDBL);}
static qboolean check_pext_fatness			(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_FATNESS);}
//static qboolean check_pext_hlbsp			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_HLBSP);}
static qboolean check_pext_tebullet			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_TE_BULLET);}
//static qboolean check_pext_hullsize			(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_HULLSIZE);}
//static qboolean check_pext_modeldbl			(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_MODELDBL);}
//static qboolean check_pext_entitydbl		(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_ENTITYDBL);}
//static qboolean check_pext_entitydbl2		(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_ENTITYDBL2);}
static qboolean check_pext_floatcoords		(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_FLOATCOORDS);}
//static qboolean check_pext_vweap			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_VWEAP);}
static qboolean check_pext_q2bsp			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_Q2BSP_);}
static qboolean check_pext_q3bsp			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_Q3BSP_);}
static qboolean check_pext_colourmod		(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_COLOURMOD);}
//static qboolean check_pext_splitscreen		(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_SPLITSCREEN);}
static qboolean check_pext_hexen2			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_HEXEN2);}
static qboolean check_pext_spawnstatic		(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_SPAWNSTATIC2);}
static qboolean check_pext_customtempeffects(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_CUSTOMTEMPEFFECTS);}
static qboolean check_pext_256packetentities(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_256PACKETENTITIES);}
static qboolean check_pext_showpic			(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_SHOWPIC);}
static qboolean check_pext_setattachment	(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_SETATTACHMENT);}
//static qboolean check_pext_chunkeddownloads	(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_CHUNKEDDOWNLOADS);}
static qboolean check_pext_csqc				(extcheck_t *extcheck) {return !!(extcheck->pext1 & PEXT_CSQC);}
//static qboolean check_pext_dpflags			(extcheck_t *extcheck) {return check_pext_rpd(extcheck,PEXT_DPFLAGS);}
#define check_pext_pointparticle			NULL

//static qboolean check_pext2_prydoncursor	(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_PRYDONCURSOR);}
//static qboolean check_pext2_voicechat		(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_VOICECHAT);}
//static qboolean check_pext2_setangledelta	(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_SETANGLEDELTA);}
//static qboolean check_pext2_replacementdeltas(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_REPLACEMENTDELTAS);}
//static qboolean check_pext2_maxplayers		(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_MAXPLAYERS);}
//static qboolean check_pext2_predinfo		(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_PREDINFO);}
//static qboolean check_pext2_newsizeencoding	(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_NEWSIZEENCODING);}
static qboolean check_pext2_infoblobs		(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_INFOBLOBS);}
//static qboolean check_pext2_stunaware		(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_STUNAWARE);}
static qboolean check_pext2_vrinputs		(extcheck_t *extcheck) {return !!(extcheck->pext2 & PEXT2_VRINPUTS);}

//rerelease stomped on things. make sure our earlier extension reports correctly.
static qboolean check_notrerelease		(extcheck_t *extcheck) {return !extcheck->world->remasterlogic;}
//static qboolean check_rerelease			(extcheck_t *extcheck) {return !!extcheck->world->remasterlogic;}

#define NOBI NULL, 0,{NULL},
qc_extension_t QSG_Extensions[] = {
	//these don't have well-defined names...
	{"??TOMAZ_STRINGS",					check_notrerelease,	6,{"tq_zone", "tq_unzone",  "tq_strcat", "tq_substring", "tq_stof", "tq_stov"}},
	{"??TOMAZ_FILE",					NULL,	4,{"tq_fopen", "tq_fclose", "tq_fgets", "tq_fputs"}},
	{"??MVDSV_BUILTINS",				NULL,	21,{"executecommand", "mvdtokenize", "mvdargc", "mvdargv",
												"teamfield", "substr", "mvdstrcat", "mvdstrlen", "str2byte",
												"str2short", "mvdnewstr", "mvdfreestr", "conprint", "readcmd",
												"mvdstrcpy", "strstr", "mvdstrncpy", "logtext", "redirectcmd",
												"mvdcalltimeofday", "forcedemoframe"}},
	{"BX_COLOREDTEXT"},
	{"DP_CON_SET",						NULL,	0,{NULL}, "The 'set' console command exists, and can be used to create/set cvars."},
#ifndef SERVERONLY
	{"DP_CON_SETA",						NULL,	0,{NULL}, "The 'seta' console command exists, like the 'set' command, but also marks the cvar for archiving, allowing it to be written into the user's config. Use this command in your default.cfg file."},
#endif
	{"DP_CSQC_ROTATEMOVES"},
	{"DP_EF_ADDITIVE",					check_notrerelease},
	{"DP_EF_BLUE",						check_notrerelease},						//hah!! This is QuakeWorld!!!
	{"DP_EF_FULLBRIGHT"},				//Rerouted to hexen2 support.
	{"DP_EF_NODEPTHTEST"},				//for cheats
	{"DP_EF_NODRAW",					check_notrerelease},					//implemented by sending it with no modelindex
	{"DP_EF_NOGUNBOB"},					//nogunbob. sane people should use csqc instead.
	{"DP_EF_NOSHADOW"},
	{"DP_EF_RED",						check_notrerelease},
	{"DP_ENT_ALPHA",					check_pext_trans},			//transparent entites
	{"DP_ENT_COLORMOD",					check_pext_colourmod},
	{"DP_ENT_CUSTOMCOLORMAP"},
	{"DP_ENT_EXTERIORMODELTOCLIENT"},
	{"DP_ENT_SCALE",					check_pext_scale},	//listed above
	{"DP_ENT_TRAILEFFECTNUM",			NULL,	1,{"particleeffectnum"}, "self.traileffectnum=particleeffectnum(\"myeffectname\"); can be used to attach a particle trail to the given server entity. This is equivelent to calling trailparticles each frame."},
	//only in dp6 currently {"DP_ENT_GLOW"},
	{"DP_ENT_VIEWMODEL"},
	{"DP_GECKO_SUPPORT",				NULL,	7,{"gecko_create", "gecko_destroy", "gecko_navigate", "gecko_keyevent", "gecko_mousemove", "gecko_resize", "gecko_get_texture_extent"}},
	{"DP_GFX_FONTS",					NULL,	2,{"findfont", "loadfont"}},	//note: font slot numbers/names are not special in fte.
//	{"DP_GFX_FONTS_FREETYPE"},	//extra cvars are not supported.
	{"DP_GFX_QUAKE3MODELTAGS",			check_pext_setattachment,	1,{"setattachment"}},
	{"DP_GFX_SKINFILES"},
	{"DP_GFX_SKYBOX"},	//according to the spec. :)
	{"DP_HALFLIFE_MAP"},		//entity can visit a hl bsp
	{"DP_HALFLIFE_MAP_CVAR"},
	//to an extent {"DP_HALFLIFE_SPRITE"},
	{"DP_INPUTBUTTONS"},
	{"DP_LIGHTSTYLE_STATICVALUE"},
	{"DP_LITSUPPORT"},
	{"DP_MONSTERWALK",					NULL,	0,{NULL}, "MOVETYPE_WALK is valid on non-player entities. Note that only players receive acceleration etc in line with none/bounce/fly/noclip movetypes on the player, thus you will have to provide your own accelerations (incluing gravity) yourself."},
	{"DP_MOVETYPEBOUNCEMISSILE",		check_notrerelease},		//I added the code for hexen2 support.
	{"DP_MOVETYPEFOLLOW"},
	{"DP_QC_ASINACOSATANATAN2TAN",		NULL,	5,{"asin", "acos", "atan", "atan2", "tan"}},
	{"DP_QC_CHANGEPITCH",				NULL,	1,{"changepitch"}},
	{"DP_QC_COPYENTITY",				NULL,	1,{"copyentity"}},
	{"DP_QC_CRC16",						NULL,	1,{"crc16"}},
	{"DP_QC_CVAR_DEFSTRING",			NULL,	1,{"cvar_defstring"}},
	{"DP_QC_CVAR_STRING",				NULL,	1,{"cvar_string"}},	//448 builtin.
	{"DP_QC_CVAR_TYPE",					NULL,	1,{"cvar_type"}},
	{"DP_QC_DIGEST_SHA256"},
	{"DP_QC_EDICT_NUM",					NULL,	1,{"edict_num"}},
	{"DP_QC_ENTITYDATA",				NULL,	5,{"numentityfields", "entityfieldname", "entityfieldtype", "getentityfieldstring", "putentityfieldstring"}},
	{"DP_QC_ETOS",						NULL,	1,{"etos"}},
	{"DP_QC_FINDCHAIN",					NULL,	1,{"findchain"}},
	{"DP_QC_FINDCHAINFLOAT",			NULL,	1,{"findchainfloat"}},
	{"DP_QC_FINDFLAGS",					NULL,	1,{"findflags"}},
	{"DP_QC_FINDCHAINFLAGS",			NULL,	1,{"findchainflags"}},
	{"DP_QC_FINDFLOAT",					NULL,	1,{"findfloat"}},
	{"DP_QC_FS_SEARCH",					NULL,	4,{"search_begin", "search_end", "search_getsize", "search_getfilename"}},
	{"DP_QC_FS_SEARCH_PACKFILE",		NULL,	4,{"search_begin", "search_end", "search_getsize", "search_getfilename"}},
	{"DP_QC_GETLIGHT",					check_notrerelease,	1,{"getlight"}},
	{"DP_QC_GETSURFACE",				NULL,	6,{"getsurfacenumpoints", "getsurfacepoint", "getsurfacenormal", "getsurfacetexture", "getsurfacenearpoint", "getsurfaceclippedpoint"}},
	{"DP_QC_GETSURFACEPOINTATTRIBUTE",	NULL,	1,{"getsurfacepointattribute"}},
	{"DP_QC_GETTAGINFO",				NULL,	2,{"gettagindex", "gettaginfo"}},
	{"DP_QC_I18N",						NULL,	0,{NULL}, "Specifies that the engine uses $MODULE.dat.$LANG.po files that translates the dotranslate_* globals on load - these are usually created via the _(\"foo\") qcc intrinsic."},
	{"DP_QC_MINMAXBOUND",				NULL,	3,{"min", "max", "bound"}},
	{"DP_QC_MULTIPLETEMPSTRINGS",		NULL,	0,{NULL}, "Superseded by DP_QC_UNLIMITEDTEMPSTRINGS. Functions that return a temporary string will not overwrite/destroy previous temporary strings until at least 16 strings are returned (or control returns to the engine)."},
	{"DP_QC_RANDOMVEC",					check_notrerelease,	1,{"randomvec"}},
	{"DP_QC_RENDER_SCENE",				NULL,	0,{NULL}, "clearscene+addentity+setviewprop+renderscene+setmodel are available to menuqc. WARNING: DP advertises this extension without actually supporting it, FTE does actually support it."},
	{"DP_QC_SINCOSSQRTPOW",				NULL,	4,{"sin", "cos", "sqrt", "pow"}},
	{"DP_QC_SPRINTF",					NULL,	1,{"sprintf"}, "Provides the sprintf builtin, which allows for rich formatting along the lines of C's function with the same name. Not to be confused with QC's sprint builtin."},
	{"DP_QC_STRFTIME",					NULL,	1,{"strftime"}},
	{"DP_QC_STRING_CASE_FUNCTIONS",		NULL,	2,{"strtolower", "strtoupper"}},
	{"DP_QC_STRINGBUFFERS",				NULL,	10,{"buf_create", "buf_del", "buf_getsize", "buf_copy", "buf_sort", "buf_implode", "bufstr_get", "bufstr_set", "bufstr_add", "bufstr_free"}},
	{"DP_QC_STRINGCOLORFUNCTIONS",		NULL,	2,{"strlennocol", "strdecolorize"}},
	{"DP_QC_STRREPLACE",				NULL,	2,{"strreplace", "strireplace"}},
	{"DP_QC_TOKENIZEBYSEPARATOR",		NULL,	1,{"tokenizebyseparator"}},
	{"DP_QC_TRACEBOX",					check_notrerelease,	1,{"tracebox"}},
	{"DP_QC_TRACETOSS"},
	{"DP_QC_TRACE_MOVETYPE_HITMODEL"},
	{"DP_QC_TRACE_MOVETYPE_WORLDONLY"},
	{"DP_QC_TRACE_MOVETYPES"},		//this one is just a lame excuse to add another extension...
	{"DP_QC_UNLIMITEDTEMPSTRINGS",		NULL,	0,{NULL}, "Supersedes DP_QC_MULTIPLETEMPSTRINGS, superseded by FTE_QC_PERSISTENTTEMPSTRINGS. Specifies that all temp strings will be valid at least until the QCVM returns."},
	{"DP_QC_URI_ESCAPE",				NULL,	2,{"uri_escape", "uri_unescape"}},
#ifdef WEBCLIENT
	{"DP_QC_URI_GET",					NULL,	1,{"uri_get"}},
	{"DP_QC_URI_POST",					NULL,	1,{"uri_get"}},
#endif
	{"DP_QC_VECTOANGLES_WITH_ROLL"},
	{"DP_QC_VECTORVECTORS",				NULL,	1,{"vectorvectors"}},
	{"DP_QC_WHICHPACK",					NULL,	1,{"whichpack"}},
	{"DP_QUAKE2_MODEL"},
	{"DP_QUAKE2_SPRITE"},
	{"DP_QUAKE3_MAP"},
	{"DP_QUAKE3_MODEL"},
	{"DP_REGISTERCVAR",					NULL,	1,{"registercvar"}},
	{"DP_SND_SOUND7_WIP2"},	//listed only to silence xonotic, if anyone tries running that.
	{"DP_SND_STEREOWAV"},
	{"DP_SND_OGGVORBIS"},
	{"DP_SOLIDCORPSE"},
	{"DP_SPRITE32"},				//hmm... is it legal to advertise this one?
	{"DP_SV_BOTCLIENT",					NULL,	2,{"spawnclient", "clienttype"}},
	{"DP_SV_CLIENTCAMERA",				NULL,	0,{NULL}, "Works like svc_setview except also handles pvs."},
	{"DP_SV_CLIENTCOLORS",				NULL,	0,{NULL}, "Provided only for compatibility with DP."},
	{"DP_SV_CLIENTNAME",				NULL,	0,{NULL}, "Provided only for compatibility with DP."},
	{"DP_SV_CUSTOMIZEENTITYFORCLIENT",	NULL,	0,{NULL}, "Deprecated feature for compat with DP. Can be slow, incompatible with splitscreen, usually malfunctions with mvds."},
	{"DP_SV_DRAWONLYTOCLIENT"},
	{"DP_SV_DROPCLIENT",				NULL,	1,{"dropclient"}, "Equivelent to quakeworld's stuffcmd(self,\"disconnect\\n\"); hack"},
	{"DP_SV_EFFECT",					NULL,	1,{"effect"}},
	{"DP_SV_NODRAWTOCLIENT"},		//I prefer my older system. Guess I might as well remove that older system at some point.
	{"DP_SV_PLAYERPHYSICS",				NULL,	0,{NULL}, "Allows reworking parts of NQ player physics. USE AT OWN RISK - this necessitates NQ physics and is thus guarenteed to break prediction."},
//	{"DP_SV_POINTPARTICLES",			check_pext_pointparticle,	3,{"particleeffectnum", "pointparticles", "trailparticles"}, "Specifies that pointparticles (and trailparticles) exists in ssqc as well as csqc (and that dp's trailparticles argument fuckup will normally work). ssqc values can be passed to csqc for use, the reverse is not true. Does NOT mean that DP's effectinfo.txt is supported, only that ssqc has functionality equivelent to csqc."},
	{"DP_SV_POINTSOUND",				NULL,	1,{"pointsound"}},
	{"DP_SV_PRECACHEANYTIME",			NULL,	0,{NULL}, "Specifies that the various precache builtins can be called at any time. WARNING: precaches are sent reliably while sound events, modelindexes, and particle events are not. This can mean sounds and particles might not work the first time around, or models may take a while to appear (after the reliables are received and the model is loaded from disk). Always attempt to precache a little in advance in order to reduce these issues (preferably at the start of the map...)"},
	{"DP_SV_PRINT",						NULL,	1,{"print"}, "Says that the print builtin can be used from nqssqc (as well as just csqc), bypassing the developer cvar issues."},
	{"DP_SV_ROTATINGBMODEL",			check_notrerelease,0,{NULL},		"Engines that support this support avelocity on MOVETYPE_PUSH entities, pushing entities out of the way as needed."},
	{"DP_SV_SETCOLOR",					NULL,	1,{"setcolor"}},
	{"DP_SV_SHUTDOWN"},
	{"DP_SV_SPAWNFUNC_PREFIX"},
	{"DP_SV_WRITEPICTURE",				NULL,	1,{"WritePicture"}},
	{"DP_SV_WRITEUNTERMINATEDSTRING",	NULL,	1,{"WriteUnterminatedString"}},
	{"DP_TE_BLOOD",						NULL,	1,{"te_blood"}},
	{"_DP_TE_BLOODSHOWER",				NULL,	1,{"te_bloodshower"}, "Requires external particle config."},
	{"DP_TE_CUSTOMFLASH",				NULL,	1,{"te_customflash"}},
	{"DP_TE_EXPLOSIONRGB",				NULL,	1,{"te_explosionrgb"}},
	{"_DP_TE_FLAMEJET",					NULL,	1,{"te_flamejet"}, "Requires external particle config."},
	{"DP_TE_PARTICLECUBE",				NULL,	1,{"te_particlecube"}},
	{"DP_TE_PARTICLERAIN",				NULL,	1,{"te_particlerain"}},
	{"DP_TE_PARTICLESNOW",				NULL,	1,{"te_particlesnow"}},
	{"_DP_TE_PLASMABURN",				NULL,	1,{"te_plasmaburn"}, "Requires external particle config."},
	{"_DP_TE_QUADEFFECTS1",				NULL,	4,{"te_gunshotquad", "te_spikequad", "te_superspikequad", "te_explosionquad"}, "Requires external particle config."},
	{"DP_TE_SMALLFLASH",				NULL,	1,{"te_smallflash"}},
	{"DP_TE_SPARK",						NULL,	1,{"te_spark"}},
	{"DP_TE_STANDARDEFFECTBUILTINS",	NULL,	14,{	"te_gunshot", "te_spike", "te_superspike", "te_explosion", "te_tarexplosion", "te_wizspike", "te_knightspike",
													"te_lavasplash", "te_teleport", "te_explosion2", "te_lightning1", "te_lightning2", "te_lightning3", "te_beam"}},
	{"DP_VIEWZOOM"},
	{"EXT_BITSHIFT",					NULL,	1,{"bitshift"}},
	{"EXT_CSQC",						check_pext_csqc,	2,{"multicast","clientstat"}},	//this is the base csqc extension. I'm not sure what needs to be separate and what does not.
	{"EXT_CSQC_SHARED",					check_pext_csqc},				//this is a separate extension because it requires protocol modifications. note: this is also the extension that extends the allowed stats.
	{"EXT_DIMENSION_VISIBILITY"},
	{"EXT_DIMENSION_PHYSICS"},
	{"EXT_DIMENSION_GHOST"},
//	{"EX_EXTENDED_EF",					NULL,	0,{NULL}, "Provides EF_CANDLELIGHT..."},
//	{"EX_MOVETYPE_GIB",					NULL,	0,{NULL}, "Adds MOVETYPE_GIB - basically MOVETYPE_BOUNCE with gravity controlled by a cvar instead of just setting the .gravity field..."},
	{"EX_PROMPT",						NULL,	3,{"ex_prompt", "ex_promptchoice", "ex_clearprompt"}, "Engine-driven alternative to centerprint menus."},
	{"FRIK_FILE",						check_notrerelease,	11,{"stof", "fopen","fclose","fgets","fputs","strlen","strcat","substring","stov","strzone","strunzone"}},
	{"FTE_CALLTIMEOFDAY",				NULL,	1,{"calltimeofday"}, "Replication of mvdsv functionality (call calltimeofday to cause 'timeofday' to be called, with arguments that can be saved off to a global). Generally strftime is simpler to use."},
	{"FTE_CSQC_ALTCONSOLES",			NULL,	4,{"con_getset", "con_printf", "con_draw", "con_input"}, "The engine tracks multiple consoles. These may or may not be directly visible to the user."},
	{"FTE_CSQC_BASEFRAME",				NULL,	0,{NULL}, "Specifies that .basebone, .baseframe2, .baselerpfrac, baseframe1time, etc exist in csqc. These fields affect all bones in the entity's model with a lower index than the .basebone field, allowing you to give separate control to the legs of a skeletal model, without affecting the torso animations."},
#ifdef HALFLIFEMODELS
	{"FTE_CSQC_HALFLIFE_MODELS"},		//hl-specific skeletal model control
#endif
	{"FTE_CSQC_SERVERBROWSER",			NULL,	12,{"gethostcachevalue", "gethostcachestring", "resethostcachemasks", "sethostcachemaskstring", "sethostcachemasknumber",
													"resorthostcache", "sethostcachesort", "refreshhostcache", "gethostcachenumber", "gethostcacheindexforkey",
													"addwantedhostcachekey", "getextresponse"}, "Provides builtins to query the engine's serverbrowser servers list from ssqc. Note that these builtins are always available in menuqc."},
	{"FTE_CSQC_SKELETONOBJECTS",		NULL,	15,{"skel_create", "skel_build", "skel_get_numbones", "skel_get_bonename", "skel_get_boneparent", "skel_find_bone",
													"skel_get_bonerel", "skel_get_boneabs", "skel_set_bone", "skel_premul_bone", "skel_premul_bones", "skel_copybones",
													"skel_delete", "frameforname", "frameduration"}, "Provides container objects for skeletal bone data, which can be modified on a per bone basis if needed. This allows you to dynamically generate animations (or just blend them with greater customisation) instead of being limited to a single animation or two."},
	{"FTE_CSQC_RAWIMAGES",				NULL,	2,{"r_uploadimage","r_readimage"}, "Provides raw rgba image access to csqc. With this, the csprogs can read textures into qc-accessible memory, modify it, and then upload it to the renderer."},
	{"FTE_CSQC_RENDERTARGETS",			NULL,	0,{NULL}, "VF_RT_DESTCOLOUR exists and can be used to redirect any rendering to a texture instead of the screen."},
	{"FTE_CSQC_REVERB",					NULL,	1,{"setup_reverb"}, "Specifies that the mod can create custom reverb effects. Whether they will actually be used or not depends upon the sound driver."},
	{"FTE_CSQC_WINDOWCAPTION",			NULL,	1,{"setwindowcaption"}, "Provides csqc with the ability to change the window caption as displayed when running windowed or in the task bar when switched out."},
	{"FTE_ENT_SKIN_CONTENTS",			NULL,	0,{NULL}, "self.skin = CONTENTS_WATER; makes a brush entity into water. use -16 for a ladder."},
	{"FTE_ENT_UNIQUESPAWNID"},
	{"FTE_EXTENDEDTEXTCODES"},
	{"FTE_FORCESHADER",					NULL,	1,{"shaderforname"}, "Allows csqc to override shaders on models with an explicitly named replacement. Also allows you to define shaders with a fallback if it does not exist on disk."},	//I'd rename this to _CSQC_ but it does technically provide this builtin to menuqc too, not that the forceshader entity field exists there... but whatever.
	{"FTE_FORCEINFOKEY",				NULL,	1,{"forceinfokey"},	"Provides an easy way to change a user's userinfo from the server."},
	{"FTE_GFX_QUAKE3SHADERS",			NULL,	0,{NULL},	"specifies that the engine has full support for vanilla quake3 shaders"},
	{"FTE_GFX_REMAPSHADER",				NULL,	0,{NULL},	"With the raw power of stuffcmds, the r_remapshader console command is exposed! This mystical command can be used to remap any shader to another. Remapped shaders that specify $diffuse etc in some form will inherit the textures implied by the surface."},
	{"FTE_GFX_IQM_HITMESH",				NULL,	0,{NULL},	"Supports hitmesh iqm extensions. Also supports geomsets and embedded events."},
	{"FTE_GFX_MODELEVENTS",				NULL,	1,{"processmodelevents", "getnextmodelevent", "getmodeleventidx"},	"Provides a query for per-animation events in model files, including from progs/foo.mdl.events files."},
	{"FTE_ISBACKBUFFERED",				NULL,	1,{"isbackbuffered"}, "Allows you to check if a client has too many reliable messages pending."},
	{"FTE_MEMALLOC",					NULL,	4,{"memalloc", "memfree", "memcpy", "memfill8"}, "Allows dynamically allocating memory. Use pointers to access this memory. Memory will not be saved into saved games."},
#ifdef HAVE_MEDIA_DECODER
	#if defined(_WIN32) && !defined(WINRT)
	{"FTE_MEDIA_AVI",					NULL,	0,{NULL}, "playfilm command supports avi files."},
	#endif
	#ifdef Q2CLIENT
	{"FTE_MEDIA_CIN",					NULL,	0,{NULL}, "playfilm command supports q2 cin files."},
	#endif
	#ifdef Q3CLIENT
	{"FTE_MEDIA_ROQ",					NULL,	0,{NULL}, "playfilm command supports q3 roq files."},
	#endif
#endif
	{"FTE_MULTIPROGS",					NULL,	5,{"externcall", "addprogs", "externvalue", "externset", "instr"}, "Multiple progs.dat files can be loaded inside the same qcvm. Insert new ones with addprogs inside the 'init' function, and use externvalue+externset to rewrite globals (and hook functions) to link them together. Note that the result is generally not very clean unless you carefully design for it beforehand."},	//multiprogs functions are available.
	{"FTE_MULTITHREADED",				NULL,	3,{"sleep", "fork", "abort"}, "Faux multithreading, allowing multiple contexts to run in sequence."},
	{"FTE_MVD_PLAYERSTATS",				NULL,	0,{NULL}, "In csqc, getplayerstat can be used to query any player's stats when playing back MVDs. isdemo will return 2 in this case."},
#ifdef SERVER_DEMO_PLAYBACK
	{"FTE_MVD_PLAYBACK",				NOBI	"The server itself is able to play back MVDs."},
#endif
#ifdef SVCHAT
	{"FTE_QC_NPCCHAT",					NULL,	1,{"chat"}},	//server looks at chat files. It automagically branches through calling qc functions as requested.
#endif
#ifdef PSET_SCRIPT
	{"FTE_PART_SCRIPT",					NULL,	0,{NULL}, "Specifies that the r_particledesc cvar can be used to select a list of particle effects to load from particles/foo.cfg, the format of which is documented elsewhere."},
	{"FTE_PART_NAMESPACES",				NULL,	0,{NULL}, "Specifies that the engine can use foo.bar to load effect foo from particle description bar. When used via ssqc, this should cause the client to download whatever effects as needed."},
#ifdef HAVE_LEGACY
	{"FTE_PART_NAMESPACE_EFFECTINFO",	NULL,	0,{NULL}, "Specifies that effectinfo.bar can load effects from effectinfo.txt for DP compatibility."},
#endif
#endif

	{"FTE_PEXT_SETVIEW",				check_pext_setview, 0,{NULL}, "NQ's svc_setview works correctly even in quakeworld"},
	{"FTE_PEXT_LIGHTSTYLECOL",			check_pext_lightstylecol},	//lightstyles may have colours.
	{"FTE_PEXT_VIEW2",					check_pext_view2},		//secondary view.
//	{"FTE_PEXT_ACURATETIMINGS",			check_pext_senttimings},		//allows full interpolation
//	{"FTE_PEXT_SOUNDDBL",				check_pext_sounddbl},	//twice the sound indexes
	{"FTE_PEXT_FATNESS",				check_pext_fatness},		//entities may be expanded along their vertex normals
	{"FTE_PEXT_TE_BULLET",				check_pext_tebullet},	//additional particle effect. Like TE_SPIKE and TE_SUPERSPIKE
//	{"FTE_PEXT_HULLSIZE",				check_pext_},	//means we can tell a client to go to crouching hull
//	{"FTE_PEXT_MODELDBL",				check_pext_modeldbl},	//max of 512 models
//	{"FTE_PEXT_ENTITYDBL",				check_pext_},	//max of 1024 ents
//	{"FTE_PEXT_ENTITYDBL2",				check_pext_},	//max of 2048 ents
	{"FTE_PEXT_FLOATCOORDS",			check_pext_floatcoords},
	{"FTE_PEXT_Q2BSP",					check_pext_q2bsp, 0,{NULL},"Specifies that the client supports q2bsps."},
	{"FTE_PEXT_Q3BSP",					check_pext_q3bsp, 0,{NULL},"Specifies that the client supports q3bsps."},	//quake3 bsp support. dp probably has an equivelent, but this is queryable per client.
	{"FTE_HEXEN2",						check_pext_hexen2,	3,{"particle2", "particle3", "particle4"}},				//client can use hexen2 maps. server can use hexen2 progs
	{"FTE_PEXT_SPAWNSTATIC",			check_pext_spawnstatic},	//means that static entities can have alpha/scale and anything else the engine supports on normal ents. (Added for >256 models, while still being compatible - previous system failed with -1 skins)
	{"FTE_PEXT_CUSTOMTENTS",			check_pext_customtempeffects,	2,{"RegisterTempEnt", "CustomTempEnt"}},
	{"FTE_PEXT_256PACKETENTITIES",		check_pext_256packetentities, 0,{NULL},"Specifies that the client is not limited to vanilla's limit of only 64 ents visible at once."},	//client is able to receive unlimited packet entities (server caps itself to 256 to prevent insanity).
//	{"PEXT_CHUNKEDDOWNLOADS",			check_pext_chunkeddownloads},
//	{"PEXT_DPFLAGS",					check_pext_dpflags},


	{"FTE_QC_BASEFRAME",				NULL,	0,{NULL}, "Specifies that .basebone and .baseframe exist in ssqc. These fields affect all bones in the entity's model with a lower index than the .basebone field, allowing you to give separate control to the legs of a skeletal model, without affecting the torso animations, from ssqc."},
	{"FTE_QC_FILE_BINARY",				NULL,	4,{"fread","fwrite","fseek","fsize"}, "Extends FRIK_FILE with binary read+write, as well as allowing seeking. Requires pointers."},
	{"FTE_QC_CHANGELEVEL_HUB",			NULL,	0,{NULL}, "Adds an extra argument to changelevel which is carried over to the next map in the 'startspot' global. Maps will be saved+reloaded until the extra argument is omitted again, purging all saved maps. Saved games will contain a copy of each preserved map. parm1-parm64 globals can be used, giving more space to transfer more player data."},
	{"FTE_QC_CHECKCOMMAND",				NULL,	1,{"checkcommand"}, "Provides a way to test if a console command exists, and whether its a command/alias/cvar. Does not say anything about the expected meanings of any arguments or values."},
	{"FTE_QC_CHECKPVS",					NULL,	1,{"checkpvs"}},
	{"FTE_QC_CROSSPRODUCT",				NULL,	1,{"crossproduct"}},
	{"FTE_QC_CUSTOMSKINS",				NULL,	1,{"setcustomskin", "loadcustomskin", "applycustomskin", "releasecustomskin"}, "The engine supports the use of q3 skins, as well as the use of such skin 'files' to specify rich top+bottom colours, qw skins, geomsets, or texture composition even on non-players.."},
	{"FTE_QC_DIGEST_SHA1",				NOBI	"The digest_hex builtin supports 160-bit sha1 hashes."},
	{"FTE_QC_DIGEST_SHA224",			NOBI	"The digest_hex builtin supports 224-bit sha2 hashes."},
	{"FTE_QC_DIGEST_SHA384",			NOBI	"The digest_hex builtin supports 384-bit sha2 hashes."},
	{"FTE_QC_DIGEST_SHA512",			NOBI	"The digest_hex builtin supports 512-bit sha2 hashes."},
	{"FTE_QC_FS_SEARCH_SIZEMTIME",		NULL,	2,{"search_getfilesize", "search_getfilemtime"}},
	{"FTE_QC_HARDWARECURSORS",			NULL,	0,{NULL}, "setcursormode exists in both csqc+menuqc, and accepts additional arguments to specify a cursor image to use when this module has focus. If the image exceeds hardware limits (or hardware cursors are unsupported), it will be emulated using regular draws - this at least still avoids conflicting cursors as only one will ever be used, even if console+menu+csqc are all overlayed."},
	{"FTE_QC_HASHTABLES",				NULL,	6,{"hash_createtab", "hash_destroytab", "hash_add", "hash_get", "hash_delete", "hash_getkey"}, "Provides efficient string-based lookups."},
	{"FTE_QC_INFOKEY",					check_notrerelease,	2,{"infokey", "stof"}, "QuakeWorld's infokey builtin works, and reports at least name+topcolor+bottomcolor+ping(in ms)+ip(unmasked, but not always ipv4)+team(aka bottomcolor in nq). Does not require actual localinfo/serverinfo/userinfo, but they're _highly_ recommended to any engines with csqc"},
	{"FTE_QC_INTCONV",					NULL,	6,{"stoi", "itos", "stoh", "htos", "itof", "ftoi"}, "Provides string<>int conversions, including hex representations."},
	{"FTE_QC_MATCHCLIENTNAME",			NULL,	1,{"matchclientname"}},
	{"FTE_QC_MULTICAST",				NULL,	1,{"multicast"}, "QuakeWorld's multicast builtin works along with MSG_MULTICAST, but also with unicast support."},
//	{"FTE_QC_MESHOBJECTS",				NULL,	0,{"mesh_create", "mesh_build", "mesh_getvertex", "mesh_getindex", "mesh_setvertex", "mesh_setindex", "mesh_destroy"}, "Provides qc with the ability to create its own meshes."},
	{"FTE_QC_PAUSED"},
#ifdef QCGC
	{"FTE_QC_PERSISTENTTEMPSTRINGS",	NOBI	 "Supersedes DP_QC_MULTIPLETEMPSTRINGS. Temp strings are garbage collected automatically, and do not expire while they're still in use. This makes strzone redundant."},
#endif
#ifdef RAGDOLL
	{"FTE_QC_RAGDOLL_WIP",				NULL,	1,{"skel_ragupdate", "skel_set_bone_world", "skel_mmap"}},
#endif
	{"FTE_QC_SENDPACKET",				NULL,	1,{"sendpacket"}, "Allows the use of out-of-band udp packets to/from other hosts. Includes the SV_ParseConnectionlessPacket event."},
	{"FTE_QC_STUFFCMDFLAGS",			NULL,	1,{"stuffcmdflags"}, "Variation on regular stuffcmd that gives control over how spectators/mvds should be treated."},
	{"FTE_QC_TRACETRIGGER"},
#ifdef Q2CLIENT
	{"FTE_QUAKE2_CLIENT",				NULL,	0,{NULL}, "This engine is able to act as a quake2 client"},
#endif
#ifdef Q2SERVER
	{"FTE_QUAKE2_SERVER",				NULL,	0,{NULL}, "This engine is able to act as a quake2 server"},
#endif
#ifdef Q3CLIENT
	{"FTE_QUAKE3_CLIENT",				NULL,	0,{NULL}, "This engine is able to act as a quake3 client"},
#endif
#ifdef Q3SERVER
	{"FTE_QUAKE3_SERVER",				NULL,	0,{NULL}, "This engine is able to act as a quake3 server"},
#endif
	{"FTE_SOLID_BSPTRIGGER",			NOBI			"Allows for mappers to use shaped triggers instead of being limited to axially aligned triggers."},
	{"FTE_SOLID_LADDER",				NOBI			"Allows a simple trigger to remove effects of gravity (solid 20). obsolete. will prolly be removed at some point as it is not networked properly. Use FTE_ENT_SKIN_CONTENTS"},
	{"FTE_SPLITSCREEN",					NOBI			"Client supports splitscreen, controlled via cl_splitscreen. Servers require allow_splitscreen 1 if splitscreen is to be used over the internet. Mods that use csqc will need to be aware for this to work properly. per-client networking may be problematic."},

#ifdef SQL
	// serverside SQL functions for managing an SQL database connection
	{"FTE_SQL",							NULL,	9,{"sqlconnect","sqldisconnect","sqlopenquery","sqlclosequery","sqlreadfield","sqlerror","sqlescape","sqlversion",
												  "sqlreadfloat"}, "Provides sql* builtins which can be used for sql database access"},
#ifdef USE_MYSQL
	{"FTE_SQL_MYSQL",					NULL,	0, NULL, {NULL}, "SQL functionality is able to utilise mysql"},
#endif
#ifdef USE_SQLITE
	{"FTE_SQL_SQLITE",					NULL,	0,{NULL}, "SQL functionality is able to utilise sqlite databases"},
#endif
#endif

	//eperimental advanced strings functions.
	//reuses the FRIK_FILE builtins (with substring extension)
	{"FTE_STRINGS",						check_notrerelease,	17,{"stof", "strlen","strcat","substring","stov","strzone","strunzone",
												   "strstrofs", "str2chr", "chr2str", "strconv", "infoadd", "infoget", "strncmp", "strcasecmp", "strncasecmp", "strpad"}, "Extra builtins (and additional behaviour) to make string manipulation easier"},
	{"FTE_SV_POINTPARTICLES",			check_pext_pointparticle,	3,{"particleeffectnum", "pointparticles", "trailparticles"}, "Specifies that particleeffectnum, pointparticles, and trailparticles exist in ssqc as well as csqc. particleeffectnum acts as a precache, allowing ssqc values to be networked up with csqc for use. Use in combination with FTE_PART_SCRIPT+FTE_PART_NAMESPACES to use custom effects. This extension is functionally identical to the DP version, but avoids any misplaced assumptions about the format of the client's particle descriptions."},
	{"FTE_SV_REENTER"},
	{"FTE_TE_STANDARDEFFECTBUILTINS",	NULL,	14,{"te_gunshot", "te_spike", "te_superspike", "te_explosion", "te_tarexplosion", "te_wizspike", "te_knightspike", "te_lavasplash",
												   "te_teleport", "te_lightning1", "te_lightning2", "te_lightning3", "te_lightningblood", "te_bloodqw"}, "Provides builtins to replace writebytes, with a QW compatible twist."},
#ifdef TERRAIN
	{"FTE_TERRAIN_MAP",					NULL,	1,{"terrain_edit"}, "This engine supports .hmp files, as well as terrain embedded within bsp files."},
	{"FTE_RAW_MAP",						NULL,	7,{"brush_get","brush_create","brush_delete","brush_selected","brush_getfacepoints","brush_calcfacepoints","brush_findinvolume"}, "This engine supports directly loading .map files, as well as realtime editing of the various brushes."},
#endif
	{"FTE_INFOBLOBS",					check_pext2_infoblobs,	0,{"forceinfokeyblob", "getplayerkeyblob", "setlocaluserinfoblob", "getlocaluserinfoblob", "serverkeyblob"}, "Removes the length limits on user/server/local info strings, and allows embedded nulls and other otherwise-reserved characters. This can be used to network avatar images and the like, or other binary data."},
	{"FTE_VRINPUTS",					check_pext2_vrinputs,	0,{NULL}, "input_weapon, input_left_*, input_right_*, input_head_* work, both in csqc (as inputs with suitable plugin/hardware support) and ssqc (available in PlayerPreThink)."},

	{"KRIMZON_SV_PARSECLIENTCOMMAND",	NULL,	3,{"clientcommand", "tokenize", "argv"}, "SSQC's SV_ParseClientCommand function is able to handle client 'cmd' commands. The tokenizing parts also work in csqc."},	//very very similar to the mvdsv system.
	{"NEH_CMD_PLAY2"},
	{"NEH_RESTOREGAME"},
	//{"PRYDON_CLIENTCURSOR",			check_pext2_prydoncursor},
//	{"QEX_QC_DRAWING",					NULL,
//	{"QEX_QC_FORMATTEDPRINTS",			check_rerelease,	3,{"centerprint_qex","sprint_qex","bprint_qex"}},
	{"QSG_CVARSTRING",					NULL,	1,{"qsg_cvar_string"}},
//	{"QUAKE_EX",						check_rerelease},	//we arn't. there'll be lots of other conflicts, so just don't advertise it.
	{"QW_ENGINE",						check_notrerelease,	3,{"infokey", "stof", "logfrag"}},	//warning: interpretation of .skin on players can be dodgy, as can some other QW features that differ from NQ.
	{"QWE_MVD_RECORD",					NOBI	"You can use the easyrecord command to record MVD demos serverside."},	//Quakeworld extended get the credit for this one. (mvdsv)
	{"TEI_MD3_MODEL"},
	{"TEI_SHOWLMP2",					check_pext_showpic,	6,{"showpic", "hidepic", "movepic", "changepic", "showpicent", "hidepicent"}},	//telejano doesn't actually export the moveent/changeent (we don't want to either cos it would stop frik_file stuff being autoregistered)
	{"TENEBRAE_GFX_DLIGHTS",			NULL,	0,{NULL}, "Allows ssqc to attach rtlights to entities with various special properties."},
//	{"TQ_RAILTRAIL"},	//treat this as the ZQ style railtrails which the client already supports, okay so the preparse stuff needs strengthening.
	{"ZQ_MOVETYPE_FLY",					NOBI	"MOVETYPE_FLY works on players."},
	{"ZQ_MOVETYPE_NOCLIP",				NOBI	"MOVETYPE_NOCLIP works on players."},
	{"ZQ_MOVETYPE_NONE",				NOBI	"MOVETYPE_NONE works on players."},
//	{"ZQ_QC_PARTICLE"},	//particle builtin works in QW ( we don't mimic ZQ fully though)
	{"ZQ_VWEP",							NULL,	1,{"precache_vwep_model"}},


	{"ZQ_QC_STRINGS",					check_notrerelease,	7,{"stof", "strlen","strcat","substring","stov","strzone","strunzone"}, "The strings-only subset of FRIK_FILE is supported."}	//a trimmed down FRIK_FILE.
};
unsigned int QSG_Extensions_count = sizeof(QSG_Extensions)/sizeof(QSG_Extensions[0]);
#endif
