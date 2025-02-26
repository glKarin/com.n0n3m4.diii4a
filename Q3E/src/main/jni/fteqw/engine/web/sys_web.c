#include "quakedef.h"
#include <emscripten/emscripten.h>

#ifndef isDedicated
qboolean isDedicated;
#endif

quakeparms_t	parms;

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[1024];

	va_start (argptr,error);
	vsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);
	COM_WorkerAbort(string);
	Sys_Printf("Error: %s\n", string);

	Con_Print ("Quake Error: ");
	Con_Print (string);
	Con_Print ("\n");

	Host_Shutdown ();
	emscriptenfte_alert(string);
	emscriptenfte_abortmainloop("Sys_Error", true);
	exit (1);
}

void Sys_RecentServer(char *command, char *target, char *title, char *desc)
{
}

qboolean Sys_RandomBytes(qbyte *string, int len)
{
	return false;
}

static qboolean sys_supportsansi;
static char ansiremap[8] = {'0', '4', '2', '6', '1', '5', '3', '7'};
static size_t ApplyColour(char *out, size_t outsize, unsigned int chrflags)
{
	char *s, *e;
	static int oldflags = CON_WHITEMASK;
	int bg, fg;

	if (!sys_supportsansi)
		return 0;
	if (oldflags == chrflags)
		return 0;
	s = out;
	e = out+outsize;
	oldflags = chrflags;

	*out++='\x1b';
	*out++='[';
	*out++='0';
	*out++=';';
	if (chrflags & CON_BLINKTEXT)
	{	//set blink flag
		*out++='5';
		*out++=';';
	}

	bg = (chrflags & CON_BGMASK) >> CON_BGSHIFT;
	fg = (chrflags & CON_FGMASK) >> CON_FGSHIFT;

	// don't handle intensive bit for background
	// as terminals differ too much in displaying \e[1;7;3?m
	bg &= 0x7;

	if (chrflags & CON_NONCLEARBG)
	{
		if (fg & 0x8) // intensive bit set for foreground
		{	// set bold/intensity ansi flag
			*out++='1';
			*out++=';';
			fg &= 0x7; // strip intensive bit
		}

		// set foreground and background colors
		*out++='3';
		*out++=ansiremap[fg];
		*out++=';';
		*out++='4';
		*out++=ansiremap[bg];
		*out++='m';
	}
	else
	{
		switch(fg)
		{
		//to get around wierd defaults (like a white background) we have these special hacks for colours 0 and 7
		case COLOR_BLACK:
			// set inverse
			*out++='7';
			*out++='m';
			break;
		case COLOR_GREY:
			*out++='1';
			*out++=';';
			*out++='3';
			*out++='0';
			*out++='m';
			break;
		case COLOR_WHITE:
			// set nothing else
			*out++='m';
			break;
		default:
			if (fg & 0x8) // intensive bit set for foreground
			{
				// set bold/intensity ansi flag
				*out++='1';
				*out++=';';
				fg &= 0x7; // strip intensive bit
			}

			*out++='3';
			*out++=ansiremap[fg];
			*out++='m';
			break;
		}
	}
	return out-s;
}
//print into stdout
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;	
	char text[65536];
	conchar_t	ctext[countof(text)], *e, *c;
	unsigned int len = 0;
	unsigned int w, codeflags;
		
	va_start (argptr,fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	//make sense of any markup
	e = COM_ParseFunString(CON_WHITEMASK, text, ctext, sizeof(ctext), false);

	//convert to utf-8 for the js to make sense of
	for (c = ctext; c < e; )
	{
		c = Font_Decode(c, &codeflags, &w);
		if (codeflags & CON_HIDDEN)
			continue;

		if ((codeflags&CON_RICHFORECOLOUR) || (w == '\n' && (codeflags&CON_NONCLEARBG)))
			codeflags = CON_WHITEMASK;	//make sure we don't get annoying backgrounds on other lines.
		len+=ApplyColour(text+len,sizeof(text)-1-len, codeflags);

		//dequake it as required, so its only codepoints the browser will understand. should probably deal with linefeeds specially.
		if (w >= 0xe000 && w < 0xe100)
		{	//quake-encoded mess
			if ((w & 0x7f) >= 0x20)
				w &= 0x7f;	//regular (discoloured) ascii
			else if (w & 0x80)
			{	//c1 glyphs
				static char tab[32] = "---#@.@@@@ # >.." "[]0123456789.---";
				w = tab[w&31];
			}
			else
			{	//c0 glyphs
				static char tab[32] = ".####.#### # >.." "[]0123456789.---";
				w = tab[w&31];
			}
		}
		else if (w < ' ' && w != '\t' && w != '\r' && w != '\n')
			w = '?';	//c0 chars are awkward
	
		len += utf8_encode(text+len, w, sizeof(text)-1-len);
	}

	len+=ApplyColour(text+len,sizeof(text)-1-len, CON_WHITEMASK);	//force it back to white at the end of the print... just in case

	text[len] = 0;
	

	//now throw it at the browser's console.log.
	emscriptenfte_print(text);
}

#if 1
//use Performance.now() instead of Date.now() - its likely to both provide higher precision and no NTP/etc issues.
double Sys_DoubleTime (void)
{
	double t = emscriptenfte_uptime_ms()/1000;	//we need it as seconds...
	static double old = -99999999;
	if (t < old)
		t = old;	//don't let t step backwards, ever. this shouldn't happen, but some CPUs don't keep their high-precision timers synced properly.
	return old=t;
}
unsigned int Sys_Milliseconds(void)
{
	return Sys_DoubleTime() * (uint64_t)1000;
}
#else
unsigned int Sys_Milliseconds(void)
{
	static int first = true;
	static unsigned long oldtime = 0, curtime = 0;
	unsigned long newtime;

	newtime = emscriptenfte_ticks_ms();	//return Date.now()

	if (first)
	{
		first = false;
		oldtime = newtime;
	}
	if (newtime < oldtime)
		Con_Printf("Sys_Milliseconds stepped backwards!\n");
	else
		curtime += newtime - oldtime;
	oldtime = newtime;
	return curtime;
}

//return the current time, in the form of a double
double Sys_DoubleTime (void)
{
	return Sys_Milliseconds() / 1000.0;
}
#endif

//create a directory. we don't do dirs.
void Sys_mkdir (const char *path)
{
}
qboolean Sys_rmdir (const char *path)
{
	return true;
}

//unlink a file
qboolean Sys_remove (const char *path)
{
	emscriptenfte_buf_delete(path);
	return true;
}

qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return emscriptenfte_buf_rename(oldfname, newfname);
}
qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace)
{	//not implemented. we could try querying local storage quotas, but our filesystem is otherwise purely ram so doesn't have much of a limit in 64bit browsers. hurrah for swap space.
	*freespace = 0;	//just in case.
	return false;
}

//someone used the 'quit' command
#include "glquake.h"
void Sys_Quit (void)
{
	if (host_initialized)
	{
		qglClearColor(0,0,0,1);
		qglClear(GL_COLOR_BUFFER_BIT);
		Draw_FunString (0, 0, "Reload the page to restart");

		Host_Shutdown();
	}

	exit (0);
}


struct enumctx_s
{
	char name[MAX_OSPATH];
	const char *gpath;
	size_t gpathlen;
	const char *match;
	int (*callback)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *);
	void *ctx;
	searchpathfuncs_t *spath;
	int ret;
};
static void Sys_EnumeratedFile(void *vctx, size_t fsize)
{	//called for each enumerated file.
	//we don't need the whole EnumerateFiles2 thing as our filesystem is flat, so */* isn't an issue for us (we don't expect a lot of different 'files' if only because they're a pain to download).
	struct enumctx_s *ctx = vctx;
	if (!ctx->ret)
		return;	//we're meant to stop when if it returns false...
	if (!strncmp(ctx->name, ctx->gpath, ctx->gpathlen))		//ignore any gamedir prefix
		if (wildcmp(ctx->match, ctx->name+ctx->gpathlen))	//match it within the searched gamedir...
			ctx->ret = ctx->callback(ctx->name+ctx->gpathlen, fsize, 0, ctx->ctx, ctx->spath);	//call the callback
}
int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	struct enumctx_s ctx;
	char tmp[MAX_OSPATH];
	if (!gpath)
		gpath = "";
	ctx.gpathlen = strlen(gpath);
	if (ctx.gpathlen && gpath[ctx.gpathlen-1] != '/')
	{	//make sure gpath is /-terminated.
		if (ctx.gpathlen >= sizeof(tmp)-1)
			return false;	//just no...
		Q_strncpyz(tmp, gpath, sizeof(tmp));
		gpath = tmp;
		tmp[ctx.gpathlen++] = '/';
	}
	ctx.gpath = gpath;
	ctx.match = match;
	ctx.callback = func;
	ctx.ctx = parm;
	ctx.spath = spath;
	ctx.ret = true;
	emscritenfte_buf_enumerate(Sys_EnumeratedFile, &ctx, sizeof(ctx.name));
	return ctx.ret;
}

//blink window if possible (it's not)
void Sys_ServerActivity(void)
{
}

void Sys_CloseLibrary(dllhandle_t *lib)
{
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	return NULL;
}
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	return NULL;
}



void Sys_BrowserRedirect_f(void)
{
	emscriptenfte_window_location(Cmd_Argv(1));
}
void Sys_OpenFile_f(void)
{
	emscriptenfte_openfile();
}


static void Sys_Register_File_Associations_f(void)
{	//we should be able to register 'web+foo://' schemes here. we can't skip the web+ part though, which is a shame.
	const char *s;
	char scheme[MAX_OSPATH];
	const char *schemes = fs_manifest->schemes;
	for (s = schemes; (s=COM_ParseOut(s,scheme,sizeof(scheme)));)
	{
		EM_ASM({
			try{
			if (navigator.registerProtocolHandler)
			navigator.registerProtocolHandler(
				UTF8ToString($0),
				document.location.origin+document.location.pathname+"?%s"+document.location.hash,
				UTF8ToString($1));
			} catch(e){}
			}, va("%s%s", strncmp(scheme,"web+",4)?"web+":"", scheme), fs_manifest->formalname?fs_manifest->formalname:fs_manifest->installation);
	}
}
char *Sys_URIScheme_NeedsRegistering(void)
{	//we have no way to query if we're registered or not. cl_main will default to bypassing this.
	return NULL;
}

void Sys_Init(void)
{
	extern cvar_t vid_width, vid_height, vid_fullscreen;
	//vid_fullscreen takes effect only on mouse clicks, any suggestion to do a vid_restart is pointless.
	vid_fullscreen.flags &= ~CVAR_VIDEOLATCH;
	//these are not really supported. so silence any spam that suggests we do something about something not even supported.
	vid_width.flags &= ~CVAR_VIDEOLATCH;
	vid_height.flags &= ~CVAR_VIDEOLATCH;

	Cmd_AddCommandD("sys_browserredirect", Sys_BrowserRedirect_f, "Navigates the browser to a different url. For sites using quake maps as a more interesting sitemap.");
	if (EM_ASM_INT(return window.showOpenFilePicker!=undefined;))	//doesn't work in firefox.
		Cmd_AddCommandD("sys_openfile", Sys_OpenFile_f, "Opens a file picker");	//opens file picker
	if (EM_ASM_INT(return Module['mayregisterscemes'] != false;))	//needs to be able to pass args via the url. don't bother adding the command if it'll fail. hurrah for checkcmd
		Cmd_AddCommandD("sys_register_file_associations", Sys_Register_File_Associations_f, "Register this page as the default handler for web+scheme handlers.\n");

	//can't really do feature detection for this... either we spit out unreadable text or we don't...
	sys_supportsansi = EM_ASM_INT(return navigator.userAgent.indexOf("FireFox")!=-1;);
}
void Sys_Shutdown(void)
{
	emscriptenfte_setupmainloop(NULL);
}



int VARGS Sys_DebugLog(char *file, char *fmt, ...)
{
	return 0;
};



qboolean Sys_InitTerminal(void)
{
	return true;
}
char *Sys_ConsoleInput(void)
{
	return NULL;
}
void Sys_CloseTerminal (void)
{
}

int Sys_MainLoop(double newtime)
{
	extern cvar_t vid_vsync;
	static double oldtime;
	double time;

	if (newtime)
		newtime /= 1000;	//use RAF's timing for slightly greater precision.
	else
		newtime = Sys_DoubleTime ();	//otherwise fall back on internally consistent timing...
	if (newtime < oldtime)
		newtime = oldtime;	//don't let ourselves go backwards...
	if (!oldtime)
		oldtime = newtime;
	time = newtime - oldtime;
	if (!host_initialized)
	{
		Sys_Printf ("Starting "FULLENGINENAME"\n");
		Host_Init (&parms);
		return 1;
	}

	oldtime = newtime;
	Host_Frame (time);

	return vid_vsync.ival;
}

int QDECL main(int argc, char **argv)
{
	memset(&parms, 0, sizeof(parms));

	parms.basedir = "";

	parms.argc = argc;
	parms.argv = (const char**)argv;
#ifdef CONFIG_MANIFEST_TEXT
	parms.manifest = CONFIG_MANIFEST_TEXT;
#endif

	COM_InitArgv (parms.argc, parms.argv);

	TL_InitLanguages("");

	emscriptenfte_setupmainloop(Sys_MainLoop);
	return 0;
}

qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	return false;
}

#ifdef WEBCLIENT
qboolean Sys_RunInstaller(void)
{       //not implemented
	return false;
}
#endif

/*static char *clipboard_buffer;
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, const char *utf8), void *ctx)
{
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, const char *text)
{
	free(clipboard_buffer);
	clipboard_buffer = strdup(text);
}*/

#ifdef MULTITHREAD
#include <SDL_thread.h>	//FIXME: swap this out for sys_linux_threads.c (our pthreads code)
/* Thread creation calls */
void *Sys_CreateThread(char *name, int (*func)(void *), void *args, int priority, int stacksize)
{
	// SDL threads do not support setting thread stack size
	return (void *)SDL_CreateThread(func, args);
}

void Sys_WaitOnThread(void *thread)
{
	SDL_WaitThread((SDL_Thread *)thread, NULL);
}


/* Mutex calls */
// SDL mutexes don't have try-locks for mutexes in the spec so we stick with 1-value semaphores
void *Sys_CreateMutex(void)
{
	return (void *)SDL_CreateSemaphore(1);
}

qboolean Sys_TryLockMutex(void *mutex)
{
	return !SDL_SemTryWait(mutex);
}

qboolean Sys_LockMutex(void *mutex)
{
	return !SDL_SemWait(mutex);
}

qboolean Sys_UnlockMutex(void *mutex)
{
	return !SDL_SemPost(mutex);
}

void Sys_DestroyMutex(void *mutex)
{
	SDL_DestroySemaphore(mutex);
}

/* Conditional wait calls */
typedef struct condvar_s
{
	SDL_mutex *mutex;
	SDL_cond *cond;
} condvar_t;

void *Sys_CreateConditional(void)
{
	condvar_t *condv;
	SDL_mutex *mutex;
	SDL_cond *cond;
	
	condv = (condvar_t *)malloc(sizeof(condvar_t));
	if (!condv)
		return NULL;
		
	mutex = SDL_CreateMutex();
	cond = SDL_CreateCond();
	
	if (mutex)
	{
		if (cond)
		{
			condv->cond = cond;
			condv->mutex = mutex;
		
			return (void *)condv;
		}
		else
			SDL_DestroyMutex(mutex);
	}
	
	free(condv);
	return NULL;	
}

qboolean Sys_LockConditional(void *condv)
{
	return !SDL_mutexP(((condvar_t *)condv)->mutex);
}

qboolean Sys_UnlockConditional(void *condv)
{
	return !SDL_mutexV(((condvar_t *)condv)->mutex);
}

qboolean Sys_ConditionWait(void *condv)
{
	return !SDL_CondWait(((condvar_t *)condv)->cond, ((condvar_t *)condv)->mutex);
}

qboolean Sys_ConditionSignal(void *condv)
{
	return !SDL_CondSignal(((condvar_t *)condv)->cond);
}

qboolean Sys_ConditionBroadcast(void *condv)
{
	return !SDL_CondBroadcast(((condvar_t *)condv)->cond);
}

void Sys_DestroyConditional(void *condv)
{
	condvar_t *cv = (condvar_t *)condv;
	
	SDL_DestroyCond(cv->cond);
	SDL_DestroyMutex(cv->mutex);
	free(cv);
}
#endif

void Sys_Sleep (double seconds)
{
	//SDL_Delay(seconds * 1000);
}

