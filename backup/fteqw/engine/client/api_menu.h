 /*
 * Copyright (c) 2015-2018
 * Marco Cawthorne  All rights reserved.
 * 
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this. If not, see <http://www.gnu.org/licenses/>.
 */

#define NATIVEMENU_API_VERSION_MIN 0	//will be updated any time a symbol is renamed.
#define NATIVEMENU_API_VERSION_MAX 0	//bumped for any change.
#ifndef NATIVEMENU_API_VERSION			//so you can hold back the reported version in order to work with older engines.
#define NATIVEMENU_API_VERSION NATIVEMENU_API_VERSION_MAX	//version reported to the other side.
#endif

struct vfsfile_s;
struct serverinfo_s;
struct searchpathfuncs_s;
struct model_s;
struct font_s;
struct shader_s;

#ifndef __QUAKEDEF_H__
	#ifdef __cplusplus
		typedef enum {qfalse, qtrue} qboolean;//false and true are forcivly defined.
	#else
		typedef enum {false, true}	qboolean;
	#endif
	typedef float vec_t;
	typedef vec_t vec2_t[2];
	typedef vec_t vec3_t[3];
	typedef vec_t vec4_t[4];
	#ifdef _MSC_VER
		#define QDECL __cdecl
	#else
		#define QDECL
	#endif

	#include <stdint.h>
	typedef uint64_t qofs_t;
#endif

#if 1 //c++ or standard C
	#include "cl_master.h"
#endif
enum slist_test_e;
enum hostcachekey_e;	//obtained via calls to gethostcacheindexforkey
enum fs_relative;
enum com_tokentype_e;

struct menu_inputevent_args_s
{
	enum {
		MIE_KEYDOWN		= 0,
		MIE_KEYUP		= 1,
		MIE_MOUSEDELTA	= 2,
		MIE_MOUSEABS	= 3,
		MIE_JOYAXIS		= 4,
	} eventtype;
	unsigned int devid;
	union
	{
		struct
		{
			unsigned int scancode;
			unsigned int charcode;
		} key;
		struct
		{
			float delta[2];
			float screen[2]; //virtual coords
		} mouse;
		struct
		{
			unsigned int axis;
			float val;
		} axis;
	};
};

typedef enum
{
	MI_INIT,		//initial startup
	MI_RENDERER,	//renderer restarted, any models/shaders/textures handles are no longer valid
	MI_RESOLUTION,	//video mode changed (scale or physical size) but without any gpu resources getting destroyed. you'll want to reload fonts.
} mintreason_t;

typedef struct
{
	struct model_s *model;
	int frame[2];
	float frametime[2];
	float frameweight[2];
	vec4_t matrix[3]; //axis/angles+origin
} menuentity_t;
typedef struct
{
	//these are in virtual coords, thus they need to be floats so that they can be rounded to ints more cleanly... yeah, scaling sucks.
	vec2_t pos;
	vec2_t size;

	float time;	//affects shader effects
	vec_t fov[2];
	vec4_t viewmatrix[3];

	struct model_s *worldmodel;
	int numentities;
	menuentity_t *entlist;
} menuscene_t;

typedef struct {
	int							api_version;	//this may be higher than you expect.
	const char					*engine_version;

	int (*checkextension)		(const char *ext);
	void (QDECL *error)			(const char *err, ...);
	void (*printf)				(const char *text, ...);
	void (*dprintf)				(const char *text, ...);
	void (*localcmd)			(const char *cmd);
	float (*cvar_float)			(const char *name);
	const char *(*cvar_string)	(const char *name, qboolean effective);	//NULL if it doesn't exist. return value lasts until cvar_set is called, etc, so don't cache. effective=true reports its active value, not the value that the user wanted.
	const char *(*cvar_default)	(const char *name);
	void (*cvar_set)			(const char *name, const char *value);
	void (*registercvar)		(const char *name, const char *defaultvalue, unsigned int flags, const char *description);
	void (*registercommand)		(const char *name, const char *description);

	char *(*parsetoken)			(const char *data, char *out, int outlen, enum com_tokentype_e *toktype);

	int (*isserver)				(void);
	int (*getclientstate)		(char const**disconnectionreason);
	void (*localsound)			(const char *sample, int channel, float volume);

	// file input / search crap
	struct vfsfile_s *(*fopen)	(const char *filename, const char *modestring, enum fs_relative fsroot);	//modestring should be one of rb,r+b,wb,w+b,ab,wbp. Mostly use a root of FS_GAMEONLY for writes, otherwise FS_GAME for reads.
	void (*fclose)				(struct vfsfile_s *fhandle);
	char *(*fgets)				(struct vfsfile_s *fhandle, char *out, size_t outsize);	//returns output buffer, or NULL
	void (*fprintf)				(struct vfsfile_s *fhandle, const char *s, ...);
	void (*enumeratefiles)		(const char *match, int (QDECL *callback)(const char *fname, qofs_t fsize, time_t mtime, void *ctx, struct searchpathfuncs_s *package), void *ctx);
	qboolean (QDECL *nativepath)(const char *fname, enum fs_relative relativeto, char *out, int outlen);	//Converts a relative path to a printable system path. All paths are considered to be utf-8. WARNING: This means that windows users will need to use _wfopen etc if they use the resulting path of this function in any system calls. WARNING: this function can and WILL fail for dodgy paths (eg blocking writes to "../engine.dll")

	// Drawing stuff
	void (*drawsetcliparea)		(float x, float y, float width, float height);
	void (*drawresetcliparea)	(void);
	struct shader_s *(*cachepic)(const char *name);
	qboolean (*drawgetimagesize)(struct shader_s *pic, int *x, int *y);
	void (*drawquad)			(const vec2_t position[4], const vec2_t texcoords[4], struct shader_s *pic, const vec4_t rgba, unsigned int be_flags);

	float (*drawstring)			(const vec2_t position, const char *text, struct font_s *font, float height, const vec4_t rgba, unsigned int be_flags);
	float (*stringwidth)		(const char *text, struct font_s *font, float height);
	struct font_s *(*loadfont)	(const char *facename, float intendedheight);	//with ttf fonts, you'll probably want one for each size.
	void (*destroyfont)			(struct font_s *font);

	// 3D scene stuff
	struct model_s *(*cachemodel)(const char *name);
	qboolean (*getmodelsize)	(struct model_s *model, vec3_t out_mins, vec3_t out_maxs);
	void (*renderscene)			(menuscene_t *scene);

	// Menu specific stuff
	void (*pushmenu)				(void *ctx);	//will have key focus.
	qboolean (*ismenupushed)		(void *ctx);	//reports if its still pushed (but not necessarily the active one!).
	void (*killmenu)				(void *ctx);	//force-removes a menu.
	int (*setmousecursor)			(const char *cursorname, float hot_x, float hot_y, float scale);	//forces absolute mouse coords whenever cursorname isn't NULL
	const char *(*keynumtostring)	(int keynum, int modifier);
	int (*stringtokeynum)			(const char *key, int *modifier);
	int (*findkeysforcommand)		(int bindmap, const char *command, int *out_scancodes, int *out_modifiers, int keycount);

	// Server browser stuff
	enum hostcachekey_e (*gethostcacheindexforkey)	(const char *key);
	struct serverinfo_s *(*getsortedhost)			(int idx);
	char *(*gethostcachestring)						(struct serverinfo_s *host, enum hostcachekey_e fld);
	float (*gethostcachenumber)						(struct serverinfo_s *host, enum hostcachekey_e fld);
	void (*resethostcachemasks)						(void);
	void (*sethostcachemaskstring)					(qboolean or_, enum hostcachekey_e fld, const char *str, enum slist_test_e op);
	void (*sethostcachemasknumber)					(qboolean or_, enum hostcachekey_e fld, int num, enum slist_test_e op);
	void (*sethostcachesort)						(enum hostcachekey_e fld, qboolean descending);
	int (*resorthostcache)							(void);
	void (*refreshhostcache)						(qboolean fullreset);
	qboolean (*sendhostcachequeries)				(void);	//returns true while there are still waiting for servers. should be called each frame while you still care about the servers.
} menu_import_t;

typedef struct {
	int		api_version;

	void	(*Init)				(mintreason_t reason, float vwidth, float vheight, int pwidth, int pheight);
	void	(*Shutdown)			(mintreason_t reason);
	void	(*DrawLoading)		(double frametime);	//pure loading screen.
	void	(*Toggle)			(int wantmode);
	qboolean(*ConsoleCommand)	(const char *cmdline, int argc, char const*const*argv);

	void	(*Draw)				(void *ctx, double frametime);					//draws a menu.
	qboolean(*InputEvent)		(void *ctx, struct menu_inputevent_args_s ev);	//return true to prevent the engine handling it (ie: because you already did).
	void	(*Closed)			(void *ctx);									//a pushed menu was closed.
} menu_export_t;

#ifndef NATIVEEXPORT
	#ifdef _WIN32
		#define NATIVEEXPORTPROTO __declspec(dllexport)
		#define NATIVEEXPORT NATIVEEXPORTPROTO
	#else
		#define NATIVEEXPORTPROTO
		#define NATIVEEXPORT __attribute__((visibility("default")))
	#endif
#endif

NATIVEEXPORTPROTO menu_export_t *QDECL GetMenuAPI	(menu_import_t *import); 
