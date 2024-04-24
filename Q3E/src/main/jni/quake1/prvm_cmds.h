
#ifndef PRVM_CMDS_H
#define PRVM_CMDS_H

// AK
// Basically every vm builtin cmd should be in here.
// All 3 builtin and extension lists can be found here
// cause large (I think they will) parts are from pr_cmds the same copyright like in pr_cmds
// also applies here


/*
============================================================================
common cmd list:
=================

		checkextension(string)
		error(...[string])
		objerror(...[string)
		print(...[strings])
		bprint(...[string])
		sprint(float clientnum,...[string])
		centerprint(...[string])
vector	normalize(vector)
float	vlen(vector)
float	vectoyaw(vector)
vector	vectoangles(vector)
float	random()
		cmd(string)
		float cvar (string)
		cvar_set (string,string)
		dprint(...[string])
string	ftos(float)
float	fabs(float)
string	vtos(vector)
string	etos(entity)
float	stof(...[string])
entity	spawn()
		remove(entity e)
entity	find(entity start, .string field, string match)

entity	findfloat(entity start, .float field, float match)
entity	findentity(entity start, .entity field, entity match)

entity	findchain(.string field, string match)

entity	findchainfloat(.string field, float match)
entity	findchainentity(.string field, entity match)

string	precache_file(string)
string	precache_sound (string sample)
		coredump()
		traceon()
		traceoff()
		eprint(entity e)
float	rint(float)
float	floor(float)
float	ceil(float)
entity	nextent(entity)
float	sin(float)
float	cos(float)
float	sqrt(float)
vector	randomvec()
float	registercvar (string name, string value, float flags)
float	min(float a, float b, ...[float])
float	max(float a, float b, ...[float])
float	bound(float min, float value, float max)
float	pow(float a, float b)
		copyentity(entity src, entity dst)
float	fopen(string filename, float mode)
		fclose(float fhandle)
string	fgets(float fhandle)
		fputs(float fhandle, string s)
float	strlen(string s)
string	strcat(string,string,...[string])
string	substring(string s, float start, float length)
vector	stov(string s)
string	strzone(string s)
		strunzone(string s)
float	tokenize(string s)
string	argv(float n)
float	isserver()
float	clientcount()
float	clientstate()
		clientcommand(float client, string s) (for client and menu)
		changelevel(string map)
		localsound(string sample)
vector	getmousepos()
float	gettime()
		loadfromdata(string data)
		loadfromfile(string file)
		parseentitydata(entity ent, string data)
float	mod(float val, float m)
const string	cvar_string (string)
float	cvar_type (string)
		crash()
		stackdump()

float	search_begin(string pattern, float caseinsensitive, float quiet)
void	search_end(float handle)
float	search_getsize(float handle)
string	search_getfilename(float handle, float num)

string	chr(float ascii)

float	itof(intt ent)
entity	ftoe(float num)

-------will be removed soon----------
float	altstr_count(string)
string	altstr_prepare(string)
string	altstr_get(string,float)
string	altstr_set(string altstr, float num, string set)
string	altstr_ins(string altstr, float num, string set)
--------------------------------------

entity	findflags(entity start, .float field, float match)
entity	findchainflags(.float field, float match)

const string	VM_cvar_defstring (string)

perhaps only : Menu : WriteMsg
===============================

		WriteByte(float data, float dest, float desto)
		WriteChar(float data, float dest, float desto)
		WriteShort(float data, float dest, float desto)
		WriteLong(float data, float dest, float desto)
		WriteAngle(float data, float dest, float desto)
		WriteCoord(float data, float dest, float desto)
		WriteString(string data, float dest, float desto)
		WriteEntity(entity data, float dest, float desto)

Client & Menu : draw functions & video functions (& gecko functions)
===================================================

float	iscachedpic(string pic)
string	precache_pic(string pic)
		freepic(string s)
float	drawcharacter(vector position, float character, vector scale, vector rgb, float alpha, float flag)
float	drawstring(vector position, string text, vector scale, vector rgb, float alpha, float flag)
float	drawcolorcodedstring(vector position, string text, vector scale, float alpha, float flag)
float	stringwidth(string text, float handleColors)
float	drawpic(vector position, string pic, vector size, vector rgb, float alpha, float flag)
float	drawsubpic(vector position, vector size, string pic, vector srcPos, vector srcSize, vector rgb, float alpha, float flag)
float	drawfill(vector position, vector size, vector rgb, float alpha, float flag)
		drawsetcliparea(float x, float y, float width, float height)
		drawresetcliparea()
vector	getimagesize(string pic)

float	cin_open(string file, string name)
void	cin_close(string name)
void	cin_setstate(string name, float type)
float	cin_getstate(string name)
void	cin_restart(string name)

float[bool] gecko_create( string name )
void gecko_destroy( string name )
void gecko_navigate( string name, string URI )
float[bool] gecko_keyevent( string name, float key, float eventtype ) 
void gecko_mousemove( string name, float x, float y )

==============================================================================
menu cmd list:
===============

		setkeydest(float dest)
float	getkeydest()
		setmousetarget(float target)
float	getmousetarget()

		callfunction(...,string function_name)
		writetofile(float fhandle, entity ent)
float	isfunction(string function_name)
vector	getresolution(float number)
string	keynumtostring(float keynum)
string	findkeysforcommand(string command)
float	getserverliststat(float type)
string	getserverliststring(float fld, float hostnr)

float	stringtokeynum(string key)

		resetserverlistmasks()
		setserverlistmaskstring(float mask, float fld, string str)
		setserverlistmasknumber(float mask, float fld, float num, float op)
		resortserverlist()
		setserverlistsort(float field, float descending)
		refreshserverlist()
float	getserverlistnumber(float fld, float hostnr)
float	getserverlistindexforkey(string key)
		addwantedserverlistkey(string key)
*/

#include "quakedef.h"
#include "progdefs.h"
#include "progsvm.h"
#include "clprogdefs.h"
#include "mprogdefs.h"

#include "cl_video.h"

//============================================================================
// nice helper macros

#ifndef VM_NOPARMCHECK
#define VM_SAFEPARMCOUNTRANGE(p1,p2,f)	if(prog->argc < p1 || prog->argc > p2) prog->error_cmd(#f " wrong parameter count %i (" #p1 " to " #p2 " expected ) !", prog->argc)
#define VM_SAFEPARMCOUNT(p,f)	if(prog->argc != p) prog->error_cmd(#f " wrong parameter count %i (" #p " expected ) !", prog->argc)
#else
#define VM_SAFEPARMCOUNTRANGE(p1,p2,f)
#define VM_SAFEPARMCOUNT(p,f)
#endif

#define	VM_RETURN_EDICT(e)		(prog->globals.ip[OFS_RETURN] = PRVM_EDICT_TO_PROG(e))

#define VM_TEMPSTRING_MAXSIZE MAX_INPUTLINE

// general functions
void VM_CheckEmptyString (prvm_prog_t *prog, const char *s);
/// Returns the length of the *out string excluding the \0 terminator.
size_t VM_VarString(prvm_prog_t *prog, int first, char *out, size_t outsize);
qbool PRVM_ConsoleCommand(prvm_prog_t *prog, const char *text, size_t textlen, int *func, qbool preserve_self, int curself, double ptime, const char *error_message);
prvm_stringbuffer_t *BufStr_FindCreateReplace (prvm_prog_t *prog, int bufindex, unsigned flags, const char *format);
void BufStr_Set(prvm_prog_t *prog, prvm_stringbuffer_t *stringbuffer, int strindex, const char *str);
void BufStr_Del(prvm_prog_t *prog, prvm_stringbuffer_t *stringbuffer);
void BufStr_Flush(prvm_prog_t *prog);

// builtins
void VM_checkextension (prvm_prog_t *prog);
void VM_error (prvm_prog_t *prog);
void VM_objerror (prvm_prog_t *prog);
void VM_print (prvm_prog_t *prog);
void VM_bprint (prvm_prog_t *prog);
void VM_sprint (prvm_prog_t *prog);
void VM_centerprint (prvm_prog_t *prog);
void VM_normalize (prvm_prog_t *prog);
void VM_vlen (prvm_prog_t *prog);
void VM_vectoyaw (prvm_prog_t *prog);
void VM_vectoangles (prvm_prog_t *prog);
void VM_random (prvm_prog_t *prog);
void VM_localsound(prvm_prog_t *prog);
void VM_break (prvm_prog_t *prog);
void VM_localcmd(prvm_prog_t *prog);
void VM_cvar (prvm_prog_t *prog);
void VM_cvar_string(prvm_prog_t *prog);
void VM_cvar_type (prvm_prog_t *prog);
void VM_cvar_defstring (prvm_prog_t *prog);
void VM_cvar_set (prvm_prog_t *prog);
void VM_dprint (prvm_prog_t *prog);
void VM_ftos (prvm_prog_t *prog);
void VM_fabs (prvm_prog_t *prog);
void VM_vtos (prvm_prog_t *prog);
void VM_etos (prvm_prog_t *prog);
void VM_stof(prvm_prog_t *prog);
void VM_itof(prvm_prog_t *prog);
void VM_ftoe(prvm_prog_t *prog);
void VM_strftime(prvm_prog_t *prog);
void VM_spawn (prvm_prog_t *prog);
void VM_remove (prvm_prog_t *prog);
void VM_find (prvm_prog_t *prog);
void VM_findfloat (prvm_prog_t *prog);
void VM_findchain (prvm_prog_t *prog);
void VM_findchainfloat (prvm_prog_t *prog);
void VM_findflags (prvm_prog_t *prog);
void VM_findchainflags (prvm_prog_t *prog);
void VM_precache_file (prvm_prog_t *prog);
void VM_precache_sound (prvm_prog_t *prog);
void VM_coredump (prvm_prog_t *prog);

void VM_stackdump (prvm_prog_t *prog);
void VM_crash(prvm_prog_t *prog); // REMOVE IT
void VM_traceon (prvm_prog_t *prog);
void VM_traceoff (prvm_prog_t *prog);
void VM_eprint (prvm_prog_t *prog);
void VM_rint (prvm_prog_t *prog);
void VM_floor (prvm_prog_t *prog);
void VM_ceil (prvm_prog_t *prog);
void VM_nextent (prvm_prog_t *prog);

void VM_changelevel (prvm_prog_t *prog);
void VM_sin (prvm_prog_t *prog);
void VM_cos (prvm_prog_t *prog);
void VM_sqrt (prvm_prog_t *prog);
void VM_randomvec (prvm_prog_t *prog);
void VM_registercvar (prvm_prog_t *prog);
void VM_min (prvm_prog_t *prog);
void VM_max (prvm_prog_t *prog);
void VM_bound (prvm_prog_t *prog);
void VM_pow (prvm_prog_t *prog);
void VM_log (prvm_prog_t *prog);
void VM_asin (prvm_prog_t *prog);
void VM_acos (prvm_prog_t *prog);
void VM_atan (prvm_prog_t *prog);
void VM_atan2 (prvm_prog_t *prog);
void VM_tan (prvm_prog_t *prog);

void VM_Files_Init(prvm_prog_t *prog);
void VM_Files_CloseAll(prvm_prog_t *prog);

void VM_fopen(prvm_prog_t *prog);
void VM_fclose(prvm_prog_t *prog);
void VM_fgets(prvm_prog_t *prog);
void VM_fputs(prvm_prog_t *prog);
void VM_writetofile(prvm_prog_t *prog); // only used by menu

void VM_strlen(prvm_prog_t *prog);
void VM_strcat(prvm_prog_t *prog);
void VM_substring(prvm_prog_t *prog);
void VM_stov(prvm_prog_t *prog);
void VM_strzone(prvm_prog_t *prog);
void VM_strunzone(prvm_prog_t *prog);

// KrimZon - DP_QC_ENTITYDATA
void VM_numentityfields(prvm_prog_t *prog);
void VM_entityfieldname(prvm_prog_t *prog);
void VM_entityfieldtype(prvm_prog_t *prog);
void VM_getentityfieldstring(prvm_prog_t *prog);
void VM_putentityfieldstring(prvm_prog_t *prog);

// DRESK - String Length (not counting color codes)
void VM_strlennocol(prvm_prog_t *prog);
// DRESK - Decolorized String
void VM_strdecolorize(prvm_prog_t *prog);
// DRESK - String Uppercase and Lowercase Support
void VM_strtolower(prvm_prog_t *prog);
void VM_strtoupper(prvm_prog_t *prog);

void VM_tokenize (prvm_prog_t *prog);
void VM_tokenizebyseparator (prvm_prog_t *prog);
void VM_argv (prvm_prog_t *prog);

void VM_isserver(prvm_prog_t *prog);
void VM_clientcount(prvm_prog_t *prog);
void VM_clientstate(prvm_prog_t *prog);
// not used at the moment -> not included in the common list
void VM_getostype(prvm_prog_t *prog);
void VM_getmousepos(prvm_prog_t *prog);
void VM_gettime(prvm_prog_t *prog);
void VM_getsoundtime(prvm_prog_t *prog);
void VM_soundlength(prvm_prog_t *prog);
void VM_loadfromdata(prvm_prog_t *prog);
void VM_parseentitydata(prvm_prog_t *prog);
void VM_loadfromfile(prvm_prog_t *prog);
void VM_modulo(prvm_prog_t *prog);

void VM_search_begin(prvm_prog_t *prog);
void VM_search_end(prvm_prog_t *prog);
void VM_search_getsize(prvm_prog_t *prog);
void VM_search_getfilename(prvm_prog_t *prog);
void VM_chr(prvm_prog_t *prog);
void VM_iscachedpic(prvm_prog_t *prog);
void VM_precache_pic(prvm_prog_t *prog);
void VM_freepic(prvm_prog_t *prog);
void VM_drawcharacter(prvm_prog_t *prog);
void VM_drawstring(prvm_prog_t *prog);
void VM_drawcolorcodedstring(prvm_prog_t *prog);
void VM_stringwidth(prvm_prog_t *prog);
void VM_drawpic(prvm_prog_t *prog);
void VM_drawrotpic(prvm_prog_t *prog);
void VM_drawsubpic(prvm_prog_t *prog);
void VM_drawfill(prvm_prog_t *prog);
void VM_drawsetcliparea(prvm_prog_t *prog);
void VM_drawresetcliparea(prvm_prog_t *prog);
void VM_getimagesize(prvm_prog_t *prog);

void VM_findfont(prvm_prog_t *prog);
void VM_loadfont(prvm_prog_t *prog);

void VM_makevectors (prvm_prog_t *prog);
void VM_vectorvectors (prvm_prog_t *prog);

void VM_keynumtostring (prvm_prog_t *prog);
void VM_getkeybind (prvm_prog_t *prog);
void VM_findkeysforcommand (prvm_prog_t *prog);
void VM_stringtokeynum (prvm_prog_t *prog);
void VM_setkeybind (prvm_prog_t *prog);
void VM_getbindmaps (prvm_prog_t *prog);
void VM_setbindmaps (prvm_prog_t *prog);

void VM_cin_open(prvm_prog_t *prog);
void VM_cin_close(prvm_prog_t *prog);
void VM_cin_setstate(prvm_prog_t *prog);
void VM_cin_getstate(prvm_prog_t *prog);
void VM_cin_restart(prvm_prog_t *prog);

void VM_gecko_create(prvm_prog_t *prog);
void VM_gecko_destroy(prvm_prog_t *prog);
void VM_gecko_navigate(prvm_prog_t *prog);
void VM_gecko_keyevent(prvm_prog_t *prog);
void VM_gecko_movemouse(prvm_prog_t *prog);
void VM_gecko_resize(prvm_prog_t *prog);
void VM_gecko_get_texture_extent(prvm_prog_t *prog);

void VM_drawline (prvm_prog_t *prog);

void VM_bitshift (prvm_prog_t *prog);

void VM_altstr_count(prvm_prog_t *prog);
void VM_altstr_prepare(prvm_prog_t *prog);
void VM_altstr_get(prvm_prog_t *prog);
void VM_altstr_set(prvm_prog_t *prog);
void VM_altstr_ins(prvm_prog_t *prog);

void VM_buf_create(prvm_prog_t *prog);
void VM_buf_del (prvm_prog_t *prog);
void VM_buf_getsize (prvm_prog_t *prog);
void VM_buf_copy (prvm_prog_t *prog);
void VM_buf_sort (prvm_prog_t *prog);
void VM_buf_implode (prvm_prog_t *prog);
void VM_bufstr_get (prvm_prog_t *prog);
void VM_bufstr_set (prvm_prog_t *prog);
void VM_bufstr_add (prvm_prog_t *prog);
void VM_bufstr_free (prvm_prog_t *prog);

void VM_buf_loadfile(prvm_prog_t *prog);
void VM_buf_writefile(prvm_prog_t *prog);
void VM_bufstr_find(prvm_prog_t *prog);
void VM_matchpattern(prvm_prog_t *prog);

void VM_changeyaw (prvm_prog_t *prog);
void VM_changepitch (prvm_prog_t *prog);

void VM_uncolorstring (prvm_prog_t *prog);

void VM_strstrofs (prvm_prog_t *prog);
void VM_str2chr (prvm_prog_t *prog);
void VM_chr2str (prvm_prog_t *prog);
void VM_strconv (prvm_prog_t *prog);
void VM_strpad (prvm_prog_t *prog);
void VM_infoadd (prvm_prog_t *prog);
void VM_infoget (prvm_prog_t *prog);
void VM_strncmp (prvm_prog_t *prog);
void VM_strncmp (prvm_prog_t *prog);
void VM_strncasecmp (prvm_prog_t *prog);
void VM_registercvar (prvm_prog_t *prog);
void VM_wasfreed (prvm_prog_t *prog);

void VM_strreplace (prvm_prog_t *prog);
void VM_strireplace (prvm_prog_t *prog);

void VM_crc16(prvm_prog_t *prog);
void VM_digest_hex(prvm_prog_t *prog);

void VM_SetTraceGlobals(prvm_prog_t *prog, const trace_t *trace);
void VM_ClearTraceGlobals(prvm_prog_t *prog);

void VM_uri_escape (prvm_prog_t *prog);
void VM_uri_unescape (prvm_prog_t *prog);
void VM_whichpack (prvm_prog_t *prog);

void VM_etof (prvm_prog_t *prog);
void VM_uri_get (prvm_prog_t *prog);
void VM_netaddress_resolve (prvm_prog_t *prog);

void VM_tokenize_console (prvm_prog_t *prog);
void VM_argv_start_index (prvm_prog_t *prog);
void VM_argv_end_index (prvm_prog_t *prog);

void VM_buf_cvarlist(prvm_prog_t *prog);
void VM_cvar_description(prvm_prog_t *prog);

void VM_CL_getextresponse (prvm_prog_t *prog);
void VM_SV_getextresponse (prvm_prog_t *prog);

// Common functions between menu.dat and clsprogs
void VM_CL_isdemo (prvm_prog_t *prog);
void VM_CL_videoplaying (prvm_prog_t *prog);

void VM_isfunction(prvm_prog_t *prog);
void VM_callfunction(prvm_prog_t *prog);

void VM_sprintf(prvm_prog_t *prog);

void VM_getsurfacenumpoints(prvm_prog_t *prog);
void VM_getsurfacepoint(prvm_prog_t *prog);
void VM_getsurfacepointattribute(prvm_prog_t *prog);
void VM_getsurfacenormal(prvm_prog_t *prog);
void VM_getsurfacetexture(prvm_prog_t *prog);
void VM_getsurfacenearpoint(prvm_prog_t *prog);
void VM_getsurfaceclippedpoint(prvm_prog_t *prog);
void VM_getsurfacenumtriangles(prvm_prog_t *prog);
void VM_getsurfacetriangle(prvm_prog_t *prog);

// physics builtins
void VM_physics_enable(prvm_prog_t *prog);
void VM_physics_addforce(prvm_prog_t *prog);
void VM_physics_addtorque(prvm_prog_t *prog);
void VM_nudgeoutofsolid(prvm_prog_t *prog);

void VM_coverage(prvm_prog_t *prog);

#endif
