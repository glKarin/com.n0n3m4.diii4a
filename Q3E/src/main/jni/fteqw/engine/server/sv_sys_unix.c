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
#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE	//we need this in order to fix up broken backtraces. make sure its defined only where needed so we still some posixy conformance test on one plat.
#endif
#endif

#include <signal.h>
#include <sys/types.h>
#include <dlfcn.h>
#include "quakedef.h"


#undef malloc

#ifdef NeXT
#include <libc.h>
#endif

#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

#ifdef MULTITHREAD
#include <pthread.h>
#endif

#if defined(__linux__)
//if we're chrooting, then we need to poke a few other systems to ensure libraries are loaded
#include "netinc.h"
#endif

#ifdef HAVE_GNUTLS
qboolean SSL_InitGlobal(qboolean isserver);
void GnuTLS_Shutdown(void);
#endif


// callbacks
void Sys_Linebuffer_Callback (struct cvar_s *var, char *oldvalue);

cvar_t sys_nostdout = CVAR("sys_nostdout","0");
cvar_t sys_extrasleep = CVAR("sys_extrasleep","0");
cvar_t sys_colorconsole = CVARD("sys_colorconsole", "1", "Parse colour escapes, with ansi colours on stdout.");
cvar_t sys_timestamps = CVARD("sys_timestamps", "0", "Show timesamps on stdout prints.");
cvar_t sys_linebuffer = CVARC("sys_linebuffer", "1", Sys_Linebuffer_Callback);

static qboolean	stdin_ready;
static qboolean noconinput = false;

static struct termios orig, changes;

#ifdef SUBSERVERS
jmp_buf sys_sv_serverforked;
#endif

/*
===============================================================================

				REQUIRED SYS FUNCTIONS

===============================================================================
*/

/*
============
Sys_mkdir

============
*/
void Sys_mkdir (const char *path)
{
	if (mkdir (path, 0755) != -1)
		return;
//	if (errno != EEXIST)
//		Sys_Error ("mkdir %s: %s",path, strerror(errno));
}
qboolean Sys_rmdir (const char *path)
{
	if (rmdir (path) == 0)
		return true;
	if (errno == ENOENT)
		return true;
	return false;
}

qboolean Sys_remove (const char *path)
{
#ifdef __unix__
	return unlink(path);
#else
	return system(va("rm \"%s\"", path));
#endif
}

qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return !rename(oldfname, newfname);
}


#if _POSIX_C_SOURCE >= 200112L
	#include <sys/statvfs.h>
#endif
qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace)
{
#if _POSIX_C_SOURCE >= 200112L
	//posix 2001
	struct statvfs inf;
	if(0==statvfs(path, &inf))
	{
		*freespace = inf.f_bsize*(quint64_t)inf.f_bavail;
		return true;
	}
#endif
	return false;
}

int Sys_DebugLog(char *file, char *fmt, ...)
{
	va_list argptr;
	char data[1024];
	int fd;
	size_t result;

	va_start(argptr, fmt);
	vsnprintf (data,sizeof(data)-1, fmt, argptr);
	va_end(argptr);

	if (strlen(data) >= sizeof(data)-1)
		Sys_Error("Sys_DebugLog was stomped\n");

	fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (fd)
	{
		result = write(fd, data, strlen(data)); // do something with the result

		if (result != strlen(data))
			Con_Printf("Sys_DebugLog() write: Filename: %s, expected %lu, result was %lu (%s)\n",file,(unsigned long)strlen(data),(unsigned long)result,strerror(errno));

		close(fd);
		return 0;
	}
	return 1;
}


static quint64_t timer_basetime;	//used by all clocks to bias them to starting at 0
static void Sys_ClockType_Changed(cvar_t *var, char *oldval);
static cvar_t sys_clocktype = CVARFCD("sys_clocktype", "", CVAR_NOTFROMSERVER, Sys_ClockType_Changed, "Controls which system clock to base timings from.\n0: auto\n"
	"1: gettimeofday (may be discontinuous).\n"
	"2: monotonic.");
static enum
{
	QCLOCK_AUTO = 0,

	QCLOCK_GTOD,
	QCLOCK_MONOTONIC,
	QCLOCK_REALTIME,

	QCLOCK_INVALID
} timer_clocktype;
static quint64_t Sys_GetClock(quint64_t *freq)
{
	quint64_t t;
	if (timer_clocktype == QCLOCK_MONOTONIC)
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		*freq = 1000000000;
		t = (ts.tv_sec*(quint64_t)1000000000) + ts.tv_nsec;
	}
	else if (timer_clocktype == QCLOCK_REALTIME)
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		*freq = 1000000000;
		t = (ts.tv_sec*(quint64_t)1000000000) + ts.tv_nsec;

		//WARNING t can go backwards
	}
	else //if (timer_clocktype == QCLOCK_GTOD)
	{
		struct timeval tp;
		gettimeofday(&tp, NULL);
		*freq = 1000000;
		t = tp.tv_sec*(quint64_t)1000000 + tp.tv_usec;

		//WARNING t can go backwards
	}
	return t - timer_basetime;
}
static void Sys_ClockType_Changed(cvar_t *var, char *oldval)
{
	int newtype = var?var->ival:0;
	if (newtype >= QCLOCK_INVALID)
		newtype = QCLOCK_AUTO;
	if (newtype <= QCLOCK_AUTO)
		newtype = QCLOCK_MONOTONIC;

	if (newtype != timer_clocktype)
	{
		quint64_t oldtime, oldfreq;
		quint64_t newtime, newfreq;

		oldtime = Sys_GetClock(&oldfreq);
		timer_clocktype = newtype;
		timer_basetime = 0;
		newtime = Sys_GetClock(&newfreq);

		timer_basetime = newtime - (newfreq * (oldtime) / oldfreq);

		/*if (host_initialized)
		{
			const char *clockname = "unknown";
			switch(timer_clocktype)
			{
			case QCLOCK_GTOD:		clockname = "gettimeofday";	break;
			case QCLOCK_MONOTONIC:	clockname = "monotonic";	break;
			case QCLOCK_REALTIME:	clockname = "realtime";	break;
			case QCLOCK_AUTO:
			case QCLOCK_INVALID:	break;
			}
			Con_Printf("Clock %s, wraps after %"PRIu64" days, %"PRIu64" years\n", clockname, (((quint64_t)-1)/newfreq)/(24*60*60), (((quint64_t)-1)/newfreq)/(24*60*60*365));
		}*/
	}
}
static void Sys_InitClock(void)
{
	quint64_t freq;

	Cvar_Register(&sys_clocktype, "System vars");

	//calibrate it, and apply.
	Sys_ClockType_Changed(NULL, NULL);
	timer_basetime = 0;
	timer_basetime = Sys_GetClock(&freq);
}
double Sys_DoubleTime (void)
{
	quint64_t denum, num = Sys_GetClock(&denum);
	return num / (long double)denum;
}
unsigned int Sys_Milliseconds (void)
{
	quint64_t denum, num = Sys_GetClock(&denum);
	num *= 1000;
	return num / denum;
}

/*
================
Sys_Error
================
*/
void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);
	COM_WorkerAbort(string);
	fprintf(stderr, "Fatal error: %s\n",string);

	if (!noconinput)
	{
		tcsetattr(STDIN_FILENO, TCSADRAIN, &orig);
		fcntl (STDIN_FILENO, F_SETFL, fcntl (STDIN_FILENO, F_GETFL, 0) & ~O_NDELAY);
	}

	//we used to fire sigsegv. this resulted in people reporting segfaults and not the error message that appeared above. resulting in wasted debugging.
	//abort should trigger a SIGABRT and still give us the same stack trace. should be more useful that way.
	abort();

	exit (1);
}

static qboolean useansicolours;
static int ansiremap[8] = {0, 4, 2, 6, 1, 5, 3, 7};
static void ApplyColour(unsigned int chr)
{
	static int oldchar = CON_WHITEMASK;
	int bg, fg;
	chr &= CON_FLAGSMASK;

	if (oldchar == chr)
		return;
	oldchar = chr;
	if (!useansicolours)	//don't spew weird chars when redirected to a file.
		return;

	printf("\e[0;"); // reset

	if (chr & CON_BLINKTEXT)
		printf("5;"); // set blink

	bg = (chr & CON_BGMASK) >> CON_BGSHIFT;
	fg = (chr & CON_FGMASK) >> CON_FGSHIFT;

	// don't handle intensive bit for background
	// as terminals differ too much in displaying \e[1;7;3?m
	bg &= 0x7;

	if (chr & CON_NONCLEARBG)
	{
		if (fg & 0x8) // intensive bit set for foreground
		{
			printf("1;"); // set bold/intensity ansi flag
			fg &= 0x7; // strip intensive bit
		}

		// set foreground and background colors
		printf("3%i;4%im", ansiremap[fg], ansiremap[bg]);
	}
	else
	{
		switch(fg)
		{
		//to get around wierd defaults (like a white background) we have these special hacks for colours 0 and 7
		case COLOR_BLACK:
			printf("7m"); // set inverse
			break;
		case COLOR_GREY:
			printf("1;30m"); // treat as dark grey
			break;
		case COLOR_WHITE:
			printf("m"); // set nothing else
			break;
		default:
			if (fg & 0x8) // intensive bit set for foreground
			{
				printf("1;"); // set bold/intensity ansi flag
				fg &= 0x7; // strip intensive bit
			}

			printf("3%im", ansiremap[fg]); // set foreground
			break;
		}
	}
}

#define putch(c) putc(c, stdout);
/*static void Sys_PrintColouredChar(unsigned int chr)
{
	ApplyColour(chr);

	chr = chr & CON_CHARMASK;

	if ((chr > 128 || chr < 32) && chr != 10 && chr != 13 && chr != 9)
		printf("[%02x]", chr);
	else
		chr &= ~0x80;

	putch(chr);
}*/

/*
================
Sys_Printf
================
*/
#define	MAXPRINTMSG	4096
char	coninput_text[256];
int		coninput_len;
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;

	if (sys_nostdout.value)
		return;

	if (1)
	{
		char		msg[MAXPRINTMSG];
		unsigned char *t;

		va_start (argptr,fmt);
		vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
		va_end (argptr);

#ifdef SUBSERVERS
		if (SSV_IsSubServer())
		{
			if (SSV_PrintToMaster(msg))
				return;
		}
#endif

		//if we're not linebuffered, kill the currently displayed input line, add the new text, and add more output.
		if (!sys_linebuffer.value)
		{
			int i;

			for (i = 0; i < coninput_len; i++)
				putch('\b');
			putch('\b');
			for (i = 0; i < coninput_len; i++)
				putch(' ');
			putch(' ');
			for (i = 0; i < coninput_len; i++)
				putch('\b');
			putch('\b');
		}


		if (sys_colorconsole.value)
		{
			wchar_t w;
			conchar_t *e, *c;
			conchar_t ctext[MAXPRINTMSG];
			unsigned int codeflags, codepoint;
			static qboolean wasnl = false;
			e = COM_ParseFunString(CON_WHITEMASK, msg, ctext, sizeof(ctext), false);
			for (c = ctext; c < e; )
			{
				c = Font_Decode(c, &codeflags, &codepoint);
				if (codeflags & CON_HIDDEN)
					continue;

				if ((codeflags&CON_RICHFORECOLOUR) || (codepoint == '\n' && (codeflags&CON_NONCLEARBG)))
					codeflags = CON_WHITEMASK;	//make sure we don't get annoying backgrounds on other lines.
				ApplyColour(codeflags);

				if (wasnl && sys_timestamps.ival)
				{
					char buffer[64];
					time_t unixtime = time(NULL);
					strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", localtime(&unixtime));
					for (w = 0; w < buffer[w]; w++)
						putc(buffer[w], stdout);
				}
				w = codepoint;
				wasnl = (w == '\n');
				if (w >= 0xe000 && w < 0xe100)
				{
					/*not all quake chars are ascii compatible, so map those control chars to safe ones so we don't mess up anyone's xterm*/
					if ((w & 0x7f) > 0x20)
						putc(w&0x7f, stdout);
					else if (w & 0x80)
					{
						static char tab[32] = "---#@.@@@@ # >.." "[]0123456789.---";
						putc(tab[w&31], stdout);
					}
					else
					{
						static char tab[32] = ".####.#### # >.." "[]0123456789.---";
						putc(tab[w&31], stdout);
					}
				}
				else
				{
					/*putwc doesn't like me. force it in utf8*/
					if (w >= 0x80)
					{
						if (w > 0x800)
						{
							putc(0xe0 | ((w>>12)&0x0f), stdout);
							putc(0x80 | ((w>>6)&0x3f), stdout);
						}
						else
							putc(0xc0 | ((w>>6)&0x1f), stdout);
						putc(0x80 | (w&0x3f), stdout);
					}
					else
						putc(w, stdout);
				}
			}
			ApplyColour(CON_WHITEMASK);
		}
		else
		{
			for (t = (unsigned char*)msg; *t; t++)
			{
				if (*t >= 146 && *t < 156)
					*t = *t - 146 + '0';
				if (*t >= 0x12 && *t <= 0x1b)
					*t = *t - 0x12 + '0';
				if (*t == 143)
					*t = '.';
				if (*t == 157 || *t == 158 || *t == 159)
					*t = '-';
				if (*t >= 128)
					*t -= 128;
				if (*t == 16)
					*t = '[';
				if (*t == 17)
					*t = ']';
				if (*t == 0x1c)
					*t = 249;

				*t &= 0x7f;
				if ((*t > 128 || *t < 32) && *t != 10 && *t != 13 && *t != 9)
					printf("[%02x]", *t);
				else
					putc(*t, stdout);
			}
		}

		//and put the input line back
		if (!sys_linebuffer.value)
		{
			if (coninput_len)
				printf("]%s", coninput_text);
			else
				putch(']');
		}
	}
	else
	{
		va_start (argptr,fmt);
		vprintf (fmt,argptr);
		va_end (argptr);
	}

	fflush(stdout);
}



#if 0
/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	static char		text[2048];
	unsigned char		*p;

	if (sys_nostdout.value || SSV_IsSubServer())
		return;

	va_start (argptr,fmt);
	vsnprintf (text,sizeof(text)-1, fmt,argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
	fflush(stdout);
}

#endif

/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
#ifdef HAVE_SERVER
	SV_Shutdown();
#endif
#ifdef HAVE_GNUTLS
	GnuTLS_Shutdown();
#endif
	if (!noconinput)
	{
		tcsetattr(STDIN_FILENO, TCSADRAIN, &orig);
		fcntl (STDIN_FILENO, F_SETFL, fcntl (STDIN_FILENO, F_GETFL, 0) & ~O_NDELAY);
	}
	exit (0);		// appkit isn't running
}

#if 1
static char *Sys_LineInputChar(char *line)
{
	char c;
	while(*line)
	{
		c = *line++;
		if (c == '\r' || c == '\n')
		{
			coninput_text[coninput_len] = 0;
			putch ('\n');
			putch (']');
			coninput_len = 0;
			fflush(stdout);
			return coninput_text;
		}
		if (c == 8)
		{
			if (coninput_len)
			{
				putch (c);
				putch (' ');
				putch (c);
				coninput_len--;
				coninput_text[coninput_len] = 0;
			}
			continue;
		}
		if (c == '\t')
		{
			int i;
			char *s = Cmd_CompleteCommand(coninput_text, true, true, 0, NULL);
			if(s)
			{
				for (i = 0; i < coninput_len; i++)
					putch('\b');
				for (i = 0; i < coninput_len; i++)
					putch(' ');
				for (i = 0; i < coninput_len; i++)
					putch('\b');

				strcpy(coninput_text, s);
				coninput_len = strlen(coninput_text);
				printf("%s", coninput_text);
			}
			continue;
		}
		putch (c);
		coninput_text[coninput_len] = c;
		coninput_len++;
		coninput_text[coninput_len] = 0;
		if (coninput_len == sizeof(coninput_text))
			coninput_len = 0;
	}
	fflush(stdout);
	return NULL;
}
#endif
/*
================
Sys_ConsoleInput

Checks for a complete line of text typed in at the console, then forwards
it to the host command processor
================
*/
void Sys_Linebuffer_Callback (struct cvar_s *var, char *oldvalue)
{	//reconfigures the tty to send a char at a time (or line at a time)

	if (noconinput)
		return;	//oh noes! we already hungup!

	changes = orig;
	if (var->value)
	{
		changes.c_lflag |= (ICANON|ECHO);
	}
	else
	{
		changes.c_lflag &= ~(ICANON|ECHO);
		changes.c_cc[VTIME] = 0;
		changes.c_cc[VMIN] = 1;
	}
	tcsetattr(STDIN_FILENO, TCSADRAIN, &changes);
}

char *Sys_ConsoleInput (void)
{
	static char	text[256];
	int	len;

	if (!stdin_ready || noconinput==true)
		return NULL;		// the select didn't say it was ready
	stdin_ready = false;

//libraries and muxers and things can all screw with our stdin blocking state.
//if a server sits around waiting for its never-coming stdin then we're screwed.
//and don't assume that it won't block just because select told us it was readable, select lies.
//so force it non-blocking so we don't get any nasty surprises.
#if defined(__linux__)
	{
		int fl = fcntl (STDIN_FILENO, F_GETFL, 0);
		if (!(fl & O_NDELAY))
		{
			fcntl(STDIN_FILENO, F_SETFL, fl | O_NDELAY);
//			Sys_Printf(CON_WARNING "stdin flags became blocking - gdb bug?\n");
		}
	}
#endif

	len = read (STDIN_FILENO, text, sizeof(text)-1);
	if (len < 0)
	{
		int err = errno;
		switch(err)
		{
		case EINTR:		//unix sucks
		case EAGAIN:	//a select fuckup?
			break;
		case EIO:
			noconinput |= 2;
			stdin_ready = true;
			return NULL;
		default:
			Con_Printf("error %i reading from stdin\n", err);
			noconinput = true;	//we don't know what it was, but don't keep triggering it.
			return NULL;
		}
	}
	if (noconinput&2)
	{	//posix job stuff sucks - there's no way to detect when we're directly pushed to the foreground after being backgrounded.
		Con_Printf("Welcome back!\n");
		noconinput &= ~2;
	}

	/*if (len == 0)
	{
		// end of file? doesn't really make sense. depend upon sighup instead
		Con_Printf("EOF reading from stdin\n");
		noconinput = true;
		return NULL;
	}*/
	if (len < 1)
		return NULL;
	text[len-1] = 0;	// rip off the /n and terminate

	if (sys_linebuffer.value == 0)
		return Sys_LineInputChar(text);
	return text;
}

/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
	Sys_InitClock();

	Cvar_Register (&sys_nostdout, "System configuration");
	Cvar_Register (&sys_extrasleep,	"System configuration");

	Cvar_Register (&sys_colorconsole, "System configuration");
	Cvar_Register (&sys_timestamps, "System configuration");
	Cvar_Register (&sys_linebuffer, "System configuration");
}

void Sys_Shutdown (void)
{
}

#if defined(__linux__) && defined(__GLIBC__)
#include <execinfo.h>
#ifdef __i386__
#include <ucontext.h>
#endif
static void Friendly_Crash_Handler(int sig, siginfo_t *info, void *vcontext)
{
	int fd;
	void *array[10];
	size_t size;
	int firstframe = 0;
	char signame[32];

	switch(sig)
	{
	case SIGILL:	strcpy(signame, "SIGILL");	break;
	case SIGFPE:	strcpy(signame, "SIGFPE");	break;
	case SIGBUS:	strcpy(signame, "SIGBUS");	break;
	case SIGABRT:	strcpy(signame, "SIGABRT");	break;
	case SIGSEGV:	Q_snprintfz(signame, sizeof(signame), "SIGSEGV (%p)", info->si_addr);	break;
	default:	Q_snprintfz(signame, sizeof(signame), "%i", sig);	break;
	}

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

#if defined(__i386__)
	//x86 signals don't leave the stack in a clean state, so replace the signal handler with the real crash address, and hide this function
	{
		ucontext_t *uc = vcontext;
		array[1] = (void*)uc->uc_mcontext.gregs[REG_EIP];
		firstframe = 1;
	}
#elif defined(__amd64__)
	//amd64 is sane enough, but this function and the libc signal handler are on the stack, and should be ignored.
	firstframe = 2;
#endif

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %s:\n", signame);
	backtrace_symbols_fd(array+firstframe, size-firstframe, 2);

	fd = open("crash.log", O_WRONLY|O_CREAT|O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
	if (fd != -1)
	{
		time_t rawtime;
		struct tm * timeinfo;
		char buffer [80];

		time (&rawtime);
		timeinfo = localtime (&rawtime);
		strftime (buffer, sizeof(buffer), "Time: %Y-%m-%d %H:%M:%S\n",timeinfo);
		write(fd, buffer, strlen(buffer));

		Q_snprintfz(buffer, sizeof(buffer), "Ver: %i.%02i%s\n", FTE_VER_MAJOR, FTE_VER_MINOR,
#ifdef OFFICIAL_RELEASE
			" (official)");
#else
			"");
#endif
		write(fd, buffer, strlen(buffer));

#if defined(SVNREVISION) && defined(SVNDATE)
		Q_snprintfz(buffer, sizeof(buffer), "Revision: %s\nBinary: %s\n", STRINGIFY(SVNREVISION), STRINGIFY(SVNDATE));
#else
		Q_snprintfz(buffer, sizeof(buffer),
		#ifdef SVNREVISION
			"Revision: "STRINGIFY(SVNREVISION)"\n"
		#endif
		"Binary: "__DATE__" "__TIME__"\n");
#endif
		write(fd, buffer, strlen(buffer));

		backtrace_symbols_fd(array + firstframe, size - firstframe, fd);
		write(fd, "\n", 1);
		close(fd);
	}
	exit(1);
}
#endif

#ifdef SQL
#include "sv_sql.h"
#endif
static int Sys_CheckChRoot(void)
{	//also warns if run as root.
	int ret = false;
#ifdef __linux__
	//three ways to use this:
	//nonroot-with-SUID-root -- chroots+drops to a fixed path when run as a regular user. the homedir mechanism can be used for writing files.
	//root -chroot foo -uid bar -- requires root, changes the filesystem and then switches user rights before starting the game itself.
	//root -chroot foo -- requires root, changes the filesystem and leaves the process with far far too many rights

	uid_t ruid, euid, suid;
	int arg = COM_CheckParm("-chroot");
	const char *newroot = arg?com_argv[arg+1]:NULL;
	const char *newhome;

	getresuid(&ruid, &euid, &suid);
//	printf("ruid %u, euid %u, suid %u\n", ruid, euid, suid);
	if (!euid && ruid != euid)
	{	//if we're running SUID-root then assume the admin set it up that way in order to use chroot without making any libraries available inside the jail.
		//however, chroot needs a certain level of sandboxing to prevent somehow running suid programs with eg a custom /etc/passwd, etc.
		//this means we can't allow
		//FIXME other games. should use the list in fs.c
		if (COM_CheckParm("-quake"))
			newroot = "/usr/share/games/quake";
		else if (COM_CheckParm("-quake2"))
			newroot = "/usr/share/games/quake2";
		else if (COM_CheckParm("-quake3"))
			newroot = "/usr/share/games/quake3";
		else if (COM_CheckParm("-hexen2") || COM_CheckParm("-portals"))
			newroot = "/usr/share/games/hexen2";
		else
#ifdef GAME_SHORTNAME
			newroot = "/usr/share/games/" GAME_SHORTNAME;
#else
			newroot = "/usr/share/games/quake";
#endif

		//just read the environment name
		newhome = getenv("USER");
	}
	else
	{
		newhome = NULL;
		arg = COM_CheckParm("-uid");
		if (arg)
			ruid = strtol(com_argv[arg+1], NULL, 0);
	}

	if (newroot)
	{	//chroot requires running as root, which sucks.
		//make sure there's no suid programs in the new root dir that might get confused by /etc/ being something else.
		//this binary MUST NOT be inside the new root.

		//make sure we don't crash on any con_printfs.
#ifdef MULTITHREAD
		Sys_ThreadsInit();
#endif

		//FIXME: should we temporarily try swapping uid+euid so we don't have any more access than a non-suid binary for this initial init stuff?
		{
			struct addrinfo *info;
			if (getaddrinfo("master.quakeservers.net", NULL, NULL, &info) == 0)	//make sure we've loaded /etc/resolv.conf etc, otherwise any dns requests are going to fail, which would mean no masters.
				freeaddrinfo(info);
		}

#if defined(SQL) && defined(HAVE_SERVER)
		SQL_Available();
#endif
#ifdef HAVE_GNUTLS
		SSL_InitGlobal(false);	//we need to load the known CA certs while we still can, as well as any shared objects
		//SSL_InitGlobal(true);	//make sure we load our public cert from outside the sandbox. an exploit might still be able to find it in memory though. FIXME: disabled in case this reads from somewhere bad - we're still root.
#endif

		{	//this protects against stray setuid programs like su reading passwords from /etc/passwd et al
			//there shouldn't be anyway so really this is pure paranoia.
			//(the length thing is to avoid overflows inside va giving false negatives.)
			struct stat s;
			if (strlen(newroot) > 4096 || lstat(va("%s/etc/", newroot), &s) != -1)
			{
				printf("refusing to chroot to %s - contains an /etc directory\n", newroot);
				return -1;
			}
			if (strlen(newroot) > 4096 || lstat(va("%s/proc/", newroot), &s) != -1)
			{
				printf("refusing to chroot to %s - contains a /proc directory\n", newroot);
				return -1;
			}
		}

		printf("Changing root dir to \"%s\"\n", newroot);
		if (chroot(newroot))
		{
			printf("chroot call failed\n");
			return -1;
		}
		chdir("/");	//chroot does NOT change the working directory, so we need to make sure that happens otherwise still a way out.

		//signal to the fs.c code to use an explicit base home dir.
		if (newhome)
			setenv("FTEHOME", va("/user/%s", newhome), true);
		else
			setenv("FTEHOME", va("/user/%i", ruid), true);

		//these paths are no longer valid.
		setenv("HOME", "", true);
		setenv("XDG_DATA_HOME", "", true);

		setenv("PWD", "/", true);
	
		ret = true;
	}

	if (ruid != euid || newroot)
	{
		if (setresuid(ruid, ruid, ruid))	//go back to our original user, assuming we were SUIDed
		{
			printf("error dropping priveledges\n");
			return -1;
		}
		getresuid(&ruid, &euid, &suid);

		if (setuid(0) != -1 || errno != EPERM)
		{
			printf("priveledges were not dropped...\n");
			return -1;
		}
		getresuid(&ruid, &euid, &suid);
	}


	if (!ruid || !euid || !suid)
		printf("WARNING: you should NOT be running this as root!\n");
#endif
	return ret;
}

#ifdef _POSIX_C_SOURCE
static void SigCont(int code)
{	//lets us know when we regained foreground focus.
	int fl = fcntl (STDIN_FILENO, F_GETFL, 0);
	if (!(fl & O_NDELAY))
		fcntl(STDIN_FILENO, F_SETFL, fl | O_NDELAY);
	noconinput &= ~2;
}
#endif

/*
=============
main
=============
*/
int main(int argc, char *argv[])
{
	float maxsleep;
	quakeparms_t	parms;
//	fd_set	fdset;
//	extern	int		net_socket;
	char bindir[MAX_OSPATH];

	signal(SIGPIPE, SIG_IGN);
	tcgetattr(STDIN_FILENO, &orig);
	changes = orig;

	memset (&parms, 0, sizeof(parms));

	COM_InitArgv (argc, (const char **)argv);
	parms.argc = com_argc;
	parms.argv = com_argv;
#ifdef CONFIG_MANIFEST_TEXT
	parms.manifest = CONFIG_MANIFEST_TEXT;
#endif

	//decide if we should be printing colours to the stdout or not.
	if (COM_CheckParm("-nocolour")||COM_CheckParm("-nocolor"))
		useansicolours = false;
	else
		useansicolours = (isatty(STDOUT_FILENO) || COM_CheckParm("-colour") || COM_CheckParm("-color"));
	if (COM_CheckParm("-nostdin"))
		noconinput = true;

	switch(Sys_CheckChRoot())
	{
	case true:
		parms.basedir = "/";
		break;
	case false:
		parms.basedir = "./";
#ifdef __linux__
		{	//attempt to figure out where the exe is located
			int l = readlink("/proc/self/exe", bindir, sizeof(bindir)-1);
			if (l > 0)
			{
				bindir[l] = 0;
				*COM_SkipPath(bindir) = 0;
				printf("Binary is located at \"%s\"\n", bindir);
				parms.binarydir = bindir;
			}
		}
/*#elif defined(__bsd__)
		{	//attempt to figure out where the exe is located
			int l = readlink("/proc/self/exe", bindir, sizeof(bindir)-1);
			if (l > 0)
			{
				bindir[l] = 0;
				*COM_SkipPath(bindir) = 0;
				printf("Binary is located at "%s"\n", bindir);
				parms.binarydir = bindir;
			}
		}
*/
#endif
		break;
	default:
		return -1;
	}



#if defined(__linux__) && defined(__GLIBC__)
	if (!COM_CheckParm("-nodumpstack"))
	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		act.sa_sigaction = Friendly_Crash_Handler;
		act.sa_flags = SA_SIGINFO | SA_RESTART;
		sigaction(SIGILL, &act, NULL);
		sigaction(SIGFPE, &act, NULL);
		sigaction(SIGSEGV, &act, NULL);
		sigaction(SIGABRT, &act, NULL);
		sigaction(SIGBUS, &act, NULL);
	}
#endif

#ifdef _POSIX_C_SOURCE
	signal(SIGTTIN, SIG_IGN);	//have to ignore this if we want to not lock up when running backgrounded.
	signal(SIGCONT, SigCont);
	signal(SIGCHLD, SIG_IGN);	//mapcluster stuff might leak zombie processes if we don't do this.
#endif


#ifdef SUBSERVERS
	if (COM_CheckParm("-clusterslave"))
		isClusterSlave = true;
#endif

	TL_InitLanguages(parms.basedir);

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	maxsleep = SV_Frame();

#ifdef SUBSERVERS
	if (setjmp(sys_sv_serverforked))
		noconinput = true;
#endif

//
// main loop
//
	while (1)
	{
		if (noconinput != true)
			stdin_ready |= NET_Sleep(maxsleep, true);
		else
		{
			NET_Sleep(maxsleep, false);
			stdin_ready = false;
		}

		maxsleep = SV_Frame();

	// extrasleep is just a way to generate a fucked up connection on purpose
		if (sys_extrasleep.value)
			usleep (sys_extrasleep.value);
	}
	return 0;
}

static int Sys_EnumerateFiles2 (const char *truepath, int apathofs, const char *match, int (*func)(const char *, qofs_t, time_t modtime, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	DIR *dir;
	char file[MAX_OSPATH];
	const char *s;
	struct dirent *ent;
	struct stat st;
	const char *wild;
	const char *apath = truepath+apathofs;

	//if there's a * in a system path, then we need to scan its parent directory to figure out what the * expands to.
	//we can just recurse quicklyish to try to handle it.
	wild = strchr(apath, '*');
	if (!wild)
		wild = strchr(apath, '?');
	if (wild)
	{
		char subdir[MAX_OSPATH];
		for (s = wild+1; *s && *s != '/'; s++)
			;
		while (wild > truepath)
		{
			if (*(wild-1) == '/')
				break;
			wild--;
		}
		memcpy(file, truepath, wild-truepath);
		file[wild-truepath] = 0;

		dir = opendir(file);
		memcpy(subdir, wild, s-wild);
		subdir[s-wild] = 0;
		if (dir)
		{
			do
			{
				ent = readdir(dir);
				if (!ent)
					break;
				if (*ent->d_name != '.')
				{
					if (wildcmp(subdir, ent->d_name))
					{
						memcpy(file, truepath, wild-truepath);
						Q_snprintfz(file+(wild-truepath), sizeof(file)-(wild-truepath), "%s%s", ent->d_name, s);
						if (!Sys_EnumerateFiles2(file, apathofs, match, func, parm, spath))
						{
							closedir(dir);
							return false;
						}
					}
				}
			} while(1);
			closedir(dir);
		}
		return true;
	}


	dir = opendir(truepath);
	if (!dir)
	{
		Con_DLPrintf((errno==ENOENT)?2:1, "Failed to open dir %s\n", truepath);
		return true;
	}
	do
	{
		ent = readdir(dir);
		if (!ent)
			break;
		if (*ent->d_name != '.')
		{
			if (wildcmp(match, ent->d_name))
			{
				Q_snprintfz(file, sizeof(file), "%s/%s", truepath, ent->d_name);

				if (stat(file, &st) == 0)
				{
					Q_snprintfz(file, sizeof(file), "%s%s%s", apath, ent->d_name, S_ISDIR(st.st_mode)?"/":"");

					if (!func(file, st.st_size, st.st_mtime, parm, spath))
					{
//						Con_DPrintf("giving up on search after finding %s\n", file);
						closedir(dir);
						return false;
					}
				}
				else if (lstat(file, &st) == 0)
					;//okay, so bad symlink, just mute it
//				else
//					fprintf(stderr, "Stat failed for \"%s\"\n", file);
			}
		}
	} while(1);
	closedir(dir);
	return true;
}
int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, qofs_t, time_t modtime, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	char apath[MAX_OSPATH];
	char truepath[MAX_OSPATH];
	char *s;

	if (!gpath)
		gpath = "";
	*apath = '\0';

	Q_strncpyz(apath, match, sizeof(apath));
	for (s = apath+strlen(apath)-1; s >= apath; s--)
	{
		if (*s == '/')
		{
			s[1] = '\0';
			match += s - apath+1;
			break;
		}
	}
	if (s < apath)	//didn't find a '/'
		*apath = '\0';

	Q_snprintfz(truepath, sizeof(truepath), "%s/%s", gpath, apath);
	return Sys_EnumerateFiles2(truepath, strlen(gpath)+1, match, func, parm, spath);
}

void Sys_CloseLibrary(dllhandle_t *lib)
{
	if (sys_nounload)
		return;
	dlclose((void*)lib);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	int i;
	dllhandle_t *lib;

	lib = dlopen (name, RTLD_LAZY);
	if (!lib && !strstr(name, ARCH_DL_POSTFIX))
		lib = dlopen (va("%s"ARCH_DL_POSTFIX, name), RTLD_LAZY);
	if (!lib)
	{
		const char *err = dlerror();
		//I hate this string check
		Con_DLPrintf(strstr(err, "No such file or directory")?2:0,"%s\n", err);
		return NULL;
	}

	if (funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = dlsym(lib, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Con_DPrintf("Unable to find symbol \"%s\" in \"%s\"\n", funcs[i].name, name);
			Sys_CloseLibrary((dllhandle_t*)lib);
			lib = NULL;
		}
	}

	return (dllhandle_t*)lib;
}
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	if (!module)
		return NULL;
	return dlsym(module, exportname);
}

void Sys_ServerActivity(void)
{
}

qboolean Sys_RandomBytes(qbyte *string, int len)
{
	qboolean res = false;
	int fd = open("/dev/urandom", 0);
	if (fd != -1)
	{
		res = (read(fd, string, len) == len);
		close(fd);
	}
	return res;
}

#ifdef MANIFESTDOWNLOADS
#include "fs.h"
static qboolean Sys_DoInstall(void)
{
	char fname[MAX_QPATH];
	float pct = 0;
	qboolean applied = false;
#ifdef __unix__
	qboolean showprogress = isatty(STDOUT_FILENO);
#else
	qboolean showprogress = false;
#endif

#if 1
	FS_CreateBasedir(NULL);
#else
	char basedir[MAX_OSPATH];
	if (!FS_SystemPath("", FS_ROOT, basedir, sizeof(basedir)))
		return true;
	FS_CreateBasedir(basedir);
#endif

	*fname = 0;
	for(;;)
	{
		while(FS_DownloadingPackage())
		{
			const char *cur = "";
			float newpct = 50;
			HTTP_CL_Think(&cur, &newpct);

			if (*cur && Q_strncmp(fname, cur, sizeof(fname)-1))
			{
				Q_strncpyz(fname, cur, sizeof(fname));
				Con_Printf("Downloading: %s\n", fname);
			}
			if (showprogress && (int)(pct*10) != (int)(newpct*10))
			{
				pct = newpct;
				Sys_Printf("%5.1f%%\r", pct);
			}

			Sys_Sleep(10/1000.0);
			COM_MainThreadWork();
		}

		if (!applied)
		{
			if (!PM_MarkUpdates())
				break;	//no changes to apply
			PM_ApplyChanges();
			applied = true;	//don't keep applying.
			continue;
		}
		break;
	}
	if (showprogress)
		Sys_Printf("     \r");
	return true;
}
qboolean Sys_RunInstaller(void)
{
	if (COM_CheckParm("-install"))
	{	//install THEN run
		Sys_DoInstall();
		return false;
	}
	if (COM_CheckParm("-doinstall"))
	{
		//install only, then quit
		return Sys_DoInstall();
	}
	if (!com_installer)
		return false;
	return Sys_DoInstall();
}
#endif
