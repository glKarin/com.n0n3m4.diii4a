#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#ifdef FTEENGINE
	//included from fte itself, to borrow typedefs
#elif defined(FTEPLUGIN)
	//plugin that needs fte internals
	#include "quakedef.h"
#else
	//moderately generic plugin
	#ifdef __cplusplus
		typedef enum {qfalse, qtrue} qboolean;
	#else
		typedef enum {qfalse, qtrue} qboolean;
		#define false qfalse
		#define true qtrue
	#endif
	typedef float vec4_t[4];
	typedef float vec3_t[3];
	typedef float vec2_t[2];
	typedef unsigned char qbyte;

	#include <stdint.h>
	#define qint64_t int64_t
	#define quint64_t uint64_t
	typedef quint64_t qofs_t;

	typedef struct cvar_s cvar_t;
	typedef struct usercmd_s usercmd_t;
	typedef struct vfsfile_s vfsfile_t;
	typedef struct netadr_s netadr_t;
	enum fs_relative;
	struct searchpathfuncs_s;
#endif

#ifdef _WIN32
#	ifndef strcasecmp
#		define strcasecmp stricmp
#		define strncasecmp strnicmp
#	endif
#	if defined(_MSC_VER) && _MSC_VER >= 1900
#		define Q_vsnprintf vsnprintf
#		define Q_snprintf snprintf
#	endif
#else
#	define stricmp strcasecmp
#	define strnicmp strncasecmp
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#ifndef _VM_H
	#if __STDC_VERSION__ >= 199901L || defined(__GNUC__)
		//C99 has a stdint header which hopefully contains an intptr_t
		//its optional... but if its not in there then its unlikely you'll actually be able to get the engine to a stage where it *can* load anything
		#include <stdint.h>
		#define qintptr_t intptr_t
		#define quintptr_t uintptr_t
	#else
		#ifdef _WIN64
			typedef long long qintptr_t;
			typedef unsigned long long quintptr_t;
		#else
			#if !defined(_MSC_VER) || _MSC_VER < 1300
				#define __w64
			#endif
			typedef long __w64 qintptr_t;
			typedef unsigned long __w64 quintptr_t;
		#endif
	#endif
#endif

#ifndef NATIVEEXPORT
	#ifdef _WIN32
		#define NATIVEEXPORTPROTO __declspec(dllexport)
		#define NATIVEEXPORT NATIVEEXPORTPROTO
	#else
		#define NATIVEEXPORTPROTO
		#define NATIVEEXPORT __attribute__((visibility("default")))
	#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

//DLLs need a wrapper to add the extra parameter and call a boring function.
#ifndef QDECL
#ifdef _WIN32
#define QDECL __cdecl
#else
#define QDECL
#endif
#endif

#ifndef LIKEPRINTF
	#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
		#define LIKEPRINTF(x) __attribute__((format(printf,x,x+1)))
	#else
		#define LIKEPRINTF(x)
	#endif
#endif

#ifndef NATIVEEXPORT
#define NATIVEEXPORT QDECL
#endif

typedef int qhandle_t;
typedef void* funcptr_t;


#define PLUGMAX_SCOREBOARDNAME 64
typedef struct {
	int topcolour;
	int bottomcolour;
	int frags;
	char name[PLUGMAX_SCOREBOARDNAME];
	int ping;
	int pl;
	int activetime;
	int userid;
	int spectator;
	char userinfo[2048];
	char team[64];
} plugclientinfo_t;

typedef struct
{
	unsigned int client;
	unsigned int items;
	float armor;
	float health;
	vec3_t org;
	char nick[16];
} teamplayerinfo_t;

typedef struct {
	size_t structsize;
	int seats;
	struct
	{
		float s_avg;
		float s_mn;
		float s_mx;
		float ms_stddev;	//calculated in milliseconds for more sane numbers
		float fr_avg;
		int fr_mn;
		int fr_mx;
	} ping;
	struct
	{	//decimals
		float dropped;
		float choked;
		float invalid;
	} loss;
	float mlatency;
	float mrate;
	float vlatency;
	float vrate;
	vec3_t speed;	//player speed

	struct
	{
		float in_pps;
		float in_bps;
		float out_pps;
		float out_bps;
	} clrate;
	struct
	{
		float in_pps;
		float in_bps;
		float out_pps;
		float out_bps;
	} svrate;
	int capturing;	//avi capturing
} plugnetinfo_t;

struct wstats_s;


#define F(t, n, args) t (QDECL *n) args
#define dllhandle_t void
struct dllfunction_s;
struct zonegroup_s;
typedef struct	//core stuff
{
	//Basic builtins:
	F(void*,	GetEngineInterface,	(const char *interfacename, size_t structsize));	//retrieve a named interface struct from the engine
	F(qboolean, ExportFunction,		(const char *funcname, funcptr_t funcptr));			//export a named function to the engine
	F(qboolean, ExportInterface,	(const char *interfacename, void *interfaceptr, size_t structsize)); //export a named interface struct to the engine
	F(qboolean, GetPluginName,		(int plugnum, char *buffer, size_t bufsize));				//query loaded plugin names. -1 == active plugin
	F(void,		Print,				(const char *message));	//print on (main) console.
	F(void,		Error,				(const char *message, ...));	//abort the entire engine.
	F(void,		EndGame,			(const char *reason, ...));	//some sort of networking problem happened and we need to disconnect. Engine can continue running (displaying the message to the user).
	F(quintptr_t,GetMilliseconds,	(void));
	F(double,	GetSeconds,			(void));

	//for soft linking (with more readable error messages).
	F(dllhandle_t*,LoadDLL,			(const char *modulename, struct dllfunction_s *funcs));
	F(void*,	GetDLLSymbol,		(dllhandle_t *handle, const char *symbolname));
	F(void,		CloseDLL,			(dllhandle_t *handle));	//not guarenteed to actually do anything, of course.

	//general memory (mallocs and frees over dll boundaries is not usable on windows)
	F(void*,	Malloc,				(size_t size));
	F(void*,	Realloc,			(void *memptr, size_t size));   //doesn't zero-fill, so faster (when memptr is NULL).
	F(void,		Free,				(void *memptr));

	//for lazy mallocs
	F(void*,	GMalloc,			(struct zonegroup_s *ctx, size_t size));
	F(void,		GFree,				(struct zonegroup_s *ctx, void *ptr));
	F(void,		GFreeAll,			(struct zonegroup_s *ctx));
#define plugcorefuncs_name "Core"
} plugcorefuncs_t;

typedef struct	//subconsole handling
{
	F(qhandle_t,POpen,				(const char *conname));
	F(qboolean,	SubPrint,			(const char *subname, const char *text));	//on to sub console.
	F(qboolean,	RenameSub,			(const char *oldname, const char *newname));	//rename a console.
	F(qboolean,	IsActive,			(const char *conname));
	F(qboolean,	SetActive,			(const char *conname));
	F(qboolean,	Destroy,			(const char *conname));
	F(qboolean,	NameForNum,			(qintptr_t connum, char *conname, size_t connamelen));
	F(float,	GetConsoleFloat,	(const char *conname, const char *attribname));
	F(qboolean,	SetConsoleFloat,	(const char *conname, const char *attribname, float newvalue));
	F(qboolean,	GetConsoleString,	(const char *conname, const char *attribname, char *outvalue, size_t valuesize));
	F(qboolean,	SetConsoleString,	(const char *conname, const char *attribname, const char *newvalue));
#define plugsubconsolefuncs_name "SubConsole"
} plugsubconsolefuncs_t;

enum com_tokentype_e;
typedef struct	//console command/tokenizing/cbuf functions
{
	F(const char *,QuotedString,	(const char *string, char *buf, int buflen, qboolean omitquotes));	//generates a string with c-style markup and relevant quote types.
	F(char *,	ParseToken,			(const char *data, char *token, size_t tokenlen, enum com_tokentype_e *tokentype));	//standard quake-style token parsing.
	F(char *,	ParsePunctuation,	(const char *data, const char *punctuation, char *token, size_t tokenlen, enum com_tokentype_e *tokentype));	//use explicit punctuation.

	F(void,		TokenizeString,		(const char *msg));	//tokenize a string.
	F(void,		ShiftArgs,			(int args));	//updates tokenize state to ignore arg 0 (and updates Args).

	F(void,		Args,				(char *buffer, int bufsize));	//Gets the extra args
	F(char *,	Argv,				(int argnum, char *buffer, size_t bufsize));	//Gets a 0-based token
	F(int,		Argc,				(void));	//gets the number of tokens available.

	F(qboolean,	IsInsecure,			(void));
	F(qboolean,	AddCommand,			(const char *cmdname, void (*func)(void), const char *desc));	//Registers a console command.

	F(void,		AddText,			(const char *text, qboolean insert));
#define plugcmdfuncs_name "Cmd"
} plugcmdfuncs_t;

typedef struct	//console command and cbuf functions
{
	F(void,		SetString,			(const char *name, const char *value));
	F(void,		SetFloat,			(const char *name, float value));
	F(qboolean,	GetString,			(const char *name, char *retstring, quintptr_t sizeofretstring));
	F(float,	GetFloat,			(const char *name));
	F(cvar_t*,	GetNVFDG,			(const char *name, const char *defaultval, unsigned int flags, const char *description, const char *groupname));
	F(void,		ForceSetString,		(const char *name, const char *value));
#define plugcvarfuncs_name "Cvar"
} plugcvarfuncs_t;

typedef struct
{
	F(void,			LocalSound,			(const char *soundname, int channel, float volume));
	F(void,			RawAudio,			(int sourceid, void *data, int speed, int samples, int channels, int width, float volume));

	F(void,			Spacialize,			(unsigned int seat, int entnum, vec3_t origin, vec3_t *axis, int reverb, vec3_t velocity));
	F(qboolean,		UpdateReverb,		(size_t slot, void *reverb, size_t reverbsize));
	F(struct sfx_s*,PrecacheSound,		(const char *sample));
	F(void,			StartSound,			(int entnum, int entchannel, struct sfx_s *sfx, vec3_t origin, vec3_t velocity, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags));
	F(float,		GetChannelLevel,	(int entnum, int entchannel));
	F(int,			Voip_ClientLoudness,(unsigned int plno));
	F(qboolean,		ChangeMusicTrack,	(const char *initialtrack, const char *looptrack));

#define plugaudiofuncs_name "Audio"
} plugaudiofuncs_t;

typedef struct
{
	F(void,		BlockSizeForEncoding,(uploadfmt_t encoding, unsigned int *blockbytes, unsigned int *blockwidth, unsigned int *blockheight, unsigned int *blockdepth));
	F(const char *,FormatName,		(uploadfmt_t encoding));
#define plugimagefuncs_name "Image"
} plugimagefuncs_t;

typedef struct	//q1 client/network info
{
	F(int,		GetStats,			(int seat, unsigned int *stats, int maxstats));
	F(void,		GetPlayerInfo,		(int seat, plugclientinfo_t *info));
	F(size_t,	GetNetworkInfo,		(plugnetinfo_t *ni, size_t sizeofni));
	F(size_t,	GetLocalPlayerNumbers,(size_t firstseat, size_t numseats, int *playernums, int *spectracks));
	F(void,		GetLocationName,	(const float *pos, char *outbuffer, size_t bufferlen));
	F(qboolean,	GetLastInputFrame,	(int seat, usercmd_t *outcmd));
	F(void,		GetServerInfoRaw,	(char *info, size_t infolen));
	F(size_t,	GetServerInfoBlob,	(const char *keyname, void *buf, size_t bufsize)); //pass null buf to query size, returns 0 if it would truncate. does not null terminate.
	F(void,		SetUserInfo,		(int seat, const char *key, const char *value));
	F(void,		SetUserInfoBlob,	(int seat, const char *key, const void *value, size_t size));
	F(size_t,	GetUserInfoBlob,	(int seat, const char *key, void *buf, size_t bufsize)); //pass null buf to query size, returns 0 if it would truncate. does not null terminate.
	//EBUILTIN(void, SCR_CenterPrint, (const char *s));

	//FIXME: does this belong here?
	F(qboolean,	MapLog_Query,		(const char *packagename, const char *mapname, float *stats));

	F(size_t,	GetTeamInfo,		(teamplayerinfo_t *clients, size_t maxclients, qboolean showenemies, int seat));
	F(int,		GetWeaponStats,		(int player, struct wstats_s *result, size_t maxresults));
	F(float,	GetTrackerOwnFrags,	(int seat, char *text, size_t textsize));
	F(void,		GetPredInfo,		(int seat, vec3_t outvel));

	F(void,		ClearNotify,		(void));	//called for fast map restarts.
	F(void,		ClearClientState,	(void));	//called at the start of map changes.
	F(void,		SetLoadingState,	(qboolean newstate));	//Change the client's loading screen state.
	F(void,		UpdateGameTime,		(double));	//tells the client an updated snapshot time for interpolation/timedrift.

	void (*ForceCheatVars)			(qboolean semicheats, qboolean absolutecheats);
	qboolean (*DownloadBegun)(qdownload_t *dl);
	void (*DownloadFinished)(qdownload_t *dl);
	downloadlist_t *(*DownloadFailed)(const char *name, qdownload_t *qdl, enum dlfailreason_e failreason);
#define plugclientfuncs_name "Client2"
} plugclientfuncs_t;

struct menu_s;
typedef struct	//for menu-like stuff
{
	//for menus
	F(qboolean,		SetMenuFocus,		(qboolean wantkeyfocus, const char *cursorname, float hot_x, float hot_y, float scale)); //null cursorname=relmouse, set/empty cursorname=absmouse
	F(qboolean,		HasMenuFocus,		(void));

	F(void,			Menu_Push,			(struct menu_s *menu, qboolean prompt));
	F(void,			Menu_Unlink,		(struct menu_s *menu, qboolean forced));

	//for menu input
	F(int,			GetKeyCode,			(const char *keyname, int *out_modifier));
	F(const char*,	GetKeyName,			(int keycode, int modifier));
	F(int,			FindKeysForCommand,	(int bindmap, const char *command, int *out_keycodes, int *out_modifiers, int maxkeys));
	F(const char*,	GetKeyBind,			(int bindmap, int keynum, int modifier));
	F(void,			SetKeyBind,			(int bindmap, int keycode, int modifier, const char *newbinding));

	F(qboolean,		IsKeyDown,			(int keycode));
	F(void,			ClearKeyStates,		(void));	//forget any keys that are still held.
	F(void,			SetSensitivityScale,(float newsensitivityscale));	//this is a temporary sensitivity thing.
	F(unsigned int,	GetMoveCount,		(void));
	F(usercmd_t*,	GetMoveEntry,		(unsigned int move));	//GetMoveEntry(GetMoveCount()) gives you the partial entry. forgotten entries return NULL.

	void (*ClipboardGet) (clipboardtype_t clipboardtype, void (*callback)(void *ctx, const char *utf8), void *ctx);
	void (*ClipboardSet) (clipboardtype_t clipboardtype, const char *utf8);
	unsigned int (*utf8_decode)(int *error, const void *in, char const**out);
	unsigned int (*utf8_encode)(void *out, unsigned int unicode, int maxlen);

	unsigned int (*GetKeyDest)		(void);
	void (*KeyEvent)				(unsigned int devid, int down, int keycode, int unicode);
	void (*MouseMove)				(unsigned int devid, int abs, float x, float y, float z, float size);
	void (*JoystickAxisEvent)		(unsigned int devid, int axis, float value);
	void (*Accelerometer)			(unsigned int devid, float x, float y, float z);
	void (*Gyroscope)				(unsigned int devid, float pitch, float yaw, float roll);
	qboolean (*SetHandPosition)		(const char *devname, vec3_t org, vec3_t ang, vec3_t vel, vec3_t avel);	//for VR.
#define pluginputfuncs_name "Input"
} pluginputfuncs_t;

#if defined(FTEENGINE) || defined(FTEPLUGIN)
typedef struct
{
	F(void,		BeginReading,		(sizebuf_t *sb, struct netprim_s prim));
	F(int,		ReadCount,			(void));
	F(int,		ReadBits,			(int bits));
	F(int,		ReadByte,			(void));
	F(int,		ReadShort,			(void));
	F(int,		ReadLong,			(void));
	F(void,		ReadData,			(void *data, int len));
	F(char*,	ReadString,			(void));

	F(void,		BeginWriting,		(sizebuf_t *sb, struct netprim_s prim, void *bufferstorage, size_t buffersize));
	F(void,		WriteBits,			(sizebuf_t *sb, int value, int bits));
	F(void,		WriteByte,			(sizebuf_t *sb, int c));
	F(void,		WriteShort,			(sizebuf_t *sb, int c));
	F(void,		WriteLong,			(sizebuf_t *sb, int c));
	F(void,		WriteData,			(sizebuf_t *sb, const void *data, int len));
	F(void,		WriteString,		(sizebuf_t *sb, const char *s));

	F(qboolean,	CompareAdr,			(netadr_t *a, netadr_t *b));
	F(qboolean,	CompareBaseAdr,		(netadr_t *a, netadr_t *b));
	F(char*,	AdrToString,		(char *s, int len, netadr_t *a));
	F(size_t,	StringToAdr,		(const char *s, int defaultport, netadr_t *a, size_t addrcount, const char **pathstart));
	F(neterr_t,	SendPacket,			(struct ftenet_connections_s *col, int length, const void *data, netadr_t *to));
#ifdef HUFFNETWORK
	F(huffman_t*,Huff_CompressionCRC,	(int crc));
	F(void,		Huff_EncryptPacket,		(sizebuf_t *msg, int offset));
	F(void,		Huff_DecryptPacket,		(sizebuf_t *msg, int offset));
#endif
#define plugmsgfuncs_name "Messaging"
} plugmsgfuncs_t;
#endif

typedef struct	//for huds and menus alike
{
	F(qboolean,	GetVideoSize,	(float *vsize, unsigned int *psize));	//returns false if there's no video yet...
	//note: these use handles instead of shaders, to make them persistent over renderer restarts.
	F(qhandle_t,LoadImageData,	(const char *name, const char *mime, void *data, size_t datasize));	//load/replace a named texture
	F(qhandle_t,LoadImageShader,(const char *name, const char *defaultshader));	//loads a shader.
	F(qhandle_t,LoadImage,		(const char *name));	//wad image is ONLY for loading out of q1 gfx.wad. loads a shader. use gfx/foo.lmp for hud stuff.
	F(struct shader_s*,ShaderFromId,	(qhandle_t shaderid));
	F(void,		UnloadImage,	(qhandle_t image));
	F(int,		Image,			(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t image));
	F(int,		Image2dQuad,	(const vec2_t *points, const vec2_t *tcoords, const vec4_t *colours, qhandle_t image));
	F(int,		ImageSize,		(qhandle_t image, float *x, float *y));
	F(void,		Fill,			(float x, float y, float w, float h));
	F(void,		Line,			(float x1, float y1, float x2, float y2));
	F(void,		Character,		(float x, float y, unsigned int character));
	F(void,		String,			(float x, float y, const char *string));
	F(void,		CharacterH,		(float x, float y, float h, unsigned int flags, unsigned int character));
	F(void,		StringH,		(float x, float y, float h, unsigned int flags, const char *string));	//returns the vpixel width of the (coloured) string, in the current (variable-width) font.
	F(float,	StringWidth,	(float h, unsigned int flags, const char *string));
	F(void,		Colourpa,		(int palcol, float a));	//for legacy code
	F(void,		Colour4f,		(float r, float g, float b, float a));

	F(void,		RedrawScreen,	(void));	//redraws the entire screen and presents it. for loading screen type things.

	F(void,		LocalSound,		(const char *soundname, int channel, float volume));

	struct
	{
		//basic media poking
		F(struct cin_s *,	GetCinematic,	(struct shader_s *s));
		F(void,				SetState,		(struct cin_s *cin, int newstate));
		F(int,				GetState,		(struct cin_s *cin));
		F(void,				Reset,			(struct cin_s *cin));

		//complex media poking (web browser stuff)
		F(void,				Command,		(struct cin_s *cin, const char *command));
		F(const char *,		GetProperty,	(struct cin_s *cin, const char *key));
		F(void,				MouseMove,		(struct cin_s *cin, float x, float y));
		F(void,				Resize,			(struct cin_s *cin, int x, int y));
		F(void,				GetSize,		(struct cin_s *cin, int *x, int *y, float *aspect));
		F(void,				KeyEvent,		(struct cin_s *cin, int button, int unicode, int event));
	} media;
#define plug2dfuncs_name "2D"
} plug2dfuncs_t;

#if defined(FTEENGINE) || defined(FTEPLUGIN)
struct entity_s;
typedef struct
{
	struct
	{
		float x,y,w,h;
	} rect;
	vec2_t fov;
	vec2_t fov_viewmodel;
	vec3_t viewaxisorg[4];
	vec3_t skyroom_org;
	float time;
	unsigned int flags;
} plugrefdef_t;
typedef struct	//for huds and menus alike
{
	F(model_t *,	LoadModel,			(const char *modelname, enum mlverbosity_e sync));
	F(qhandle_t,	ModelToId,			(model_t *model));
	F(model_t *,	ModelFromId,		(qhandle_t modelid));

	F(void,			RemapShader,		(const char *sourcename, const char *destname, float timeoffset));
	F(qhandle_t,	ShaderForSkin,		(qhandle_t modelid, int surfaceidx, int skinnum, float time));
	F(skinid_t,		RegisterSkinFile,	(const char *skinname));
	F(skinfile_t *,	LookupSkin,			(skinid_t id));
	F(int,			TagNumForName,		(struct model_s *model, const char *name, int firsttag));
	F(qboolean,		GetTag,				(struct model_s *model, int tagnum, framestate_t *framestate, float *transforms));
	F(void,			ClipDecal,			(struct model_s *mod, vec3_t center, vec3_t normal, vec3_t tangent1, vec3_t tangent2, float size, unsigned int surfflagmask, unsigned int surflagmatch, void (*callback)(void *ctx, vec3_t *fte_restrict points, size_t numpoints, shader_t *shader), void *ctx));

	F(void,			NewMap,				(model_t *worldmode));
	F(void,			ClearScene,			(void));
	F(void,			AddEntity,			(struct entity_s *ent));
	F(unsigned int,	AddPolydata,		(struct shader_s *s, unsigned int befflags, size_t numverts, size_t numidx, vecV_t **vertcoord, vec2_t **texcoord, vec4_t **colour, index_t **indexes));	//allocates space for some polygons
	F(dlight_t *,	NewDlight,			(int key, const vec3_t origin, float radius, float time, float r, float g, float b));
	F(dlight_t *,	AllocDlightOrg,		(int keyidx, vec3_t keyorg));
	F(qboolean,		CalcModelLighting,	(entity_t *e, model_t *clmodel));
	F(void,			RenderScene,		(plugrefdef_t *viewer, size_t areabytes, const qbyte *areadata));
#define plug3dfuncs_name "3D"
} plug3dfuncs_t;

typedef struct	//for collision stuff
{
	F(model_t *,	LoadModel,			(const char *modelname, enum mlverbosity_e sync));
	F(const char *,	FixName,			(const char *modname, const char *worldname));
	F(const char *,	GetEntitiesString,	(struct model_s *mod));

	F(qboolean,		TransformedTrace,	(struct model_s *model, int hulloverride, framestate_t *framestate, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, qboolean capsule, struct trace_s *trace, vec3_t origin, vec3_t angles, unsigned int hitcontentsmask));
	F(model_t *,	TempBoxModel,		(const vec3_t mins, const vec3_t maxs));

	//general things that probably shouldn't be here...
	F(size_t,		IBufToInfo,			(infobuf_t *info, char *infostring, size_t maxsize, const char **priority, const char **ignore, const char **exclusive, infosync_t *sync, void *synccontext));
	F(void,			IBufFromInfo,		(infobuf_t *info, const char *infostring, qboolean append));
	F(qboolean,		SetIBufKey,			(infobuf_t *info, const char *key, const char *val));
	F(char *,		GetIBufKey,			(infobuf_t *info, const char *key));
	F(char*,		GetInfoKey,			(const char *s, const char *key));
	F(void,			SetInfoKey,			(char *s, const char *key, const char *value, int maxsize));

	//server things, shouldn't really be here but small. null in client-only builds
	F(void,			DropClient,			(struct client_s *drop));
	F(void,			ExtractFromUserinfo,(struct client_s *cl, qboolean verbose));
	F(qboolean,		ChallengePasses,	(int challenge));
#define plugworldfuncs_name "World"
} plugworldfuncs_t;

typedef struct	//for querying master servers
{
	F(size_t,		StringToAdr,		(const char *s, int defaultport, netadr_t *a, size_t numaddresses, const char **pathstart));
	F(char *,		AdrToString,		(char *s, int len, netadr_t *a));
	F(struct serverinfo_s*,InfoForServer,(netadr_t *addr, const char *brokerid));
	F(qboolean,		QueryServers,		(void));
	F(void,			QueryServer,		(struct serverinfo_s *server));
	F(void,			CheckPollSockets,	(void));
	F(unsigned int,	TotalCount,			(void));
	F(struct serverinfo_s*,InfoForNum,	(int num));
	F(char *,		ReadKeyString,		(struct serverinfo_s *server, unsigned int keynum));
	F(float,		ReadKeyFloat,		(struct serverinfo_s *server, unsigned int keynum));
	F(void,			WriteServers,		(void));
	F(char *,		ServerToString,		(char *s, int len, struct serverinfo_s *a));
#define plugmasterfuncs_name "Master"
} plugmasterfuncs_t;
#endif

struct flocation_s;
typedef struct	//for plugins that need to read/write files...
{
	F(int,		Open,			(const char *name, qhandle_t *handle, int mode));
	F(void,		Close,			(qhandle_t handle));
	F(int,		Write,			(qhandle_t handle, void *data, int len));
	F(int,		Read,			(qhandle_t handle, void *data, int len));
	F(int,		Seek,			(qhandle_t handle, qofs_t offset));
	F(qboolean, GetLen,			(qhandle_t handle, qofs_t *outsize));


	F(int,		LocateFile,		(const char *filename, unsigned int lflags, struct flocation_s *loc));
	F(vfsfile_t*,OpenVFS,		(const char *filename, const char *mode, enum fs_relative relativeto));		//opens a direct vfs file, without any access checks, and so can be used in threaded plugins
	F(qboolean,	NativePath,		(const char *name, enum fs_relative relativeto, char *out, int outlen));
	F(qboolean, Rename,			(const char *oldf, const char *newf, enum fs_relative relativeto));
	F(qboolean,	Remove,			(const char *fname, enum fs_relative relativeto));

	F(void,		EnumerateFiles,	(enum fs_relative fsroot, const char *match, int (QDECL *callback)(const char *fname, qofs_t fsize, time_t mtime, void *ctx, struct searchpathfuncs_s *package), void *ctx));

	//helpers
	F(int,		WildCmp,		(const char *wild, const char *string));
	F(const char *,GetExtension,(const char *filename, const char *ignoreext));
	F(void,		FileBase,		(const char *in, char *out, int outlen));
	F(void,		CleanUpPath,	(char *str));
	F(unsigned int,BlockChecksum,(const void *buffer, size_t length));	//mostly for pack hashes.
	F(void*,	LoadFile,		(const char *fname, size_t *fsize));	//plugfuncs->Free

	//stuff that's useful for networking.
	F(char*,	GetPackHashes,				(char *buffer, int buffersize, qboolean referencedonly));
	F(char*,	GetPackNames,				(char *buffer, int buffersize, int referencedonly, qboolean ext));
	F(qboolean,	GenCachedPakName,			(const char *pname, const char *crc, char *local, int llen));
	F(void,		PureMode,					(const char *gamedir, int mode, char *purenamelist, char *purecrclist, char *refnamelist, char *refcrclist, int seed));
	F(char*,	GenerateClientPacksList,	(char *buffer, int maxlen, int basechecksum));
#define plugfsfuncs_name "Filesystem"
} plugfsfuncs_t;

typedef struct	//for when you need basic socket access, hopefully rare...
{
	F(qhandle_t,TCPConnect,		(const char *ip, int port));
	F(qhandle_t,TCPListen,		(const char *localip, int port, int maxcount));
	F(qhandle_t,Accept,			(qhandle_t socket, char *address, int addresssize));
	F(int,		Recv,			(qhandle_t socket, void *buffer, int len));
	F(int,		Send,			(qhandle_t socket, void *buffer, int len));
	F(int,		SendTo,			(qhandle_t handle, void *data, int datasize, netadr_t *dest));
	F(void,		Close,			(qhandle_t socket));
	F(int,		SetTLSClient,	(qhandle_t sock, const char *certhostname));		//adds a tls layer to the socket (and specifies the peer's required hostname)
	F(int,		GetTLSBinding,	(qhandle_t sock, char *outdata, int *datalen));	//to avoid MITM attacks with compromised cert authorities

	//for (d)tls plugins to use.
	F(qboolean,	RandomBytes,				(qbyte *string, int len));
	F(void *,	TLS_GetKnownCertificate,	(const char *certname, size_t *size));
	F(qboolean,	CertLog_ConnectOkay,		(const char *hostname, void *cert, size_t certsize, unsigned int certlogproblems));

	#define N_WOULDBLOCK -1
	#define N_FATALERROR -2
	#define NET_CLIENTPORT -1
	#define NET_SERVERPORT -2
#define plugnetfuncs_name "Net"
} plugnetfuncs_t;

//fixme: this sucks.
struct vm_s;
typedef qintptr_t (QDECL *sys_calldll_t) (qintptr_t arg, ...);
typedef int (*sys_callqvm_t) (void *offset, quintptr_t mask, int fn, const int *arg);
typedef struct
{
	F(struct vm_s *,Create,			(const char *dllname, sys_calldll_t syscalldll, const char *qvmname, sys_callqvm_t syscallqvm));
	F(qboolean,		NonNative,		(struct vm_s *vm));
	F(void *,		MemoryBase,		(struct vm_s *vm));
	F(qintptr_t,	Call,			(struct vm_s *vm, qintptr_t instruction, ...));
	F(void,			Destroy,		(struct vm_s *vm));
#define plugq3vmfuncs_name "Quake3 QVM"
} plugq3vmfuncs_t;

#undef F

extern plugcorefuncs_t *plugfuncs;
extern plugcmdfuncs_t *cmdfuncs;
extern plugcvarfuncs_t *cvarfuncs;

#define Q_snprintf (void)Q_snprintfz
#define Q_vsnprintf (void)Q_vsnprintfz
#ifdef FTEENGINE
extern plugcorefuncs_t plugcorefuncs;
#else
void Q_strlncpy(char *d, const char *s, int sizeofd, int lenofs);
void Q_strlcpy(char *d, const char *s, int n);
void Q_strlcat(char *d, const char *s, int n);

qboolean VARGS Q_vsnprintfz (char *dest, size_t size, const char *fmt, va_list argptr);
qboolean VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...) LIKEPRINTF(3);

char	*va(const char *format, ...);
qboolean Plug_Init(void);
void Con_Printf(const char *format, ...);
void Con_DPrintf(const char *format, ...);	//not a particuarly efficient implementation, so beware.
void Sys_Errorf(const char *format, ...);
void QDECL Q_strncpyz(char *d, const char *s, int n);

#define PLUG_SHARED_BEGIN(t,p,b)		\
 {										\
	 t *p;								\
	 char inputbuffer[8192];			\
	 *(b) = ReadInputBuffer(inputbuffer, sizeof(inputbuffer));	\
	 if (*(b))						\
		 p = (t*)inputbuffer;			\
	 else								\
		 p = NULL;
#define PLUG_SHARED_END(p,b) UpdateInputBuffer(inputbuffer, b);}

typedef struct {
	char *name;
	char string[256];
	char *group;
	int flags;
	float value;
	qhandle_t handle;
	int modificationcount;
} vmcvar_t;

#define VMCvar_Register(cv) (cv->handle=cvarfuncs->Register(cv->name, cv->string, cv->flags, cv->group))
#define VMCvar_Update(cv) cvarfuncs->Update(handle, &cv->modcount, cv->string, sizeof(cv->string), &cv->value)
#define VMCvar_SetString(c,v)							\
	do{													\
		strcpy(c->string, v);							\
		c->value = (float)atof(v);						\
		Cvar_SetString(c->name, c->string);				\
	} while (0)
#define VMCvar_SetFloat(c,v)							\
	do {												\
		snprintf(c->string, sizeof(c->string), "%f", v);\
		c->value = (float)(v);							\
		Cvar_SetFloat(c->name, c->value);				\
	} while(0)											\


char *Plug_Info_ValueForKey (const char *s, const char *key, char *out, size_t outsize);
#endif

#ifdef __cplusplus
}
#endif
#endif
