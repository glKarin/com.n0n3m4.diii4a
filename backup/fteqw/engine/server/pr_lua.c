#include "quakedef.h"

#ifdef VM_LUA

#include "pr_common.h"
#include "hash.h"

#define LUA_MALLOC_TAG 0x55780128

#define luagloballist	\
	globalentity	(true, self)	\
	globalentity	(true, other)	\
	globalentity	(true, world)	\
	globalfloat		(true, time)	\
	globalfloat		(true, frametime)	\
	globalentity	(false, newmis)	\
	globalfloat		(false, force_retouch)	\
	globalstring	(true, mapname)	\
	globalfloat		(false, deathmatch)	\
	globalfloat		(false, coop)	\
	globalfloat		(false, teamplay)	\
	globalfloat		(true, serverflags)	\
	globalfloat		(false, dimension_send)	\
	globalfloat		(false, physics_mode)	\
	globalfloat		(true, total_secrets)	\
	globalfloat		(true, total_monsters)	\
	globalfloat		(true, found_secrets)	\
	globalfloat		(true, killed_monsters)	\
	globalvec		(true, v_forward)	\
	globalvec		(true, v_up)	\
	globalvec		(true, v_right)	\
	globalfloat		(true, trace_allsolid)	\
	globalfloat		(true, trace_startsolid)	\
	globalfloat		(true, trace_fraction)	\
	globalvec		(true, trace_endpos)	\
	globalvec		(true, trace_plane_normal)	\
	globalfloat		(true, trace_plane_dist)	\
	globalentity	(true, trace_ent)	\
	globalfloat		(true, trace_inopen)	\
	globalfloat		(true, trace_inwater)	\
	globalfloat		(false, trace_endcontentsf)	\
	globalint		(false, trace_endcontentsi)	\
	globalfloat		(false, trace_surfaceflagsf)	\
	globalint		(false, trace_surfaceflagsi)	\
	globalfloat		(false, cycle_wrapped)	\
	globalentity	(false, msg_entity)	\
	globalfunc		(false, main)	\
	globalfunc		(true, StartFrame)	\
	globalfunc		(true, PlayerPreThink)	\
	globalfunc		(true, PlayerPostThink)	\
	globalfunc		(true, ClientKill)	\
	globalfunc		(true, ClientConnect)	\
	globalfunc		(true, PutClientInServer)	\
	globalfunc		(true, ClientDisconnect)	\
	globalfunc		(false, SetNewParms)	\
	globalfunc		(false, SetChangeParms)	\
	globalfloat		(false, dimension_default)	\
	globalvec		(false, global_gravitydir)

//any globals or functions that the server might want access to need to be known also.
#define luaextragloballist	\
	globalstring	(true, startspot)	\
	globalfunc		(true, ClientReEnter) 

typedef struct
{
#define globalentity(required, name) int name;
#define globalint(required, name) int name;
#define globalfloat(required, name) float name;
#define globalstring(required, name) string_t name;
#define globalvec(required, name) vec3_t name;
#define globalfunc(required, name) int name;
luagloballist
luaextragloballist
#undef globalentity
#undef globalint
#undef globalfloat
#undef globalstring
#undef globalvec
#undef globalfunc

	float parm[NUM_SPAWN_PARMS];
} luaglobalvars_t;

typedef struct
{
	int type;
	qintptr_t offset;
	char *name;
	bucket_t buck;
} luafld_t;

//#define LIBLUA_STATIC

#ifdef LIBLUA_STATIC
	#ifdef _MSC_VER
		#pragma comment(lib, "liblua.lib")
	#endif
	#include <lua.h>
	//#include <lualib.h>
	#include <lauxlib.h>
#else
typedef struct lua_State lua_State;
typedef void *(QDECL *lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);
typedef const char *(QDECL *lua_Reader)(lua_State *L, void *data, size_t *size);
typedef int (QDECL *lua_CFunction) (lua_State *L);
typedef double lua_Number;
typedef long long lua_Integer;


#define lua_pcall(L,n,r,f)	lua_pcallk(L, (n), (r), (f), 0, NULL)
#define lua_call(L,n,r)		lua_callk(L, (n), (r), 0, NULL)
#define lua_pop(L,n)		lua_settop(L, -(n)-1)
#define lua_pushstring(L,s)	lua_pushfstring(L,"%s",s)

//#define lua_remove(L,idx)	(lua_rotate(L, (idx), -1), lua_pop(L, 1))
#define lua_replace(L,idx)	(lua_copy(L, -1, (idx)), lua_pop(L, 1))
#define lua_pushglobaltable(L)  (lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS))
#define luaL_getmetatable(L,n)	(lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define LUA_TNONE			(-1)
#define LUA_TNIL			0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER			3
#define LUA_TSTRING			4
#define LUA_TTABLE			5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD			8
#define LUA_NUMTAGS			9

#define LUA_RIDX_GLOBALS	2

#define LUA_REGISTRYINDEX (-1000000 - 1000)

#define lua_newstate		lua.newstate
#define lua_atpanic			lua.atpanic
#define lua_close			lua.close
#define lua_load			lua.load
#define lua_pcallk			lua.pcallk
#define lua_callk			lua.callk
#define lua_getfield		lua.getfield
#define lua_setfield		lua.setfield
#define lua_gettable		lua.gettable
#define lua_settable		lua.settable
#define lua_getglobal		lua.getglobal
#define lua_setglobal		lua.setglobal	
#define lua_error			lua.error
#define lua_type			lua.type
#define lua_typename		lua.typename
#define lua_rawget			lua.rawget
#define lua_rawgeti			lua.rawgeti
#define lua_rawgetp			lua.rawgetp
#define lua_rawset			lua.rawset
#define lua_createtable		lua.createtable
#define lua_setmetatable	lua.setmetatable
#define lua_newuserdata		lua.newuserdata

#define lua_copy			lua.copy
#define lua_gettop			lua.gettop
#define lua_settop			lua.settop
#define lua_pushboolean		lua.pushboolean
#define lua_pushnil			lua.pushnil
#define lua_pushnumber		lua.pushnumber
#define lua_pushinteger		lua.pushinteger
#define lua_pushvalue		lua.pushvalue
#define lua_pushcclosure	lua.pushcclosure
#define lua_pushfstring		lua.pushfstring
#define lua_pushliteral		lua_pushstring
#define lua_pushlightuserdata	lua.pushlightuserdata
#define lua_tolstring		lua.tolstring
#define lua_toboolean		lua.toboolean
#define lua_tonumberx		lua.tonumberx
#define lua_tointegerx		lua.tointegerx
#define lua_topointer		lua.topointer
#define lua_touserdata		lua.touserdata
#define lua_touserdata		lua.touserdata
#define lua_next			lua.next
//#define lua_remove			lua.remove

#define luaL_callmeta		lua.Lcallmeta
#define luaL_newmetatable	lua.Lnewmetatable

#define lua_newtable(L)		lua_createtable(L,0,0)
#define lua_pushcfunction(L,f) lua_pushcclosure(L,f,0)
#define lua_isnil(L,i)		(lua_type(L,i)==LUA_TNIL)
#define lua_tostring(L,i)	lua_tolstring(L,i,NULL)
#define lua_tonumber(L,i)	lua_tonumberx(L,i,NULL)
#define lua_tointeger(L,i)	lua_tointegerx(L,i,NULL)
#endif

//I'm using this struct for all the global stuff.
static struct
{
	lua_State *ctx;
	char readbuf[1024];
	pubprogfuncs_t progfuncs;
	progexterns_t progfuncsparms;
	edict_t **edicttable;
	unsigned int maxedicts;

	luaglobalvars_t globals;	//internal global structure
	hashtable_t globalfields;	//name->luafld_t
	luafld_t globflds[1024];	//fld->offset+type

	hashtable_t entityfields;	//name->luafld_t
	luafld_t entflds[1024];		//fld->offset+type
	size_t numflds;

#ifndef LIBLUA_STATIC
	qboolean triedlib;
	dllhandle_t *lib;

	lua_State *		(QDECL *newstate)		(lua_Alloc f, void *ud);
	lua_CFunction	(QDECL *atpanic)		(lua_State *L, lua_CFunction panicf);
	void			(QDECL *close)			(lua_State *L);
	int				(QDECL *load)			(lua_State *L, lua_Reader reader, void *dt, const char *chunkname, const char *mode);
	int				(QDECL *pcallk)			(lua_State *L, int nargs, int nresults, int errfunc, int ctx, lua_CFunction k);
	void			(QDECL *callk)			(lua_State *L, int nargs, int nresults,				 int ctx, lua_CFunction k);
	void			(QDECL *getfield)		(lua_State *L, int idx, const char *k);
	void			(QDECL *setfield)		(lua_State *L, int idx, const char *k);
	void			(QDECL *gettable)		(lua_State *L, int idx);
	void			(QDECL *settable)		(lua_State *L, int idx);
	void			(QDECL *getglobal)		(lua_State *L, const char *var);
	void			(QDECL *setglobal)		(lua_State *L, const char *var);
	int				(QDECL *error)			(lua_State *L);
	int				(QDECL *type)			(lua_State *L, int idx);
	const char     *(QDECL *typename)		(lua_State *L, int tp);
	int				(QDECL *rawget)			(lua_State *L, int idx);
	int				(QDECL *rawgeti)		(lua_State *L, int idx, lua_Integer n);
	int				(QDECL *rawgetp)		(lua_State *L, int idx, const void *p);
	void			(QDECL *rawset)			(lua_State *L, int idx);
	void			(QDECL *createtable)	(lua_State *L, int narr, int nrec);
	int				(QDECL *setmetatable)	(lua_State *L, int objindex);
	void		   *(QDECL *newuserdata)	(lua_State *L, size_t usize);

	void			(QDECL *copy)			(lua_State *L, int fromidx, int toidx);	//added in 5.3
//	void			(QDECL *replace)		(lua_State *L, int idx);				//removed in 5.3
	int				(QDECL *gettop)			(lua_State *L);
	int				(QDECL *settop)			(lua_State *L, int idx);
	void			(QDECL *pushboolean)	(lua_State *L, int b);
	void			(QDECL *pushnil)		(lua_State *L);
	void			(QDECL *pushnumber)		(lua_State *L, lua_Number n);
	void			(QDECL *pushinteger)	(lua_State *L, lua_Integer n);
	void			(QDECL *pushvalue)		(lua_State *L, int idx);
	void			(QDECL *pushcclosure)	(lua_State *L, lua_CFunction fn, int n);
	const char *	(QDECL *pushfstring)	(lua_State *L, const char *fmt, ...);
	void			(QDECL *pushlightuserdata)	(lua_State *L, void *p);
	const char *	(QDECL *tolstring)		(lua_State *L, int idx, size_t *len);
	int             (QDECL *toboolean)		(lua_State *L, int idx);
	lua_Number      (QDECL *tonumberx)		(lua_State *L, int idx, int *isnum);
	lua_Integer     (QDECL *tointegerx)		(lua_State *L, int idx, int *isnum);
	const void     *(QDECL *topointer)		(lua_State *L, int idx);
	void		   *(QDECL *touserdata)		(lua_State *L, int idx);
	int				(QDECL *next)			(lua_State *L, int idx);
//	void			(QDECL *remove)			(lua_State *L, int idx);

	int				(QDECL *Lcallmeta)		(lua_State *L, int obj, const char *e);
	int				(QDECL *Lnewmetatable)	(lua_State *L, const char *tname);
#endif
} lua;

static qboolean init_lua(void)
{
#ifndef LIBLUA_STATIC
	if (!lua.triedlib)
	{
		dllfunction_t luafuncs[] =
		{
			{(void*)&lua.newstate,		"lua_newstate"},
			{(void*)&lua.atpanic,		"lua_atpanic"},
			{(void*)&lua.close,			"lua_close"},
			{(void*)&lua.load,			"lua_load"},
			{(void*)&lua.pcallk,		"lua_pcallk"},
			{(void*)&lua.callk,			"lua_callk"},
			{(void*)&lua.getfield,		"lua_getfield"},
			{(void*)&lua.setfield,		"lua_setfield"},
			{(void*)&lua.gettable,		"lua_gettable"},
			{(void*)&lua.settable,		"lua_settable"},
			{(void*)&lua.getglobal,		"lua_getglobal"},
			{(void*)&lua.setglobal,		"lua_setglobal"},
			{(void*)&lua.error,			"lua_error"},
			{(void*)&lua.type,			"lua_type"},
			{(void*)&lua.typename,		"lua_typename"},
			{(void*)&lua.rawget,		"lua_rawget"},
			{(void*)&lua.rawgeti,		"lua_rawgeti"},
			{(void*)&lua.rawgetp,		"lua_rawgetp"},
			{(void*)&lua.rawset,		"lua_rawset"},
			{(void*)&lua.createtable,	"lua_createtable"},
			{(void*)&lua.setmetatable,	"lua_setmetatable"},
			{(void*)&lua.newuserdata,	"lua_newuserdata"},
	
			{(void*)&lua.copy,			"lua_copy"},
//			{(void*)&lua.replace,		"lua_replace"},
			{(void*)&lua.gettop,		"lua_gettop"},
			{(void*)&lua.settop,		"lua_settop"},
			{(void*)&lua.pushboolean,	"lua_pushboolean"},
			{(void*)&lua.pushnil,		"lua_pushnil"},
			{(void*)&lua.pushnumber,	"lua_pushnumber"},
			{(void*)&lua.pushinteger,	"lua_pushinteger"},
			{(void*)&lua.pushvalue,		"lua_pushvalue"},
			{(void*)&lua.pushcclosure,	"lua_pushcclosure"},
			{(void*)&lua.pushfstring,	"lua_pushfstring"},
			{(void*)&lua.pushlightuserdata,	"lua_pushlightuserdata"},
			{(void*)&lua.tolstring,		"lua_tolstring"},
			{(void*)&lua.toboolean,		"lua_toboolean"},
			{(void*)&lua.tonumberx,		"lua_tonumberx"},
			{(void*)&lua.tointegerx,	"lua_tointegerx"},
			{(void*)&lua.topointer,		"lua_topointer"},
			{(void*)&lua.touserdata,	"lua_touserdata"},
			{(void*)&lua.next,			"lua_next"},
//			{(void*)&lua.remove,		"lua_remove"},

			{(void*)&lua.Lcallmeta,		"luaL_callmeta"},
			{(void*)&lua.Lnewmetatable,	"luaL_newmetatable"},

			{NULL, NULL}
		};
		lua.triedlib = true;
		lua.lib = Sys_LoadLibrary("lua53", luafuncs);
	}
	if (!lua.lib)
		return false;
#endif
	return true;
}
char *QDECL Lua_AddString(pubprogfuncs_t *prinst, const char *val, int minlength, pbool demarkup);

static void lua_pushedict(lua_State *L, struct edict_s *ent)
{
	lua_rawgetp(L, LUA_REGISTRYINDEX, ent);
}

/*
static void lua_debugstack(lua_State *L)
{
	int idx;
	int top = lua_gettop(L);
	for (idx = 1; idx <= top; idx++)
	{
		if (luaL_callmeta(L, idx, "__tostring"))
		{
			Con_Printf("Stack%i: %s\n", idx, lua_tolstring(L, -1, NULL));
			lua_pop(L, 1);
		}
		else
		{
			int t = lua_type(L, idx);
			switch (t)
			{
			case LUA_TNUMBER:
				Con_Printf("Stack%i: %g\n", idx, lua_tonumber(L, idx));
				break;
			case LUA_TSTRING:
				Con_Printf("Stack%i: %s\n", idx, lua_tolstring(L, idx, NULL));
				break;
			case LUA_TBOOLEAN:
				Con_Printf("Stack%i: %s\n", idx, (lua_toboolean(L, idx) ? "true" : "false"));
				break;
			case LUA_TNIL:
				Con_Printf("Stack%i: %s\n", idx, "nil");
				break;
			case LUA_TTABLE:
				//special check for things that look like vectors.
				lua_getfield(L, idx, "x");
				lua_getfield(L, idx, "y");
				lua_getfield(L, idx, "z");
				if (lua_type(L, -3) == LUA_TNUMBER && lua_type(L, -2) == LUA_TNUMBER && lua_type(L, -1) == LUA_TNUMBER)
				{
					Con_Printf("Stack%i: '%f %f %f'\n", idx, lua_tonumberx(L, -3, NULL), lua_tonumberx(L, -2, NULL), lua_tonumberx(L, -1, NULL));
					lua_pop(L, 3);
					break;
				}
				lua_pop(L, 3);
			default:
				Con_Printf("Stack%i: %s: %p\n", idx, lua_typename(L, lua_type(L, idx)), lua_topointer(L, idx));
				break;
			}
		}
	}
}
*/

static void *my_lua_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
	if (nsize == 0)
	{
		free(ptr);
		return NULL;
	}
	else
		return realloc(ptr, nsize);
}
const char * my_lua_Reader(lua_State *L, void *data, size_t *size)
{
	vfsfile_t *f = data;
	*size = VFS_READ(f, lua.readbuf, sizeof(lua.readbuf));
	return lua.readbuf;
}

//replace lua's standard 'print' function to use the console instead of stdout. intended to use the same linebreak rules.
static int bi_lua_print(lua_State *L)
{
	//the problem is that we can only really accept strings here.
	//so lets just use the tostring function to make sure things are actually readable as strings.
	int args = lua_gettop(L);
	int i;
	const char *s;
	lua_getglobal(L, "tostring");
	//args now start at 1
	for(i = 1; i <= args; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_pcall(L, 1, 1, 0);	//pops args+func
		s = lua_tolstring(L, -1, NULL);
		if(s == NULL)
			s = "?";

		if(i > 1) Con_Printf("\t");
		Con_Printf("%s", s);
		lua_pop(L, 1);			//pop our lstring
	};
	lua_pop(L, 1);			//pop the cached tostring.
	Con_Printf("\n");
	return 0;
};
//more like quakec's print
static int bi_lua_conprint(lua_State *L)
{
	//the problem is that we can only really accept strings here.
	//so lets just use the tostring function to make sure things are actually readable as strings.
	int args = lua_gettop(L);
	int i;
	const char *s;

	lua_getglobal(L, "tostring");
	//args start at stack index 1
	for(i = 1; i <= args; i++)
	{
		lua_pushvalue(L, -1);	//dupe the tostring
		lua_pushvalue(L, i);	//dupe the argument
		lua_pcall(L, 1, 1, 0);	//pops args+func, pushes the string result
		s = lua_tolstring(L, -1, NULL);
		if(s == NULL)
			s = "?";

		Con_Printf("%s", s);
		lua_pop(L, 1);			//pop our lstring
	};
	lua_pop(L, 1);			//pop the cached tostring.
	return 0;
};
static int bi_lua_dprint(lua_State *L)
{
	if (!developer.ival)
		return 0;
	return bi_lua_conprint(L);
}

//taken from lua's baselib.c, with dependancies reduced a little.
static int bi_lua_tostring(lua_State *L)
{
//	if (lua_type(L, 1) == LUA_TNONE)
//		luaL_argerror(L, narg, "value expected");
	if (luaL_callmeta(L, 1, "__tostring"))
		return 1;
	switch (lua_type(L, 1))
	{
	case LUA_TNUMBER:
		lua_pushfstring(L, lua_tolstring(L, 1, NULL));
		break;
	case LUA_TSTRING:
		lua_pushvalue(L, 1);
		break;
	case LUA_TBOOLEAN:
		lua_pushstring(L, (lua_toboolean(L, 1) ? "true" : "false"));
		break;
	case LUA_TNIL:
		lua_pushstring(L, "nil");
		break;
	case LUA_TTABLE:
		//special check for things that look like vectors.
		lua_getfield(L, 1, "x");
		lua_getfield(L, 1, "y");
		lua_getfield(L, 1, "z");
		if (lua_type(L, -3) == LUA_TNUMBER && lua_type(L, -2) == LUA_TNUMBER && lua_type(L, -1) == LUA_TNUMBER)
		{
			lua_pushfstring(L, "'%g %g %g'", lua_tonumberx(L, -3, NULL), lua_tonumberx(L, -2, NULL), lua_tonumberx(L, -1, NULL));
			return 1;
		}
		//fallthrough
	default:
		lua_pushfstring(L, "%s: %p", lua_typename(L, lua_type(L, 1)), lua_topointer(L, 1));
		break;
	}
	return 1;
}
#define bi_lua_vtos bi_lua_tostring
#define bi_lua_ftos bi_lua_tostring

//taken from lua's baselib.c, with dependancies reduced a little.
static int bi_lua_tonumber(lua_State *L)
{
//	if (lua_type(L, 1) == LUA_TNONE)
//		luaL_argerror(L, narg, "value expected");
	if (luaL_callmeta(L, 1, "__tonumber"))
		return 1;
	switch (lua_type(L, 1))
	{
	case LUA_TSTRING:
		lua_pushnumber(L, atof(lua_tostring(L, 1)));
		break;
	case LUA_TNUMBER:
		lua_pushvalue(L, 1);
		break;
	case LUA_TBOOLEAN:
		lua_pushnumber(L, lua_toboolean(L, 1));
		break;
	case LUA_TNIL:
		lua_pushnumber(L, 0);
		break;
	case LUA_TTABLE:
		//special check for things that look like vectors.
		lua_getfield(L, 1, "x");
		lua_getfield(L, 1, "y");
		lua_getfield(L, 1, "z");
		if (lua_type(L, -3) == LUA_TNUMBER && lua_type(L, -2) == LUA_TNUMBER && lua_type(L, -1) == LUA_TNUMBER)
		{
			vec3_t v = {lua_tonumberx(L, -3, NULL), lua_tonumberx(L, -2, NULL), lua_tonumberx(L, -1, NULL)};
			lua_pushnumber(L, DotProduct(v,v));
			return 1;
		}
		lua_getfield(L, 1, "entnum");
		if (lua_type(L, -1) == LUA_TNUMBER)
			return 1;
		//fallthrough
	default:
		lua_pushfstring(L, "%s: %p", lua_typename(L, lua_type(L, 1)), lua_topointer(L, 1));
		break;
	}
	return 1;
}
#define bi_lua_stof bi_lua_tonumber

static int my_lua_panic(lua_State *L)
{
	const char *s = lua_tolstring(L, -1, NULL);
	Sys_Error("lua error: %s", s);
}

static int my_lua_entity_eq(lua_State *L)
{
	//table1=1
	//table2=2
	unsigned int entnum1, entnum2;
	lua_getfield(L, 1, "entnum");
	entnum1 = lua_tointegerx(L, -1, NULL);
	lua_getfield(L, 2, "entnum");
	entnum2 = lua_tointegerx(L, -1, NULL);
	lua_pop(L, 2);

	lua_pushboolean(L, entnum1 == entnum2);
	return 1;
}

static int my_lua_entity_tostring(lua_State *L)
{
	//table=1
	unsigned int entnum;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	lua_pop(L, 1);

	lua_pushstring(L, va("entity: %u", entnum));
	return 1;
}

static int my_lua_vec3_tostring(lua_State *L)
{
	//table=1
	lua_Number x,y,z;
	lua_getfield(L, 1, "x");
	x = lua_tonumber(L, -1);
	lua_getfield(L, 1, "y");
	y = lua_tonumber(L, -1);
	lua_getfield(L, 1, "z");
	z = lua_tonumber(L, -1);
	lua_pop(L, 3);

	lua_pushstring(L, va("'%g %g %g'", x, y, z));
	return 1;
}

static qboolean lua_readvector(lua_State *L, int idx, float *result)
{
	switch(lua_type(L, idx))
	{
	case LUA_TSTRING:
		{
			//we parse strings primnarily for easy .ent(or bsp) loading support.
			const char *str = lua_tolstring(L, idx, NULL);
			str = COM_Parse(str);
			result[0] = atof(com_token);
			str = COM_Parse(str);
			result[1] = atof(com_token);
			str = COM_Parse(str);
			result[2] = atof(com_token);
		}
		return true;
	case LUA_TTABLE:
		lua_getfield(L, idx, "x");
		result[0] = lua_tonumberx(L, -1, NULL);
		lua_getfield(L, idx, "y");
		result[1] = lua_tonumberx(L, -1, NULL);
		lua_getfield(L, idx, "z");
		result[2] = lua_tonumberx(L, -1, NULL);
		lua_pop(L, 3);
		return true;
	case LUA_TNUMBER:
		result[0] = 
		result[1] = 
		result[2] = lua_tonumber(L, idx);
		break;
	case LUA_TNIL:
		result[0] = 0;
		result[1] = 0;
		result[2] = 0;
		break;
	default:
		Con_Printf("Expected vector, got %s\n", lua_typename(L, lua_type(L, idx)));
		result[0] = 0;
		result[1] = 0;
		result[2] = 0;
		break;
	}
	return false;
}
static void lua_pushvector(lua_State *L, vec_t x, vec_t y, vec_t z)
{
	lua_newtable(L);
	//FIXME: should provide a metatable with a __tostring
	lua_pushnumber(L, x);
	lua_setfield (L, -2, "x");
	lua_pushnumber(L, y);
	lua_setfield (L, -2, "y");
	lua_pushnumber(L, z);
	lua_setfield (L, -2, "z");

	luaL_getmetatable(L, "vec3_t");
	lua_setmetatable(L, -2); 
}

static int my_lua_vec3_eq(lua_State *L)
{
	vec3_t a, b;
	lua_readvector(L, 1, a);
	lua_readvector(L, 2, b);
	lua_pushboolean(L, a[0]==b[0] && a[1]==b[1] && a[2]==b[2]);
	return 1;
}
static int my_lua_vec3_sub(lua_State *L)
{
	vec3_t a, b;
	lua_readvector(L, 1, a);
	lua_readvector(L, 2, b);
	lua_pushvector(L, a[0]-b[0], a[1]-b[1], a[2]-b[2]);
	return 1;
}
static int my_lua_vec3_mul(lua_State *L)
{
	vec3_t a, b;
	if (lua_readvector(L, 1, a) + lua_readvector(L, 2, b) == 2)
		lua_pushnumber(L, DotProduct(a,b));	//vec*vec is a dotproduct
	else
		lua_pushvector(L, a[0]*b[0], a[1]*b[1], a[2]*b[2]);	//one arg is a scaler. woot.
	return 1;
}
static int my_lua_vec3_add(lua_State *L)
{
	vec3_t a, b;
	lua_readvector(L, 1, a);
	lua_readvector(L, 2, b);
	lua_pushvector(L, a[0]+b[0], a[1]+b[1], a[2]+b[2]);
	return 1;
}
static int my_lua_vec3_len(lua_State *L)
{
	vec3_t a;
	lua_readvector(L, 1, a);
	lua_pushnumber(L, VectorLength(a));
	return 1;
}

static int my_lua_entity_set(lua_State *L)	//__newindex
{
//	Con_Printf("lua_entity_set: ");
//	my_lua_print(L);
	//table=1
	//key=2
	//value=3

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *s = lua_tolstring(L, 2, NULL);
		luafld_t *fld = Hash_GetInsensitive(&lua.entityfields, s);
		eval_t *eval;
		unsigned int entnum;
		if (fld && fld->offset >= 0)
		{
			lua_getfield(L, 1, "entnum");
			entnum = lua_tointegerx(L, -1, NULL);
			lua_pop(L, 1);
			if (entnum < lua.maxedicts && lua.edicttable[entnum] && !ED_ISFREE(lua.edicttable[entnum]))
			{
				eval = (eval_t*)((char*)lua.edicttable[entnum]->v + fld->offset);
				switch(fld->type)
				{
				case ev_float:
					eval->_float = lua_tonumberx(L, 3, NULL);
					return 0;
				case ev_vector:
					lua_readvector(L, 3, eval->_vector);
					return 0;
				case ev_integer:
					eval->_int = lua_tointegerx(L, 3, NULL);
					return 0;
				case ev_function:
					if (lua_type(L, 3) == LUA_TNIL)
						eval->function = 0;	//so the engine can distinguish between nil and not.
					else
						eval->function = fld->offset | ((entnum+1)<<10);
					lua_pushlightuserdata(L, (void *)(qintptr_t)fld->offset);	//execute only knows a function id, so we need to translate the store to match.
					lua_replace(L, 2);
					lua_rawset(L, 1);
					return 0;
				case ev_string:
					if (lua_type(L, 3) == LUA_TNIL)
						eval->string = 0;	//so the engine can distinguish between nil and not.
					else
						eval->string = fld->offset | ((entnum+1)<<10);
					lua_pushlightuserdata(L, (void *)(qintptr_t)fld->offset);	//execute only knows a string id, so we need to translate the store to match.
					lua_replace(L, 2);
					lua_rawset(L, 1);
					return 0;
				case ev_entity:
					//read the table's entnum field so we know which one its meant to be.
					lua_getfield(L, 3, "entnum");
					eval->edict = lua_tointegerx(L, -1, NULL);
					return 0;
				}
			}
		}
	}

	lua_rawset(L, 1);
	return 0;
}
static int my_lua_entity_get(lua_State *L)	//__index
{
//	Con_Printf("lua_entity_get: ");
//	my_lua_print(L);
	//table=1
	//key=2

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *s = lua_tolstring(L, 2, NULL);
		luafld_t *fld = Hash_GetInsensitive(&lua.entityfields, s);
		eval_t *eval;
		int entnum;
		if (fld)
		{
			if (fld->offset < 0)
			{	//negative offsets are not real fields, but are just there to ensure that no nils appear
				lua_rawget(L, 1);
				if (lua_isnil(L, -1))
				{
					lua_pop(L, 1);
					switch(fld->type)
					{
					case ev_float:
						lua_pushnumber(L, 0);
						return 1;
					case ev_integer:
						lua_pushinteger(L, 0);
						return 1;
					case ev_vector:
						lua_pushvector(L, 0, 0, 0);
						break;
					case ev_function:
					default:	//no idea
						lua_pushnil(L);
						break;
					case ev_string:
						lua_pushliteral(L, "");
						break;
					case ev_entity:
						lua_pushedict(L, lua.edicttable[0]);
						break;
					}
				}
				return 1;
			}
			lua_getfield(L, 1, "entnum");
			entnum = lua_tointegerx(L, -1, NULL);
			lua_pop(L, 1);
			if (entnum < lua.maxedicts && lua.edicttable[entnum])// && !lua.edicttable[entnum]->isfree)
			{
				eval = (eval_t*)((char*)lua.edicttable[entnum]->v + fld->offset);
				switch(fld->type)
				{
				case ev_float:
					lua_pushnumber(L, eval->_float);
					return 1;
				case ev_integer:
					lua_pushinteger(L, eval->_int);
					return 1;
				case ev_vector:
					lua_pushvector(L, eval->_vector[0], eval->_vector[1], eval->_vector[2]);
					return 1;
				case ev_function:
				case ev_string:
					lua_pushlightuserdata(L, (void *)(qintptr_t)(eval->function & 1023));	//execute only knows a function id, so we need to translate the store to match.
					lua_replace(L, 2);
					lua_rawget(L, 1);
					return 1;
				case ev_entity:
					//return the table for the entity via the lua registry.
					lua_pushlightuserdata(lua.ctx, lua.edicttable[eval->edict]);
					lua_gettable(lua.ctx, LUA_REGISTRYINDEX);
					return 1;
				}
			}
		}
	}

	//make sure it exists so we don't get called constantly if code loops through stuff that wasn't set.
//	lua_pushstring(L, "nil");
	lua_rawget(L, 1);
	return 1;
}
static int my_lua_global_set(lua_State *L)	//__newindex
{
//	Con_Printf("my_lua_global_set: ");
//	my_lua_print(L);
	//table=1
	//key=2
	//value=3

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *s = lua_tolstring(L, 2, NULL);
		luafld_t *fld = Hash_GetInsensitive(&lua.globalfields, s);
		eval_t *eval;
		if (fld)
		{
			eval = (eval_t*)((char*)&lua.globals + fld->offset);
			switch(fld->type)
			{
			case ev_float:
				eval->_float = lua_tonumberx(L, 3, NULL);
				return 0;
			case ev_vector:
				lua_getfield(L, 3, "x");
				eval->_vector[0] = lua_tonumberx(L, -1, NULL);
				lua_getfield(L, 3, "y");
				eval->_vector[1] = lua_tonumberx(L, -1, NULL);
				lua_getfield(L, 3, "z");
				eval->_vector[2] = lua_tonumberx(L, -1, NULL);
				return 0;
			case ev_integer:
				eval->_int = lua_tointegerx(L, 3, NULL);
				return 0;
			case ev_function:
				if (lua_type(L, 3) == LUA_TNIL)
					eval->function = 0;	//so the engine can distinguish between nil and not.
				else
					eval->function = fld->offset;
				lua_pushlightuserdata(L, (void *)fld->offset);	//execute only knows a function id, so we need to translate the store to match.
				lua_replace(L, 2);
				lua_rawset(L, 1);
				return 0;
			case ev_string:
				if (lua_type(L, 3) == LUA_TNIL)
					eval->string = 0;	//so the engine can distinguish between nil and not.
				else
					eval->string = fld->offset;
				lua_pushlightuserdata(L, (void *)fld->offset);	//execute only knows a string id, so we need to translate the store to match.
				lua_replace(L, 2);
				lua_rawset(L, 1);
				return 0;
			case ev_entity:
				//read the table's entnum field so we know which one its meant to be.
				lua_getfield(L, 3, "entnum");
				eval->edict = lua_tointegerx(L, -1, NULL);
				return 0;
			}
		}
	}

	lua_rawset(L, 1);
	return 0;
}
static int my_lua_global_get(lua_State *L)	//__index
{
//	Con_Printf("my_lua_global_get: ");
//	my_lua_print(L);
	//table=1
	//key=2

	if (lua_type(L, 2) == LUA_TSTRING)
	{
		const char *s = lua_tolstring(L, 2, NULL);
		luafld_t *fld = Hash_GetInsensitive(&lua.globalfields, s);
		eval_t *eval;
		if (fld)
		{
			eval = (eval_t*)((char*)&lua.globals + fld->offset);
			switch(fld->type)
			{
			case ev_float:
				lua_pushnumber(L, eval->_float);
				return 1;
			case ev_integer:
				lua_pushinteger(L, eval->_int);
				return 1;
			case ev_vector:
				lua_pushvector(L, eval->_vector[0], eval->_vector[1], eval->_vector[2]);
				return 1;
			case ev_function:
				lua_pushlightuserdata(L, (void *)(qintptr_t)eval->function);	//execute only knows a function id, so we need to translate the store to match.
				lua_replace(L, 2);
				lua_rawget(L, 1);
				return 1;
			case ev_entity:
				lua_pushedict(L, lua.edicttable[eval->edict]);
				return 1;
			}
		}
	}

	//make sure it exists so we don't get called constantly if code loops through stuff that wasn't set.
//	lua_pushstring(L, "nil");
	lua_rawget(L, 1);
	return 1;
}

static int bi_lua_setmodel(lua_State *L)
{
	int entnum;
	edict_t *e;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	e = (entnum>=lua.maxedicts)?NULL:lua.edicttable[entnum];
	PF_setmodel_Internal(&lua.progfuncs, e, lua_tolstring(L, 2, NULL));
	return 0;
}
static int bi_lua_precache_model(lua_State *L)
{
	PF_precache_model_Internal(&lua.progfuncs, lua_tolstring(L, 1, NULL), false);
	return 0;
}
#define bi_lua_precache_model2 bi_lua_precache_model
static int bi_lua_precache_sound(lua_State *L)
{
	PF_precache_sound_Internal(&lua.progfuncs, lua_tolstring(L, 1, NULL), false);
	return 0;
}
#define bi_lua_precache_sound2 bi_lua_precache_sound
static int bi_lua_precache_file(lua_State *L)
{
	return 0;
}
static int bi_lua_lightstyle(lua_State *L)
{
	vec3_t rgb;
	if (lua_gettop(L) >= 3)
		lua_readvector(L, 3, rgb);
	else
		VectorSet(rgb, 1, 1, 1);
	PF_applylightstyle(lua_tointegerx(L, 1, NULL), lua_tolstring(L, 2, NULL), rgb);
	return 0;
}
static int bi_lua_spawn(lua_State *L)
{
	edict_t *e = lua.progfuncs.EntAlloc(&lua.progfuncs, false, 0);
	if (e)
		lua_pushedict(L, e);
	else
		lua_pushnil(L);
	return 1;
}
static int bi_lua_remove(lua_State *L)
{
	int entnum;
	edict_t *e;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	e = (entnum>=lua.maxedicts)?NULL:lua.edicttable[entnum];
	if (e)
		lua.progfuncs.EntFree(&lua.progfuncs, e, false);
	return 0;
}
static int bi_lua_setorigin(lua_State *L)
{
	edict_t *e;
	lua_getfield(L, 1, "entnum");
	e = EDICT_NUM_UB((&lua.progfuncs), lua_tointegerx(L, -1, NULL));
	lua_readvector(L, 2, e->v->origin);
	World_LinkEdict (&sv.world, (wedict_t*)e, false);
	return 0;
}
static int bi_lua_setsize(lua_State *L)
{
	edict_t *e;
	lua_getfield(L, 1, "entnum");
	e = EDICT_NUM_UB((&lua.progfuncs), lua_tointegerx(L, -1, NULL));
	lua_readvector(L, 2, e->v->mins);
	lua_readvector(L, 3, e->v->maxs);
	VectorSubtract (e->v->maxs, e->v->mins, e->v->size);
	World_LinkEdict (&sv.world, (wedict_t*)e, false);
	return 0;
}

static int bi_lua_localcmd(lua_State *L)
{
	const char	*str = lua_tolstring(lua.ctx, 1, NULL);
	Cbuf_AddText (str, RESTRICT_INSECURE);
	return 0;
}
static int bi_lua_changelevel(lua_State *L)
{
	const char	*s, *spot;

// make sure we don't issue two changelevels (unless the last one failed)
	if (sv.mapchangelocked)
		return 0;
	sv.mapchangelocked = true;

	if (lua_type(L, 2) == LUA_TSTRING)	//and not nil or none
	{
		s = lua_tolstring(lua.ctx, 1, NULL);
		spot = lua_tolstring(lua.ctx, 2, NULL);
		Cbuf_AddText (va("\nchangelevel %s %s\n",s, spot), RESTRICT_LOCAL);
	}
	else
	{
		s = lua_tolstring(lua.ctx, 1, NULL);
		Cbuf_AddText (va("\nchangelevel %s\n",s), RESTRICT_LOCAL);
	}
	return 0;
}

static int bi_lua_stuffcmd(lua_State *L)
{
	int entnum;
	const char *str;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	str = lua_tolstring(L, 2, NULL);

	PF_stuffcmd_Internal(entnum, str, 0);
	return 0;
}
static int bi_lua_centerprint(lua_State *L)
{
	int entnum;
	const char *str;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	str = lua_tolstring(L, 2, NULL);
	if (!str)
		str = "";
	PF_centerprint_Internal(entnum, false, str);
	return 0;
}
static int bi_lua_getinfokey(lua_State *L)
{
	int entnum;
	const char *key;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	key = lua_tolstring(L, 2, NULL);

	key = PF_infokey_Internal(entnum, key);
	lua_pushstring(L, key);
	return 1;
}
#define bi_lua_infokey bi_lua_getinfokey
static int bi_lua_setinfokey(lua_State *L)
{
	int entnum;
	const char *key;
	const char *value;
	int result;
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	key = lua_tolstring(L, 2, NULL);
	value = lua_tolstring(L, 3, NULL);

	result = PF_ForceInfoKey_Internal(entnum, key, value, strlen(value));
	lua_pushinteger(L, result);
	return 1;
}
static int bi_lua_ambientsound(lua_State *L)
{
	vec3_t pos;
	const char *samp = lua_tolstring(L, 2, NULL);
	float vol = lua_tonumberx(L, 3, NULL);
	float attenuation = lua_tonumberx(L, 4, NULL);
	lua_readvector(L, 1, pos);

	PF_ambientsound_Internal(pos, samp, vol, attenuation);
	return 0;
}
static int bi_lua_sound(lua_State *L)
{
	int entnum;
	float channel = lua_tonumberx(L, 2, NULL);
	const char *samp = lua_tolstring(L, 3, NULL);
	float volume = lua_tonumberx(L, 4, NULL);
	float attenuation = lua_tonumberx(L, 5, NULL);

	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);

	//note: channel & 256 == reliable

	SVQ1_StartSound (NULL, (wedict_t*)EDICT_NUM_UB((&lua.progfuncs), entnum), channel, samp, volume*255, attenuation, 0, 0, 0);
	return 0;
}

static int bi_lua_pointcontents(lua_State *L)
{
	vec3_t pos;
	lua_readvector(L, 1, pos);
	lua_pushinteger(L, sv.world.worldmodel->funcs.PointContents(sv.world.worldmodel, NULL, pos));
	return 1;
}

static int bi_lua_setspawnparms(lua_State *L)
{
	globalvars_t pr_globals;

	lua_getfield(L, 1, "entnum");
	pr_globals.param[0].i = lua_tointegerx(L, -1, NULL);
	PF_setspawnparms(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_makestatic(lua_State *L)
{
	globalvars_t pr_globals;

	lua_getfield(L, 1, "entnum");
	pr_globals.param[0].i = lua_tointegerx(L, -1, NULL);
	PF_makestatic(&lua.progfuncs, &pr_globals);
	return 0;
}

static int bi_lua_droptofloor(lua_State *L)
{
	extern cvar_t pr_droptofloorunits;
	wedict_t	*ent;
	vec3_t		end;
	vec3_t		start;
	trace_t		trace;
	const float *gravitydir;
	static const vec3_t standardgravity = {0,0,-1};
	pubprogfuncs_t *prinst = &lua.progfuncs;
	world_t *world = prinst->parms->user;

	ent = PROG_TO_WEDICT((&lua.progfuncs), pr_global_struct->self);

	if (ent->xv->gravitydir[2] || ent->xv->gravitydir[1] || ent->xv->gravitydir[0])
		gravitydir = ent->xv->gravitydir;
	else
		gravitydir = standardgravity;

	VectorCopy (ent->v->origin, end);
	if (pr_droptofloorunits.value > 0)
		VectorMA(end, pr_droptofloorunits.value, gravitydir, end);
	else
		VectorMA(end, 256, gravitydir, end);

	VectorCopy (ent->v->origin, start);
	trace = World_Move (world, start, ent->v->mins, ent->v->maxs, end, MOVE_NORMAL, ent);

	if (trace.fraction == 1 || trace.allsolid)
		lua_pushboolean(L, false);
	else
	{
		VectorCopy (trace.endpos, ent->v->origin);
		World_LinkEdict (world, ent, false);
		ent->v->flags = (int)ent->v->flags | FL_ONGROUND;
		ent->v->groundentity = EDICT_TO_PROG(prinst, trace.ent);
		lua_pushboolean(L, true);
	}
	return 1;
}

static int bi_lua_checkbottom(lua_State *L)
{
	qboolean okay;
	int entnum;
	vec3_t up = {0,0,1};
	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);
	okay = World_CheckBottom(&sv.world, (wedict_t*)EDICT_NUM_UB((&lua.progfuncs), entnum), up);
	lua_pushboolean(L, okay);
	return 1;
}

static int bi_lua_bprint_qw(lua_State *L)
{
	int level = lua_tointegerx(L, 1, NULL);
	const char *str = lua_tolstring(L, 2, NULL);
	SV_BroadcastPrintf (level, "%s", str);
	return 0;
}
static int bi_lua_bprint_nq(lua_State *L)
{
	int level = PRINT_HIGH;
	const char *str = lua_tolstring(L, 1, NULL);
	SV_BroadcastPrintf (level, "%s", str);
	return 0;
}
static int bi_lua_sprint_qw(lua_State *L)
{
	int entnum;
	int level = lua_tointegerx(L, 2, NULL);
	const char *str = lua_tolstring(L, 3, NULL);

	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);

	if (entnum < 1 || entnum > sv.allocated_client_slots)
	{
		Con_TPrintf ("tried to sprint to a non-client\n");
		return 0;
	}
	SV_ClientPrintf (&svs.clients[entnum-1], level, "%s", str);
	return 0;
}
static int bi_lua_sprint_nq(lua_State *L)
{
	int entnum;
	int level = PRINT_HIGH;
	const char *str = lua_tolstring(L, 2, NULL);

	lua_getfield(L, 1, "entnum");
	entnum = lua_tointegerx(L, -1, NULL);

	if (entnum < 1 || entnum > sv.allocated_client_slots)
	{
		Con_TPrintf ("tried to sprint to a non-client\n");
		return 0;
	}
	SV_ClientPrintf (&svs.clients[entnum-1], level, "%s", str);
	return 0;
}

static int bi_lua_cvar_set(lua_State *L)
{
	const char *name = lua_tolstring(L, 1, NULL);
	const char *str = lua_tolstring(L, 2, NULL);
	cvar_t *var = Cvar_FindVar(name);
	if (var)
		Cvar_Set(var, str);
	return 0;
}
static int bi_lua_cvar_get(lua_State *L)
{
	const char *name = lua_tolstring(L, 1, NULL);
	cvar_t *var = Cvar_FindVar(name);
	if (var)
		lua_pushstring(L, var->string);
	else
		lua_pushnil(L);
	return 1;
}
static int bi_lua_cvar(lua_State *L)	//useless get[float]
{
	const char *name = lua_tolstring(L, 1, NULL);
	cvar_t *var = Cvar_FindVar(name);
	if (var)
		lua_pushnumber(L, var->value);
	else
		lua_pushnil(L);
	return 1;
}

static int pushset_trace_globals(lua_State *L, trace_t *trace)
{
	pr_global_struct->trace_allsolid = trace->allsolid;
	pr_global_struct->trace_startsolid = trace->startsolid;
	pr_global_struct->trace_fraction = trace->fraction;
	pr_global_struct->trace_inwater = trace->inwater;
	pr_global_struct->trace_inopen = trace->inopen;
	pr_global_struct->trace_surfaceflagsf = trace->surface?trace->surface->flags:0;
	pr_global_struct->trace_surfaceflagsi = trace->surface?trace->surface->flags:0;
//	if (pr_global_struct->trace_surfacename)
//		prinst->SetStringField(prinst, NULL, &pr_global_struct->trace_surfacename, tr->surface?tr->surface->name:"", true);
	pr_global_struct->trace_endcontentsf = trace->contents;
	pr_global_struct->trace_endcontentsi = trace->contents;
//	if (trace.fraction != 1)
//		VectorMA (trace->endpos, 4, trace->plane.normal, P_VEC(trace_endpos));
//	else
		VectorCopy (trace->endpos, P_VEC(trace_endpos));
	VectorCopy (trace->plane.normal, P_VEC(trace_plane_normal));
	pr_global_struct->trace_plane_dist =  trace->plane.dist;
	pr_global_struct->trace_ent = trace->ent?((wedict_t*)trace->ent)->entnum:0;


	lua_newtable(L);
		lua_pushnumber(L, trace->fraction);
		lua_setfield(L, -2, "fraction");
		lua_pushvector(L, trace->endpos[0], trace->endpos[1], trace->endpos[2]);
		lua_setfield(L, -2, "endpos");
		if (trace->ent)
			lua_pushedict(L, trace->ent);
		else
			lua_pushedict(L, lua.edicttable[0]);
		lua_setfield(L, -2, "ent");

		lua_pushboolean(L, trace->allsolid);
		lua_setfield(L, -2, "allsolid");
		lua_pushboolean(L, trace->startsolid);
		lua_setfield(L, -2, "startsolid");
		lua_pushboolean(L, trace->inwater);
		lua_setfield(L, -2, "inwater");
		lua_pushboolean(L, trace->inopen);
		lua_setfield(L, -2, "inopen");

		lua_newtable(L);
			lua_pushnumber(L, trace->plane.dist);
			lua_setfield(L, -2, "dist");
			lua_pushvector(L, trace->plane.normal[0], trace->plane.normal[1], trace->plane.normal[2]);
			lua_setfield(L, -2, "normal");
		lua_setfield(L, -2, "plane");
	return 1;
}

static int bi_lua_traceline(lua_State *L)
{
	vec3_t v1, v2;
	trace_t	trace;
	int		nomonsters;
	wedict_t	*ent;

	lua_readvector(L, 1, v1);
	lua_readvector(L, 2, v2);
	nomonsters = lua_tointegerx(L, 3, NULL);
	lua_getfield(L, 4, "entnum");
	ent = (wedict_t*)EDICT_NUM_UB((&lua.progfuncs), lua_tointegerx(L, -1, NULL));

	trace = World_Move (&sv.world, v1, vec3_origin, vec3_origin, v2, nomonsters|MOVE_IGNOREHULL, (wedict_t*)ent);

	return pushset_trace_globals(L, &trace);
}

static int bi_lua_tracebox(lua_State *L)
{
	vec3_t v1, v2, mins, maxs;
	trace_t	trace;
	int		nomonsters;
	wedict_t	*ent;

	lua_readvector(L, 1, v1);
	lua_readvector(L, 2, v2);
	lua_readvector(L, 3, mins);
	lua_readvector(L, 4, maxs);
	nomonsters = lua_tointegerx(L, 5, NULL);
	lua_getfield(L, 6, "entnum");
	ent = (wedict_t*)EDICT_NUM_UB((&lua.progfuncs), lua_tointegerx(L, -1, NULL));

	trace = World_Move (&sv.world, v1, mins, maxs, v2, nomonsters|MOVE_IGNOREHULL, (wedict_t*)ent);

	return pushset_trace_globals(L, &trace);
}

static int bi_lua_walkmove(lua_State *L)
{
	pubprogfuncs_t *prinst = &lua.progfuncs;
	world_t *world = prinst->parms->user;
	wedict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	int 	oldself;
	vec3_t	axis[3];

	ent = PROG_TO_WEDICT(prinst, *world->g.self);
	yaw = lua_tonumberx(L, 1, NULL);
	dist = lua_tonumberx(L, 2, NULL);

	if ( !( (int)ent->v->flags & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		lua_pushboolean(L, false);
		return 1;
	}

	World_GetEntGravityAxis(ent, axis);

	yaw = yaw*M_PI*2 / 360;

	VectorScale(axis[0], cos(yaw)*dist, move);
	VectorMA(move, sin(yaw)*dist, axis[1], move);

// save program state, because World_movestep may call other progs
	oldself = *world->g.self;

	lua_pushboolean(L, World_movestep(world, ent, move, axis, true, false, NULL));

// restore program state
	*world->g.self = oldself;

	return 1;
}
static int bi_lua_movetogoal(lua_State *L)
{
	pubprogfuncs_t *prinst = &lua.progfuncs;
	world_t *world = prinst->parms->user;
	wedict_t	*ent;
	float dist;
	ent = PROG_TO_WEDICT(prinst, *world->g.self);
	dist = lua_tonumberx(L, 1, NULL);
	World_MoveToGoal (world, ent, dist);
	return 0;
}

static int bi_lua_nextent(lua_State *L)
{
	world_t *world = &sv.world;
	int		i;
	wedict_t	*ent;

	lua_getfield(L, 1, "entnum");
	i = lua_tointegerx(L, -1, NULL);

	while (1)
	{
		i++;
		if (i == world->num_edicts)
		{
			ent = world->edicts;
			break;
		}
		ent = (wedict_t *)lua.edicttable[i];
		if (!ED_ISFREE(ent))
		{
			break;
		}
	}
	lua_pushedict(L, (struct edict_s *)ent);
	return 1;
}

static int bi_lua_nextclient(lua_State *L)
{
	world_t *world = &sv.world;
	int		i;
	wedict_t	*ent;

	lua_getfield(L, 1, "entnum");
	i = lua_tointegerx(L, -1, NULL);

	while (1)
	{
		i++;
		if (i == sv.allocated_client_slots)
		{
			ent = world->edicts;
			break;
		}
		ent = WEDICT_NUM_UB(world->progs, i);
		if (!ED_ISFREE(ent))
		{
			break;
		}
	}
	lua_pushlightuserdata(L, ent);
	lua_gettable(L, LUA_REGISTRYINDEX);
	return 1;
}

static int bi_lua_checkclient(lua_State *L)
{
	pubprogfuncs_t *prinst = &lua.progfuncs;
	wedict_t *ent;
	ent = WEDICT_NUM_PB(prinst, PF_checkclient_Internal(prinst));
	lua_pushlightuserdata(L, ent);
	lua_gettable(L, LUA_REGISTRYINDEX);
	return 1;
}

static int bi_lua_random(lua_State *L)
{
	lua_pushnumber(L, (rand ()&0x7fff) / ((float)0x8000));
	return 1;
}

static int bi_lua_makevectors(lua_State *L)
{
	vec3_t angles;
	//this is annoying as fuck in lua, what with it writing globals and stuff.
	//perhaps we should support f,u,l=makevectors(ang)... meh, cba.
	lua_readvector(L, 1, angles);
	AngleVectors (angles, pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);

#if 1
	//globals suck, and lua allows multi-return.
	lua_pushvector(L, (pr_global_struct->v_forward)[0], (pr_global_struct->v_forward)[1], (pr_global_struct->v_forward)[2]);
	lua_pushvector(L, (pr_global_struct->v_right)[0], (pr_global_struct->v_right)[1], (pr_global_struct->v_right)[2]);
	lua_pushvector(L, (pr_global_struct->v_up)[0], (pr_global_struct->v_up)[1], (pr_global_struct->v_up)[2]);
	return 3;
#else
	return 0;
#endif
}
static int bi_lua_normalize(lua_State *L)
{
	vec3_t v;
	lua_readvector(L, 1, v);

	VectorNormalize(v);

	lua_pushvector(L, v[0], v[1], v[2]);
	return 1;
}
static int bi_lua_vectoangles(lua_State *L)
{
	vec3_t forward;
	vec3_t up;
	float *uv = NULL;
	vec3_t ret;
	lua_readvector(L, 1, forward);
	if (lua_type(L, 2) != LUA_TNONE)
	{
		lua_readvector(L, 1, up);
		uv = up;
	}
	VectorAngles(forward, uv, ret, true);

	lua_pushvector(L, ret[0], ret[1], ret[2]);
	return 1;
}
static int bi_lua_vectoyaw(lua_State *L)
{
	vec3_t forward;
	vec3_t up;
	float *uv = NULL;
	vec3_t ret;
	lua_readvector(L, 1, forward);
	if (lua_type(L, 2) != LUA_TNONE)
	{
		lua_readvector(L, 1, up);
		uv = up;
	}
	VectorAngles(forward, uv, ret, true);

	lua_pushnumber(L, ret[1]);
	return 1;
}

void QCBUILTIN PF_aim (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
static int bi_lua_aim(lua_State *L)
{	//aim builtin is for keyboard users, if they still exist.
	globalvars_t pr_globals;
	lua_getfield(L, 1, "entnum");
	pr_globals.param[0].i = lua_tointeger(L, -1);
	pr_globals.param[1].f = lua_tonumber(L, 2);	//speed
	PF_aim(&lua.progfuncs, &pr_globals);
	lua_pushvector(L, pr_globals.ret.vec[0], pr_globals.ret.vec[1], pr_globals.ret.vec[2]);
	return 1;
}

static int bi_lua_tokenize(lua_State *L)
{
	const char *instring = lua_tolstring(L, 1, NULL);
	int argc = 0;
	lua_newtable(L);
	while(NULL != (instring = COM_Parse(instring)))
	{
		//lua is traditionally 1-based
		//for i=1,t.argc do
		lua_pushinteger(L, ++argc);
		lua_pushstring(L, com_token);
		lua_settable(L, -3);

		if (argc == 1)
		{
			while (*instring == ' ' || *instring == '\t')
				instring++;
			lua_pushstring(L, instring);
			lua_setfield(L, -2, "args");	//args is all-but-the-first
		}
	}
	lua_pushinteger(L, argc);
	lua_setfield(L, -2, "argc");	//argc is the count.
	return 1;
}

static int bi_lua_findradiuschain(lua_State *L)
{
	extern cvar_t sv_gameplayfix_blowupfallenzombies;
	world_t *world = &sv.world;
	edict_t	*ent, *chain;
	float	rad;
	vec3_t	org;
	vec3_t	eorg;
	int		i, j;

	chain = (edict_t *)world->edicts;

	lua_readvector(L, 1, org);
	rad = lua_tonumberx(L, 2, NULL);
	rad = rad*rad;

	for (i=1 ; i<world->num_edicts ; i++)
	{
		ent = EDICT_NUM_PB(world->progs, i);
		if (ED_ISFREE(ent))
			continue;
		if (ent->v->solid == SOLID_NOT && (progstype != PROG_QW || !((int)ent->v->flags & FL_FINDABLE_NONSOLID)) && !sv_gameplayfix_blowupfallenzombies.value)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v->origin[j] + (ent->v->mins[j] + ent->v->maxs[j])*0.5);
		if (DotProduct(eorg,eorg) > rad)
			continue;

		ent->v->chain = EDICT_TO_PROG(world->progs, chain);
		chain = ent;
	}

	lua_pushlightuserdata(L, chain);
	lua_gettable(L, LUA_REGISTRYINDEX);
	return 1;
}
static int bi_lua_findradiustable(lua_State *L)
{
	extern cvar_t sv_gameplayfix_blowupfallenzombies;
	world_t *world = &sv.world;
	edict_t	*ent, *chain;
	float	rad;
	vec3_t	org;
	vec3_t	eorg;
	int		i, j;
	int results = 1;	//lua arrays are 1-based

	chain = (edict_t *)world->edicts;

	lua_readvector(L, 1, org);
	rad = lua_tonumberx(L, 2, NULL);
	rad = rad*rad;

	lua_newtable(L);	//our return value.

	for (i=1 ; i<world->num_edicts ; i++)
	{
		ent = EDICT_NUM_PB(world->progs, i);
		if (ED_ISFREE(ent))
			continue;
		if (ent->v->solid == SOLID_NOT && (progstype != PROG_QW || !((int)ent->v->flags & FL_FINDABLE_NONSOLID)) && !sv_gameplayfix_blowupfallenzombies.value)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v->origin[j] + (ent->v->mins[j] + ent->v->maxs[j])*0.5);
		if (DotProduct(eorg,eorg) > rad)
			continue;

		lua_pushinteger(L, ++results);
		lua_pushlightuserdata(L, ent);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_settable(L, -3);
	}

	lua_pushlightuserdata(L, chain);
	lua_gettable(L, LUA_REGISTRYINDEX);
	return 1;
}
#define bi_lua_findradius bi_lua_findradiuschain


static int bi_lua_find(lua_State *L)
{
	world_t *world = &sv.world;
	edict_t	*ent;
	size_t	i;
	const char *s;
	const char *match;

	lua_getfield(L, 1, "entnum");
	i = lua_tointegerx(L, -1, NULL)+1;
	match = lua_tostring(L, 3);

	for ( ; i<world->num_edicts ; i++)
	{
		ent = EDICT_NUM_PB(world->progs, i);
		if (ED_ISFREE(ent))
			continue;
		lua_pushedict(L, ent);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
		s = lua_tostring(L, -1);
		if (!s)
			s = "";
		if (!strcmp(s, match))	//should probably do a lua comparison, but nils suck
		{
			lua_pop(L, 2);
			lua_pushedict(L, ent);
			return 1;
		}
		lua_pop(L, 2);
	}

	lua_pushedict(L, EDICT_NUM_PB(world->progs, 0));
	return 1;
}

static int bi_lua_multicast(lua_State *L)
{
	int dest;
	vec3_t org;

	dest = lua_tointegerx(L, 1, NULL);
	lua_readvector(L, 2, org);

	NPP_Flush();
	SV_Multicast (org, dest);

	return 0;
}
extern sizebuf_t csqcmsgbuffer;
static int bi_lua_writechar(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].f = lua_tonumberx(L, 1, NULL);
	PF_WriteChar(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_writebyte(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].f = lua_tonumberx(L, 1, NULL);
	PF_WriteByte(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_writeshort(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].f = lua_tonumberx(L, 1, NULL);
	PF_WriteShort(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_writelong(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].f = lua_tonumberx(L, 1, NULL);
	PF_WriteLong(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_writeangle(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].f = lua_tonumberx(L, 1, NULL);
	PF_WriteAngle(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_writecoord(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].f = lua_tonumberx(L, 1, NULL);
	PF_WriteCoord(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_writestring(lua_State *L)
{
	PF_WriteString_Internal((csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST),lua_tolstring(L, 1, NULL));
	return 0;
}
static int bi_lua_writeentity(lua_State *L)
{
	globalvars_t pr_globals;
	lua_getfield(L, 1, "entnum");
	pr_globals.param[0].f = (csqcmsgbuffer.maxsize?MSG_CSQC:MSG_MULTICAST);
	pr_globals.param[1].i = lua_tointegerx(L, -1, NULL);
	PF_WriteEntity(&lua.progfuncs, &pr_globals);
	return 0;
}

static int bi_lua_WriteChar(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].f = lua_tonumberx(L, 2, NULL);
	PF_WriteChar(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_WriteByte(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].f = lua_tonumberx(L, 2, NULL);
	PF_WriteByte(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_WriteShort(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].f = lua_tonumberx(L, 2, NULL);
	PF_WriteShort(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_WriteLong(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].f = lua_tonumberx(L, 2, NULL);
	PF_WriteLong(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_WriteAngle(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].f = lua_tonumberx(L, 2, NULL);
	PF_WriteAngle(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_WriteCoord(lua_State *L)
{
	globalvars_t pr_globals;
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].f = lua_tonumberx(L, 2, NULL);
	PF_WriteCoord(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_WriteString(lua_State *L)
{
	PF_WriteString_Internal(lua_tonumberx(L, 1, NULL),lua_tolstring(L, 2, NULL));
	return 0;
}
static int bi_lua_WriteEntity(lua_State *L)
{
	globalvars_t pr_globals;
	lua_getfield(L, 2, "entnum");
	pr_globals.param[0].f = lua_tonumberx(L, 1, NULL);
	pr_globals.param[1].i = lua_tointegerx(L, -1, NULL);
	PF_WriteEntity(&lua.progfuncs, &pr_globals);
	return 0;
}

void QCBUILTIN PF_particle (pubprogfuncs_t *prinst, globalvars_t *pr_globals);
static int bi_lua_particle(lua_State *L)
{
	globalvars_t pr_globals;
	lua_readvector(L, 1, pr_globals.param[0].vec);
	lua_readvector(L, 2, pr_globals.param[1].vec);
	pr_globals.param[2].f = lua_tonumberx(L, 3, NULL);
	pr_globals.param[3].f = lua_tonumberx(L, 4, NULL);
	PF_particle(&lua.progfuncs, &pr_globals);
	return 0;
}
static int bi_lua_ChangeYaw(lua_State *L)
{
	PF_changeyaw(&lua.progfuncs, NULL);
	return 0;
}


static int bi_lua_bitnot(lua_State *L)
{
	lua_pushinteger(L, ~lua_tointegerx(L, 1, NULL));
	return 1;
}
static int bi_lua_bitclear(lua_State *L)
{
	lua_pushinteger(L, lua_tointegerx(L, 1, NULL)&~lua_tointegerx(L, 2, NULL));
	return 1;
}
static int bi_lua_bitset(lua_State *L)
{
	lua_pushnumber(L, lua_tointegerx(L, 1, NULL)|lua_tointegerx(L, 2, NULL));
	return 1;
}
#define bi_lua_bitor bi_lua_bitset
static int bi_lua_bitand(lua_State *L)
{
	lua_pushnumber(L, lua_tointegerx(L, 1, NULL)&lua_tointegerx(L, 2, NULL));
	return 1;
}
static int bi_lua_bitxor(lua_State *L)
{
	lua_pushnumber(L, lua_tointegerx(L, 1, NULL)^lua_tointegerx(L, 2, NULL));
	return 1;
}

static int bi_lua_sin(lua_State *L)
{
	lua_pushnumber(L, sin(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_cos(lua_State *L)
{
	lua_pushnumber(L, cos(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_tan(lua_State *L)
{
	lua_pushnumber(L, tan(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_atan2(lua_State *L)
{
	lua_pushnumber(L, atan2(lua_tonumberx(L, 1, NULL), lua_tonumberx(L, 2, NULL)));
	return 1;
}
static int bi_lua_sqrt(lua_State *L)
{
	lua_pushnumber(L, sin(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_pow(lua_State *L)
{
	lua_pushnumber(L, pow(lua_tonumberx(L, 1, NULL), lua_tonumberx(L, 2, NULL)));
	return 1;
}
static int bi_lua_floor(lua_State *L)
{
	lua_pushnumber(L, floor(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_rint(lua_State *L)
{	//C rounds towards 0, so bias away from 0 by 0.5 and we'll get the right rounded value.
	lua_Number f = lua_tonumberx(L, 1, NULL);
	if (f < 0)
		lua_pushinteger(L, f - 0.5);
	else
		lua_pushinteger(L, f + 0.5);
	return 1;
}
static int bi_lua_ceil(lua_State *L)
{
	lua_pushnumber(L, ceil(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_fabs(lua_State *L)
{
	lua_pushnumber(L, fabs(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_acos(lua_State *L)
{
	lua_pushnumber(L, acos(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_asin(lua_State *L)
{
	lua_pushnumber(L, asin(lua_tonumberx(L, 1, NULL)));
	return 1;
}
static int bi_lua_atan(lua_State *L)
{
	lua_pushnumber(L, atan(lua_tonumberx(L, 1, NULL)));
	return 1;
}

typedef struct
{
	lua_State *L;
	int idx;
} luafsenum_t;
static int QDECL lua_file_enumerate(const char *fname, qofs_t fsize, time_t mtime, void *param, searchpathfuncs_t *spath)
{
	luafsenum_t *e = param;
	lua_pushinteger(e->L, e->idx++);
	lua_pushfstring(e->L, "%s", fname);
	lua_settable(e->L, -3);
	return true;
}
static int bi_lua_getfilelist(lua_State *L)
{
	luafsenum_t e;
	const char *path = lua_tolstring(L, 1, NULL);
	e.L = L;
	e.idx = 1;	//lua arrays are 1-based.

	lua_newtable(L);	//our return value.
	COM_EnumerateFiles(path, lua_file_enumerate, &e);
	return 1;
}

static int bi_lua_fclose(lua_State *L)
{
	//both fclose and __gc.
	//should we use a different function so that we can warn on dupe fcloses without bugging out on fclose+gc?
	//meh, cba
	vfsfile_t **f = lua_touserdata(L, 1);
	if (f && *f != NULL)
	{
		VFS_CLOSE(*f);
		*f = NULL;
	}
	return 0;
}
static int bi_lua_fopen(lua_State *L)
{
	vfsfile_t *f;
	const char *fname = lua_tolstring(L, 1, NULL);
	qboolean read = true;
	vfsfile_t **ud;
	if (read)
		f = FS_OpenVFS(fname, "rb", FS_GAME);
	else
		f = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
	if (!f)
	{
		lua_pushnil(L);
		return 1;
	}
	ud = lua_newuserdata(L, sizeof(vfsfile_t*));
	*ud = f;
	
	lua_newtable(L);
	lua_pushcclosure(L, bi_lua_fclose, 0);
	lua_setfield(L, -2, "__gc");
	lua_setmetatable(L, -2);
	return 1;
}
static int bi_lua_fgets(lua_State *L)
{
	vfsfile_t **f = lua_touserdata(L, 1);
	char line[8192];
	char *r = NULL;
	if (f && *f)
		r = VFS_GETS(*f, line, sizeof(line));

	if (r)
		lua_pushfstring(L, "%s", r);
	else
		lua_pushnil(L);
	return 1;
}
static int bi_lua_fputs(lua_State *L)
{
	vfsfile_t **f = lua_touserdata(L, 1);
	size_t l;
	const char *str = lua_tolstring(L, 2, &l);
	if (f && *f != NULL)
		VFS_WRITE(*f, str, l);
	return 0;
}

static int bi_lua_loadlua(lua_State *L)
{
	const char *fname = lua_tolstring(L, 1, NULL);
	vfsfile_t *sourcefile = FS_OpenVFS(fname, "rb", FS_GAME);
	if (!sourcefile)
	{
		Con_Printf("Error trying to load %s\n", fname);
		lua_pushnil(L);
	}
	else if (0 != lua_load(L, my_lua_Reader, sourcefile, va("@%s", fname), "bt"))	//load the file, embed it within a function and push it
	{
		Con_Printf("Error trying to parse %s: %s\n", fname, lua_tolstring(L, -1, NULL));
		lua_pushnil(L);
	}
	VFS_CLOSE(sourcefile);
	return 1;
}

static int bi_lua_require(lua_State *L)
{
	const char *fname = lua_tolstring(L, 1, NULL);
	vfsfile_t *sourcefile;
	const char *usename;
	if ((sourcefile = FS_OpenVFS(usename=fname, "rb", FS_GAME)))
		;
	else if ((sourcefile = FS_OpenVFS(usename=va("%s.lua", fname), "rb", FS_GAME)))
		;
	else
	{
		Con_Printf("Error trying to load %s\n", fname);
		return 1;
	}

	if (0 != lua_load(lua.ctx, my_lua_Reader, sourcefile, va("@%s", usename), "bt"))	//load the file, embed it within a function and push it
	{
		//failed - it pushed an error code instead
		Con_Printf("Error trying to parse %s: %s\n", fname, lua_tolstring(lua.ctx, -1, NULL));
		lua_pop(lua.ctx, 1);
	}
	else
	{
		if (lua_pcall(lua.ctx, 0, 0, 0) != 0)	//now call it, so its actually run.
		{
			const char *s = lua_tolstring(lua.ctx, -1, NULL);
			Con_Printf(CON_WARNING "%s\n", s);
			lua_pop(lua.ctx, 1);
		}
	}
	VFS_CLOSE(sourcefile);
	return 0;
}

static int bi_lua_vec3(lua_State *L)
{
	float x = lua_tonumberx(L, 1, NULL);
	float y = lua_tonumberx(L, 2, NULL);
	float z = lua_tonumberx(L, 3, NULL);

	lua_pushvector(L, x, y, z);
	return 1;
}
static int bi_lua_field(lua_State *L)
{
	const char *fname = lua_tostring(L, 1);
	const char *ftype = lua_tostring(L, 2);
	int t;
	size_t u;

	if (!strcmp(ftype, "string"))
		t = ev_string;
	else if (!strcmp(ftype, "float"))
		t = ev_float;
	else if (!strcmp(ftype, "integer"))
		t = ev_integer;
	else if (!strcmp(ftype, "vector"))
		t = ev_vector;
	else if (!strcmp(ftype, "function"))
		t = ev_function;
	else if (!strcmp(ftype, "entity"))
		t = ev_entity;
	else
	{
		Con_Printf("Unknown field type\n");
		return 0;
	}

	if (lua.numflds == countof(lua.entflds))
	{
		Con_Printf("Too many ent fields\n");
		return 0;
	}

	//no dupes please.
	for (u = 0; u < lua.numflds; u++)
	{
		if (!strcmp(lua.entflds[u].name, fname))
			return 0;
	}

	lua.entflds[lua.numflds].offset = -1;	//if we don't know about it yet, then its an artificial field, and present just so that self.whatever returns something other than nil.
	lua.entflds[lua.numflds].name = Lua_AddString(NULL, fname, 0, false);
	lua.entflds[lua.numflds].type = t;
	Hash_AddInsensitive(&lua.entityfields, lua.entflds[lua.numflds].name, &lua.entflds[lua.numflds], &lua.entflds[lua.numflds].buck);
	lua.numflds++;

	return 0;
}
static int bi_lua_type(lua_State *L)
{
	const char *tn;
	int t = lua_type(L, 1);
//	luaL_argcheck(L, t != LUA_TNONE, 1, "value expected");
	tn = lua_typename(L, t);
	lua_pushstring(L, tn);
	return 1;
}

static int bi_lua_logfrag(lua_State *L)
{
	//stub... noone cares
	return 0;
}

/*static int bi_lua_entities(lua_State *L)
{
	//stub... noone cares
	return 0;
}*/

#ifdef LIBLUA_STATIC
static int pairsmeta (lua_State *L, const char *method, int iszero,
                      lua_CFunction iter) {
  luaL_checkany(L, 1);
  if (luaL_getmetafield(L, 1, method) == LUA_TNIL) {  /* no metamethod? */
    lua_pushcfunction(L, iter);  /* will return generator, */
    lua_pushvalue(L, 1);  /* state, */
    if (iszero) lua_pushinteger(L, 0);  /* and initial value */
    else lua_pushnil(L);
  }
  else {
    lua_pushvalue(L, 1);  /* argument 'self' to metamethod */
    lua_call(L, 1, 3);  /* get 3 values from metamethod */
  }
  return 3;
}
static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}
static int bi_lua_pairs (lua_State *L) {
  return pairsmeta(L, "__pairs", 0, luaB_next);
}
#endif

static int bi_lua_objerror (lua_State *L)
{
	Con_Printf("\n");
	lua_pushedict(L, lua.edicttable[lua.globals.self]);

	lua_pushnil(L);
	while (lua_next(L, -2))
	{	//FIXME: this doesn't find vector fields
		if (lua_type(L, -2) == LUA_TSTRING)
			Con_Printf("%21s:", lua_tostring(L, -2));
		else if (lua_type(L, -2) == LUA_TLIGHTUSERDATA)
		{
			int i;
			const void *ud = lua_topointer(L, -2);
			for (i = 0; i < lua.numflds; i++)
				if (lua.entflds[i].offset == (qintptr_t)ud)
					break;

			if (i == lua.numflds)
				Con_Printf("%21s:", lua_typename(L, lua_type(L, -2)));
			else
				Con_Printf("%21s:", lua.entflds[i].name);
		}
		else
			Con_Printf("%21s:", lua_typename(L, lua_type(L, -2)));
		if (lua_type(L, -1) == LUA_TSTRING)
			Con_Printf(" \"%s\"\n", lua_tostring(L, -1));
		else if (lua_type(L, -1) == LUA_TNUMBER)
			Con_Printf(" \"%g\"\n", lua_tonumber(L, -1));
#ifdef LIBLUA_STATIC
		else if (lua_type(L, -1) == LUA_TFUNCTION)
		{
			lua_Debug ar = {0};
			lua_pushvalue(L, -1);
			lua_getinfo(L, ">nS", &ar);
			Con_Printf(" %s %s:%i\n", ar.name, ar.source, ar.linedefined);
		}
#endif
		else
			Con_Printf(" %s\n", lua_typename(L, lua_type(L, -1)));
		lua_pop(L, 1);
	}

	lua_pop(L, 1);

	Con_Printf("objerror: %s\n", lua_tostring(L, 1));
	return 0;
}
static int bi_lua_error (lua_State *L)
{
	SV_Error("Error: %s\n", lua_tostring(L, 1));
	return 0;
}
static int bi_lua_qtrue (lua_State *L)
{
	//the truth is what you make it...

	const char *s;

	switch(lua_type(L, 1))
	{
	case LUA_TSTRING:
		s = lua_tostring(L, 1);
		lua_pushboolean(L, *s != 0);	//empty string is considered false. WARNING: vanilla QC considered empty-but-not-null strings to be true
		break;

	default:
//	case LUA_TUSERDATA:
//	case LUA_TTHREAD:
//	case LUA_TLIGHTUSERDATA:
//	case LUA_TNONE:
//	case LUA_TNIL:
		lua_pushboolean(L, false);
		break;
	case LUA_TBOOLEAN:
		lua_pushvalue(L, 1);
		break;

	case LUA_TNUMBER:
		lua_pushboolean(L, lua_tonumber(L, 1) != 0);
		break;

	case LUA_TFUNCTION:	//functions are always considered true. otherwise they're nil and not functions.
		lua_pushboolean(L, true);
		break;
	case LUA_TTABLE:
		//might be a vector or an entity.
		lua_getfield(L, 1, "entnum");
		if (!lua_isnil(L, -1))
		{	//okay, so its a table with a valid entnum field. must be an entity.
			lua_pushboolean(L, 0!=lua_tointegerx(L, -1, NULL));	//true if its not 0/world
		}
		else
		{	//assume that its a vector.
			//note that this means that any table without x|y|z fields will be considered false.
			vec3_t v;
			lua_readvector(L, 1, v);
			lua_pushboolean(L, v[0] || v[1] || v[2]);
		}
		break;
	}
	return 1;
}

#define registerfunc(n) lua_pushcclosure(L, bi_lua_##n, 0); lua_setglobal(L, #n);
#define registerfuncn(n) registerfunc(n)	//new crap
#define registerfuncd(n) registerfunc(n)	//deprecated crap
static void my_lua_registerbuiltins(lua_State *L)
{
	lua_atpanic (L, my_lua_panic);

	lua_pushglobaltable(L);
	lua_setfield(L, -1, "_G");

	//standard lua library replacement
	//this avoids the risk of including any way to access os.execute etc, or other file access.
	registerfuncn(tostring);	//standardish
	registerfuncn(tonumber);	//standardish
	registerfuncn(type);		//standardish
	registerfuncn(print);		//'standard' lua print, except prints to console. WARNING: this adds an implicit \n. Use conprint for the quake-style version.
	registerfuncn(require);		//'standard'ish, except uses quake's filesystem instead of reading random system paths.
#ifdef LIBLUA_STATIC
	registerfuncn(pairs);
#endif
	lua_pushnil(L); lua_setglobal(L, "dofile");	//violates our sandbox
	lua_pushnil(L); lua_setglobal(L, "loadfile"); //violates our sandbox

	lua_newtable(L);
		lua_pushcclosure(L, bi_lua_fabs, 0); lua_setfield(L, -2, "abs");
		lua_pushcclosure(L, bi_lua_rint, 0); lua_setfield(L, -2, "rint");
		lua_pushcclosure(L, bi_lua_ceil, 0); lua_setfield(L, -2, "ceil");
		lua_pushcclosure(L, bi_lua_floor, 0); lua_setfield(L, -2, "floor");
		lua_pushcclosure(L, bi_lua_sin, 0); lua_setfield(L, -2, "sin");
		lua_pushcclosure(L, bi_lua_cos, 0); lua_setfield(L, -2, "cos");
		lua_pushcclosure(L, bi_lua_tan, 0); lua_setfield(L, -2, "tan");
		lua_pushcclosure(L, bi_lua_asin, 0); lua_setfield(L, -2, "asin");
		lua_pushcclosure(L, bi_lua_acos, 0); lua_setfield(L, -2, "acos");
		lua_pushcclosure(L, bi_lua_atan, 0); lua_setfield(L, -2, "atan");
		lua_pushcclosure(L, bi_lua_atan2, 0); lua_setfield(L, -2, "atan2");
		lua_pushcclosure(L, bi_lua_sqrt, 0); lua_setfield(L, -2, "sqrt");
		lua_pushcclosure(L, bi_lua_pow, 0); lua_setfield(L, -2, "pow");
		lua_pushnumber  (L, M_PI);			lua_setfield(L, -2, "pi");
	lua_setglobal(L, "math");

//	registerfuncd(entities);

	registerfuncd(loadlua);	//should probably use 'require' instead.
	registerfuncn(vec3);
	registerfuncn(field);
#undef qtrue
	registerfuncn(qtrue);	//for auto-converted code that tests for truth amongst a myriad of different custom types...

	registerfunc(setmodel);
	registerfunc(precache_model);
	registerfuncd(precache_model2);	//pointless alternative name
	registerfunc(precache_sound);
	registerfuncd(precache_sound2);	//pointless alternative name
	registerfuncd(precache_file);	//empty pointless function...
	registerfunc(lightstyle);
	registerfunc(spawn);
	registerfunc(remove);
	registerfunc(nextent);
	registerfunc(nextclient);
	registerfunc(makestatic);
	registerfunc(setorigin);
	registerfunc(setsize);

	registerfuncn(conprint);	//dprint, without the developer

	registerfunc(bprint_qw);
	registerfunc(sprint_qw);
	if (progstype == PROG_QW)
	{
		lua_pushcclosure(L, bi_lua_conprint, 0); lua_setglobal(L, "dprint");	//ignores developer
		lua_pushcclosure(L, bi_lua_bprint_qw, 0); lua_setglobal(L, "bprint");	//an extra level arg
		lua_pushcclosure(L, bi_lua_sprint_qw, 0); lua_setglobal(L, "sprint");	//an extra level arg
	}
	else
	{
		lua_pushcclosure(L, bi_lua_dprint, 0); lua_setglobal(L, "dprint");	//responds to developer
		lua_pushcclosure(L, bi_lua_bprint_nq, 0); lua_setglobal(L, "bprint");	//no level arg
		lua_pushcclosure(L, bi_lua_sprint_nq, 0); lua_setglobal(L, "sprint");	//no level arg
	}
	registerfunc(centerprint);
	registerfunc(ambientsound);
	registerfunc(sound);
	registerfunc(random);
	registerfunc(checkclient);
	registerfunc(stuffcmd);
	registerfunc(localcmd);
	registerfuncd(cvar);			//gets a float value, never a string.
	registerfuncn(cvar_get);
	registerfunc(cvar_set);
	registerfuncd(findradius);		//qc legacy compat. should probably warn when its called or sommit.
	registerfuncn(findradiuschain);	//renamed from qc. because qc's behaviour is annoying enough that we want to discourage its use.
	registerfuncn(findradiustable);	//findradius, but returns an array/table instead.

	registerfunc(objerror);
	registerfunc(error);
//	registerfuncd(break);	//won't implement

	registerfunc(traceline);
	registerfuncn(tracebox);
	registerfunc(walkmove);
	registerfunc(movetogoal);
	registerfunc(droptofloor);
	registerfunc(checkbottom);
	registerfunc(pointcontents);
	registerfuncd(ChangeYaw);

	registerfunc(setspawnparms);
	registerfunc(changelevel);
	registerfunc(logfrag);
	registerfuncd(infokey);
	registerfuncn(getinfokey);
	registerfuncn(setinfokey);
	registerfunc(multicast);
	registerfuncn(writebyte);
	registerfuncn(writechar);
	registerfuncn(writeshort);
	registerfuncn(writelong);
	registerfuncn(writeangle);
	registerfuncn(writecoord);
	registerfuncn(writestring);
	registerfuncn(writeentity);
	registerfuncd(WriteByte);
	registerfuncd(WriteChar);
	registerfuncd(WriteShort);
	registerfuncd(WriteLong);
	registerfuncd(WriteAngle);
	registerfuncd(WriteCoord);
	registerfuncd(WriteString);
	registerfuncd(WriteEntity);
	registerfuncd(particle);
	registerfuncn(bitnot);
	registerfuncn(bitclear);
	registerfuncn(bitset);
	registerfuncn(bitor);
	registerfuncn(bitand);
	registerfuncn(bitxor);
	registerfuncn(sin);
	registerfuncn(cos);
	registerfuncn(acos);
	registerfuncn(asin);
	registerfuncn(atan);
	registerfuncn(atan2);
	registerfuncn(sqrt);
	registerfuncn(pow);
	registerfunc(floor);
	registerfunc(ceil);
	registerfunc(rint);
	registerfunc(fabs);
	registerfuncn(fopen);
	registerfuncn(fclose);
	registerfuncn(fgets);
	registerfuncn(fputs);
	registerfuncn(getfilelist);
	registerfuncd(find);

	//registerfunc(strftime);
	registerfunc(tokenize);
	registerfunc(makevectors);
	registerfunc(normalize);
	registerfunc(vectoangles);
	registerfunc(vectoyaw);
	registerfuncd(aim);	//original implementation nudges v_forward up or down so that keyboard players can still play.
	registerfunc(vtos);
	registerfunc(ftos);
	registerfunc(stof);

	//registerfunc(PRECACHE_VWEP_MODEL);
	//registerfunc(SETPAUSE);


	//set a metatable on the globals table
	//this means that we can just directly use self.foo instead of blob.self.foo
	lua_pushglobaltable(L);
	if (luaL_newmetatable(L, "globals"))
	{
		lua_pushcclosure(L, my_lua_global_set, 0);	//for the luls.
		lua_setfield (L, -2, "__newindex");

		lua_pushcclosure(L, my_lua_global_get, 0);	//for the luls.
		lua_setfield (L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	lua_pop(L, 1);

	if (luaL_newmetatable(L, "vec3_t"))
	{
//		lua_pushcclosure(L, my_lua_vec3_set, 0);	//known writes should change the internal info so the engine can use the information.
//		lua_setfield (L, -2, "__newindex");

//		lua_pushcclosure(L, my_lua_vec3_get, 0);	//we need to de-translate the engine's fields too.
//		lua_setfield (L, -2, "__index");

		lua_pushcclosure(L, my_lua_vec3_tostring, 0);	//cos its prettier than seeing 'table 0x5425729' all over the place
		lua_setfield (L, -2, "__tostring");

		lua_pushcclosure(L, my_lua_vec3_eq, 0);	//for comparisons, you know?
		lua_setfield (L, -2, "__eq");

		lua_pushcclosure(L, my_lua_vec3_add, 0);	//for comparisons, you know?
		lua_setfield (L, -2, "__add");

		lua_pushcclosure(L, my_lua_vec3_sub, 0);	//for comparisons, you know?
		lua_setfield (L, -2, "__sub");

		lua_pushcclosure(L, my_lua_vec3_mul, 0);	//for comparisons, you know?
		lua_setfield (L, -2, "__mul");

		lua_pushcclosure(L, my_lua_vec3_len, 0);	//for comparisons, you know?
		lua_setfield (L, -2, "__len");

		lua_pop(L, 1);
	}
}






static edict_t *QDECL Lua_EdictNum(pubprogfuncs_t *pf, unsigned int num)
{
	int newcount;
	if (num >= lua.maxedicts)
	{
		newcount = num + 64;
		lua.edicttable = realloc(lua.edicttable, newcount*sizeof(*lua.edicttable));
		while(lua.maxedicts < newcount)
			lua.edicttable[lua.maxedicts++] = NULL;

		pf->edicttable_length = lua.maxedicts;
		pf->edicttable = lua.edicttable;
	}
	return lua.edicttable[num];
}
static unsigned int QDECL Lua_NumForEdict(pubprogfuncs_t *pf, edict_t *e)
{
	return e->entnum;
}
static int QDECL Lua_EdictToProgs(pubprogfuncs_t *pf, edict_t *e)
{
	return e->entnum;
}
static edict_t *QDECL Lua_ProgsToEdict(pubprogfuncs_t *pf, int num)
{
	return Lua_EdictNum(pf, num);
}
void Lua_EntClear (pubprogfuncs_t *pf, edict_t *e)
{
	int num = e->entnum;
	memset (e->v, 0, sv.world.edict_size);
	e->ereftype = ER_ENTITY;
	e->entnum = num;
}
edict_t *Lua_CreateEdict(unsigned int num)
{
	edict_t *e;
	e = lua.edicttable[num] = Z_Malloc(sizeof(edict_t) + sv.world.edict_size);
	e->v = (stdentvars_t*)(e+1);
#ifdef VM_Q1
	e->xv = (extentvars_t*)(e->v + 1);
#endif
	e->entnum = num;
	return e;
}
static void QDECL Lua_EntRemove(pubprogfuncs_t *pf, edict_t *e, qboolean instant)
{
	lua_State *L = lua.ctx;

	if (!ED_CanFree(e))
		return;
	e->ereftype = ER_FREE;
	e->freetime = (instant?0:sv.time); //can respawn instantly when asked.

	//clear out the lua version of the entity, so that it can be garbage collected.
	//should probably clear out its entnum field too, just in case.
	lua_pushlightuserdata(L, e);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
}
static edict_t *Lua_DoRespawn(pubprogfuncs_t *pf, edict_t *e, int num)
{
	lua_State *L = lua.ctx;
	if (!e)
		e = Lua_CreateEdict(num);
	else
		Lua_EntClear (pf, e);

	ED_Spawned((struct edict_s *) e, false);

	//create a new table for the entity, give it a suitable metatable, and store it into the registry (avoiding GC and allowing us to actually hold on to it).
	lua_pushlightuserdata(L, lua.edicttable[num]);
	lua_newtable(L);
	if (luaL_newmetatable(L, "entity"))
	{
		lua_pushcclosure(L, my_lua_entity_set, 0);	//known writes should change the internal info so the engine can use the information.
		lua_setfield (L, -2, "__newindex");

		lua_pushcclosure(L, my_lua_entity_get, 0);	//we need to de-translate the engine's fields too.
		lua_setfield (L, -2, "__index");

		lua_pushcclosure(L, my_lua_entity_tostring, 0);	//cos its prettier than seeing 'table 0x5425729' all over the place
		lua_setfield (L, -2, "__tostring");

		lua_pushcclosure(L, my_lua_entity_eq, 0);	//for comparisons, you know?
		lua_setfield (L, -2, "__eq");
	}
	lua_setmetatable(L, -2);
	lua_pushinteger(L, num);
	lua_setfield (L, -2, "entnum");	//so we know which entity it is.
	lua_settable(L, LUA_REGISTRYINDEX);
	return e;
}
static edict_t *QDECL Lua_EntAlloc(pubprogfuncs_t *pf, pbool isobject, size_t extrasize)
{
	int i;
	edict_t *e;
	for ( i=0 ; i<sv.world.num_edicts ; i++)
	{
		e = (edict_t*)EDICT_NUM_PB(pf, i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e || (ED_ISFREE(e) && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) ))
		{
			e = Lua_DoRespawn(pf, e, i);
			return (struct edict_s *)e;
		}
	}

	if (i >= sv.world.max_edicts-1)	//try again, but use timed out ents.
	{
		for ( i=0 ; i<sv.world.num_edicts ; i++)
		{
			e = (edict_t*)EDICT_NUM_PB(pf, i);
			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if (!e || ED_ISFREE(e))
			{
				e = Lua_DoRespawn(pf, e, i);
				return (struct edict_s *)e;
			}
		}

		if (i >= sv.world.max_edicts-1)
		{
			Sys_Error ("ED_Alloc: no free edicts");
		}
	}

	sv.world.num_edicts++;
	e = Lua_EdictNum(pf, i);

	e = Lua_DoRespawn(pf, e, i);

	return (struct edict_s *)e;
}

/*static int QDECL Lua_LoadEnts(pubprogfuncs_t *pf, const char *mapstring, void *ctx, void (PDECL *callback) (pubprogfuncs_t *progfuncs, struct edict_s *ed, void *ctx, const char *entstart, const char *entend))
{
	lua_State *L = lua.ctx;
	int i = 0;
	lua_getglobal(L, "LoadEnts");
	lua_newtable(L);
	while(NULL != (mapstring = COM_Parse(mapstring)))
	{
		lua_pushinteger(L, i++);
		lua_pushstring(L, com_token);
		lua_settable(L, -3);
	}
//	lua_pushinteger(L, spawnflags);

	if (lua_pcall(L, 2, 0, 0) != 0)
	{
		const char *s = lua_tolstring(L, -1, NULL);
		Con_Printf(CON_WARNING "%s\n", s);
		lua_pop(L, 1);
	}

	return sv.world.edict_size;
}*/

static const char *Lua_ParseEdict (pubprogfuncs_t *progfuncs, const char *data, struct edict_s *ent)
{
	lua_State *L = lua.ctx;

//	fdef_t		*key;
	pbool	init;
	char		keyname[256];
	int			n;
	int			nest = 1;
	char		token[8192];
	luafld_t *fld;

//	eval_t		*val;

	init = false;

// go through all the dictionary pairs
	while (1)
	{
	// parse key
		data = COM_ParseOut(data, token, sizeof(token));
		if (token[0] == '}')
		{
			if (--nest)
				continue;
			break;
		}
		if (token[0] == '{' && !token[1])
			nest++;
		if (!data)
		{
			Con_Printf ("Lua_ParseEdict: EOF without closing brace\n");
			return NULL;
		}
		if (nest > 1)
			continue;

		strncpy (keyname, token, sizeof(keyname)-1);
		keyname[sizeof(keyname)-1] = 0;

		// another hack to fix heynames with trailing spaces
		n = strlen(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value
		data = COM_ParseOut(data, token, sizeof(token));
		if (!data)
		{
			Con_Printf ("Lua_ParseEdict: EOF without closing brace\n");
			return NULL;
		}

		if (token[0] == '}')
		{
			Con_Printf ("Lua_ParseEdict: closing brace without data\n");
			return NULL;
		}

		init = true;

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;



		if (!strcmp(keyname, "angle"))	//Quake anglehack - we've got to leave it in cos it doesn't work for quake otherwise, and this is a QuakeC lib!
		{
			Q_snprintfz (keyname, sizeof(keyname), "angles");	//change it from yaw to 3d angle
			Q_snprintfz (token, sizeof(token), "0 %f 0", atof(token));	//change it from yaw to 3d angle
			goto cont;
		}
/*
		key = ED_FindField (progfuncs, keyname);
		if (!key)
		{
			if (!strcmp(keyname, "light"))	//Quake lighthack - allows a field name and a classname to go by the same thing in the level editor
				if ((key = ED_FindField (progfuncs, "light_lev")))
					goto cont;
			if (externs->badfield && externs->badfield(&progfuncs->funcs, (struct edict_s*)ent, keyname, qcc_token))
				continue;
			Con_DPrintf ("'%s' is not a field\n", keyname);
			continue;
		}
*/
cont:
		fld = Hash_GetInsensitive(&lua.entityfields, keyname);
		if (fld && fld->type == ev_float)
			lua_pushnumber(L, atof(token));
		else if (fld && fld->type == ev_integer)
			lua_pushinteger(L, atoi(token));
		else if (fld && fld->type == ev_vector)
		{
			char *e;
			float x,y,z;
			x = strtod(token, &e);
			if (*e == ' ')
				e++;
			y = strtod(e, &e);
			if (*e == ' ')
				e++;
			z = strtod(e, &e);
			if (*e == ' ')
				e++;
			lua_pushvector(L, x, y, z);
		}
		else if (fld && fld->type == ev_function)
			lua_getglobal(L, token);	//functions are nameless, except for how they're invoked. so this is only for evil mods...
		else if (fld && fld->type == ev_entity)
		{
			int num = atoi(token);
			lua_pushedict(L, EDICT_NUM_UB((&lua.progfuncs), num));
		}
		else
			lua_pushstring(L, token);
		lua_setfield(L, -2, keyname);

		/*if (!ED_ParseEpair (progfuncs, (char*)ent->fields - progfuncs->funcs.stringtable, key->ofs, key->type, qcc_token))
		{
			continue;
//			Sys_Error ("ED_ParseEdict: parse error on entities");
		}*/
	}

	if (!init)
		ent->ereftype = ER_FREE;

	return data;
}

static int QDECL Lua_LoadEnts(pubprogfuncs_t *pf, const char *mapstring, void *ctx,
														void (PDECL *memoryreset) (pubprogfuncs_t *progfuncs, void *ctx),
														void (PDECL *entspawned) (pubprogfuncs_t *progfuncs, struct edict_s *ed, void *ctx, const char *entstart, const char *entend),
														pbool(PDECL *extendedterm)(pubprogfuncs_t *progfuncs, void *ctx, const char **extline)
												) //restore the entire progs state (or just add some more ents) (returns edicts ize)
{
	lua_State *L = lua.ctx;
	struct edict_s *ed = NULL;
	const char *datastart = mapstring;

	lua_pushglobaltable(L);
	while (1)
	{
		datastart = mapstring;

		if (extendedterm)
		{
			//skip simple leading whitespace
			while (*mapstring == ' ' || *mapstring == '\t' || *mapstring == '\r' || *mapstring == '\n')
				mapstring++;
			if (mapstring[0] == '/' && mapstring[1] == '*')	//we are not reading lua here, so C-style comments are the proper form (otherwise ignored by COM_Parse, giving extensibility).
			{	//looks like we have a hidden extension.
				mapstring+=2;
				for(;;)
				{
					//skip to end of line
					if (!*mapstring)
						break;	//unexpected EOF
					else if (mapstring[0] == '*' && mapstring[1] == '/')
					{	//end of comment
						mapstring+=2;
						break;
					}
					else if (*mapstring != '\n')
					{
						mapstring++;
						continue;
					}
					mapstring++;	//skip past the \n
					while (*mapstring == ' ' || *mapstring == '\t')
						mapstring++;	//skip leading indentation

					if (mapstring[0] == '*' && mapstring[1] == '/')
					{	//end of comment
						mapstring+=2;
						break;
					}
					else if (*mapstring == '/')
						continue;	//embedded comment. ignore the line. not going to do nested comments, because those are not normally valid anyway, just C++-style inside C-style.
					else if (extendedterm(pf, ctx, &mapstring))
						;	//found a term we recognised
					else
						;	//unknown line, but this is a comment so whatever

				}
				continue;
			}
		}

		mapstring = COM_Parse(mapstring);
		if (!strcmp(com_token, "{"))
		{
			if (!ed)	//first entity
				ed = EDICT_NUM_PB(pf, 0);
			else
				ed = ED_Alloc(pf, false, 0);
			ed->ereftype = ER_ENTITY;
			if (pf->parms->entspawn)
				pf->parms->entspawn(ed, true);

			lua_pushedict(L, ed);
			mapstring = Lua_ParseEdict(pf, mapstring, ed);

			if (1)
			{	//we can't call the callback, as it would be unable to represent the function references.
				int spawnflags, killonspawnflags = *(int*)ctx;//lame. really lame
				lua_getfield(L, -1, "spawnflags");	//push -1["classname"]...
				spawnflags = lua_tointeger(L, -1);
				lua_pop(L, 1);
				if (spawnflags & killonspawnflags)
					lua.progfuncs.EntFree(&lua.progfuncs, ed, true);
				else
				{
					lua_getfield(L, -1, "classname");	//push -1["classname"]...
					//-3:globaltable, -2:enttable, -1:classname string
					lua_gettable(L, -3);	//pop the classname and look it up inside the global table (to find the function in question)
					lua.globals.self = ed->entnum;
					if (lua_pcall(L, 0, 0, 0) != 0)
					{
						const char *s = lua_tolstring(L, -1, NULL);
						lua_getfield(L, -2, "classname");	//push -1["classname"]...
						Con_Printf(CON_WARNING "spawn func %s: %s\n", lua_tolstring(L, -1, NULL), s);
						lua_pop(L, 2);
					}
				}
			}
			else
			{
				lua_pop(L, 1);
				entspawned(pf, ed, ctx, datastart, mapstring);
			}
			lua_pop(L, 1);	//pop ent table
		}
		else
			break;	//unexpected token...
	}
	lua_pop(L, 1);
	return sv.world.edict_size;
}

static eval_t *QDECL Lua_GetEdictFieldValue(pubprogfuncs_t *pf, edict_t *e, const char *fieldname, etype_t type, evalc_t *cache)
{
	eval_t *val;
	luafld_t *fld;
	fld = Hash_GetInsensitive(&lua.entityfields, fieldname);
	if (fld)
	{
		val = (eval_t*)((char*)e->v + fld->offset);
		return val;
	}
	return NULL;
}

static eval_t	*QDECL Lua_FindGlobal		(pubprogfuncs_t *prinst, const char *name, progsnum_t num, etype_t *type)
{
	eval_t *val;
	luafld_t *fld;
	fld = Hash_GetInsensitive(&lua.globalfields, name);
	if (fld)
	{
		val = (eval_t*)((char*)&lua.globals + fld->offset);
		return val;
	}

	Con_Printf("Lua_FindGlobal: %s\n", name);
	return NULL;
}
static func_t QDECL Lua_FindFunction		(pubprogfuncs_t *prinst, const char *name, progsnum_t num)
{
	eval_t *val;
	luafld_t *fld;
	fld = Hash_GetInsensitive(&lua.globalfields, name);
	if (fld)
	{
		val = (eval_t*)((char*)&lua.globals + fld->offset);
		return val->function;
	}

	Con_Printf("Lua_FindFunction: %s\n", name);
	return 0;
}

static globalvars_t *QDECL Lua_Globals(pubprogfuncs_t *prinst, int prnum)
{
//	Con_Printf("Lua_Globals: called\n");
	return NULL;
}

char *QDECL Lua_AddString(pubprogfuncs_t *prinst, const char *val, int minlength, pbool demarkup)
{
	char *ptr;
	int len = strlen(val)+1;
	if (len < minlength)
		len = minlength;
	ptr = Z_TagMalloc(len, LUA_MALLOC_TAG);
	strcpy(ptr, val);
	return ptr;
}
static string_t QDECL Lua_StringToProgs(pubprogfuncs_t *prinst, const char *str)
{
	Con_Printf("Lua_StringToProgs called instead of Lua_SetStringField\n");
	return 0;
}

//passing NULL for ed means its setting a global.
static void QDECL Lua_SetStringField(pubprogfuncs_t *prinst, edict_t *ed, string_t *fld, const char *str, pbool str_is_static)
{
	lua_State *L = lua.ctx;
	string_t val;
	string_t base;
	if (ed)
	{
		base = (ed->entnum+1)<<10;
		val = (char*)fld-(char*)ed->v;

		lua_pushedict(lua.ctx, lua.edicttable[ed->entnum]);
	}
	else
	{
		base = 0;
		val = (char*)fld-(char*)&lua.globals;

		//push the globals list
		lua_pushglobaltable(L);
	}
	*fld = base | val;	//set the engine's value

	//set the stuff so that lua can read it properly.
	lua_pushlightuserdata(L, (void *)(qintptr_t)val);
	lua_pushstring(L, str);
	lua_rawset(L, -3);

	//and pop the table
	lua_pop(L, 1);
}

static const char *ASMCALL QDECL Lua_StringToNative(pubprogfuncs_t *prinst, string_t str)
{
	const char *ret = "";
	unsigned int entnum = str >> 10;
	if (str)
	{
		str &= 1023;
		if (!entnum)
		{
			//okay, its the global table.
			lua_pushglobaltable(lua.ctx);
		}
		else
		{
			entnum-=1;
			if (entnum >= lua.maxedicts)
				return ret;	//erk...
			//get the entity's table
			lua_pushlightuserdata(lua.ctx, lua.edicttable[entnum]);
			lua_gettable(lua.ctx, LUA_REGISTRYINDEX);
		}

		//read the function from the table
		lua_pushlightuserdata(lua.ctx, (void *)(qintptr_t)str);
		lua_rawget(lua.ctx, -2);
		ret = lua_tolstring(lua.ctx, -1, NULL);
		lua_pop(lua.ctx, 2);	//pop the table+string.
		//popping the string is 'safe' on the understanding that the string reference is still held by its containing table, so don't store the string anywhere.
	}

	return ret;
}

static void Lua_Event_Touch(world_t *w, wedict_t *s, wedict_t *o, trace_t *trace)
{
	int oself = pr_global_struct->self;
	int oother = pr_global_struct->other;

	pr_global_struct->self = EDICT_TO_PROG(w->progs, s);
	pr_global_struct->other = EDICT_TO_PROG(w->progs, o);
	pr_global_struct->time = w->physicstime;

#if 1
	PR_ExecuteProgram (w->progs, s->v->touch);
#else
	lua_pushedict(lua.ctx, s);
	//lua_pushliteral(lua.ctx, "touch");
	lua_pushlightuserdata(lua.ctx, (void*)((char*)&s->v->touch-(char*)s->v));
	lua_rawget(lua.ctx, -2);
	lua_replace(lua.ctx, -2);
	if (lua_pcall(lua.ctx, 0, 0, 0) != 0)
	{
		const char *e = lua_tolstring(lua.ctx, -1, NULL);
		lua_pushedict(lua.ctx, (struct edict_s*)s);
		lua_getfield(lua.ctx, -1, "classname");
		Con_Printf(CON_WARNING "%s touch: %s\n", lua_tostring(lua.ctx, -1), e);
		lua_pop(lua.ctx, 3);	//error, enttable, classname
	}
#endif

	pr_global_struct->self = oself;
	pr_global_struct->other = oother;
}

static void Lua_Event_Think(world_t *w, wedict_t *s)
{
	pr_global_struct->self = EDICT_TO_PROG(w->progs, s);
	pr_global_struct->other = EDICT_TO_PROG(w->progs, w->edicts);

#if 0
	PR_ExecuteProgram (w->progs, s->v->think);
#else
	lua_pushedict(lua.ctx, (struct edict_s*)s);
//	lua_pushliteral(lua.ctx, "think");
	lua_pushlightuserdata(lua.ctx, (void*)((char*)&s->v->think-(char*)s->v));
	lua_rawget(lua.ctx, -2);
	lua_replace(lua.ctx, -2);
	if (lua_pcall(lua.ctx, 0, 0, 0) != 0)
	{
		const char *e = lua_tolstring(lua.ctx, -1, NULL);
		lua_pushedict(lua.ctx, (struct edict_s*)s);
		lua_getfield(lua.ctx, -1, "classname");
		Con_Printf(CON_WARNING "%s think: %s\n", lua_tostring(lua.ctx, -1), e);
		lua_pop(lua.ctx, 3);	//error, enttable, classname
	}
#endif
}

static qboolean Lua_Event_ContentsTransition(world_t *w, wedict_t *ent, int oldwatertype, int newwatertype)
{
	return false;	//always do legacy behaviour, because I cba implementing anything else.
}

static void Lua_SetupGlobals(world_t *world)
{
	int flds;
	int bucks;
	comentvars_t	*v = NULL;
	extentvars_t	*xv = (extentvars_t*)(v+1);

	memset(&lua.globals, 0, sizeof(lua.globals));
	lua.globals.physics_mode = 2;
	lua.globals.dimension_send = 255;
	lua.globals.dimension_default = 255;
	lua.globals.global_gravitydir[2] = -1;

	flds = 0;
	bucks = 64;
	Hash_InitTable(&lua.globalfields, bucks, Z_Malloc(Hash_BytesForBuckets(bucks)));

//WARNING: global is not remapped yet...
//This code is written evilly, but works well enough
#define doglobal(n, t)	\
		pr_global_ptrs->n = &lua.globals.n;	\
		lua.globflds[flds].offset = (char*)&lua.globals.n - (char*)&lua.globals;	\
		lua.globflds[flds].name = #n;		\
		lua.globflds[flds].type = t;		\
		Hash_AddInsensitive(&lua.globalfields, lua.globflds[flds].name, &lua.globflds[flds], &lua.globflds[flds].buck);	\
		flds++;
#define doglobal_v(o, f, t)	\
		lua.globflds[flds].offset = (char*)&lua.globals.o - (char*)&lua.globals;	\
		lua.globflds[flds].name = #f;		\
		lua.globflds[flds].type = t;		\
		Hash_AddInsensitive(&lua.globalfields, lua.globflds[flds].name, &lua.globflds[flds], &lua.globflds[flds].buck);	\
		flds++;
#define globalentity(required, name) doglobal(name, ev_entity)
#define globalint(required, name) doglobal(name, ev_integer)
#define globalfloat(required, name) doglobal(name, ev_float)
#define globalstring(required, name) doglobal(name, ev_string)
#define globalvec(required, name) doglobal(name, ev_vector) doglobal_v(name[0], name##_x, ev_float) doglobal_v(name[1], name##_y, ev_float) doglobal_v(name[2], name##_z, ev_float)
#define globalfunc(required, name) doglobal(name, ev_function)
	luagloballist
#undef doglobal
#define doglobal(n, t) doglobal_v(n,n,t)
	luaextragloballist

#define parm(n)\
		pr_global_ptrs->spawnparamglobals[n] = &lua.globals.parm[n];	\
		lua.globflds[flds].offset = (char*)&lua.globals.parm[n] - (char*)&lua.globals;	\
		lua.globflds[flds].name = "parm"#n;		\
		lua.globflds[flds].type = ev_float;		\
		Hash_AddInsensitive(&lua.globalfields, lua.globflds[flds].name, &lua.globflds[flds], &lua.globflds[flds].buck);	\
		flds++
	parm( 0);parm( 1);parm( 2);parm( 3);parm( 4);parm( 5);parm( 6);parm( 7);
	parm( 8);parm( 9);parm(10);parm(11);parm(12);parm(13);parm(14);parm(15);
#undef parm

	lua.numflds = 0;
	bucks = 256;
	Hash_InitTable(&lua.entityfields, bucks, Z_Malloc(Hash_BytesForBuckets(bucks)));


#define doefield(n, t)	\
		lua.entflds[lua.numflds].offset = (char*)&v->n - (char*)v;	\
		lua.entflds[lua.numflds].name = #n;		\
		lua.entflds[lua.numflds].type = t;		\
		Hash_AddInsensitive(&lua.entityfields, lua.entflds[lua.numflds].name, &lua.entflds[lua.numflds], &lua.entflds[lua.numflds].buck);	\
		lua.numflds++;
#define doefield_v(o, f, t)	\
		lua.entflds[lua.numflds].offset = (char*)&v->o - (char*)v;	\
		lua.entflds[lua.numflds].name = #f;		\
		lua.entflds[lua.numflds].type = t;		\
		Hash_AddInsensitive(&lua.entityfields, lua.entflds[lua.numflds].name, &lua.entflds[lua.numflds], &lua.entflds[lua.numflds].buck);	\
		lua.numflds++;
#define comfieldentity(name,desc) doefield(name, ev_entity)
#define comfieldint(name,desc) doefield(name, ev_integer)
#define comfieldfloat(name,desc) doefield(name, ev_float)
#define comfieldstring(name,desc) doefield(name, ev_string)
#define comfieldvector(name,desc) doefield(name, ev_vector) doefield_v(name[0], name##_x, ev_float) doefield_v(name[1], name##_y, ev_float) doefield_v(name[2], name##_z, ev_float)
#define comfieldfunction(name,typestr,desc) doefield(name, ev_function)
	comqcfields
#undef doefield
#undef doefield_v
#define doefield(n, t)	\
		lua.entflds[lua.numflds].offset = (char*)&xv->n - (char*)v;	\
		lua.entflds[lua.numflds].name = #n;		\
		lua.entflds[lua.numflds].type = t;		\
		Hash_AddInsensitive(&lua.entityfields, lua.entflds[lua.numflds].name, &lua.entflds[lua.numflds], &lua.entflds[lua.numflds].buck);	\
		lua.numflds++;
#define doefield_v(o, f, t)	\
		lua.entflds[lua.numflds].offset = (char*)&xv->o - (char*)v;	\
		lua.entflds[lua.numflds].name = #f;		\
		lua.entflds[lua.numflds].type = t;		\
		Hash_AddInsensitive(&lua.entityfields, lua.entflds[lua.numflds].name, &lua.entflds[lua.numflds], &lua.entflds[lua.numflds].buck);	\
		lua.numflds++;
	comextqcfields
	svextqcfields

	PR_SV_FillWorldGlobals(world);
}

void QDECL Lua_ExecuteProgram(pubprogfuncs_t *funcs, func_t func)
{
	unsigned int entnum = func >> 10;
	func &= 1023;
	if (!entnum)
	{
		//okay, its the global table.
		lua_pushglobaltable(lua.ctx);
	}
	else
	{
		entnum-=1;
		if (entnum >= lua.maxedicts)
			return;	//erk...
		//get the entity's table
		lua_pushlightuserdata(lua.ctx, lua.edicttable[entnum]);
		lua_gettable(lua.ctx, LUA_REGISTRYINDEX);
	}

	//read the function from the table
	lua_pushlightuserdata(lua.ctx, (void *)(qintptr_t)func);
	lua_rawget(lua.ctx, -2);

	//and now invoke it.
	if (lua_pcall(lua.ctx, 0, 0, 0) != 0)
	{
		const char *s = lua_tolstring(lua.ctx, -1, NULL);
		Con_Printf(CON_WARNING "%s\n", s);
		lua_pop(lua.ctx, 1);
	}
}

void PDECL Lua_CloseProgs(pubprogfuncs_t *inst)
{
	lua_close(lua.ctx);
	free(lua.edicttable);
	lua.edicttable = NULL;
	lua.maxedicts = 0;

	memset(&lua, 0, sizeof(lua));

	Z_FreeTags(LUA_MALLOC_TAG);
}

static void QDECL Lua_Get_FrameState(world_t *w, wedict_t *ent, framestate_t *fstate)
{
	memset(fstate, 0, sizeof(*fstate));
	fstate->g[FS_REG].frame[0] = ent->v->frame;
	fstate->g[FS_REG].frametime[0] = ent->xv->frame1time;
	fstate->g[FS_REG].lerpweight[0] = 1;
	fstate->g[FS_REG].endbone = 0x7fffffff;

	fstate->g[FST_BASE].frame[0] = ent->xv->baseframe;
	fstate->g[FST_BASE].frametime[0] = ent->xv->/*base*/frame1time;
	fstate->g[FST_BASE].lerpweight[0] = 1;
	fstate->g[FST_BASE].endbone = ent->xv->basebone;

#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	if (ent->xv->skeletonindex)
		skel_lookup(w, ent->xv->skeletonindex, fstate);
#endif
}

qboolean PR_LoadLua(void)
{
	world_t *world = &sv.world;
	pubprogfuncs_t *pf;
	vfsfile_t *sourcefile = NULL;
	if ((sourcefile = FS_OpenVFS("qwprogs.lua", "rb", FS_GAME)))
		progstype = PROG_QW;
	else if ((sourcefile = FS_OpenVFS("progs.lua", "rb", FS_GAME)))
		progstype = PROG_NQ;
	else
		return false;

	if (!init_lua())
	{
		VFS_CLOSE(sourcefile);
		Con_Printf("WARNING: Found progs.lua, but could load load lua library\n");
		return false;
	}

	pf = svprogfuncs = &lua.progfuncs;

	pf->Shutdown = Lua_CloseProgs;
	pf->AddString = Lua_AddString;
	pf->EdictNum = Lua_EdictNum;
	pf->NumForEdict = Lua_NumForEdict;
	pf->EdictToProgs = Lua_EdictToProgs;
	pf->ProgsToEdict = Lua_ProgsToEdict;
	pf->EntAlloc = Lua_EntAlloc;
	pf->EntFree = Lua_EntRemove;
	pf->EntClear = Lua_EntClear;
	pf->FindGlobal = Lua_FindGlobal;
	pf->load_ents = Lua_LoadEnts;
	pf->globals = Lua_Globals;
	pf->GetEdictFieldValue = Lua_GetEdictFieldValue;
	pf->SetStringField = Lua_SetStringField;
	pf->StringToProgs = Lua_StringToProgs;
	pf->StringToNative = Lua_StringToNative;
	pf->ExecuteProgram = Lua_ExecuteProgram;
	pf->FindFunction = Lua_FindFunction;

	world->Event_Touch = Lua_Event_Touch;
	world->Event_Think = Lua_Event_Think;
	world->Event_Sound = SVQ1_StartSound;
	world->Event_ContentsTransition = Lua_Event_ContentsTransition;
	world->Get_CModel = SVPR_GetCModel;
	world->Get_FrameState = Lua_Get_FrameState;

	world->progs = pf;
	world->progs->parms = &lua.progfuncsparms;
	world->progs->parms->user = world;
	world->progs->parms->Printf = PR_Printf;
	world->progs->parms->DPrintf = PR_DPrintf;
	world->usesolidcorpse = true;

	Lua_SetupGlobals(world);

	svs.numprogs = 0;	//Why is this svs?
#ifdef VM_Q1
	world->edict_size = sizeof(stdentvars_t) + sizeof(extentvars_t);
#else
	world->edict_size = sizeof(stdentvars_t);
#endif

	//force items2 instead of serverflags
	sv.haveitems2 = true;

	//initalise basic lua context
	lua.ctx = lua_newstate(my_lua_alloc, NULL);					//create our lua state
//	luaL_openlibs(lua.ctx); 
	my_lua_registerbuiltins(lua.ctx);

	//spawn the world, woo.
	world->edicts = (wedict_t*)pf->EntAlloc(pf,false,0);

	//load the gamecode now. it should be safe for it to call various builtins.
	if (0 != lua_load(lua.ctx, my_lua_Reader, sourcefile, "progs.lua", "bt"))	//load the file, embed it within a function and push it
	{
		Con_Printf("Error trying to parse %s: %s\n", "progs.lua", lua_tolstring(lua.ctx, -1, NULL));
		lua_pop(lua.ctx, 1);
	}
	else
	{
		if (lua_pcall(lua.ctx, 0, 0, 0) != 0)
		{
			const char *s = lua_tolstring(lua.ctx, -1, NULL);
			Con_Printf(CON_WARNING "%s\n", s);
			lua_pop(lua.ctx, 1);
		}
	}
	VFS_CLOSE(sourcefile);

	return true;
}
#endif	//VM_LUA
