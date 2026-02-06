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
// sys.h -- non-portable functions

//
// file IO
// for the most part, we use stdio.
// if your system doesn't have stdio then urm... well.
//
void Sys_mkdir (const char *path);	//not all pre-unix systems have directories (including dos 1)
qboolean Sys_rmdir (const char *path);
qboolean Sys_remove (const char *path);
qboolean Sys_Rename (const char *oldfname, const char *newfname);
qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace);	//false for not-implemented or other error. path will be a system path, but may be relative (if basedir isn't properly known). path MAY be a file, or may be a slash-terminated directory.

//
// memory protection
//
void Sys_MakeCodeWriteable (void * startaddr, unsigned long length);

//
// system IO
//
int VARGS Sys_DebugLog(char *file, char *fmt, ...) LIKEPRINTF(2);

NORETURN void VARGS Sys_Error (const char *error, ...) LIKEPRINTF(1);
// an error will cause the entire program to exit

void VARGS Sys_Printf (char *fmt, ...) LIKEPRINTF(1);
// send text to the console
void Sys_Warn (char *fmt, ...) LIKEPRINTF(1);
//like Sys_Printf. dunno why there needs to be two of em.

char *Sys_URIScheme_NeedsRegistering(void);	//returns the name of one of the current manifests uri schemes that isn't registered (but should be registerable).
void Sys_Quit (void);
void Sys_RecentServer(char *command, char *target, char *title, char *desc);
qboolean Sys_RunInstaller(void);

typedef struct dllfunction_s {
	void **funcptr;
	char *name;
} dllfunction_t;
#define dllhandle_t void
extern qboolean sys_nounload;	//blocks Sys_CloseLibrary. set before stack trace fatal shutdowns.
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs);
void Sys_CloseLibrary(dllhandle_t *lib);
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname);
char *Sys_GetNameForAddress(dllhandle_t *module, void *address);

qboolean LibZ_Init(void);
qboolean LibJPEG_Init(void);
qboolean LibPNG_Init(void);

qboolean Sys_RunFile(const char *fname, int nlen);

unsigned int Sys_Milliseconds (void);
double Sys_DoubleTime (void);
qboolean Sys_RandomBytes(qbyte *string, int len);

qboolean Sys_ResolveFileURL(const char *inurl, int inlen, char *out, int outlen);

char *Sys_ConsoleInput (void);

typedef enum
{
	CBT_CLIPBOARD,	//ctrl+c, ctrl+v
	CBT_SELECTION,	//select-to-copy, middle-to-paste
} clipboardtype_t;
void Sys_Clipboard_PasteText(clipboardtype_t clipboardtype, void (*callback)(void *ctx, const char *utf8), void *ctx);	//calls the callback once the text is available (maybe instantly). utf8 arg may be NULL if the clipboard was unavailable.
void Sys_SaveClipboard(clipboardtype_t clipboardtype, const char *text); //a stub would do nothing.

//stuff for dynamic dedicated console -> gfx and back.
void Sys_CloseTerminal (void);
qboolean Sys_InitTerminal (void);
void Con_PrintToSys(void);

void Sys_ServerActivity(void);
//make window flash on the taskbar - someone said something/connected

void Sys_SendKeyEvents (void);
// Perform Key_Event () callbacks until the input que is empty

int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t modtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath);

void Sys_Vibrate(float count);
struct searchpathfuncs_s *Sys_OpenTitleStore(void); //sdl3

qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate);

#if defined(__GNUC__)
	#define qatomic32_t qint32_t
	#define FTE_Atomic32_Inc(ptr) __sync_add_and_fetch(ptr, 1)	//returns the AFTER the operation.
	#define FTE_Atomic32_Dec(ptr) __sync_add_and_fetch(ptr, -1)	//returns the AFTER the operation.
	#define FTE_Atomic_Insert(head, newnode, newnodenext) do newnodenext = head; while(!__sync_bool_compare_and_swap(&head, newnodenext, newnode)) //atomically insert into a linked list, being sure to not corrupt the pointers
#elif defined(_WIN32)
	#define qatomic32_t long
	#define FTE_Atomic32_Inc(ptr) _InterlockedIncrement(ptr)
	#define FTE_Atomic32_Dec(ptr) _InterlockedDecrement(ptr)
	#define FTE_Atomic_Insert(head, newnode, newnodenext) do newnodenext = head; while(newnodenext != _InterlockedCompareExchangePointer(&head, newnode, newnodenext))
#else
	#define qatomic32_t qint32_t
	#define FTE_Atomic32_Inc(ptr) FTE_Atomic32Mutex_Add(ptr, 1)
	#define FTE_Atomic32_Dec(ptr) FTE_Atomic32Mutex_Add(ptr, -1)
	#define FTE_Atomic_Insert(head, newnode, newnodenext) do newnodenext = head; while(!FTE_AtomicPtr_ConditionalReplace(&head, newnodenext, newnode))
#endif


typedef enum wgroup_e
{
	WG_MAIN		= 0,
	WG_LOADER	= 1,
	WG_COUNT	= 2 //main and loaders
} wgroup_t;
typedef struct
{
	void *(QDECL *CreateMutex)(void);
	qboolean (QDECL *LockMutex)(void *mutex);
	qboolean (QDECL *UnlockMutex)(void *mutex);
	void (QDECL *DestroyMutex)(void *mutex);

	void (*AddWork)(wgroup_t thread, void(*func)(void *ctx, void *data, size_t a, size_t b), void *ctx, void *data, size_t a, size_t b);	//low priority
	void (*WaitForCompletion)(void *priorityctx, int *address, int sleepwhilevalue);
#define plugthreadfuncs_name "Threading"
} plugthreadfuncs_t;

#ifdef MULTITHREAD
#if defined(_WIN32) && defined(_DEBUG)
void Sys_SetThreadName(unsigned int dwThreadID, char *threadName);
#endif

void Sys_ThreadsInit(void);
//qboolean Sys_IsThread(void *thread);
qboolean Sys_IsMainThread(void);
qboolean Sys_IsThread(void *thread);
void *Sys_CreateThread(char *name, int (*func)(void *), void *args, int priority, int stacksize);
void Sys_WaitOnThread(void *thread);
void Sys_DetachThread(void *thread);
void Sys_ThreadAbort(void);

#define THREADP_IDLE -5
#define THREADP_NORMAL 0
#define THREADP_HIGHEST 5

void *QDECL Sys_CreateMutex(void);
qboolean Sys_TryLockMutex(void *mutex);
qboolean QDECL Sys_LockMutex(void *mutex);
qboolean QDECL Sys_UnlockMutex(void *mutex);
void QDECL Sys_DestroyMutex(void *mutex);

/* Conditional wait calls */
void *Sys_CreateConditional(void);
qboolean Sys_LockConditional(void *condv);
qboolean Sys_UnlockConditional(void *condv);
qboolean Sys_ConditionWait(void *condv);		//lock first
qboolean Sys_ConditionSignal(void *condv);		//lock first
qboolean Sys_ConditionBroadcast(void *condv);	//lock first
void Sys_DestroyConditional(void *condv);

//to try to catch leaks more easily.
#ifdef USE_MSVCRT_DEBUG
void *Sys_CreateMutexNamed(char *file, int line);
#define Sys_CreateMutex() Sys_CreateMutexNamed(__FILE__, __LINE__)
#endif

#else
	#ifdef __GNUC__	//gcc complains about if (true) when these are maros. msvc complains about static not being called in headers. gah.
		static inline qboolean Sys_MutexStub(void) {return qtrue;}
		static inline void *Sys_CreateMutex(void) {return NULL;}
		#define Sys_IsMainThread() Sys_MutexStub()
		#define Sys_DestroyMutex(m) Sys_MutexStub()
		#define Sys_IsMainThread() Sys_MutexStub()
		#define Sys_LockMutex(m) Sys_MutexStub()
		#define Sys_UnlockMutex(m) Sys_MutexStub()
		#ifndef __cplusplus
			static inline qboolean Sys_IsThread(void *thread) {return (!thread)?qtrue:qfalse;}
		#endif
	#else
		#define Sys_IsMainThread() (qboolean)(qtrue)
		#define Sys_CreateMutex() (void*)(NULL)
		#define Sys_LockMutex(m) (qboolean)(qtrue)
		#define Sys_UnlockMutex(m) (qboolean)(qtrue)
		#define Sys_DestroyMutex(m) (void)0
		#define Sys_IsThread(t) (!t)
	#endif
#endif

void Sys_Sleep(double seconds);


#define UPD_OFF 0
#define UPD_STABLE 1
#define UPD_TESTING 2

#if defined(WEBCLIENT) && defined(PACKAGEMANAGER)
	#if defined(_WIN32) && !defined(SERVERONLY) && !defined(_XBOX)
		#define HAVEAUTOUPDATE
	#endif
	#if defined(__linux__) && !defined(ANDROID)
		#define HAVEAUTOUPDATE
	#endif
#endif

#ifdef HAVEAUTOUPDATE
qboolean Sys_SetUpdatedBinary(const char *fname);	//attempts to overwrite the working binary.
qboolean Sys_EngineMayUpdate(void);					//says whether the system code is able/allowed to overwrite itself.
#else
#define Sys_EngineMayUpdate() false
#define Sys_SetUpdatedBinary(n) false
#endif

void Sys_Init (void);
void Sys_Shutdown(void);

