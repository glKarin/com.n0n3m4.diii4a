#include "quakedef.h"
#include <xtl.h>

#define MAXPRINTMSG 1024 // Yes
qboolean isDedicated = false;

/*Timers, supposedly xbox supports QueryPerformanceCounter stuff*/
double Sys_DoubleTime (void)
{
	static int			first = 1;
	static LARGE_INTEGER		qpcfreq;
	LARGE_INTEGER		PerformanceCount;
	static LONGLONG			oldcall;
	static LONGLONG			firsttime;
	LONGLONG			diff;

	QueryPerformanceCounter (&PerformanceCount);
	if (first)
	{
		first = 0;
		QueryPerformanceFrequency(&qpcfreq);
		firsttime = PerformanceCount.QuadPart;
		diff = 0;
	}
	else
		diff = PerformanceCount.QuadPart - oldcall;
	if (diff >= 0)
		oldcall = PerformanceCount.QuadPart;
	return (oldcall - firsttime) / (double)qpcfreq.QuadPart;
}
unsigned int Sys_Milliseconds (void)
{
	return Sys_DoubleTime()*1000;
}

NORETURN void VARGS Sys_Error (const char *error, ...)
{
	COM_WorkerAbort(error);

	//FIXME: panic! everyone panic!
	//you might want to figure out some way to display the message...
	for(;;)
		;
}

void Sys_Sleep(double seconds)
{	//yields to other processes/threads for a bit.
}

void Sys_Quit (void)
{
#if 0
	Host_Shutdown ();
	//successful execution.
	//should go back to the system menu or something, or possibly longjmp out of the main loop.
	for (;;)
		;
	exit(1);
#endif
}

void Sys_Shutdown(void)
{
}
void Sys_Init(void)
{	//register system-specific cvars here
}

/*prints to dedicated server console or debug output*/
void VARGS Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf(msg,sizeof(msg)-1, fmt,argptr);
	msg[sizeof(msg)-1] = 0;	//_vsnprintf sucks.
	va_end (argptr);

	//no idea what the stdout is hooked up to, lets try it anyway.
	printf("%s", msg);
}

/*returns the system video mode settings, ish*/
qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	//FIXME: use XGetVideoStandard or XGetVideoFlags or something
	*width = 640;
	*height = 480;
	*bpp = 32;
	*refreshrate = 60;
	return true;
}

/*dedicated server type stuff*/
qboolean Sys_InitTerminal(void)
{
	return false;	//failure
}
void Sys_CloseTerminal (void)
{
}
char *Sys_ConsoleInput (void)
{	//returns any text typed on the stdin, when acting as a dedicated server.
	//this includes debugger commands if we're integrated with fteqccgui
	return NULL;
}

/*various system notifications*/
void Sys_ServerActivity(void)
{	//player joined the server or said something. would normally flash the app in the system tray.
}

void Sys_RecentServer(char *command, char *target, char *title, char *desc) {

}

/*check any system message queues here*/
void Sys_SendKeyEvents(void)
{
}

qboolean Sys_RandomBytes(qbyte *string, int len)
{
	//FIXME: should return some cryptographically strong random number data from some proper crypto library. C's rand function isn't really strong enough.
	return false;
}

/*filesystem stuff*/
void Sys_mkdir (const char *path)
{
}
void Sys_rmdir (const char *path)
{
}
qboolean Sys_remove (const char *path)
{
	return false;	//false for failure
}
qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return false;	//false for failure
}
int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	//if func returns false, abort with false return value, otherwise return true.
	//use wildcmd to compare two filenames
	//note that match may contain wildcards and multiple directories with multiple wildcards all over.
	return true;
}

/*consoles don't tend to need system clipboards, so this is fully internal to our engine*/
#define SYS_CLIPBOARD_SIZE  256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, char *utf8), void *ctx)
{
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, char *text)
{
 	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
}

/*dynamic library stubs*/
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	Con_Printf("Sys_LoadLibrary: %s\n", name);
	return NULL;
}
void Sys_CloseLibrary(dllhandle_t *lib)
{
}
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	return NULL;	//unsupported
}
char *Sys_GetNameForAddress(dllhandle_t *module, void *address)
{
	return NULL;	//unsupported (on most platforms, actually)
}

void main( int argc, char **argv)
{
	float time, lasttime;
	quakeparms_t parms;

	memset(&parms, 0, sizeof(parms));

	parms.argc = argc;
	parms.argv = argv;
#ifdef CONFIG_MANIFEST_TEXT
	parms.manifest = CONFIG_MANIFEST_TEXT;
#endif

	COM_InitArgv(parms.argc, parms.argv);
	TL_InitLanguages(parms.basedir);
	Host_Init(&parms);

	//main loop
	lasttime = Sys_DoubleTime();

	while (1)
	{
		time = Sys_DoubleTime();
		Host_Frame(time - lasttime);
		lasttime = time;
	}

}

/*input stubs
in_generic.c should make these kinda useless, but sometimes you need more than the following three functions:
void IN_JoystickAxisEvent(unsigned int devid, int axis, float value);
void IN_KeyEvent(unsigned int devid, int down, int keycode, int unicode);
void IN_MouseMove(unsigned int devid, int abs, float x, float y, float z, float size);
that said, if they're enough, then just call those in Sys_SendKeyEvents. You can also call them from other threads just fine.
their devid values should be assigned only when a button is first pressed, or something, so controllers get assigned to seats in the order that they're pressed. so player 1 always hits a button first to ensure that they are player 1.
or just hardcode those based upon usbport numbers, but that's unreliable with usb hubs.
*/

void INS_Shutdown (void)
{
}
void INS_ReInit (void)
{
}
void INS_Move(void)
{
	//accululates system-specific inputs on a per-seat basis.
}
void INS_Init (void)
{
}
void INS_Accumulate(void)	//input polling
{
}
void INS_Commands (void)	//used to Cbuf_AddText joystick button events in windows.
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
#if 0
	unsigned int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].controller)
			callback(ctx, "joy", sdljoy[i].devname, &sdljoy[i].qdevid);
#endif
}

void INS_SetupControllerAudioDevices (void) // Not used
{
}

void INS_UpdateGrabs (void) // Not used
{
}

