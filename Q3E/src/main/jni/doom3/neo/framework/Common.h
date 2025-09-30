/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __COMMON_H__
#define __COMMON_H__

/*
==============================================================

  Common

==============================================================
*/

typedef enum {
	EDITOR_NONE					= 0,
	EDITOR_RADIANT				= BIT(1),
	EDITOR_GUI					= BIT(2),
	EDITOR_DEBUGGER				= BIT(3),
	EDITOR_SCRIPT				= BIT(4),
	EDITOR_LIGHT				= BIT(5),
	EDITOR_SOUND				= BIT(6),
	EDITOR_DECL					= BIT(7),
	EDITOR_AF					= BIT(8),
	EDITOR_PARTICLE				= BIT(9),
	EDITOR_PDA					= BIT(10),
	EDITOR_AAS					= BIT(11),
	EDITOR_MATERIAL				= BIT(12)
#ifdef _RAVEN
	,
	EDITOR_REVERB				= BIT(13),
	EDITOR_PLAYBACKS			= BIT(14),
	EDITOR_MODVIEW				= BIT(15),
	EDITOR_LOGVIEW				= BIT(16),
	EDITOR_ENTVIEW				= BIT(17),

	// Just flags to prevent caching of unneeded assets
	EDITOR_RENDERBUMP			= BIT(18),
	EDITOR_SPAWN_GUI			= BIT(19),

	// Specifies that a decl validation run is happening
	EDITOR_DECL_VALIDATING		= BIT(20),

	EDITOR_FX = BIT(21), // 9

	EDITOR_ALL					= -1
#endif
} toolFlag_t;

#ifdef _RAVEN
// RAVEN BEGIN
// mekberg: added more save types
typedef enum {
	ST_REGULAR,
	ST_QUICK,
	ST_AUTO,
	ST_CHECKPOINT,
} saveType_t;
// RAVEN END
#endif

#define STRTABLE_ID				"#str_"
#define STRTABLE_ID_LENGTH		5

extern idCVar		com_version;
extern idCVar		com_skipRenderer;
extern idCVar		com_asyncInput;
extern idCVar		com_asyncSound;
extern idCVar		com_machineSpec;
extern idCVar		com_purgeAll;
extern idCVar		com_developer;
extern idCVar		com_allowConsole;
extern idCVar		com_speeds;
extern idCVar		com_showFPS;
extern idCVar		com_showMemoryUsage;
extern idCVar		com_showAsyncStats;
extern idCVar		com_showSoundDecoders;
extern idCVar		com_makingBuild;
extern idCVar		com_updateLoadSize;
extern idCVar		com_videoRam;

extern int			time_gameFrame;			// game logic time
extern int			time_gameDraw;			// game present time
extern int			time_frontend;			// renderer frontend time
extern int			time_backend;			// renderer backend time

extern int			com_frameTime;			// time for the current frame in milliseconds
extern volatile int	com_ticNumber;			// 60 hz tics, incremented by async function
extern int			com_editors;			// current active editor(s)
extern bool			com_editorActive;		// true if an editor has focus

#ifdef _WIN32
const char			DMAP_MSGID[] = "DMAPOutput";
const char			DMAP_DONE[] = "DMAPDone";
extern HWND			com_hwndMsg;
extern bool			com_outputMsg;
#endif

struct MemInfo_t {
	idStr			filebase;

	int				total;
	int				assetTotals;

	// memory manager totals
	int				memoryManagerTotal;

	// subsystem totals
	int				gameSubsystemTotal;
	int				renderSubsystemTotal;

	// asset totals
	int				imageAssetsTotal;
	int				modelAssetsTotal;
	int				soundAssetsTotal;
#ifdef _RAVEN
	int				animsAssetsCount;
	int				animsAssetsTotal;

	int				aasAssetsCount;
	int				aasAssetsTotal;
#endif
#ifdef _HUMANHEAD
	int				animAssetsTotal;	// HUMANHEAD pdm
#endif
};
#ifdef _RAVEN
typedef MemInfo_t MemInfo;

// RAVEN BEGIN
// bdube: forward declarations
class idInterpreter;
class idProgram;

// RAVEN BEGIN
// bdube: added timing dict
extern bool                        com_debugHudActive;                // The debug hud is active in the game
// RAVEN END
#endif

class idCommon
{
	public:
		virtual						~idCommon(void) {}

		// Initialize everything.
		// if the OS allows, pass argc/argv directly (without executable name)
		// otherwise pass the command line in a single string (without executable name)
		virtual void				Init(int argc, const char **argv, const char *cmdline) = 0;

		// Shuts down everything.
		virtual void				Shutdown(void) = 0;

		// Shuts down everything.
		virtual void				Quit(void) = 0;

		// Returns true if common initialization is complete.
		virtual bool				IsInitialized(void) const = 0;

		// Called repeatedly as the foreground thread for rendering and game logic.
		virtual void				Frame(void) = 0;

		// Called repeatedly by blocking function calls with GUI interactivity.
		virtual void				GUIFrame(bool execCmd, bool network) = 0;

		// Called 60 times a second from a background thread for sound mixing,
		// and input generation. Not called until idCommon::Init() has completed.
		virtual void				Async(void) = 0;

		// Checks for and removes command line "+set var arg" constructs.
		// If match is NULL, all set commands will be executed, otherwise
		// only a set with the exact name.  Only used during startup.
		// set once to clear the cvar from +set for early init code
		virtual void				StartupVariable(const char *match, bool once) = 0;

		// Initializes a tool with the given dictionary.
		virtual void				InitTool(const toolFlag_t tool, const idDict *dict) = 0;

		// Activates or deactivates a tool.
		virtual void				ActivateTool(bool active) = 0;

		// Writes the user's configuration to a file
		virtual void				WriteConfigToFile(const char *filename) = 0;

		// Writes cvars with the given flags to a file.
		virtual void				WriteFlaggedCVarsToFile(const char *filename, int flags, const char *setCmd) = 0;

		// Begins redirection of console output to the given buffer.
		virtual void				BeginRedirect(char *buffer, int buffersize, void (*flush)(const char *)) = 0;

		// Stops redirection of console output.
		virtual void				EndRedirect(void) = 0;

		// Update the screen with every message printed.
		virtual void				SetRefreshOnPrint(bool set) = 0;

		// Prints message to the console, which may cause a screen update if com_refreshOnPrint is set.
		virtual void				Printf(const char *fmt, ...)id_attribute((format(printf,2,3))) = 0;

		// Same as Printf, with a more usable API - Printf pipes to this.
		virtual void				VPrintf(const char *fmt, va_list arg) = 0;

		// Prints message that only shows up if the "developer" cvar is set,
		// and NEVER forces a screen update, which could cause reentrancy problems.
		virtual void				DPrintf(const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

		// Prints WARNING %s message and adds the warning message to a queue for printing later on.
		virtual void				Warning(const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

		// Prints WARNING %s message in yellow that only shows up if the "developer" cvar is set.
		virtual void				DWarning(const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

		// Prints all queued warnings.
		virtual void				PrintWarnings(void) = 0;

		// Removes all queued warnings.
		virtual void				ClearWarnings(const char *reason) = 0;

		// Issues a C++ throw. Normal errors just abort to the game loop,
		// which is appropriate for media or dynamic logic errors.
		virtual void				Error(const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

		// Fatal errors quit all the way to a system dialog box, which is appropriate for
		// static internal errors or cases where the system may be corrupted.
		virtual void				FatalError(const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

		// Returns a pointer to the dictionary with language specific strings.
		virtual const idLangDict 	*GetLanguageDict(void) = 0;

#ifdef _RAVEN
		virtual const char* GetLocalizedString(const char* key, int langIndex) = 0;
		virtual const char* GetLocalizedString(const char* key) = 0;
		virtual int GetUserCmdMSec(void) = 0;
		virtual int GetUserCmdHz(void) = 0;

// RAVEN BEGIN
// bdube: new exports
								// Modview thinks in the middle of a game frame
	virtual void				ModViewThink ( void ) = 0;

// rjohnson: added option for guis to always think
	virtual void				RunAlwaysThinkGUIs ( int time ) = 0;

								// Debbugger hook to check if a breakpoint has been hit
	virtual void				DebuggerCheckBreakpoint ( idInterpreter* interpreter, idProgram* program, int instructionPointer ) = 0;

// scork: need to test if validating to catch some model errors that would stop the validation and convert to warnings...
	virtual bool				DoingDeclValidation( void ) = 0;

	virtual void				LoadToolsDLL( void ) = 0;

// mekberg: added
	virtual int					GetRModeForMachineSpec( int machineSpec ) const = 0;
	virtual void				SetDesiredMachineSpec( int machineSpec ) = 0;
// RAVEN END
#endif

		// Returns key bound to the command
		virtual const char 		*KeysFromBinding(const char *bind) = 0;

		// Returns the binding bound to the key
		virtual const char 		*BindingFromKey(const char *key) = 0;

		// Directly sample a button.
		virtual int					ButtonState(int key) = 0;

		// Directly sample a keystate.
		virtual int					KeyState(int key) = 0;

#ifdef _HUMANHEAD
        // HUMANHEAD pdm
        virtual void				FixupKeyTranslations(const char *src, char *dst, int lengthAllocated) = 0;
        virtual void				MaterialKeyForBinding(const char *binding, char *keyMaterial, char *key, bool &isBound) = 0;

        //HUMANHEAD rww
        virtual void				SetGameSensitivityFactor(float factor) = 0; //allows game logic to set a sensitivity factor for input
        //HUMANHEAD END
#endif
};

extern idCommon 		*common;

#ifdef _HUMANHEAD
// Profiling not enabled, compile it out
#define PROFILE_START(n, m)
#define PROFILE_STOP(n, m)
#define PROFILE_SCOPE(n, m)

#define PROFILE_START_EXPENSIVE(n, m)
#define PROFILE_STOP_EXPENSIVE(n, m)
#define PROFILE_SCOPE_EXPENSIVE(n, m)
#endif

//k for Android large stack memory allocate limit
#define _DYNAMIC_ALLOC_STACK_OR_HEAP

#if 0
#define _ALLOC_DEBUG(x) x
#else
#define _ALLOC_DEBUG(x)
#endif

#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP

#ifdef __ANDROID__
#define _DYNAMIC_ALLOC_MAX_STACK "262144" // 256k
#else
#define _DYNAMIC_ALLOC_MAX_STACK "524288" // 512k
#endif

#define _DYNAMIC_ALLOC_CVAR_DECL idCVar harm_r_maxAllocStackMemory("harm_r_maxAllocStackMemory", _DYNAMIC_ALLOC_MAX_STACK, CVAR_INTEGER|CVAR_RENDERER|CVAR_ARCHIVE, "Control allocate temporary memory when load model data, default value is `" _DYNAMIC_ALLOC_MAX_STACK "` bytes(Because stack memory is limited by OS:\n 0 = Always heap;\n Negative = Always stack;\n Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory)).")
#define _DYNAMIC_ALLOC_CVAR_EXTERN extern idCVar harm_r_maxAllocStackMemory

#if 1
extern idCVar harm_r_maxAllocStackMemory;
#define HARM_MAX_STACK_ALLOC_SIZE (harm_r_maxAllocStackMemory.GetInteger())
#else
#define HARM_MAX_STACK_ALLOC_SIZE (1024 * 512)
#endif

struct idAllocAutoHeap {
public:
    idAllocAutoHeap()
            : data(NULL)
    { }

    ~idAllocAutoHeap() {
        Free();
    }

    void * Alloc(size_t size) {
        Free();
        data = calloc(size, 1);
        _ALLOC_DEBUG(common->Printf("%p alloca on heap memory %p(%zu bytes)\n", this, data, size));
        return data;
    }

    void * Alloc16(size_t size) {
        Free();
        data = calloc(size + 15, 1);
        void *ptr = ((void *)(((intptr_t)data + 15) & ~15));
        _ALLOC_DEBUG(common->Printf("%p alloca16 on heap memory %p(%zu bytes) <- %p(%zu bytes)\n", this, ptr, size, data, size + 15));
        return ptr;
    }

    bool IsAlloc(void) const {
        return data != NULL;
    }

private:
    void *data;

    void Free(void) {
        if(data) {
            _ALLOC_DEBUG(common->Printf("%p free alloca16 heap memory %p\n", this, data));
            free(data);
            data = NULL;
        }
    }
    void * operator new(size_t);
    void * operator new[](size_t);
    void operator delete(void *);
    void operator delete[](void *);
    idAllocAutoHeap(const idAllocAutoHeap &);
    idAllocAutoHeap & operator=(const idAllocAutoHeap &);
};

// alloc in heap memory
#define _alloca16_heap( x )					((void *)((((intptr_t)calloc( (x)+15 ,1 )) + 15) & ~15))

// Using heap memory. Also reset RLIMIT_STACK by call `setrlimit`.
#define _DROID_ALLOC16_DEF(T, varname, alloc_size) \
	T *varname; \
	_DROID_ALLOC16(T, varname, alloc_size)

#define _DROID_ALLOC16(T, varname, alloc_size) \
	idAllocAutoHeap _allocAutoHeap##_##varname; \
    size_t _alloc_size##_##varname = alloc_size; \
	varname = (T *) (HARM_MAX_STACK_ALLOC_SIZE == 0 || (HARM_MAX_STACK_ALLOC_SIZE > 0 && (_alloc_size##_##varname) >= HARM_MAX_STACK_ALLOC_SIZE) ? _allocAutoHeap##_##varname.Alloc16(_alloc_size##_##varname) : _alloca16(_alloc_size##_##varname)); \
	if(_allocAutoHeap##_##varname.IsAlloc()) { \
		_ALLOC_DEBUG(common->Printf("Alloca16 on heap memory %s %p(%zu bytes)\n", #varname, varname, _alloc_size##_##varname)); \
	}

#define _DROID_ALLOC_DEF(T, varname, alloc_size) \
	T *varname;                                     \
    _DROID_ALLOC(T, varname, alloc_size);

#define _DROID_ALLOC(T, varname, alloc_size) \
	idAllocAutoHeap _allocAutoHeap##_##varname; \
    size_t _alloc_size##_##varname = alloc_size; \
	varname = (T *) (HARM_MAX_STACK_ALLOC_SIZE == 0 || (HARM_MAX_STACK_ALLOC_SIZE > 0 && (_alloc_size##_##varname) >= HARM_MAX_STACK_ALLOC_SIZE) ? _allocAutoHeap##_##varname.Alloc(_alloc_size##_##varname) : _alloca(_alloc_size##_##varname)); \
	if(_allocAutoHeap##_##varname.IsAlloc()) { \
		_ALLOC_DEBUG(common->Printf("Alloca on heap memory %s %p(%zu bytes)\n", #varname, varname, _alloc_size##_##varname)); \
	}

// free memory when not call alloca()
#define _DROID_FREE(varname) \
	{ \
		_ALLOC_DEBUG(common->Printf("Free alloca heap memory %p\n", varname)); \
	}

#endif

#endif /* !__COMMON_H__ */
