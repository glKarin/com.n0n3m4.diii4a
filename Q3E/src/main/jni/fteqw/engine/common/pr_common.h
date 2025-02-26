#ifndef PR_COMMON_H
#define PR_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif

#include "progtype.h"
#include "progslib.h"

#define AREAGRIDPERENT 16

struct wedict_s
{
	enum ereftype_e	ereftype;
	float			freetime; // sv.time when the object was freed
	int				entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//world
#ifdef VM_Q1
	comentvars_t	*v;
	comextentvars_t	*xv;
#else
	union {
		comentvars_t	*v;
		comentvars_t	*xv;
	};
#endif
	/*the above is shared with qclib*/
#ifdef USEAREAGRID
	areagridlink_t	gridareas[AREAGRIDPERENT];	//on overflow, use the inefficient overflow list.
	size_t			gridareasequence;	//used to avoid iterrating the same ent twice.
#else
	link_t	area;
#endif
	pvscache_t pvsinfo;
	int lastruntime;
	int solidsize;

#ifdef USERBE
	entityrbe_t rbe;
#endif
	/*the above is shared with ssqc*/
};

#define G_PROG G_FLOAT

//the checkextension system asks for a name for the extension.
//the ebfs version is a function that returns a builtin number.
//thus this system requires various builtins to exist at specific numbers.
//this competes against checkbuiltin(funcreference).
typedef struct {
	world_t *world;
	unsigned int pext1, pext2;
} extcheck_t;
typedef struct qc_extension_s {
	char *name;
	qboolean(*extensioncheck)(extcheck_t *info);
	int numbuiltins;
	char *builtinnames[21];	//extend freely
	char *description;
} qc_extension_t;

extern qc_extension_t QSG_Extensions[];
extern unsigned int QSG_Extensions_count;

pbool QDECL QC_WriteFile(const char *name, void *data, int len);
void *VARGS PR_CB_Malloc(int size);	//these functions should be tracked by the library reliably, so there should be no need to track them ourselves.
void VARGS PR_CB_Free(void *mem);

int PR_Printf (const char *fmt, ...);
int PR_DPrintf (const char *fmt, ...);
void PF_InitTempStrings(pubprogfuncs_t *prinst);
string_t PR_TempString(pubprogfuncs_t *prinst, const char *str);	//returns a tempstring containing str
char *PF_TempStr(pubprogfuncs_t *prinst);	//returns a tempstring which can be filled in with whatever junk you want.

#ifdef QCVM_64
	#define VM_VECTORARG(name, ofs) vec3_t name = {G_FLOAT(ofs+0),G_FLOAT(ofs+1),G_FLOAT(ofs+2)}
#else
	#define VM_VECTORARG(name, ofs) pvec_t *name = G_VECTOR(ofs)
#endif

#define	RETURN_SSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_SetString(prinst, s))	//static - exe will not change it.
#define	RETURN_TSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_TempString(prinst, s))	//temp (static but cycle buffers)
extern cvar_t pr_tempstringsize;
extern cvar_t pr_tempstringcount;
extern cvar_t pr_enable_profiling;
extern cvar_t pr_fixbrokenqccarrays;
extern cvar_t pr_gc_threaded;

extern int qcinput_scan;
extern int qcinput_unicode;
int MP_TranslateFTEtoQCCodes(keynum_t code);
keynum_t MP_TranslateQCtoFTECodes(int code);
qboolean WPhys_Push (world_t *w, wedict_t *pusher, vec3_t move, vec3_t amove);

#ifdef ENGINE_ROUTING
//sv_move.c routing
void QCBUILTIN PF_route_calculate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PR_Route_Shutdown (world_t *world);
void PR_Route_Visualise (void);
void PR_Route_Init (void);
#endif

//known progs versions...
enum
{
	PROGHEADER_CRC_QW		= 54730,
	PROGHEADER_CRC_NQ		= 5927,
	PROGHEADER_CRC_PREREL	= 26940,	//prerelease
	PROGHEADER_CRC_TENEBRAE	= 32401,	//tenebrae
	PROGHEADER_CRC_H2		= 38488,	//basic hexen2
	PROGHEADER_CRC_H2MP		= 26905,	//hexen2 mission pack uses slightly different defs... *sigh*...
	PROGHEADER_CRC_H2DEMO	= 14046,	//I'm guessing this is from the original release or something
	PROGHEADER_CRC_CSQC		= 22390,
	PROGHEADER_CRC_CSQC_DP	= 52195,
	PROGHEADER_CRC_MENUQC	= 10020
};

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(pubprogfuncs_t *progfuncs, char *format, ...) LIKEPRINTF(2);
void QCBUILTIN PF_print (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_dprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_error (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_floor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ceil (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_anglemod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_anglesub (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vectorvectors (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_crossproduct (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Tokenize  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_tokenizebyseparator  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_tokenize_console  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ArgV  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_argv_start_index  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_argv_end_index  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindString (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindList (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_nextent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Sin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Cos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Sqrt (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bound (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_mod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strlen(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strcat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_createbuffer (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ftos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fabs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vtos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_etos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_mod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_substring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stov (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strzone(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strunzone(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Spawn (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_spawn_object (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_respawnedict (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_entityprotection (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_copyentity (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_droptofloor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_checkbottom (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_min (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_max (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_registercvar (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_pow (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Logarithm (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_asin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_acos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_atan (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_atan2 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_tan (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_localcmd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_sprintf_internal (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals, const char *s, int firstarg, char *outbuf, int outbuflen);
void QCBUILTIN PF_sprintf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_random (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fclose (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fputs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fgets (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fwrite (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fread (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fseek32 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fsize32 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fseek64 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fsize64 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_normalize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vhlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_changeyaw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_changepitch (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vectoyaw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vectoangles (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rotatevectorsbyangles (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rotatevectorsbymatrix (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findchain (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findchainfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_coredump (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_setwatchpoint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_traceon (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_traceoff (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_eprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_begin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_end (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getsize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getfilename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getfilesize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getfilemtime (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getpackagename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_fopen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_isfunction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_callfunction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_writetofile(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_loadfromfile (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_loadfromdata (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_parseentitydata(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_generateentitydata(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WasFreed (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_break (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_crc16 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

enum
{	//return values for cvar_type builtin.
	CVAR_TYPEFLAG_EXISTS		=1u<<0, //cvar actually exists.
	CVAR_TYPEFLAG_SAVED			=1u<<1, //cvar is flaged for archival (might need cfg_save to actually save)
	CVAR_TYPEFLAG_PRIVATE		=1u<<2, //QC is not allowed to read.
	CVAR_TYPEFLAG_ENGINE		=1u<<3, //cvar was created by the engine itself (not user/mod created)
	CVAR_TYPEFLAG_HASDESCRIPTION=1u<<4, //cvar_description will return something (hopefully) useful
	CVAR_TYPEFLAG_READONLY		=1u<<5, //cvar may not be changed by qc.
	//any extras added here should be shared with DP.
};
void QCBUILTIN PF_cvar_type (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_uri_escape  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_uri_unescape  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_uri_get  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_itos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stoi (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stoh (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_htos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ftoi (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_itof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ftou (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_utof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PR_fclose_progs (pubprogfuncs_t *prinst);
const char *PF_VarString (pubprogfuncs_t *prinst, int	first, struct globalvars_s *pr_globals);
void PR_ProgsAdded(pubprogfuncs_t *prinst, int newprogs, const char *modulename);
void PR_AutoCvar(pubprogfuncs_t *prinst, cvar_t *var);
void QCBUILTIN PF_numentityfields (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findentityfield (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_entityfieldref (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_entityfieldname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_entityfieldtype (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getentityfieldstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_putentityfieldstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_checkcommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_argescape(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);


void QCBUILTIN PF_getsurfacenumpoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacepoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacenormal(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacetexture(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacenearpoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfaceclippedpoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacenumtriangles(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacetriangle(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacepointattribute(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_checkpvs(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_setattachment(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

#ifndef SKELETALOBJECTS
	#define PF_gettaginfo			PF_Fixme
	#define PF_gettagindex			PF_Fixme
	#define PF_skintoname			PF_Fixme
	#define PF_frametoname			PF_Fixme
	#define PF_skel_set_bone_world	PF_Fixme
	#define PF_skel_mmap			PF_Fixme
	#define PF_skel_ragedit			PF_Fixme
	#define PF_frameduration		PF_Fixme
	#define PF_modelframecount		PF_Fixme
	#define PF_frameforname			PF_Fixme
	#define PF_frameforaction		PF_Fixme
	#define PF_skel_delete			PF_Fixme
	#define PF_skel_copybones		PF_Fixme
	#define PF_skel_premul_bones	PF_Fixme
	#define PF_skel_premul_bone		PF_Fixme
	#define PF_skel_postmul_bones	PF_Fixme
	#define PF_skel_postmul_bone	PF_Fixme
	#define PF_skel_set_bone		PF_Fixme
	#define PF_skel_get_boneabs		PF_Fixme
	#define PF_skel_get_bonerel		PF_Fixme
	#define PF_skel_find_bone		PF_Fixme
	#define PF_skel_get_boneparent	PF_Fixme
	#define PF_skel_get_bonename	PF_Fixme
	#define PF_skel_get_numbones	PF_Fixme
	#define PF_skel_build			PF_Fixme
	#define PF_skel_build_ptr		PF_Fixme
	#define PF_skel_create			PF_Fixme
	#define PF_skinforname			PF_Fixme

	#define PF_processmodelevents	PF_Fixme
	#define PF_getnextmodelevent	PF_Fixme
	#define PF_getmodeleventidx		PF_Fixme
#else
	void QCBUILTIN PF_skel_set_bone_world (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_mmap(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_ragedit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_create (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_build (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_build_ptr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_numbones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_bonename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_boneparent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_find_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_bonerel (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_boneabs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_set_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_premul_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_premul_bones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_postmul_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_postmul_bones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_copybones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_delete (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frametoname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skintoname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frameforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frameforaction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frameduration (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_modelframecount (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skinforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_gettaginfo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_gettagindex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

	void QCBUILTIN PF_processmodelevents (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_getnextmodelevent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_getmodeleventidx (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#endif

#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	void skel_lookup(world_t *prinst, int skelidx, framestate_t *fte_restrict out);
	void skel_dodelete(world_t *world);
	void skel_reset(world_t *world);
	void skel_reload(void);
	void skel_updateentbounds(world_t *w, wedict_t *ent);
#endif
void QCBUILTIN PF_physics_supported(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_physics_enable(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_physics_addforce(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_physics_addtorque(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_pushmove(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

#ifdef TERRAIN
void QCBUILTIN PF_terrain_edit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_get(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_create(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_delete(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_selected(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_getfacepoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_calcfacepoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_brush_findinvolume(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_patch_getcp(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_patch_getmesh(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_patch_create(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_patch_evaluate(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#endif

void QCBUILTIN PF_touchtriggers(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(pubprogfuncs_t *progfuncs, char *format, ...) LIKEPRINTF(2);
cvar_t *PF_Cvar_FindOrGet(const char *var_name);
void QCBUILTIN PF_cvar_string (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvars_haveunsaved (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_set (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_setf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ArgC (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strreplace (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strireplace (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_randomvector (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
int PR_QCFile_From_VFS (pubprogfuncs_t *prinst, const char *name, vfsfile_t *f, qboolean write);
int PR_QCFile_From_Buffer (pubprogfuncs_t *prinst, const char *name, void *buffer, size_t offset, size_t len);
void QCBUILTIN PF_fopen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_fcopy (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_frename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fremove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fexists (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rmtree (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_FindString (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindFloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindFlags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findchain (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findchainfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findchainflags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bitshift(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

struct qcstate_s *PR_CreateThread(pubprogfuncs_t *prinst, float retval, float resumetime, qboolean wait);
void PR_ClearThreads(pubprogfuncs_t *prinst);
void PR_RunThreads(world_t *world);
void QCBUILTIN PF_Abort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Fork(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Sleep(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externcall (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externrefcall (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externvalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externset (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_instr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_strlennocol (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strdecolorize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strtolower (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strtoupper (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strftime (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_strstrofs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_str2chr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_chr2str (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strconv (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_infoadd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_infoget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strncmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strncasecmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strpad (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strtrim (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_digest_hex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_digest_ptr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_findradius_list (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_findradius (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_edict_for_num (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_num_for_edict (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_defstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_description (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

//these functions are from pr_menu.c
#define DRAWFLAG_NORMAL		0
#define DRAWFLAG_ADD		1
#define DRAWFLAG_MODULATE	2
//#define DRAWFLAG_MODULATE2	3
#define DRAWFLAG_2D			(1u<<2)
#define DRAWFLAG_TWOSIDED	0x400
#define DRAWFLAG_LINES		0x800
void QCBUILTIN PF_SubConGetSet (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_SubConPrintf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_SubConDraw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_SubConInput (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_is_cached_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_precache_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_free_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_uploadimage (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_readimage (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawcharacter (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawrawstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawcolouredstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawline (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawfill (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawsetcliparea (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawresetcliparea (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawgetimagesize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_stringwidth (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawsubpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawrotpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawrotsubpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#ifdef HAVE_LEGACY
void QCBUILTIN PF_CL_drawrotpic_dp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#endif
void QCBUILTIN PF_CL_findfont (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_loadfont (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#if defined(CSQC_DAT) && !defined(SERVERONLY)
void QCBUILTIN PF_R_PolygonBegin(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_R_PolygonVertex(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_R_PolygonEnd(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#else
#define PF_R_PolygonBegin PF_Fixme
#define PF_R_PolygonVertex PF_Fixme
#define PF_R_PolygonEnd PF_Fixme
#endif

void QCBUILTIN PF_cl_getresolution (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcachevalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcachestring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_resethostcachemasks(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_sethostcachemaskstring(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_sethostcachemasknumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_resorthostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_sethostcachesort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_refreshhostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcachenumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcacheindexforkey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_addwantedhostcachekey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getextresponse(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_netaddress_resolve(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getmousepos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_GetBindMap (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_SetBindMap (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_keynumtostring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_findkeysforcommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_findkeysforcommandex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_stringtokeynum(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getkeybind (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setkeybind (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setmousetarget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getmousetarget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setmousepos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setcursormode (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getcursormode (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_clipboard_set(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setwindowcaption (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_playingdemo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_runningserver (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getgamedirinfo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getpackagemanagerinfo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_addprogs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_create (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_destroy (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_command (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_keyevent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_mousemove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_resize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_get_texture_extent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_getposition (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_getproperty (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_setstate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_getstate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_media_restart (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

typedef enum{
	SLIST_HOSTCACHEVIEWCOUNT,
	SLIST_HOSTCACHETOTALCOUNT,
	SLIST_MASTERQUERYCOUNT,
	SLIST_MASTERREPLYCOUNT,
	SLIST_SERVERQUERYCOUNT,
	SLIST_SERVERREPLYCOUNT,
	SLIST_SORTFIELD,
	SLIST_SORTDESCENDING
} hostcacheglobal_t;
void QCBUILTIN PF_shaderforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_remapshader (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_cl_sprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_bprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_clientcount (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_localsound(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_queueaudio(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getqueuedaudiotime(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_SendPacket(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getlocaluserinfoblob (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getlocaluserinfostring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setlocaluserinfo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
const char *PF_cl_serverkey_internal(const char *keyname);
void QCBUILTIN PF_cl_serverkey (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_serverkeyfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_serverkeyblob (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_cl_gp_querywithcb(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gp_getbuttontype(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gp_rumble(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gp_rumbletriggers(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gp_setledcolor(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gp_settriggerfx(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void search_close_progs(pubprogfuncs_t *prinst, qboolean complain);

void QCBUILTIN PF_buf_create  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_del  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_getsize  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_copy  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_sort  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_implode  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_get  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_set  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_add  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_free  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_find  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_cvarlist  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_loadfile  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_writefile  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_hash_createtab	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_destroytab	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_add	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_get	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_getcb	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_delete	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_getkey	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_json_parse				(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_value_type		(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_integer			(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_float			(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_string			(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_find_object_child	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_length			(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_child_at_index	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_json_get_name				(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#ifdef FTE_TARGET_WEB
void QCBUILTIN PF_js_run_script				(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#else
#define PF_js_run_script PF_Ignore
#endif
void QCBUILTIN PF_base64encode(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_base64decode(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_memalloc	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memrealloc(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memfree	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memcmp	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memcpy	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memfill8	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memgetval	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memsetval	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memptradd	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memstrsize(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_soundlength (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_calltimeofday (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_gettimef (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_gettimed (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_whichpack (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

int QDECL QCEditor (pubprogfuncs_t *prinst, const char *filename, int *line, int *statement, int firststatement, char *reason, pbool fatal);
void PR_Common_Shutdown(pubprogfuncs_t *progs, qboolean errored);
void PR_Common_SaveGame(vfsfile_t *f, pubprogfuncs_t *prinst, qboolean binary);
qboolean PR_Common_LoadGame(pubprogfuncs_t *prinst, char *command, const char **file);

const void *PR_GetReadQCPtr(pubprogfuncs_t *prinst, int qcptr, int qcsize);
void *PR_GetWriteQCPtr(pubprogfuncs_t *prinst, int qcptr, int qcsize);

uploadfmt_t PR_TranslateTextureFormat(int qcformat);
int PR_UnTranslateTextureFormat(uploadfmt_t fteformat);

//FIXME
pbool PR_RunWarning (pubprogfuncs_t *ppf, char *error, ...);


/*these are server ones, provided by pr_cmds.c, as required by pr_q1qvm.c*/
int PF_ForceInfoKey_Internal(unsigned int entnum, const char *key, const char *value, size_t valsize);
#ifdef VM_Q1
void PR_SV_FillWorldGlobals(world_t *w);
model_t *QDECL SVPR_GetCModel(world_t *w, int modelindex);
void QCBUILTIN PF_WriteByte (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteChar (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteShort (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteLong (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteAngle (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteCoord (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteFloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteEntity (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_multicast (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_svtraceline (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_changelevel (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_applylightstyle(int style, const char *val, vec3_t rgb);
void PF_ambientsound_Internal (float *pos, const char *samp, float vol, float attenuation);
void QCBUILTIN PF_makestatic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_logfrag (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ExecuteCommand  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_setspawnparms (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_precache_vwep_model(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
int PF_checkclient_Internal (pubprogfuncs_t *prinst);
int PF_precache_sound_Internal (pubprogfuncs_t *prinst, const char *s, qboolean queryonly);
int PF_precache_model_Internal (pubprogfuncs_t *prinst, const char *s, qboolean queryonly);
void PF_setmodel_Internal (pubprogfuncs_t *prinst, edict_t *e, const char *m);
char *PF_infokey_Internal (int entnum, const char *value);
void PF_stuffcmd_Internal(int entnum, const char *str, unsigned int flags);
void PF_centerprint_Internal (int entnum, qboolean plaque, const char *s);
void PF_WriteString_Internal (int target, const char *str);
pbool QDECL ED_CanFree (edict_t *ed);
#endif

#ifdef HAVE_LEGACY
unsigned int FTEToDPContents(unsigned int contents);
#endif

#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10
#define MOVETYPE_BOUNCEMISSILE	11		// bounce w/o gravity
#define MOVETYPE_FOLLOW			12		// track movement of aiment
#define MOVETYPE_H2PUSHPULL		13		// pushable/pullable object
#define MOVETYPE_H2SWIM			14		// should keep the object in water
#define MOVETYPE_6DOF			30		// flightsim mode
#define MOVETYPE_WALLWALK		31		// walks up walls and along ceilings
#define MOVETYPE_PHYSICS		32
#define MOVETYPE_FLY_WORLDONLY	33		//fly that collides only with world, keeping spectators within the world but free to pass through doors. avoids pvs issues with q3map2.

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block
#define	SOLID_PHASEH2			5		// hexen2 flag - this is apparently a modifier for movetype rather than solidity - passes through FL_MONSTER or MOVETYPE_WALK ents
#define	SOLID_CORPSE			5		// non-solid to solid_slidebox entities and itself.
#define SOLID_LADDER			20		//spike: legacy. forces FTECONTENTS_LADDER.
#define SOLID_PORTAL			21		//1: traces always use point-size. 2: various movetypes automatically transform entities. 3: traces that impact portal bbox use a union. 4. traces ignore part of the world within the portal's box
#define SOLID_BSPTRIGGER		22		//spike: like solid trigger, except uses bsp checks instead of just aabb.
#define	SOLID_PHYSICS_BOX		32		// deprecated. physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_SPHERE	33		// deprecated. physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_CAPSULE	34		// deprecated. physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define SOLID_PHYSICS_TRIMESH	35
#define SOLID_PHYSICS_CYLINDER  36

#define	GEOMTYPE_NONE      -1
#define	GEOMTYPE_SOLID      0
#define	GEOMTYPE_BOX		1
#define	GEOMTYPE_SPHERE		2
#define	GEOMTYPE_CAPSULE	3
#define	GEOMTYPE_TRIMESH	4
#define	GEOMTYPE_CYLINDER	5
#define	GEOMTYPE_CAPSULE_X	6
#define	GEOMTYPE_CAPSULE_Y	7
#define	GEOMTYPE_CAPSULE_Z	8
#define	GEOMTYPE_CYLINDER_X	9
#define	GEOMTYPE_CYLINDER_Y	10
#define	GEOMTYPE_CYLINDER_Z	11


#define JOINTTYPE_POINT 1
#define JOINTTYPE_HINGE 2
#define JOINTTYPE_SLIDER 3
#define JOINTTYPE_UNIVERSAL 4
#define JOINTTYPE_HINGE2 5
#define JOINTTYPE_FIXED -1

typedef struct
{
	int version;
	int wedictsize;	//sizeof(wedict_t)

	qboolean (QDECL *RegisterPhysicsEngine)(const char *enginename, void(QDECL*start_physics)(world_t*world));	//returns false if there's already one active.
	void (QDECL *UnregisterPhysicsEngine)(const char *enginename);	//returns false if there's already one active.
	qboolean (QDECL *GenerateCollisionMesh)(world_t *world, model_t *mod, wedict_t *ed, vec3_t geomcenter);
	void (QDECL *ReleaseCollisionMesh) (wedict_t *ed);
	void (QDECL *LinkEdict)(world_t *w, wedict_t *ed, qboolean touchtriggers);

	void (QDECL *VectorAngles)(const float *forward, const float *up, float *result, qboolean meshpitch);
	void (QDECL *AngleVectors)(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
#define plugrbefuncs_name "RBE"
} rbeplugfuncs_t;
#define RBEPLUGFUNCS_VERSION 1

qofs_t PR_ReadBytesString(char *str);


#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

#define CLIENTTYPE_DISCONNECTED	0
#define CLIENTTYPE_REAL			1
#define CLIENTTYPE_BOT			2
#define CLIENTTYPE_NOTACLIENT	3

enum
{
	RESSTATE_NOTKNOWN = 0,
	RESSTATE_NOTLOADED = 1,
	RESSTATE_LOADING = 2,
	RESSTATE_FAILED = 3,
	RESSTATE_LOADED = 4,

	RESSTATE_UNSUPPORTED = -1
};
enum
{
	RESTYPE_MODEL,
	RESTYPE_SOUND,
	RESTYPE_PARTICLE,
	RESTYPE_SHADER,
	RESTYPE_SKIN,
	RESTYPE_TEXTURE
};

//shared constants
typedef enum
{
	VF_MIN = 1,
	VF_MIN_X = 2,
	VF_MIN_Y = 3,
	VF_SIZE = 4,
	VF_SIZE_X = 5,
	VF_SIZE_Y = 6,
	VF_VIEWPORT = 7,
	VF_FOV = 8,
	VF_FOV_X = 9,
	VF_FOV_Y = 10,
	VF_ORIGIN = 11,
	VF_ORIGIN_X = 12,
	VF_ORIGIN_Y = 13,
	VF_ORIGIN_Z = 14,
	VF_ANGLES = 15,
	VF_ANGLES_X = 16,
	VF_ANGLES_Y = 17,
	VF_ANGLES_Z = 18,
	VF_DRAWWORLD = 19,
	VF_ENGINESBAR = 20,
	VF_DRAWCROSSHAIR = 21,
	VF_CARTESIAN_ANGLES = 22,
	VF_MINDIST = 23,
	VF_MAXDIST = 24,

	//this is a DP-compatibility hack.
	VF_CL_VIEWANGLES_V = 33,
	VF_CL_VIEWANGLES_X = 34,
	VF_CL_VIEWANGLES_Y = 35,
	VF_CL_VIEWANGLES_Z = 36,


	//33-36 used by DP...
	VF_PERSPECTIVE = 200,
	//201 used by DP... WTF? CLEARSCREEN
	VF_ACTIVESEAT = 202,
	VF_AFOV = 203,	//aproximate fov (match what the engine would normally use for the fov cvar). p0=fov, p1=zoom
	VF_SCREENVSIZE = 204,
	VF_SCREENPSIZE = 205,
	VF_VIEWENTITY = 206,
	VF_STATSENTITY = 207,	//the player number for the stats.
	VF_SCREENVOFFSET = 208,

	VF_RT_SOURCECOLOUR	= 209,
	VF_RT_DEPTH			= 210,
	VF_RT_RIPPLE		= 211,	/**/
	VF_RT_DESTCOLOUR0	= 212,
	VF_RT_DESTCOLOUR1	= 213,
	VF_RT_DESTCOLOUR2	= 214,
	VF_RT_DESTCOLOUR3	= 215,
	VF_RT_DESTCOLOUR4	= 216,
	VF_RT_DESTCOLOUR5	= 217,
	VF_RT_DESTCOLOUR6	= 218,
	VF_RT_DESTCOLOUR7	= 219,
	VF_ENVMAP			= 220,	//cubemap image for reflectcube
	VF_USERDATA			= 221,
	VF_SKYROOM_CAMERA	= 222,
	VF_PIXELPSCALE		= 223,	//[dpi_x, dpi_y, dpi_y/dpi_x]
	VF_PROJECTIONOFFSET	= 224,	//allows for off-axis projections.
	VF_VRBASEORIENTATION= 225,	//specifies the worldspace coords+angles of the VR room space.
	//WARNING: update fteqcc when new entries are added.


	VF_DP_CLEARSCREEN		= 201, //misnomer - NOTOVERLAY would be a better name. when set to false prevents any and all post-proc things that might write colour values in areas with no geometry there.
	VF_DP_FOG_DENSITY		= 202, //misassigned - fuck DP and their complete lack of respect for existing implemenetations
	VF_DP_FOG_COLOR			= 203, //misassigned
	VF_DP_FOG_COLOR_R		= 204, //misassigned
	VF_DP_FOG_COLOR_G		= 205, //misassigned
	VF_DP_FOG_COLOR_B		= 206, //misassigned
	VF_DP_FOG_ALPHA			= 207, //misassigned
	VF_DP_FOG_START			= 208, //misassigned
	VF_DP_FOG_END   		= 209, //misassigned
	VF_DP_FOG_HEIGHT		= 210, //misassigned
	VF_DP_FOG_FADEDEPTH		= 211, //misassigned
	VF_DP_MAINVIEW			= 400, // defective. should have been a 1-based viewid instead, allowing for per-view motionblur instead of disabling it outright
	VF_DP_MINFPS_QUALITY	= 401,	//multiplier for lod and culling to try to reduce costs.
} viewflags;

/*FIXME: this should be changed*/
#define CSQC_API_VERSION 1.0f

#define CSQCRF_VIEWMODEL				1 //Not drawn in mirrors
#define CSQCRF_EXTERNALMODEL			2 //drawn ONLY in mirrors
#define CSQCRF_DEPTHHACK				4 //fun depthhack
#define CSQCRF_ADDITIVE					8 //add instead of blend
#define CSQCRF_USEAXIS					16 //use v_forward/v_right/v_up as an axis/matrix - predraw is needed to use this properly
#define CSQCRF_NOSHADOW					32 //don't cast shadows upon other entities (can still be self shadowing, if the engine wishes, and not additive)
#define CSQCRF_FRAMETIMESARESTARTTIMES	64 //EXT_CSQC_1: frame times should be read as (time-frametime).
#define CSQCRF_FIRSTPERSON				1024 //also not drawn in mirrors, just without the VIEWMODEL hacks attached
//#define CSQCRFDP_USETRANSPARENTOFFSET	64 // Allows QC to customize origin used for transparent sorting via transparent_origin global, helps to fix transparent sorting bugs on a very large entities
////#define CSQCRF_NOAUTOADD			128 // removed in favour of predraw return values.
//#define CSQCRFDP_WORLDOBJECT			128 // for large outdoor entities that should not be culled.
//#define CSQCRFDP_FULLBRIGHT			256
//#define CSQCRFDP_NOSHADOW				512
//#define CSQCRF_UNUSED					2048
//#define CSQCRFDP_MODELLIGHT			4096 // CSQC-set model light
//#define CSQCRFDP_DYNAMICMODELLIGHT	8192 // origin-dependent model light

/*only read+append+write are standard frik_file*/
#define FRIK_FILE_READ		0 /*read-only*/
#define FRIK_FILE_APPEND	1 /*append (write-only, but offset begins at end of previous file)*/
#define FRIK_FILE_WRITE		2 /*write-only*/
#define FRIK_FILE_INVALID	3 /*no idea what this is for, presume placeholder*/
#define FRIK_FILE_READNL	4 /*fgets ignores newline chars, returning the entire thing in one lump*/
#define FRIK_FILE_MMAP_READ	5 /*fgets returns a pointer. memory is not guarenteed to be released.*/
#define FRIK_FILE_MMAP_RW	6 /*fgets returns a pointer. file is written upon close. memory is not guarenteed to be released.*/

#define FRIK_FILE_READ_DELAY (7) /*internal special mode where the file is not read until the first read. this avoids extra slowness with xonotic (where it uses fopen to see if (large) binary file exists, resulting in large binary files getting decompressed repeatedly then discarded without reading)*/
#define FRIK_FILE_STREAM	(8) /*access goes via the vfs, we don't need to track the read/write info here*/

#define MASK_DELTA 1
#define MASK_STDVIEWMODEL 2

enum lightfield_e
{
	lfield_origin=0,
	lfield_colour=1,
	lfield_radius=2,
	lfield_flags=3,
	lfield_style=4,
	lfield_angles=5,
	lfield_fov=6,
	lfield_corona=7,
	lfield_coronascale=8,
	lfield_cubemapname=9,
	lfield_ambientscale=10,
	lfield_diffusescale=11,
	lfield_specularscale=12,
	lfield_rotation=13,
	lfield_dietime=14,
	lfield_rgbdecay=15,
	lfield_radiusdecay=16,
	lfield_stylestring=17,
	lfield_nearclip=18,
	lfield_owner=19,
};
enum csqc_input_event
{
	/*devid is the player id (on android, its the multitouch id and is always present even in single player)*/
	CSIE_KEYDOWN = 0,		/*syscode, unicode, devid	the two codes are not both guarenteed to be set at the same time, and may happen as separate events*/
	CSIE_KEYUP = 1,			/*syscode, unicode, devid	as keydown, unicode up events are not guarenteed*/
	CSIE_MOUSEDELTA = 2,	/*x, y, devid				mouse motion. x+y are relative*/
	CSIE_MOUSEABS = 3,		/*x, y, devid				*/
	CSIE_ACCELEROMETER = 4,	/*x, y, z*/
	CSIE_FOCUS = 5,			/*mouse, key, devid.		if has, the game window has focus. (true/false/-1)*/
	CSIE_JOYAXIS = 6,		/*axis, value, devid*/
	CSIE_GYROSCOPE = 7,		/*x, y, z					rotational acceleration*/
	CSIE_PASTE = 8,			/*syscode, unicode, devid	like keydown, but no scancodes*/
};

enum getgamedirinfo_e
{
	GGDI_GAMEDIR=0,			//the publically visible gamedir reported by servers.
	GGDI_DESCRIPTION=1,		//some text from the .fmf or a gamedirin
	GGDI_OVERRIDES=2,		//some text you can parse for custom info.
	GGDI_LOADCOMMAND=3,		//returns a string which can be localcmded to load the mod, with whatever quirks are needed to activate it properly.
	GGDI_ICON=4,			//returns a string which can be drawpiced.
	GGDI_ALLGAMEDIRS=5,		//; delimited list basegames;gamedirs ordering
};
enum packagemanagerinfo_e
{
	GPMI_NAME,			//name of the package, for use with the pkg command.
	GPMI_CATEGORY,		//category text
	GPMI_TITLE,			//name of the package, for showing the user.
	GPMI_VERSION,		//version info (may have multiple with the same name but different versions)
	GPMI_DESCRIPTION,	//some blurb
	GPMI_LICENSE,		//what license its distributed under
	GPMI_AUTHOR,		//name of the person(s) who created it
	GPMI_WEBSITE,		//where to contribute/find out more info/etc
	GPMI_INSTALLED,		//current state
	GPMI_ACTION,		//desired state
	GPMI_AVAILABLE,		//whether it may be downloaded or not.
	GPMI_FILESIZE,		//size of the download.
	GPMI_GAMEDIR,		//so you know which mod(s) its relevant for
	GPMI_MAPS,			//list of maps provided by this package (can be used with the `map $PackageName:$MapName` command).
	GPMI_PREVIEWIMG,	//a uri usable with drawpic etc
};

#ifdef TERRAIN
enum terrainedit_e
{
	ter_reload,			//
	ter_save,			//
	ter_sethole,		//vector pos, float radius, floatbool hole
	ter_height_set,		//vector pos, float radius, float newheight
	ter_height_smooth,	//vector pos, float radius, float percent
	ter_height_spread,	//vector pos, float radius, float percent
	ter_raise,			//vector pos, float radius, float heightchange
	ter_lower,			//vector pos, float radius, float heightchange
	ter_tex_kill,		//vector pos, void junk, void junk, string texname
	ter_tex_get,		//vector pos, void junk, float imagenum
	ter_tex_blend,		//vector pos, float radius, float percent, string texname
	ter_tex_concentrate,	//vector pos, float radius, float percent
	ter_tex_noise,		//vector pos, float radius, float percent
	ter_tex_blur,		//vector pos, float radius, float percent
	ter_water_set,		//vector pos, float radius, float newwaterheight
	ter_mesh_add,		//entity ent
	ter_mesh_kill,		//vector pos, float radius
	ter_tint,			//vector pos, float radius, float percent, vector newcol, float newalph
	ter_height_flatten,	//vector pos, float radius, float percent
	ter_tex_replace,	//vector pos, float radius, string texname
	ter_reset,			//vector pos, float radius
	ter_reloadsect,		//vector pos, float radius

	ter_ents_wipe_deprecated,		//none
	ter_ents_concat_deprecated,	//string
	ter_ents_get,		//none, returns the map's entity string.

//	ter_poly_add,		//add a poly, woo
//	ter_poly_remove,	//remove polys

//	ter_autopaint_h,	//vector pos, float radius, float percent, string tex1, string tex2				(paint tex1/tex2
//	ter_autopaint_n	//vector pos, float radius, float percent, string tex1, string tex2

	ter_tex_mask,		//string tex

	ter_ent_get,		//int idx -> string
	ter_ent_set,		//int idx, string keyvals
	ter_ent_add,		//string keyvals -> int
	ter_ent_count,		// -> int

//	ter_cmd_count
};
#endif

enum
{
	GE_MAXENTS			= -1,
	GE_ACTIVE			= 0,
	GE_ORIGIN			= 1,
	GE_FORWARD			= 2,
	GE_RIGHT			= 3,
	GE_UP				= 4,
	GE_SCALE			= 5,
	GE_ORIGINANDVECTORS	= 6,
	GE_ALPHA			= 7,
	GE_COLORMOD			= 8,
	GE_PANTSCOLOR		= 9,
	GE_SHIRTCOLOR		= 10,
	GE_SKIN				= 11,
	GE_MINS				= 12,
	GE_MAXS				= 13,
	GE_ABSMIN			= 14,
	GE_ABSMAX			= 15,
	GE_LIGHT			= 16,

	GE_MODELINDEX		= 200,
	GE_MODELINDEX2		= 201,
	GE_EFFECTS			= 202,
	GE_FRAME			= 203,
	GE_ANGLES			= 204,
	GE_FATNESS			= 205,
	GE_DRAWFLAGS		= 206,
	GE_ABSLIGHT			= 207,
	GE_GLOWMOD			= 208,
	GE_GLOWSIZE			= 209,
	GE_GLOWCOLOUR		= 210,
	GE_RTSTYLE			= 211,
	GE_RTPFLAGS			= 212,
	GE_RTCOLOUR			= 213,
	GE_RTRADIUS			= 214,
	GE_TAGENTITY		= 215,
	GE_TAGINDEX			= 216,
	GE_GRAVITYDIR		= 217,
	GE_TRAILEFFECTNUM	= 218,

//	GE_MOVETYPE,
//	GE_LATENCY,
//	GE_VIEWANGLES
//	GE_MOVEMENT,
//	GE_VELOCITY,
};

//If I do it like this, I'll never forget to register something...
#define ENDLIST
#ifndef HAVE_LEGACY
#define legacycsqcglobals
#else
#define legacycsqcglobals	\
	globalstring(trace_dphittexturename)			/*for dp compat*/	\
	globalfloatdep (trace_dpstartcontents,		"Does not support mod-specific contents.")			/*for dp compat*/	\
	globalfloatdep (trace_dphitcontents,		"Does not support mod-specific contents.")				/*for dp compat*/	\
	globalfloatdep (trace_dphitq3surfaceflags,	"Does not support mod-specific surface flags")	/*for dp compat*/	\
	globalfloatdep (trace_surfaceflagsf,		"Does not support all mod-specific surface flags.")				/*float		written by traceline, for mods that lack ints*/	\
	globalfloatdep (trace_endcontentsf,			"Does not support all mod-specific contents.")				/*float		written by traceline EXT_CSQC_1, for mods that lack ints*/	\
	ENDLIST
#endif
#define csqcglobals	\
	globalfunction(CSQC_Init,				"void(float apilevel, string enginename, float engineversion)")	\
	globalfunction(CSQC_WorldLoaded,		"void()")	\
	globalfunction(CSQC_Shutdown,			"void()")	\
	globalfunction(CSQC_UpdateView,			"void(float vwidth, float vheight, float notmenu)")	\
	globalfunction(CSQC_UpdateViewLoading,	"void(float vwidth, float vheight, float notmenu)")	\
	globalfunction(CSQC_DrawHud,			"void(vector viewsize, float scoresshown)")/*simple csqc*/	\
	globalfunction(CSQC_DrawScores,			"void(vector viewsize, float scoresshown)")/*simple csqc*/	\
	globalfunction(CSQC_Parse_StuffCmd,		"void(string msg)")	\
	globalfunction(CSQC_Parse_CenterPrint,	"float(string msg)")	\
	globalfunction(CSQC_Parse_Print,		"void(string printmsg, float printlvl)")	\
	globalfunction(CSQC_Parse_Event,		"void()")	\
	globalfunction(CSQC_Parse_Damage,		"float(float save, float take, vector inflictororg)")	\
	globalfunction(CSQC_Parse_SetAngles,	"float(vector angles, float isdelta)")	\
	globalfunction(CSQC_PlayerInfoChanged,	"void(float playernum)")	\
	globalfunction(CSQC_ServerInfoChanged,	"void()")	\
	globalfunction(CSQC_InputEvent,			"float(float evtype, float scanx, float chary, float devid)")	\
	globalfunction(CSQC_Input_Frame,		"void()")/*EXT_CSQC_1*/	\
	globalfunction(CSQC_RendererRestarted,	"void(string rendererdescription)")	\
	globalfunction(CSQC_GenerateMaterial,	"string(string shadername)")	\
	globalfunction(CSQC_ConsoleCommand,		"float(string cmd)")	\
	globalfunction(CSQC_ConsoleLink,		"float(string text, string info)")	\
	globalfunction(GameCommand,				"void(string cmdtext)")	/*DP extension*/\
	\
	globalfunction(CSQC_Ent_Spawn,			"void(float newentnum)")	\
	globalfunction(CSQC_Ent_Update,			"void(float isnew)")	\
	globalfunction(CSQC_Ent_Remove,			"void()")	\
	\
	globalfunction(CSQC_Event_Sound,		"float(float entnum, float channel, string soundname, float vol, float attenuation, vector pos, float pitchmod, float flags"/*", float timeofs*/")")	\
	globalfunction(CSQC_ServerSound,		"DEP(\"use CSQC_Event_Sound\") float(float channel, string soundname, vector pos, float vol, float attenuation, float flags"/*", float timeofs*/")")/*obsolete, use event_sound*/	\
	/*globalfunction(CSQC_LoadResource,		"float(string resname, string restype)")*//*EXT_CSQC_1*/	\
	globalfunction(CSQC_Parse_TempEntity,	"float()")/*EXT_CSQC_ABSOLUTLY_VILE*/	\
	\
	globalfunction(CSQC_MapEntityEdited,	"void(int entidx, string newentdata)")\
	globalfunction(StartFrame,				"void()")\
	globalfunction(EndFrame,				"void()")\
	\
	/*These are pointers to the csqc's globals.*/	\
	globalfloat	(time)				/*float		The simulation(aka: smoothed server) time, speed drifts based upon latency*/	\
	globalfloat	(frametime)			/*float		Client render frame interval*/	\
	globalfloat	(gamespeed)			/*float		Multiplier for real time -> simulation time*/	\
	globalfloat	(cltime)				/*float		Clientside map uptime indepent of gamespeed, latency, and the server in general*/	\
	globalfloat	(clframetime)			/*float		time since last video frame*/	\
	globalfloat	(servertime)			/*float		Server time of latest inbound network frame*/	\
	globalfloat	(serverprevtime)		/*float		Server time of previous inbound network frame */	\
	globalfloat	(serverdeltatime)		/*float		new-old */	\
	globalfloat	(physics_mode)		/*float		Written before entering most qc functions*/	\
	globalentity(self)				/*entity	Written before entering most qc functions*/	\
	globalentity(other)				/*entity	Written before entering most qc functions*/	\
	\
	globalfloat	(deathmatch)			/*for simplecsqc*/	\
	globalfloat	(coop)				/*for simplecsqc*/	\
	\
	globalfloat	(maxclients)				/*float		max number of players allowed*/	\
	globalfloat	(numclientseats)			/*float		number of seats/splitscreen clients running on this client*/	\
	\
	globalvector(v_forward)					/*vector	written by anglevectors*/	\
	globalvector(v_right)					/*vector	written by anglevectors*/	\
	globalvector(v_up)						/*vector	written by anglevectors*/	\
	\
	globalfloat	(trace_allsolid)			/*bool		written by traceline*/	\
	globalfloat	(trace_startsolid)			/*bool		written by traceline*/	\
	globalfloat	(trace_fraction)			/*float		written by traceline*/	\
	globalfloat	(trace_inwater)				/*bool		written by traceline*/	\
	globalfloat	(trace_inopen)				/*bool		written by traceline*/	\
	globalvector(trace_endpos)				/*vector	written by traceline*/	\
	globalvector(trace_plane_normal)		/*vector	written by traceline*/	\
	globalfloat	(trace_plane_dist)			/*float		written by traceline*/	\
	globalentity(trace_ent)					/*entity	written by traceline*/	\
	globalint	(trace_surfaceflagsi)		/*int		written by traceline*/	\
	globalstring(trace_surfacename)			/*string	written by traceline*/	\
	globalint	(trace_endcontentsi)		/*int		written by traceline EXT_CSQC_1*/	\
	globalint	(trace_brush_id)			/*int		written by traceline*/	\
	globalint	(trace_brush_faceid)		/*int		written by traceline*/	\
	globalint	(trace_surface_id)			/*int		written by traceline*/	\
	globalint	(trace_bone_id)				/*int		written by traceline*/	\
	globalint	(trace_triangle_id)			/*int		written by traceline*/	\
	globalfloat (trace_networkentity)		/*float		written by traceline*/	\
	legacycsqcglobals \
	\
	globalfloat(clientcommandframe)			/*float		the next frame that will be sent*/ \
	globalfloat(servercommandframe)			/*float		the most recent frame received from the server*/ \
	\
	globalfloat(player_localentnum)			/*float		the entity number the local player is looking out from*/	\
	globalfloat(player_localnum)			/*float		the player number of the local player*/	\
	globalfloat(intermission)				/*float		set when the client receives svc_intermission*/	\
	globalfloat(intermission_time)			/*float		set when the client receives svc_intermission*/	\
	globalvector(view_angles)				/*float		set to the view angles at the start of each new frame (EXT_CSQC_1)*/ \
	\
	globalvector(pmove_org)					/*deprecated. read/written by runplayerphysics*/ \
	globalvector(pmove_vel)					/*deprecated. read/written by runplayerphysics*/ \
	globalvector(pmove_mins)				/*deprecated. read/written by runplayerphysics*/ \
	globalvector(pmove_maxs)				/*deprecated. read/written by runplayerphysics*/ \
	globalfloat (pmove_jump_held)			/*deprecated. read/written by runplayerphysics*/ \
	globalfloat (pmove_waterjumptime)		/*deprecated. read/written by runplayerphysics*/ \
	globalfloat (pmove_onground)			/*deprecated. read/written by runplayerphysics*/ \
	\
	globalfloat (input_sequence)			/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat (input_timelength)			/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalvector(input_angles)				/*vector	filled by getinputstate, read by runplayerphysics*/ \
	globalvector(input_movevalues)			/*vector	filled by getinputstate, read by runplayerphysics*/ \
	globalfloat (input_buttons)				/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat (input_impulse)				/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat (input_lightlevel)			/*unused float		filled by getinputstate, read by runplayerphysics*/ \
	globaluint64(input_weapon)				/*unused float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat (input_servertime)			/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat (input_clienttime)			/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalvector(input_cursor_screen)		/*float		filled by getinputstate*/ \
	globalvector(input_cursor_trace_start)	/*float		filled by getinputstate*/ \
	globalvector(input_cursor_trace_endpos)	/*float		filled by getinputstate*/ \
	globalfloat (input_cursor_entitynumber)	/*float		filled by getinputstate*/ \
	globaluint64(input_head_status)			/*filled by getinputstate, for vr*/ \
	globalvector(input_head_origin)			/*filled by getinputstate, for vr*/ \
	globalvector(input_head_angles)			/*filled by getinputstate, for vr*/ \
	globalvector(input_head_velocity)		/*filled by getinputstate, for vr*/ \
	globalvector(input_head_avelocity)		/*filled by getinputstate, for vr*/ \
	globaluint64(input_head_weapon)			/*filled by getinputstate, for vr*/ \
	globaluint64(input_left_status)			/*filled by getinputstate, for vr*/ \
	globalvector(input_left_origin)			/*filled by getinputstate, for vr*/ \
	globalvector(input_left_angles)			/*filled by getinputstate, for vr*/ \
	globalvector(input_left_velocity)		/*filled by getinputstate, for vr*/ \
	globalvector(input_left_avelocity)		/*filled by getinputstate, for vr*/ \
	globaluint64(input_left_weapon)			/*filled by getinputstate, for vr*/ \
	globaluint64(input_right_status)		/*filled by getinputstate, for vr*/ \
	globalvector(input_right_origin)		/*filled by getinputstate, for vr*/ \
	globalvector(input_right_angles)		/*filled by getinputstate, for vr*/ \
	globalvector(input_right_velocity)		/*filled by getinputstate, for vr*/ \
	globalvector(input_right_avelocity)		/*filled by getinputstate, for vr*/ \
	globaluint64(input_right_weapon)		/*filled by getinputstate, for vr*/ \
	\
	globalvector(global_gravitydir)			/*vector	used when .gravitydir is 0 0 0 */ \
	globalfloat (dimension_default)			/*float		default value for dimension_hit+dimension_solid*/ \
	globalfloat (autocvar_vid_conwidth)		/*float		hackfix for dp mods*/	\
	globalfloat (autocvar_vid_conheight)	/*float		hackfix for dp mods*/	\
	globalfloat (cycle_wrapped)	\
	ENDLIST


//note: doesn't even have to match the clprogs.dat :)
typedef struct {

#define comfieldfloat(csqcname,desc) float csqcname;
#define comfieldvector(csqcname,desc) vec3_t csqcname;
#define comfieldentity(csqcname,desc) int csqcname;
#define comfieldstring(csqcname,desc) string_t csqcname;
#define comfieldfunction(csqcname, typestr,desc) func_t csqcname;
comqcfields
#undef comfieldfloat
#undef comfieldvector
#undef comfieldentity
#undef comfieldstring
#undef comfieldfunction

#ifdef VM_Q1
} csqcentvars_t;
typedef struct {
#endif

#define comfieldfloat(name,desc) float name;
#define comfieldint(name,desc) int name;
#define comfieldvector(name,desc) vec3_t name;
#define comfieldentity(name,desc) int name;
#define comfieldstring(name,desc) string_t name;
#define comfieldfunction(name, typestr,desc) func_t name;
comextqcfields
csqcextfields
#undef comfieldfloat
#undef comfieldint
#undef comfieldvector
#undef comfieldentity
#undef comfieldstring
#undef comfieldfunction

#ifdef VM_Q1
} csqcextentvars_t;
#else
} csqcentvars_t;
#endif

typedef struct csqcedict_s
{
	enum ereftype_e	ereftype;
	float			freetime; // sv.time when the object was freed
	int				entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//world
#ifdef VM_Q1
	csqcentvars_t	*v;
	csqcextentvars_t	*xv;
#else
	union {
		csqcentvars_t	*v;
		csqcentvars_t	*xv;
	};
#endif
	/*the above is shared with qclib*/
#ifdef USEAREAGRID
	areagridlink_t	gridareas[AREAGRIDPERENT];	//on overflow, use the inefficient overflow list.
	size_t			gridareasequence;	//used to avoid iterrating the same ent twice.
#else
	link_t	area;
#endif
	pvscache_t pvsinfo;
	int lastruntime;
	int solidsize;
#ifdef USERBE
	entityrbe_t rbe;
#endif
	/*the above is shared with ssqc*/

	//add whatever you wish here
	trailkey_t trailstate;
	int		skinobject;
} csqcedict_t;

typedef struct qcstate_s
{
	float resumetime;
	qboolean waiting;
	struct qcthread_s *thread;
	int self;
	int selfid;
	int other;
	int otherid;
	float returnval;

	struct qcstate_s *next;
} qcstate_t;

#ifdef __cplusplus
};
#endif
#endif
