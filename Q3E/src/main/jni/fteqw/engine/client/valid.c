#include "quakedef.h"
#include <ctype.h>

//ruleset validation:
//internal rulesets are taken at their word.
//	we cannot verify the client build itself.
//	its too easy to spoof replies from the client (eg by using a proxy) so there's not much point trying to sign things by hashing the engine binary etc
//	and private keys in gpl software is not feasable.
//custom/external rulesets may get out of sync with the engine. or the file hand-edited, or poorly updated etc.
//	we DO hash the rules and report them with the hash. this stops trivial text-editing cheaters.
//	we sort the rules etc so comments or reordering does not affect anything, allowing for some maintainence of the files without functional changes.
//	this ensures newcomers to a tournament do not have to worry about inadvertantly using a cheat, they just need to update their ruleset files (presumably alongside the rest of whatever mod(s) they're playing with).

#ifdef _WIN32
#include "winquake.h"
#endif

qboolean care_f_modified;
qboolean f_modified_particles;


typedef struct {
	char *rulecond;
	char *rulename;
	char *rulevalue;
} rulesetrule_t;
typedef struct {
	hashfunc_t *hashfunc;
	char *filename;
	char *hash;
	size_t numhashes;	//we may allow more than one hash.
} rulesetfilehashes_t;
typedef struct ruleset_s {
	struct ruleset_s *next;
	char *rulesetname;

	size_t rules;
	rulesetrule_t *rule;

	size_t filehashes;
	rulesetfilehashes_t *filehash;

	qboolean flagged;

	qbyte hash[20];	//sha1
	qboolean external;
} ruleset_t;

static ruleset_t *ruleset_list;
static ruleset_t *ruleset_current;




static void RulesetLatch(cvar_t *cvar);
static void QDECL rulesetcallback(cvar_t *var, char *oldval)
{
	Validation_Apply_Ruleset();
}

cvar_t allow_f_version		= CVAR("allow_f_version", "1");
cvar_t allow_f_server		= CVAR("allow_f_server", "1");
cvar_t allow_f_skins		= CVAR("allow_f_skins", "1");
cvar_t allow_f_ruleset		= CVAR("allow_f_ruleset", "1");
cvar_t allow_f_scripts		= CVAR("allow_f_scripts", "1");
#ifdef HAVE_LEGACY
cvar_t allow_f_modified		= CVAR("allow_f_modified", "1");
cvar_t allow_f_fakeshaft	= CVAR("allow_f_fakeshaft", "1");
cvar_t auth_validateclients	= CVAR("auth_validateclients", "1");
#endif
cvar_t allow_f_system		= CVAR("allow_f_system", "0");
cvar_t allow_f_cmdline		= CVAR("allow_f_cmdline", "0");
cvar_t ruleset			= CVARCD("ruleset", "none", rulesetcallback, "Known rulesets are:\nnone: no explicit rules, all 'minor cheats' are allowed.\nstrict: equivelent to the smackdown ruleset. Note that this will block certain graphical enhancements too.");

#ifdef HAVE_LEGACY
#define SECURITY_INIT_BAD_CHECKSUM	1
#define SECURITY_INIT_BAD_VERSION	2
#define SECURITY_INIT_ERROR			3
#define SECURITY_INIT_NOPROC		4

typedef struct signed_buffer_s {
	qbyte *buf;
	unsigned long size;
} signed_buffer_t;

typedef signed_buffer_t *(*Security_Verify_Response_t) (int playernum, unsigned char *, char *userinfo, char *serverinfo);
typedef int (*Security_Init_t) (char *);
typedef signed_buffer_t *(*Security_Generate_Crc_t) (int playernum, char *userinfo, char *serverinfo);
typedef signed_buffer_t *(*Security_IsModelModified_t) (char *, int, qbyte *, int);
typedef void (*Security_Supported_Binaries_t) (void *);
typedef void (*Security_Shutdown_t) (void);


static Security_Verify_Response_t Security_Verify_Response;
static Security_Init_t Security_Init;
static Security_Generate_Crc_t Security_Generate_Crc;
static Security_IsModelModified_t Security_IsModelModified;
static Security_Supported_Binaries_t Security_Supported_Binaries;
static Security_Shutdown_t Security_Shutdown;


#if 0//def _WIN32
static void *secmodule;
#endif
#endif

static void Validation_Version(void)
{
	char sr[256];
	char *s = sr;
	char authbuf[256];
	char *auth = authbuf;

	extern cvar_t r_drawflat;

	//print certain allowed 'cheat' options.
	//realtime lighting (shadows can show around corners)
	//drawflat is just lame
	//24bits can be considered eeeevil, by some.
#ifdef RTLIGHTS
	if (r_shadow_realtime_world.ival)
		*s++ = 'W';
	else if (r_shadow_realtime_dlight.ival)
		*s++ = 'S';
#endif
	if (r_drawflat.ival || r_lightmap.ival)
		*s++ = 'F';
	if (gl_load24bit.ival)
		*s++ = 'H';

	*s = '\0';

	if (!allow_f_version.ival)
		return;	//suppress it

#ifdef HAVE_LEGACY
	if (Security_Generate_Crc)
	{
		signed_buffer_t *resp;

		resp = NULL;//Security_Generate_Crc(cl.playerview[0].playernum, cl.players[cl.playerview[0].playernum].userinfo, cl.serverinfo);
		if (!resp || !resp->buf)
			auth = "";
		else
			Q_snprintfz(auth, sizeof(authbuf), " crc: %s", resp->buf);
	}
	else
#endif
		auth = "";

	if (*sr)
		Cbuf_AddText (va("say %s "PLATFORM"/%s/%s%s\n", version_string(), q_renderername, sr, auth), RESTRICT_RCON);
	else
		Cbuf_AddText (va("say %s "PLATFORM"/%s%s\n", version_string(), q_renderername, auth), RESTRICT_RCON);
}
void Validation_CheckIfResponse(char *text)
{
#ifdef HAVE_LEGACY
	//client name, version type(os-renderer where it matters, os/renderer where renderer doesn't), 12 char hex crc
	int f_query_client;
	int i;
	char *crc;
	char *versionstring;

	if (!Security_Verify_Response)
		return;	//valid or not, we can't check it.

	if (!auth_validateclients.ival)
		return;

	//do the parsing.
	{
		char *comp;
		int namelen;

		for (crc = text + strlen(text) - 1; crc > text; crc--)
			if ((unsigned)*crc > ' ')
				break;

		//find the crc.
		for (i = 0; i < 29; i++)
		{
			if (crc <= text)
				return;	//not enough chars.
			if ((unsigned)crc[-1] <= ' ')
				break;
			crc--;
		}

		//we now want 3 string seperated tokens, so the first starts at the fourth found ' ' + 1
		i = 7;
		for (comp = crc-1; ; comp--)
		{
			if (comp < text)
				return;
			if (*comp == ' ')
			{
				i--;
				if (!i)
					break;
			}

		}

		versionstring = comp+1;
		if (comp <= text)
			return;	//not enough space for the 'name:'
		if (*(comp-1) != ':')
			return;	//whoops. not a say.

		namelen = comp - text-1;

		for (f_query_client = 0; f_query_client < cl.allocated_client_slots; f_query_client++)
		{
			if (strlen(cl.players[f_query_client].name) == namelen)
				if (!strncmp(cl.players[f_query_client].name, text, namelen))
					break;
		}
		if (f_query_client == cl.allocated_client_slots)
			return; //looks like a validation, but it's not from a known client.
	}

	{
		char *match = DISTRIBUTION" v";
		if (strncmp(versionstring, match, strlen(match)))
			return;	//this is not us
	}

	//now do the validation
	{
		signed_buffer_t *resp;

		resp = NULL;//Security_Verify_Response(f_query_client, crc, cl.players[f_query_client].userinfo, cl.serverinfo);

		if (resp && resp->size && *resp->buf)
			Con_Printf(CON_NOTICE "Authentication Successful.\n");
		else// if (!resp)
			Con_Printf(CON_ERROR "AUTHENTICATION FAILED.\n");
	}
#endif
}

void InitValidation(void)
{
	Cvar_Register(&allow_f_version,	"Authentication");
	Cvar_Register(&allow_f_server,	"Authentication");
#ifdef HAVE_LEGACY
	Cvar_Register(&allow_f_modified,	"Authentication");
	Cvar_Register(&allow_f_fakeshaft,	"Authentication");
#endif
	Cvar_Register(&allow_f_skins,	"Authentication");
	Cvar_Register(&allow_f_ruleset,	"Authentication");
	Cvar_Register(&allow_f_scripts,	"Authentication");
	Cvar_Register(&allow_f_system,	"Authentication");
	Cvar_Register(&allow_f_cmdline,	"Authentication");
	Cvar_Register(&ruleset,		"Authentication");

#ifdef HAVE_LEGACY
#if 0//def _WIN32
	secmodule = LoadLibrary("fteqw-security.dll");
	if (secmodule)
	{
		Security_Verify_Response	= (void*)GetProcAddress(secmodule, "Security_Verify_Response");
		Security_Init				= (void*)GetProcAddress(secmodule, "Security_Init");
		Security_Generate_Crc		= (void*)GetProcAddress(secmodule, "Security_Generate_Crc");
		Security_IsModelModified	= (void*)GetProcAddress(secmodule, "Security_IsModelModified");
		Security_Supported_Binaries	= (void*)GetProcAddress(secmodule, "Security_Supported_Binaries");
		Security_Shutdown			= (void*)GetProcAddress(secmodule, "Security_Shutdown");
	}
#endif

	if (Security_Init)
	{
		switch(Security_Init(va("%s %.2f %i", DISTRIBUTION, 2.57, version_number())))
		{
		case SECURITY_INIT_BAD_CHECKSUM:
			Con_Printf("Checksum failed. Security module does not support this build. Go upgrade it.\n");
			break;
		case SECURITY_INIT_BAD_VERSION:
			Con_Printf("Version failed. Security module does not support this version. Go upgrade.\n");
			break;
		case SECURITY_INIT_ERROR:
			Con_Printf("'Generic' security error. Stop hacking.\n");
			break;
		case SECURITY_INIT_NOPROC:
			Con_Printf("/proc/* does not exist. You will need to upgrade/reconfigure your kernel.\n");
			break;
		case 0:
			Cvar_Register(&auth_validateclients,	"Authentication");
			return;
		}
#if 0//def _WIN32
		FreeLibrary(secmodule);
#endif
	}
	Security_Verify_Response	= NULL;
	Security_Init				= NULL;
	Security_Generate_Crc		= NULL;
	Security_IsModelModified	= NULL;
	Security_Supported_Binaries	= NULL;
	Security_Shutdown			= NULL;
#endif
}

//////////////////////
//f_modified

#ifdef HAVE_LEGACY
#define FMOD_DM 1
#define FMOD_TF 2
static const struct {
	const char *name;
	unsigned int flags;
	unsigned int hashes;
	const char *hash;
} modifiles[] =
/*Note: I don't know what that 'debug' package refers to, these are just the hashes from ezquake.*/
{
	{"progs/armor.mdl", FMOD_DM | FMOD_TF, 4,			"\xef\xb8\xd1\x18\x73\xd9\x43\xfe\x13\x43\x10\xbb\x90\x90\x6a\xef\x96\x86\x04\x2b"	//quake
														"\xfc\x4f\x26\x8d\x7c\x1e\xbb\xfa\xbc\x28\x11\xc8\x32\x3d\x63\x21\x4f\x59\x0b\xfa"	//debug
														"\x2a\xc5\x72\x77\x13\x7c\xe3\xde\x23\x85\xbc\xbb\xba\x25\x85\x34\x49\xd6\x1b\x6e"	//ruohis
														"\xb2\xe7\x47\x56\x46\x57\x46\x52\xfa\x20\x03\xc1\x0c\xc9\x92\x1e\x72\x7c\x57\x27"},//plaguespack
	{"progs/backpack.mdl", FMOD_DM | FMOD_TF, 2,		"\xeb\xda\xce\x80\x82\xd2\xf7\x18\xc2\xe7\x11\xa9\xaa\x09\xe6\xff\x05\x60\x0a\x05"	//quake
														"\x82\xf3\xc1\xe7\x2e\xc2\x3d\x0c\xc0\x04\x1a\xd3\x52\xed\x51\x72\x23\xf2\xba\x45"},//debug
	{"progs/bolt2.mdl", FMOD_DM | FMOD_TF, 1,			"\xde\xa8\xfb\x14\xe4\x7a\x23\x99\x43\x60\x80\xc8\x54\xa4\xee\xeb\xa9\x99\x4a\x02"},//quake
	{"progs/end1.mdl", FMOD_DM | FMOD_TF, 2,			"\x9f\xd1\xfe\xd1\x32\xc6\x67\x5d\xe6\xa0\x72\x1d\xd7\x39\xab\x14\xc4\x35\xf4\x4b"	//quake
														"\x22\x93\xff\x51\x3d\xb8\x31\x8b\xbf\xbe\x88\x89\xe7\xdc\x17\x09\x04\x8e\x57\xd9"},//ruohis
	{"progs/end2.mdl", FMOD_DM | FMOD_TF, 3,			"\x20\xe6\xee\x98\x79\xcd\x7a\x10\x0d\x62\x31\x83\x48\x18\x9a\x2a\x1a\x9e\xd2\x64"	//quake
														"\x10\xb9\xa1\xe3\x6a\xe3\xc4\x28\x21\x93\xf6\x1a\x99\xdf\x19\x9d\xcb\xbf\xce\x5b"	//unknown
														"\x4b\x1e\xa7\x2e\x22\xcf\xf6\x0e\xa5\x3b\xd5\x86\x3d\x43\x45\x11\x54\x20\x71\xe6"},//ruohis
	{"progs/end3.mdl", FMOD_DM | FMOD_TF, 3,			"\x6d\x33\x1e\x7f\xb0\x4b\x4f\xe0\x1a\x5a\xc2\x4a\xe7\xdd\xae\x2c\x19\x33\x97\x09"	//quake
														"\x10\xb9\xa1\xe3\x6a\xe3\xc4\x28\x21\x93\xf6\x1a\x99\xdf\x19\x9d\xcb\xbf\xce\x5b"	//unknown
														"\xa0\xed\xc3\x04\x34\x25\x4f\x2b\xca\xbb\x7a\x12\xcf\x97\x5c\x75\x97\x65\x5b\x13"},//ruohis
	{"progs/end4.mdl", FMOD_DM | FMOD_TF, 3,			"\xaa\x72\xb5\xa4\x8d\xc3\x00\xdd\xa2\x8d\x0f\x13\x55\x0e\x7f\x79\x76\x2b\xa5\xcc"	//quake
														"\x10\xb9\xa1\xe3\x6a\xe3\xc4\x28\x21\x93\xf6\x1a\x99\xdf\x19\x9d\xcb\xbf\xce\x5b"	//unkown
														"\xc1\x54\x5a\x4d\xba\xaa\xa8\x1f\x2b\x5b\x0a\x60\x3f\xff\x08\x75\xde\x03\xaf\xc7"},//ruohis
	{"progs/eyes.mdl", FMOD_DM | FMOD_TF, 1,			"\xa8\x25\x6f\x27\x82\xf7\xb8\xc6\x52\x5d\xf7\x3e\x3e\x16\x1d\x91\x57\xe5\x79\x5d"},//quake
	{"progs/g_light.mdl", FMOD_DM | FMOD_TF, 4,			"\x66\x93\x9b\xcb\x91\x4d\xd7\x1f\xf6\x82\x6a\x9a\x3e\x2e\x6b\x5d\xac\xf5\x58\xc4"	//quake
														"\x78\xb4\x78\x9d\x49\xc8\x44\xfb\x9d\x6d\x6b\xc5\x39\x78\x73\xab\x48\xd8\xcf\x1f"	//debug
														"\x92\xb8\x31\x62\x16\xd6\x39\x1f\x5d\x71\x42\x14\x26\x28\xfc\xfb\xd6\xd8\x66\xc8"	//plague
														"\xb8\x50\x2d\xfe\xb4\x6e\xd5\x6a\xeb\xfc\x79\xe5\x6d\x6f\xe4\x43\xda\x16\xf6\x82"},//ruohis
	{"progs/g_nail.mdl", FMOD_DM | FMOD_TF, 5,			"\x71\x9f\x20\x7b\x0f\xd0\x7c\xa7\x53\x49\xf2\x91\x4b\x26\x2f\x93\x40\x74\x0e\x35"	//quake
														"\x5d\x9f\x61\xc6\x85\x1a\x51\x55\x63\xcc\xe0\x6d\x1a\x17\x61\xef\x73\xb1\xb1\x36"	//debug
														"\xc6\x31\x2e\xcb\xab\x64\x10\x1f\x81\xe6\x0b\x5e\x42\x86\x65\x5e\xf4\x1e\x41\xd9"	//plague
														"\x7e\xa3\x7d\xbe\x2b\x27\x31\xb5\x9a\xe8\x7d\x0c\xbd\x87\xf3\x26\xee\x31\xdc\x20"	//ruohis
														"\x64\x19\xdd\x86\x85\x6f\xe6\xeb\x4c\x5b\x7e\xf2\xda\xae\x30\x32\x60\x38\xc1\x10"},//pdp
	{"progs/g_nail2.mdl", FMOD_DM | FMOD_TF, 4,			"\x99\x44\xcb\x97\x1a\xe1\xe3\x88\xca\x6e\xec\xec\x8f\x6c\x3f\x88\x0c\xcf\x7e\xac"	//quake
														"\x69\x83\xd5\x9c\x27\x85\x4c\xe5\xb4\x34\xa2\xce\x3e\xd9\x2b\x47\x55\x86\xfa\x7d"	//debug
														"\x01\xe4\x3f\x7d\x44\x43\x8a\x73\x39\x24\xb6\x4a\xeb\x13\x73\xba\x36\x85\x6f\x6a"	//plague
														"\xca\x15\xdf\x38\x50\x43\xdb\xe2\x41\x58\x06\x5c\x8c\x54\xec\x67\xb2\xf0\xb3\x3d"},//ruohis
	{"progs/g_rock.mdl", FMOD_DM | FMOD_TF, 5,			"\x83\xdd\x17\x90\x4c\x95\x0d\x15\x44\x45\x5f\x9b\x72\x3b\x84\x10\x8e\x06\x97\x46"	//quake
														"\x53\x92\x4e\x33\x52\xa2\xa5\xa5\x56\xa8\xb9\x68\x47\x66\x22\x5e\xc7\xee\xba\xe4"	//debug
														"\xb0\xc7\x1f\xe3\x18\x06\x20\x35\x3c\x97\xa6\x8b\x55\xc5\x96\x12\xde\x1b\x54\xb2"	//plague
														"\x9a\xe2\xe3\xd2\xcf\x2a\x3a\x1e\x53\x1d\xf2\xa2\xdd\x2a\x45\x60\xfa\x2a\x73\xd8"	//ruohis
														"\xfe\x90\xe9\x10\xd2\x1e\x40\x3b\xad\x71\x9d\x9a\x59\xf0\x85\x90\x8f\x08\x10\x77"},//pdp
	{"progs/g_rock2.mdl", FMOD_DM | FMOD_TF, 5,			"\x20\xec\x47\x5a\xdc\x1c\x21\xd0\x60\xaf\xb8\xd6\xab\x3e\x81\xaf\x5b\x0b\x33\xba"	//quake
														"\xf9\x10\x4e\xe2\x41\xbc\x53\x0f\x2b\xee\x43\x60\xec\x7e\x57\x3d\x4c\x12\x75\xbc"	//debug
														"\xe9\xb3\x4e\x67\x27\x59\x32\xde\x37\x43\xbd\xda\x5c\x75\x7a\xc9\xf9\xf1\xf4\x97"	//plagur
														"\x58\x0c\x35\x54\x88\xfd\x09\x6a\x80\x4b\x21\xae\xde\x71\x1d\xc7\xe6\x0f\x9d\x10"	//ruohis
														"\xd9\x80\x42\x3c\x56\x3a\xa4\xd8\xeb\x31\xf9\xef\xaf\x10\x63\xb7\xad\x39\x8c\xb2"},//pdp
	{"progs/g_shot.mdl", FMOD_DM | FMOD_TF, 5,			"\xe1\x35\xa7\x35\x90\x60\xee\xc1\xb5\x40\x89\x9f\x1c\xfd\xde\x6c\x67\x1d\xec\x7e"	//quake
														"\x28\xa8\xbb\x7b\x98\x8f\x43\x99\x47\x37\x5e\x97\x2b\x8a\xbc\x6c\xb7\x4d\xa6\xd3"	//debug
														"\x58\x63\x48\xd3\x37\x3d\x3a\x4a\xe4\x43\xfc\x0e\x89\x2f\xa4\x55\x19\x85\x07\xf8"	//plague
														"\xd8\xaf\x4c\xc7\x02\xf9\x3a\xbc\x88\xb5\x52\xbb\x30\xca\x6f\x6f\x54\xb5\x2a\x5b"	//ruohis
														"\x5f\x01\x9f\x3a\x9e\xa2\x17\xd8\xfa\x87\x7a\xdc\x28\x16\xe3\xc6\x19\x11\x99\xaa"},//pdp
	{"progs/gib1.mdl", FMOD_DM | FMOD_TF, 3,			"\xa4\x9e\xdc\x99\x4a\xf7\x9b\x6e\x1e\x0a\x71\x25\x7b\xc7\x1f\x70\x92\x70\x77\x09"	//quake
														"\xfd\x06\xee\x69\x83\x84\xac\x8b\x3e\xa4\xc5\xf9\x22\x37\x51\xd7\xff\xa4\xd5\x55"	//debug
														"\xf8\x57\xff\x05\x96\xa0\x73\x10\x55\xd0\xe5\xc7\x0b\x04\x6c\x9c\x8a\xeb\xd2\x96"},//ruohis
	{"progs/gib2.mdl", FMOD_DM | FMOD_TF, 3,			"\x9b\xe2\xc3\x5a\xd9\x58\x63\xd6\x7a\xd2\x44\x10\xad\x48\xda\xb3\xbb\x9f\x1e\x5f"	//quake
														"\xdf\xa1\x51\x32\x08\x82\xe6\x50\x97\xf7\xf0\xef\x71\x4e\x89\x89\xe3\x5b\x50\x65"	//debug
														"\x87\xd6\x23\xdf\x47\x90\xea\x51\x97\x34\xdb\xbf\xdb\x63\x7b\xed\xfb\x1b\x1a\x13"},//ruohis
	{"progs/gib3.mdl", FMOD_DM | FMOD_TF, 3,			"\x61\x8e\x55\xc6\x63\x4f\xea\x13\x45\xda\xc9\x20\x2e\x21\x40\x06\x50\xf3\x98\x7b"	//quake
														"\x46\x0b\x8f\x79\x50\x72\x5c\xe5\xf5\xf3\x2f\x88\x80\x5a\x49\x75\x99\xed\xa3\x19"	//debug
														"\x1e\xf0\x5d\xff\x2f\x95\x06\x76\x84\xc7\x36\xdd\x33\x2d\xd7\xa0\x33\xfa\x6a\x06"},//ruohis
	{"progs/grenade.mdl", FMOD_DM | FMOD_TF, 4,			"\xb8\xff\xdf\x60\x0c\x1f\x87\xfc\x25\xc3\xf3\xd9\xaf\xdc\xaa\x61\xbf\x7c\xc3\x0e"	//quake
														"\x12\xdb\xf5\xda\x02\xfc\xd4\x41\x5a\xd3\x4d\x76\x88\x08\x49\xa4\xea\x6c\xd2\xd5"	//debug
														"\x10\xb9\xa1\xe3\x6a\xe3\xc4\x28\x21\x93\xf6\x1a\x99\xdf\x19\x9d\xcb\xbf\xce\x5b"	//plague
														"\x1e\x57\xe5\xec\xe8\x61\x5d\xa3\x49\xdf\xb6\xe2\xd9\x72\x53\xf3\x10\x89\xc7\x7a"},//ruohis
	{"progs/invisibl.mdl", FMOD_DM | FMOD_TF, 3,		"\x89\xf3\xe2\x23\x7f\x65\x79\x84\x25\x0d\x7e\x43\xae\x0b\x10\xee\x75\xa7\xd6\xba"	//quake
														"\x60\xae\xac\xe5\xfd\xe8\x2f\x8b\x78\x8e\xef\xb8\xe4\x6a\x23\x8d\xe3\x0b\xdb\xc3"	//debug
														"\x7e\x7e\x50\xb7\x19\x70\xdc\x84\x39\x1c\x5f\x8f\x79\x29\xdf\xc0\xdd\x93\x6b\xe8"},//ruohis
	{"progs/invulner.mdl", FMOD_DM | FMOD_TF, 3,		"\x75\xe1\x7e\x98\x35\x4f\x0d\xfd\x1c\x64\xd2\x06\xc8\x0d\x5c\x72\x7a\x53\x1f\x87"	//quake
														"\x3e\x0a\xb0\x57\x6d\xfa\x9a\x00\xcb\xc8\xc2\xa4\xcc\xec\xb0\xa7\x49\x70\xe5\xa9"	//debug
														"\x18\xa1\xcc\xdd\x41\x3d\x14\x9e\x57\xdc\xa8\xa2\xb7\x4e\x74\x82\x1b\x30\xaf\x09"},//ruohis
	{"progs/missile.mdl", FMOD_DM | FMOD_TF, 4,			"\xe8\xee\xdf\x9a\xc1\x72\x58\x18\xf8\x36\xbb\xb3\xab\x29\x6e\x99\xa9\xb2\x6a\xd4"	//quake
														"\x78\xa0\xe7\x2a\xd4\x93\x93\xc3\x88\x67\x57\x73\xd2\x99\x26\x24\xfd\x0b\x19\x8f"	//debug
														"\xec\xb3\x47\xe0\xe2\xd2\x03\xad\x07\x62\x14\x2a\xdf\xf2\xe1\x99\x42\x9f\x22\xfb"	//plague
														"\xca\x4a\x84\x7e\xf9\x7e\xb0\xb1\xd8\x94\x89\x3d\x4e\xd1\xb4\xe6\x58\x98\xc4\x56"},//ruohis
	{"progs/quaddama.mdl", FMOD_DM | FMOD_TF, 3,		"\x63\xf6\x60\x27\x05\x84\xdc\x32\xdf\x63\x75\x05\xa7\xc3\x14\x96\x9b\x94\x25\x01"	//quake
														"\x56\xa1\x10\x90\xdb\xad\x63\x1b\xe3\xd9\x9b\xbc\x4e\x6e\x8d\xff\x60\x12\xcd\xce"	//debug
														"\x6c\xcf\x93\xdb\xb8\xa0\x06\x70\x56\x8c\xb2\x90\xa7\xfb\x7a\xf9\xcb\x99\x36\x42"},//ruohis
	{"progs/s_spike.mdl", FMOD_DM | FMOD_TF, 4,			"\xe5\xf8\x08\xf3\xe2\x42\xc2\xcd\x1f\xb0\x71\x4f\x0a\x88\xb9\xaf\x9f\x8e\x19\x52"	//quake
														"\xcc\xe4\x59\xb1\xf0\xcc\x5d\xbc\xab\x93\x6e\x65\x24\xdd\x72\x3e\xc6\x6f\x44\x10"	//debug
														"\x11\x1b\xc6\xe7\x30\x7f\x3a\x70\xda\xa5\x51\x00\xd1\x5b\x4a\xb8\xac\x45\x36\xe2"	//plague
														"\xc5\x84\xbf\x40\x9c\x5a\xb8\xca\x24\xaa\x8b\x49\xc6\xd9\x07\xbe\x56\x67\x66\x6b"},//ruohis
	{"progs/spike.mdl", FMOD_DM | FMOD_TF, 4,			"\xaf\xad\xd9\xeb\x28\x2f\x3b\xfb\x34\x2c\xcc\x67\x1a\xc2\x6e\x92\x33\xa2\xe1\x09"	//quake
														"\x44\xb9\x8b\xe7\xe4\x53\xa7\x92\x6b\x22\x5c\x43\x5e\xa6\x21\x40\x6b\x8c\x38\xef"	//debug
														"\x95\xcb\xf1\x28\x91\xed\xb8\xaf\xff\x00\x83\x6a\x3f\xc0\x29\xeb\xcb\xbb\xa2\x28"	//plague
														"\x80\x59\x22\xd8\xf7\xe9\x99\x02\x66\xfd\x32\x67\x64\x52\x55\x54\x03\xa4\xbd\x67"},//ruohis
	{"progs/suit.mdl", FMOD_DM | FMOD_TF, 2,			"\xdd\xb9\xdc\xb7\x3b\xa0\x8d\xed\x5e\xfc\x6e\x41\x5a\x8d\xe3\x8e\x25\xbf\x63\x40"	//quake
														"\xf3\xe4\x4d\xbf\xc1\x27\x09\xe6\x00\x4f\x82\xf4\x56\xef\xfe\x21\x97\xa8\x00\xac"},//ruohis
	{"progs/player.mdl", FMOD_DM, 2,					"\xb4\x0a\xca\x95\x2e\xe6\x1b\x02\xa5\xe9\x55\x66\x1c\xef\xa7\xd4\x2f\x58\x84\xb4"	//quake
														"\x59\x34\x22\xc0\x8d\x7d\xb9\x42\x72\xf4\x2f\xc5\x10\x07\xee\xf3\x32\x11\xe2\x41"},//capnbubs
	{"progs/player.mdl", FMOD_TF, 1,					"\x58\xb6\xca\x8f\xef\x97\x7a\x02\x3d\xee\x6e\xa9\x46\x4f\xe4\xc1\xc9\x33\xa5\x18"},//tf
	{"progs/tf_flag.mdl", FMOD_TF, 1,					"\xf0\x9d\x96\x6e\xef\x9e\x1b\xc9\x40\x1a\xcb\x84\x2e\x12\xee\xca\x28\x3d\x1d\x1b"},//tf
	{"progs/turrbase.mdl", FMOD_TF, 1,					"\xd5\x73\xda\x0c\xcb\x03\x27\x82\x6b\xbe\x6c\x9e\xf7\x95\xfb\x94\xed\x4b\xc0\xc7"},//tf
	{"progs/turrgun.mdl", FMOD_TF, 1,					"\x6f\x12\x4b\x31\xb5\x7c\x8d\x7d\xc8\x85\xf1\x8d\xf7\x62\x36\x1f\x7b\x95\x72\xa2"},//tf
	{"progs/disp.mdl", FMOD_TF, 1,						"\x4a\xfa\x96\x55\x57\x24\x4e\xd9\xdf\x84\x0f\x14\xa8\xdd\x59\x58\xfc\x52\x67\xe9"},//tf
	{"progs/tf_stan.mdl", FMOD_TF, 1,					"\x1b\x2a\x73\x1b\xc2\x69\x30\x76\x70\xe2\x79\xe7\xaa\x9d\x2b\x25\x2a\xdb\xf7\xdd"},//tf
	{"progs/hgren2.mdl", FMOD_TF, 1,					"\x5f\x71\xc3\x1d\x0f\x3c\xc5\x58\xfc\x04\x15\xdb\xdf\x4b\x35\xab\x97\xee\xac\x6a"},//tf
	{"progs/grenade2.mdl", FMOD_TF, 1,					"\xef\x6c\x11\x4b\x74\xf0\xc6\xeb\x73\xea\xcd\x2a\x16\x87\x6a\x87\x9d\x40\xe7\xe5"},//tf
	{"progs/detpack.mdl", FMOD_TF, 1,					"\xe1\xd4\x73\xaf\x38\xdc\x98\x0c\xc0\x83\x39\x6d\x33\x00\xa7\xd7\x5e\x67\x6a\x61"},//tf
	{"progs/s_bubble.spr", FMOD_DM | FMOD_TF, 1,		"\xf2\xfa\xbe\x3d\x54\x86\xb3\x73\x74\x07\x91\x70\xf3\x85\xe2\xdf\x0e\xd9\xb5\x4d"},//quake
	{"progs/s_explod.spr", FMOD_DM | FMOD_TF, 1,		"\x06\x7b\x5a\x29\x88\x1f\xdf\xac\x94\x08\xa2\x50\x5f\x90\x75\x5c\x0b\x5d\xd9\xb9"},//quake
	{"maps/b_bh100.bsp", FMOD_DM | FMOD_TF, 4,			"\x02\xe3\xe7\x65\x4d\x5b\xa8\x94\x74\xe6\x92\x80\xf0\xe5\x00\xf7\xcc\x7f\x66\xde"	//quake
														"\xb6\xd5\x7f\x52\x7f\x70\x90\xd4\x22\x96\xd6\xab\x69\x8a\xd1\xf3\xb9\x0e\xbc\xe9"	//ruohis1
														"\xc1\x35\x21\xb4\x9c\x42\x16\x68\x3e\xce\xe3\x21\xfc\xf8\xb9\xfa\xe1\x7a\x38\x09"	//ruohis2
														"\xff\x55\xb4\xc6\x45\xb8\x7c\xad\x29\x67\x35\xa7\xcf\x5f\x37\x35\x79\xb4\xad\x10"},//generations
	{"sound/buttons/airbut1.wav", FMOD_DM | FMOD_TF, 2,	"\x1e\x14\xd5\x47\xeb\xe8\x25\xc9\x3c\x58\xe5\x26\x21\xd0\xdf\xc8\xef\x92\x67\x22"	//quake
														"\xe0\xbc\xfb\xd5\x31\xe4\x83\x20\x4d\xc5\x11\xaa\x53\xe0\x9c\x08\x11\xce\x03\xdf"},//mindgrind
	{"sound/items/armor1.wav", FMOD_DM | FMOD_TF, 2,	"\xb0\x8d\x48\x44\x1d\x0a\x0b\xef\xb4\xa8\xcd\x3a\x67\xb4\x87\x3d\xcc\x4f\xdd\xe4"	//quake
														"\xed\xda\x47\x1b\x6a\xcd\x60\x5a\x99\x04\x94\x83\x1b\x65\xeb\x65\xcf\xf6\x41\x44"},//mindgrind
	{"sound/items/damage.wav", FMOD_DM | FMOD_TF, 2,	"\xac\x78\x77\x19\xc5\x52\xa2\x92\x56\x02\xc3\x90\x64\xf2\xa6\x7b\x4f\x65\xab\x56"	//quake
														"\xa3\x87\xa0\x3d\xc7\xd4\x99\x17\x31\xbf\xda\x8d\x6f\xde\xe2\x42\xec\xab\xfd\x5b"},//mindgrind
	{"sound/items/damage2.wav", FMOD_DM | FMOD_TF, 2,	"\x4d\x9b\x9a\x71\x18\xb2\x76\x1c\x9f\x97\xbf\xfc\xfa\xe6\xa5\x5e\x0e\xda\xaf\x68"	//quake
														"\x9c\x77\xeb\xd0\x3c\xd8\xf1\x0b\x7e\x1c\x0e\x12\x9b\x44\xfa\x50\x5b\x65\xd4\x2c"},//mindgrind
	{"sound/items/damage3.wav", FMOD_DM | FMOD_TF, 2,	"\x6c\xde\x07\xfa\x39\x6d\x30\x6e\xed\xf3\x18\x7d\x58\x25\x27\x90\x7a\x1e\xd0\x63"	//quake
														"\x71\x3b\x0c\x2e\xc9\xcf\x52\xdd\xe5\xe1\x5f\x8e\xd9\x8f\xc4\x0a\x70\xb9\xd8\xa2"},//mindgrind
	{"sound/items/health1.wav", FMOD_DM | FMOD_TF, 1,	"\xb2\x17\xdd\xeb\x80\x9b\x28\xa1\xf2\xe3\x10\x78\xbf\x01\x8d\xdb\x96\x5a\x49\x0b"},//quake
	{"sound/items/inv1.wav", FMOD_DM | FMOD_TF, 2,		"\xb8\x40\x12\xa0\x30\xd4\x88\xb5\xe0\x06\x24\xc6\xfd\x9d\xe5\x39\x98\x4b\x5a\xad"	//quake
														"\xba\x08\x77\x78\x51\x2c\xf5\x63\x32\x59\x83\x55\x86\x6d\xb4\x7d\xcc\x30\xc8\xcc"},//mindgrind
	{"sound/items/inv2.wav", FMOD_DM | FMOD_TF, 2,		"\x5d\x02\xb6\x9a\xa2\x24\x9d\x2d\x7c\xb1\x27\xc1\x8a\x90\x9e\x01\xd3\xf7\x21\xa5"	//quake
														"\x8c\xbc\xa3\xb2\x5f\x6b\xb5\x7e\x67\x4d\x5e\x67\x40\x40\x05\x24\x0b\xcd\x2e\x3b"},//mindgrind
	{"sound/items/inv3.wav", FMOD_DM | FMOD_TF, 2,		"\x77\x57\x78\xfb\x26\x28\x62\x9c\x5c\xd0\x21\x61\x61\x56\x2d\xf4\x29\x54\x4a\xa3"	//quake
														"\x33\x54\x94\xa1\x52\x2c\x3e\x7f\x8a\x52\x44\x91\xf0\x1c\x89\x02\x55\xb2\x5e\xd9"},//mindgrind
	{"sound/items/itembk2.wav", FMOD_DM | FMOD_TF, 2,	"\x9b\x51\x8c\x17\x27\x05\x03\x3b\xd2\xae\x9a\x75\xd7\xa7\xdc\xf7\x36\x1e\x1a\xf0"	//quake
														"\xad\x6d\xdd\x41\x4b\xc2\xa8\x45\x5e\xb7\x56\xef\x5a\x91\xb3\x5a\x0f\xa7\x01\x29"},//mindgrind
	{"sound/player/land.wav", FMOD_DM | FMOD_TF, 1,		"\x41\x5e\xbb\xb8\x8e\xad\x87\xf0\xd5\x3c\x32\x13\x52\x20\x2d\x2e\x38\x9e\x8e\x33"},//quake
	{"sound/player/land2.wav", FMOD_DM | FMOD_TF, 1,	"\x5c\xb5\x13\x6c\x89\x15\x6c\xc4\x42\xac\xab\xee\x4e\xd2\x7c\x08\x86\x23\x55\xd9"},//quake
	{"sound/misc/outwater.wav", FMOD_DM | FMOD_TF, 2,	"\x36\xfc\xb6\x9c\xba\xe9\x20\x9c\x18\x84\x5f\x59\x9f\x6d\xe7\x50\xfd\x3d\x50\xa7"	//quake
														"\x15\x85\xe9\x6b\x26\x01\xab\xfe\x11\xc8\xed\x80\x12\x70\xcf\xe7\x80\x49\xef\x1d"},//mindgrind
	{"sound/weapons/pkup.wav", FMOD_DM | FMOD_TF, 2,	"\x23\x35\xa4\x05\x60\xab\xbb\x09\xa0\x67\xce\x77\x3d\xe2\x2f\xb5\x01\x57\x71\xf2"	//quake
														"\xbc\x1f\x26\xda\x68\x7c\xf4\x12\x94\xe7\x91\x9e\x4d\x43\x0a\xce\x77\x5e\xe3\xc4"},//mindgrind
	{"sound/player/plyrjmp8.wav", FMOD_DM | FMOD_TF, 2,	"\x46\xe9\xe2\x75\xf2\x54\xad\xc9\x03\x7b\xd4\x6c\xd4\xc9\xf0\xee\xda\x86\x33\x5d"	//quake
														"\x63\x82\x9e\x5a\x6e\xf0\xb3\xb9\xe1\x7d\x04\x8a\xec\x8b\xc1\x72\x77\xf9\x7d\x54"},//mindgrind
	{"sound/items/protect.wav", FMOD_DM | FMOD_TF, 2,	"\x51\x13\xff\x38\xc7\x99\x67\x7d\x6d\x25\x77\x75\x92\x31\x23\xbf\xaa\x3b\x22\xb6"	//quake
														"\x3f\x33\x48\x59\x2a\x88\x8e\xa2\x64\x29\xb8\xb4\xc5\x79\x06\xe1\xb1\x8c\x1f\x19"},//mindgrind
	{"sound/items/protect2.wav", FMOD_DM | FMOD_TF, 2,	"\x59\xd1\x8d\x58\x2c\x9f\x48\xae\x8e\x11\xf7\x22\x4c\xeb\x7e\x48\x97\xc9\x54\x3c"	//quake
														"\xb0\x77\xe9\x29\xfe\x78\x93\x0d\x37\x90\x48\xb4\x62\x3d\xd8\x02\x8d\x71\x0b\xce"},//mindgrind
	{"sound/items/protect3.wav", FMOD_DM | FMOD_TF, 2,	"\x43\x11\x9a\x86\x04\xa4\xe6\xc9\x0e\xd9\xd5\xcb\x4c\x7e\xb5\xa1\x20\xdd\x77\xae"	//quake
														"\x4c\x31\x19\x76\x1c\x90\xb6\x07\xe2\x01\x06\x00\x7e\x36\x58\xc3\xb1\x90\x9f\x99"},//mindgrind
	{"sound/items/r_item1.wav", FMOD_DM | FMOD_TF, 2,	"\x41\x29\xe1\xdd\x04\xb8\xb3\xfb\xfa\xf5\x6c\x77\xea\xf8\x1d\xe8\x63\x9f\x42\xa1"	//quake
														"\xd4\x54\x57\x0f\x5f\x4c\xb8\xa8\x3a\x90\x01\x00\x9c\x28\xab\xd3\x48\xc8\x3b\xa9"},//mindgrind
	{"sound/items/r_item2.wav", FMOD_DM | FMOD_TF, 4,	"\x47\x40\xda\xd9\x1a\x19\x7b\x5a\x0d\x86\x2d\xc0\xde\x79\xf6\x18\x3a\xd9\x7b\xc4"	//quake
														"\xd2\x00\x2a\xf6\xca\xaa\xce\x5f\x92\x16\xf9\x25\xb7\x2c\x60\xf7\x25\xa5\x0d\x23"	//us
														"\x46\xe6\xa8\x83\x13\xc3\x4c\xbc\x6e\xa9\x7a\xa0\xda\xea\x81\x9d\x10\xbd\x19\xf6"	//ru
														"\x39\xde\xd0\xd2\x33\x8d\xb7\x2b\x81\x30\xef\x09\xcf\xba\x90\xbe\xce\xec\x71\xbf"},//mindgrind
	{"sound/misc/water1.wav", FMOD_DM | FMOD_TF, 2,		"\xc7\xdb\x25\x86\xe6\x0a\xf3\xba\x65\x39\xb5\xfd\xd6\xb9\x7e\x2d\x04\x93\xfd\xf9"	//quake
														"\x18\x03\x8a\x83\x3e\xe5\x05\x08\x65\x40\xbf\x6e\xa0\xb9\x5f\xaa\x07\xd8\xca\xe1"},//mindgrind
	{"sound/misc/water2.wav", FMOD_DM | FMOD_TF, 2,		"\xec\x75\xda\xcc\x80\xd7\x5c\xfb\x5b\x3b\xd7\xe2\xad\x60\x35\x9f\x85\x6b\x11\x4e"	//quake
														"\x17\x6b\x0a\xfb\x33\xbc\x5f\x06\xc5\x7c\xab\x5f\xaa\x01\x0c\x11\x2e\x61\x8b\x27"},//mindgrind
	{"sound/misc/menu1.wav", FMOD_DM | FMOD_TF, 2,		"\x17\xb8\x19\x2e\x5f\x1c\x0e\x0c\xf8\xec\xa0\xd7\x7e\xc2\x78\xb2\x3c\x92\xe1\xb0"	//quake
														"\x73\x63\x3f\xf5\x3c\x61\x35\xd6\xd4\x41\x67\x9f\x05\xc2\x5c\x9d\x25\xc4\x15\x96"},//sean
	{"sound/misc/menu2.wav", FMOD_DM | FMOD_TF, 2,		"\xb1\x9a\x71\xd6\x2b\x7e\x40\x0a\x3b\x05\x3a\xb0\xcb\xc2\x4b\x2a\xf3\x7f\xcd\x61"	//quake
														"\xeb\x31\x56\xca\x89\x71\x57\x7b\x4e\xb1\xfd\x58\x90\x51\x56\xc4\xc4\x36\xc9\x6f"},//sean
	{"sound/misc/menu3.wav", FMOD_DM | FMOD_TF, 2,		"\x9a\x0b\x12\xae\x2e\x7d\x21\x3a\x90\x09\x4f\xc3\xed\x32\x43\x2d\x76\x8f\xc1\x97"	//quake
														"\xfe\xa5\x0c\xe6\x49\xf9\x0c\x30\x09\x5c\x2f\xb7\x70\x1c\x6d\x2b\x1f\xff\x2f\xd9"},//sean
	{"sound/misc/talk.wav", FMOD_DM | FMOD_TF, 2,		"\x1c\x32\x15\x5b\x26\xd8\xf2\x1a\x9f\x8e\x22\xb0\x56\x6f\xc0\x49\x8b\x5e\x35\xa8"	//quake
														"\x66\x60\x59\xab\x9d\xd4\xe1\xc7\xab\x9a\xdf\x00\x07\xfc\x14\xbd\xee\xae\xb2\x4a"},//sean
	{"sound/misc/basekey.wav", FMOD_DM | FMOD_TF, 2,	"\xce\x34\x61\x92\xd3\x6d\x80\x22\x4a\x62\x52\x19\xe9\xf7\x43\x8f\x64\xfe\xfd\xa6"	//quake
														"\xd4\x71\x98\xc9\xef\x0f\x0e\xc6\xf9\x4b\xe6\x14\x34\x39\xf4\xba\x18\xdf\x76\x53"},//gpl
	{"sound/doors/runeuse.wav", FMOD_DM | FMOD_TF, 2,	"\x59\x6d\x1d\xf4\xe8\x93\x9b\xe9\x25\xfd\xed\x15\x9c\x49\x8a\x66\x72\x25\xc6\x1a"	//quake
														"\x00\xb9\x76\xc5\xb3\x88\x89\x8f\x72\x0d\xa1\x85\xec\x42\x31\x4f\xba\xe3\x91\xac"},//sean
	{"gfx/colormap.lmp", FMOD_DM | FMOD_TF, 1,			"\x95\x37\x3b\xa9\x97\x63\x01\x17\x38\x1d\x76\x0a\x7d\xe6\x66\x48\x75\x2a\x2a\x3c"},//quake
	{"gfx/palette.lmp", FMOD_DM | FMOD_TF, 1,			"\x42\xe2\xa2\xa6\xda\xf7\xd0\xba\x1f\x35\x63\xe2\xad\xf8\x51\x6d\x4a\x5d\xa4\x49"},//quake
};
static enum {
	FMOD_UNCHECKED,
	FMOD_MODIFIED,
	FMOD_UNMODIFIED,
} modifiles_status[countof(modifiles)];
static qboolean modified_neednotify;
static double modified_timestamp; //so we don't spam reanouncements
static void Validation_FilesModified (void)
{
	size_t i;
	unsigned int flagmatch = cl.teamfortress?FMOD_TF:FMOD_DM;
	char buf[512];
	qboolean evilhaxxor = false;

	modified_timestamp = 0;
	modified_neednotify = true;
	Q_strncpyz(buf, "modified:", sizeof(buf));
	for (i = 0; i < countof(modifiles); i++)
	{
		if ((modifiles[i].flags & flagmatch) && modifiles_status[i] == FMOD_MODIFIED)
		{
			evilhaxxor = true;
			if (strlen(buf)>240)
			{
				modified_neednotify = false;//just spam at this point
				Q_strncatz(buf, " & more...", sizeof(buf));
				break;
			}
			else
			{
				Q_strncatz(buf, " ", sizeof(buf));
				Q_strncatz(buf, COM_SkipPath(modifiles[i].name), sizeof(buf));
			}
		}
	}
	if (!evilhaxxor)
		Q_strncpyz(buf, "all models okay", sizeof(buf));

	CL_SendClientCommand(true, "say %s", buf);
}
static qboolean Validation_IsModified(void)
{
	unsigned int flagmatch = cl.teamfortress?FMOD_TF:FMOD_DM;
	size_t i;
	for (i = 0; i < countof(modifiles); i++)
	{
		if ((modifiles[i].flags & flagmatch) && modifiles_status[i] == FMOD_MODIFIED)
			return true;
	}
	modified_timestamp = 0;
	modified_neednotify = true;
	return false;
}
static void Validation_WarnModified(void *ctx, void *data, size_t a, size_t b)
{
	if (modified_neednotify)
	{
		if (modified_timestamp && modified_timestamp > realtime)
			return;
		CL_SendClientCommand(true, "say warning: stuff changed! Previous f_modified response is no longer valid.");
		modified_timestamp = realtime+3.0;	//mute further anouncements for a while.
	}
}
#endif
qboolean Ruleset_FileLoaded(const char *filename, const qbyte *filedata, size_t filesize)
{	//usually called on worker threads
	qbyte digest[20];
	size_t i, j;

	if (ruleset_current && ruleset_current->filehashes)
	{
		size_t filehashes = ruleset_current->filehashes;
		rulesetfilehashes_t *filehash = ruleset_current->filehash;
		for (i = 0; i < filehashes; i++, filehash++)
		{
			if (!strcmp(filename, filehash->filename))
			{
				CalcHash(filehash->hashfunc, digest, sizeof(digest), filedata, filesize);

				for (j = 0; j < filehash->numhashes; j++)
				{
					if (!memcmp(digest, filehash->hash+j*filehash->hashfunc->digestsize, filehash->hashfunc->digestsize))
						return true;	//it matched one of the allowed hashes...
				}
				{
					char base16[512+1];
					//none of the hashes matched. bail. refuse usage of the file.
					base16[Base16_EncodeBlock(digest, filehash->hashfunc->digestsize, base16, sizeof(base16)-1)] = 0;
					Con_Printf(CON_ERROR"ERROR: File version \"%s\" \"%s\" is not permitted by ruleset %s\n", filename, base16, ruleset_current->rulesetname);
				}
				return false;
			}
		}
	}
#ifdef HAVE_LEGACY
	else for (i = 0; i < countof(modifiles); i++)
	{
		if (!strcmp(filename, modifiles[i].name) && (modifiles[i].flags & (cl.teamfortress?FMOD_TF:FMOD_DM)))
		{
			unsigned int status;
			CalcHash(&hash_sha1, digest, sizeof(digest), filedata, filesize);

			for (j = 0; j < modifiles[i].hashes; j++)
			{
				if (!memcmp(digest, modifiles[i].hash+j*20, 20))
					break;
			}
			status = (j==modifiles[i].hashes)?FMOD_MODIFIED:FMOD_UNMODIFIED;
			if (status == FMOD_MODIFIED && modifiles_status[i] == FMOD_UNMODIFIED)
				COM_AddWork(WG_MAIN, Validation_WarnModified, NULL, NULL, 0, 0); //make sure its the main thread that does the anouncing.
			modifiles_status[i] = status;
			return true;
		}
	}
#endif
	return true;	//nothing restricting it.
}
void Validation_FlushFileList(void)
{
#ifdef HAVE_LEGACY
	//just wipe the lot, resetting it back to 'unchecked'
	memset(modifiles_status, 0, sizeof(modifiles_status));
	modified_neednotify = false;
	modified_timestamp = 0;
#endif
}

/////////////////////////
//minor (codewise) responses

static void Validation_Server(void)
{
	char adr[MAX_ADR_SIZE];

#ifdef warningmsg
#pragma warningmsg("is allowing the user to turn this off practical?..")
#endif
	if (!allow_f_server.ival)
		return;
	Cbuf_AddText(va("say server is %s\n", NET_AdrToString(adr, sizeof(adr), &cls.netchan.remote_address)), RESTRICT_LOCAL);
}

static void Validation_Skins(void)
{
	extern cvar_t r_fullbrightSkins, r_fb_models, ruleset_allow_fbmodels;
	int percent = r_fullbrightSkins.value*100;

	if (!allow_f_skins.ival)
		return;

	RulesetLatch(&r_fb_models);
	RulesetLatch(&r_fullbrightSkins);

	if (percent < 0)
		percent = 0;
	if (percent > cls.allow_fbskins*100)
		percent = cls.allow_fbskins*100;
	if (percent)
		Cbuf_AddText(va("say all player skins %i%% fullbright%s\n", percent, (r_fb_models.ival == 1 && ruleset_allow_fbmodels.ival)?" (non-player 100%%)":(r_fb_models.value?" (plus luma)":"")), RESTRICT_LOCAL);
	else if (r_fb_models.ival == 1 && ruleset_allow_fbmodels.ival)
		Cbuf_AddText("say non-player entities glow in the dark like a bright big cheat\n", RESTRICT_LOCAL);
	else if (r_fb_models.ival)
		Cbuf_AddText("say luma textures only\n", RESTRICT_LOCAL);
	else
		Cbuf_AddText("say Only cheaters use full bright skins\n", RESTRICT_LOCAL);
}

static void Validation_Scripts(void)
{	//subset of ruleset
	if (!allow_f_scripts.ival)
		return;
	if (ruleset_allow_frj.ival)
		Cbuf_AddText("say scripts are allowed\n", RESTRICT_LOCAL);
	else
		Cbuf_AddText("say scripts are capped\n", RESTRICT_LOCAL);
}

#ifdef HAVE_LEGACY
static void Validation_FakeShaft(void)
{
	extern cvar_t cl_truelightning;
	if (!allow_f_fakeshaft.ival)
		return;
	if (cl_truelightning.value > 0.999)
		Cbuf_AddText("say fakeshaft on\n", RESTRICT_LOCAL);
	else if (cl_truelightning.value > 0)
		Cbuf_AddText(va("say fakeshaft %.1f%%\n", cl_truelightning.value), RESTRICT_LOCAL);
	else
		Cbuf_AddText("say fakeshaft off\n", RESTRICT_LOCAL);
}
#endif

static void Validation_System(void)
{	//subset of ruleset
	if (!allow_f_system.ival)
		return;
	Cbuf_AddText("say f_system not supported\n", RESTRICT_LOCAL);
}

static void Validation_CmdLine(void)
{
	if (!allow_f_cmdline.ival)
		return;
	Cbuf_AddText("say f_cmdline not supported\n", RESTRICT_LOCAL);
}

//////////////////////
//rulesets

/*when a ruleset is activated, we flag its various cvars are applicable to rulesets, and notify the servers whenever they're changed after having been queried.
changing a cvar to something and then back MUST still warn, in case its a cvar that locks down a command before being switched back.

we used to latch cvars so they couldn't be changed after querying, but this results in potential race conditions with other reasons to latch cvars. changing a ruleset cvar is now equivelent to just changing ruleset midgame.
*/

static ruleset_t *Ruleset_Find(const char *name)	//finds a ruleset by name
{
	ruleset_t *rs;
	for (rs = ruleset_list; rs; rs = rs->next)
	{
		if (!Q_strcasecmp(name, rs->rulesetname))
			break;
	}
	return rs;
}

static int QDECL Ruleset_SortRuleCB (const void *v1, const void *v2)
{
	const rulesetrule_t *r1 = v1;
	const rulesetrule_t *r2 = v2;
	int r = Q_strcmp(r1->rulename, r2->rulename);
	if (!r)
	{
		r = Q_strcmp(r1->rulecond, r2->rulecond);
		if (!r)
			r = Q_strcmp(r1->rulevalue, r2->rulevalue);
	}
	return r;
}
static int QDECL Ruleset_SortFileCB (const void *v1, const void *v2)
{
	const rulesetfilehashes_t *r1 = v1;
	const rulesetfilehashes_t *r2 = v2;
	return Q_strcmp(r1->filename, r2->filename);
}

static ruleset_t *Ruleset_Parse(const char *name, char *file)
{
	ruleset_t *rs = Z_Malloc(sizeof(*rs) + strlen(name)+1);
	char *line = file, *linestart, *eol;	//evil non-const...
	unsigned r, hashidx=~0, h;
	hashfunc_t *hashfunc = &hash_sha1;
	void *hctx;
	char fname[MAX_QPATH];

	*fname = 0;
	rs->rulesetname = (char*)(rs+1);
	strcpy(rs->rulesetname, name);

	for (line = file; line; )
	{
		eol = strchr(line, '\n');
		if (eol)
			*eol = 0;

		if (*line)
		{
			linestart = line;
			line = COM_Parse(line);
			if (!strcmp(com_token, "set"))
			{
				char *var, *val;
				line = COM_Parse(line);
				var = Z_StrDup(com_token);
				line = COM_Parse(line);
				val = Z_StrDup(com_token);

				r = rs->rules;
				Z_ReallocElements((void**)&rs->rule, &rs->rules, r+1, sizeof(*rs->rule));
				rs->rule[r].rulecond = NULL;
				rs->rule[r].rulename = var;
				rs->rule[r].rulevalue = val;

				hashidx = ~0;
			}
			else if (!strcmp(com_token, "if"))
			{
				char *var, *val, *cond = NULL;
				line = COM_Parse(line);
				if (*com_token)
					Z_StrCat(&cond, com_token);
				line = COM_Parse(line);
				if (*com_token == '>' || *com_token == '<' || *com_token == '=' || *com_token == '!')
				{
					if (*com_token)
						Z_StrCat(&cond, com_token);
					line = COM_Parse(line);
					if (*com_token)
						Z_StrCat(&cond, com_token);
					line = COM_Parse(line);
				}
				var = Z_StrDup(com_token);
				line = COM_Parse(line);
				val = Z_StrDup(com_token);

				r = rs->rules;
				Z_ReallocElements((void**)&rs->rule, &rs->rules, r+1, sizeof(*rs->rule));
				rs->rule[r].rulecond = cond;
				rs->rule[r].rulename = var;
				rs->rule[r].rulevalue = val;

				hashidx = ~0;
			}
			/*else if (!strcmp(com_token, "alias"))
			{
				line = COM_Parse(line);
				//alias = Z_StrDup(com_token);
			}*/
			else if (!strcmp(com_token, "sha1file"))
			{
				hashfunc = &hash_sha1;
				line = COM_ParseOut(line, fname, sizeof(fname));
				line = COM_Parse(line);

				for (hashidx = 0; hashidx < rs->filehashes; hashidx++)
				{
					if (!strcmp(rs->filehash[hashidx].filename, fname) && hashfunc == rs->filehash[hashidx].hashfunc)
						goto extrafilehash;
				}

				Z_ReallocElements((void**)&rs->filehash, &rs->filehashes, hashidx+1, sizeof(*rs->filehash));
				rs->filehash[hashidx].filename = Z_StrDup(fname);
				rs->filehash[hashidx].numhashes = 0;
				rs->filehash[hashidx].hash = NULL;
				rs->filehash[hashidx].hashfunc = hashfunc;
extrafilehash:
				if (!*com_token)
					;
				else if (strlen(com_token) != hashfunc->digestsize*2)
					Con_Printf("Ruleset %s file %s is of incorrect length.\n", name, fname);
				else
				{
					h = rs->filehash[hashidx].numhashes++;
					rs->filehash[hashidx].hash = BZ_Realloc(rs->filehash[hashidx].hash, rs->filehash[hashidx].numhashes*hashfunc->digestsize);
					if (hashfunc->digestsize != Base16_DecodeBlock(com_token, rs->filehash[hashidx].hash+h*hashfunc->digestsize, hashfunc->digestsize))
						Con_Printf("Ruleset %s file %s is not base16.\n", name, fname);
				}
			}
			else if (!strcmp(com_token, "+"))
			{
				line = COM_Parse(line);
				if (hashidx != ~0 && hashfunc)
					goto extrafilehash;
			}
			else if (*com_token)
			{
				Con_Printf("%s.rules: Unknown directive \"%s\".\n", name, com_token);
				hashidx = ~0;
				line += strlen(line);
			}	//else blank line

			line = COM_Parse(line);
			if (*com_token)
				Con_Printf("%s.rules: Trailing junk at end of line \"%s\".\n", name, linestart);
		}

		if (eol)
			*eol++ = '\n';
		line = eol;
	}

	//sort it for consistency
	qsort(rs->rule, rs->rules, sizeof(*rs->rule), Ruleset_SortRuleCB);
	qsort(rs->filehash, rs->filehashes, sizeof(*rs->filehash), Ruleset_SortFileCB);

	hashfunc = &hash_sha1;
	hctx = alloca(hashfunc->contextsize);
	hashfunc->init(hctx);
	for (r = 0; r < rs->rules; r++)
	{	//be sure to hash the nulls too. This prevents cheating lamas from using weird cvar names to fake results.
		char *rc = rs->rule[r].rulecond;
		if (!rc) rc = "";
		hashfunc->process(hctx, rc, strlen(rc)+1);
		hashfunc->process(hctx, rs->rule[r].rulename, strlen(rs->rule[r].rulename)+1);
		hashfunc->process(hctx, rs->rule[r].rulevalue, strlen(rs->rule[r].rulevalue)+1);
	}
	for (r = 0; r < rs->filehashes; r++)
	{	//be sure to hash the nulls too. This prevents cheating lamas from using weird file names to fake results.
		hash_sha1.process(hctx, rs->filehash[r].filename, strlen(rs->filehash[r].filename)+1);
		hash_sha1.process(hctx, rs->filehash[r].hash, rs->filehash[r].numhashes*rs->filehash[r].hashfunc->digestsize);	//should this be base16ed first?...
	}
	hashfunc->terminate(rs->hash, hctx);

	rs->next = ruleset_list;
	ruleset_list = rs;

	return rs;
}
#ifdef HAVE_LEGACY
static ruleset_t *Ruleset_ParseInternal(const char *name, const char *file)
{
	ruleset_t *rs;
	char *lazy;
	lazy = Z_StrDup(file);
	rs = Ruleset_Parse(name, lazy);
	Z_Free(lazy);
	return rs;
}
#endif

static int QDECL Ruleset_Read(const char *fname, qofs_t size, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	ruleset_t *rs;
	size_t fsize;
	char name[128];
	char *file = FS_LoadMallocFile(fname, &fsize);
	if (file && fsize == size)	//o.O
	{
		COM_StripExtension(fname, name, sizeof(name));
		rs = Ruleset_Parse(name, file);
		if (rs)
			rs->external = true;
	}
	Z_Free(file);
	return true;
}
void Ruleset_Shutdown(void)
{
	ruleset_current = NULL;
	while (ruleset_list)
	{
		ruleset_t *rs = ruleset_list;
		ruleset_list = rs->next;

		while (rs->rules)
		{
			rs->rules--;
			Z_Free(rs->rule[rs->rules].rulename);
			Z_Free(rs->rule[rs->rules].rulevalue);
			Z_Free(rs->rule[rs->rules].rulecond);
		}
		Z_Free(rs->rule);

		while (rs->filehashes)
		{
			rs->filehashes--;
			Z_Free(rs->filehash[rs->filehashes].filename);
			Z_Free(rs->filehash[rs->filehashes].hash);
		}
		Z_Free(rs->filehash);

		Z_Free(rs);
	}
}
void Ruleset_Scan(void)
{
	COM_EnumerateFiles("*.rules", Ruleset_Read, NULL);

#ifdef HAVE_LEGACY	//TCs should probably include their own.
	Ruleset_ParseInternal("strict",
//			"alias smackdown\n"
//			"alias qcon\n"
			"set ruleset_allow_shaders 0\n"	/*users can potentially create all sorts of wallhacks or spiked models with this*/
			"set ruleset_allow_watervis 0\n" /*oh noes! users might be able to see underwater if they're already in said water. oh wait. what? why do we care, dude*/
			"set r_vertexlight 0\n"
			"set ruleset_allow_playercount 0\n"
			"set ruleset_allow_frj 0\n"
			"set ruleset_allow_packet 0\n"
			"set ruleset_allow_particle_lightning 0\n"
			"set ruleset_allow_overlong_sounds 0\n"
			"set ruleset_allow_larger_models 0\n"
			"set ruleset_allow_modified_eyes 0\n"
			"set ruleset_allow_sensitive_texture_replacements 0\n"
			"set ruleset_allow_localvolume 0\n"
			"set ruleset_allow_fbmodels 0\n"
			"set ruleset_allow_triggers 0\n"
			"set r_particlesystem classic\n"	/*block custom particles*/
			"set r_part_density 1\n"	/*don't let people thin them out*/
			"set scr_autoid_team 0\n"	/*sort of a wallhack*/
			"set tp_disputablemacros 0\n"
			"set cl_instantrotate 0\n"
			"set v_projectionmode 0\n"	/*no extended fovs*/
			"set r_shadow_realtime_world 0\n" /*static lighting can be used to cast shadows around corners*/
			"set ruleset_allow_in 0\n"
			"set r_projection 0\n"
			"set gl_shadeq1_name *\n"
			"set cl_rollalpha 20\n"
			"set cl_iDrive 0\n"
			);

	Ruleset_ParseInternal("thunderdome",
			"set ruleset_allow_shaders 0\n"	/*users can potentially create all sorts of wallhacks or spiked models with this*/
			"set ruleset_allow_watervis 0\n" /*oh noes! users might be able to see underwater if they're already in said water. oh wait. what? why do we care, dude*/
			"set r_vertexlight 0\n"
			"set ruleset_allow_playercount 0\n"
			"set ruleset_allow_frj 0\n"
			"set ruleset_allow_packet 0\n"
			"set ruleset_allow_particle_lightning 0\n"
			"set ruleset_allow_overlong_sounds 0\n"
			"set ruleset_allow_larger_models 0\n"
			"set ruleset_allow_modified_eyes 0\n"
			"set ruleset_allow_sensitive_texture_replacements 0\n"
			"set ruleset_allow_localvolume 0\n"
			"set ruleset_allow_fbmodels 0\n"
			"set ruleset_allow_triggers 0\n"
			"set scr_autoid_team 0\n"
			"set tp_disputablemacros 0\n"
			"set cl_instantrotate 0\n"
			"set v_projectionmode 0\n"	/*no extended fovs*/
			"set r_shadow_realtime_world 0\n" /*static lighting can be used to cast shadows around corners*/
			"set ruleset_allow_in 0\n"
			"set r_projection 0\n"
			"set gl_shadeq1_name *\n"
		//	"set cl_rollalpha 20\n"
			"set cl_iDrive 0\n"
			);

	Ruleset_ParseInternal("nqr",
//			"alias eql\n"
			"set ruleset_allow_larger_models 0\n"
			"set ruleset_allow_watervis 0\n" /*block seeing through turbs, as well as all our cool graphics stuff. apparently we're not allowed.*/
			"set ruleset_allow_overlong_sounds 0\n"
			"set ruleset_allow_particle_lightning 0\n"
			"set ruleset_allow_packet 0\n"
			"set ruleset_allow_frj 0\n"
			"set ruleset_allow_modified_eyes 0\n"
			"set ruleset_allow_sensitive_texture_replacements 0\n"
			"set ruleset_allow_localvolume 0\n"
			"set ruleset_allow_shaders 0\n"
			"set ruleset_allow_fbmodels 0\n"
			"set r_vertexlight 0\n"
			"set v_projectionmode 0\n"
			"set sbar_teamstatus 0\n"
			"set ruleset_allow_in 0\n"
			"set r_projection 0\n"
			"set gl_shadeq1_name *\n"
			"set cl_iDrive 0\n"
			);
#endif
}

static void RulesetLatch(cvar_t *cvar)
{
	cvar->flags |= CVAR_RULESETLATCH;
}

void Validation_DelatchRulesets(void)
{	//game has come to an end, allow the ruleset to be changed
	if (Cvar_ApplyLatches(CVAR_RULESETLATCH, true))
		Con_DPrintf("Ruleset deactivated\n");
}

const char *Ruleset_GetRulesetName(void)
{
	if (ruleset_current)
		return ruleset_current->rulesetname;
	else
		return NULL;
}

static void Validation_OldRuleset(void)
{
	const char *rsname = Ruleset_GetRulesetName();

	if (rsname)
		Cbuf_AddText(va("say Ruleset: %s\n", rsname), RESTRICT_LOCAL);
	else
		Cbuf_AddText("say No specific ruleset\n", RESTRICT_LOCAL);
}

static void Validation_AllChecks(void)
{
	extern cvar_t cl_iDrive;
	char servername[22];
	char playername[16];
	char *rawenginebuild = version_string();
	char enginebuild[64];
	char localpnamelen = strlen(cl.players[cl.playerview[0].playernum].name);
	const char *ruleset;
	char extras[2][8];
	size_t i, j;
	qboolean hadspace;

	//figure out the padding for the player's name.
	if (localpnamelen >= 15)
		playername[0] = 0;
	else
	{
		//pad the left side to compensate for the player name prefix the server will add in the final svc_print
		memset(playername, ' ', 15-localpnamelen);
		playername[15-localpnamelen] = 0;
	}

	for (i = 0, j = 0, hadspace=false; rawenginebuild[i]&& j+1<countof(enginebuild); i++)
	{
		if (rawenginebuild[i] == ' ')
		{
			if (!strncmp(rawenginebuild+i, " SVN ", 5))
			{
				i+=5-1;
				continue;
			}
			if (hadspace && !(isdigit(rawenginebuild[i-1])&&isdigit(rawenginebuild[i+1])))
				continue;
			hadspace=true;
			enginebuild[j++] = '_';
		}
		else
			enginebuild[j++] = rawenginebuild[i];
	}
	enginebuild[j++] = 0;

	//get the current server address
	NET_AdrToString(servername, sizeof(servername), &cls.netchan.remote_address);

	//get the ruleset names
	ruleset = Ruleset_GetRulesetName();
	if (!ruleset)
		ruleset = "default";

	extras[0][0] = '-';	//extra restrictions.
	extras[1][0] = '+';	//extra cheats.
	extras[0][1] = extras[1][1] = 0;
#ifdef HAVE_LEGACY
	Q_strncatz(extras[Validation_IsModified()], "m", sizeof(extras[0]));
#endif
	Q_strncatz(extras[1/*!!allow_scripts.ival*/], "s", sizeof(extras[0]));		//we can't track whether a +foo was from a bind or an alias invoked from a bind, so we have to list it under the +, for now.
	Q_strncatz(extras[0/*!!enemyforceskins.ival*/], "f", sizeof(extras[0]));	//we don't support per-player skin forcing, so this should always be under the -.
	Q_strncatz(extras[!!cl_iDrive.ival], "i", sizeof(extras[0]));
	if (!extras[0][1])
		*extras[0] = 0;
	if (!extras[1][1])
		*extras[1] = 0;

	//now send it
	CL_SendClientCommand(true, "say \"%s%21s " "%16s %s" /*FIXME*/"_dbg" "%s%s\"", playername, servername, enginebuild, ruleset, extras[1], extras[0]);
}

void Ruleset_Check(char *keyval, char *out, size_t outsize)
{
	size_t l;
	char *status;
	char *b64;
	ruleset_t *rs;

	if (strchr(keyval, '^'))	//don't let people corrupt scoreboards...
		keyval = "?";

	b64 = strchr(keyval, ':');
	if (!b64 || b64==keyval)
	{	//can't validate it. don't bother showing it.
		rs = Ruleset_Find(keyval);
		if (!rs || rs->external)
		{
			status = S_COLOR_RED;
		}
		else
		{
			//valid internal name... no hash so we can't validate it, but the only way someone can generate this is if they mod the engine, so its as valid as its going to get, so consider it okay.
			status = S_COLOR_GREEN;
			keyval = rs->rulesetname;
		}
	}
	else
	{
		char digest[20];
		*b64++ = 0;
		Base64_DecodeBlock(b64, NULL, digest, sizeof(digest));

		for (rs = ruleset_list; rs; rs = rs->next)
			if (rs->external && !memcmp(rs->hash, digest, sizeof(digest)))
				break;

		if (rs)
		{
			status = S_COLOR_GREEN;
			keyval = rs->rulesetname;	//use our local name for it
		}
		else status = S_COLOR_RED;	//they changed their ruleset file, or they're trying to spoof an internal name...
	}

	if (!*keyval)
		keyval = "default";

	if (*keyval)
	{
		l = strlen(status);
		if (outsize > l)
		{
			memcpy(out, status, l);
			out += l;
			outsize -= l;
		}
		l = strlen(keyval);
		if (outsize > l)
		{
			memcpy(out, keyval, l);
			out += l;
			outsize -= l;
		}
	}
	*out = 0;
}

static int Ruleset_CheckRuleConditionIsOkay(char *cond)	//-1 on error
{
	if (cond)
	{
		enum ct_e {CT_EQUAL, CT_UNEQUAL, CT_GEQUAL, CT_LESS, CT_LEQUAL, CT_GREATER, CT_INVALID1, CT_INVALID2} checktype;
		char *key, *value, c;
		qboolean not = (*cond=='!');
		int truth = 1;
		if (not)
			cond++;

		key = cond;
		while (*cond=='$'||isalnum(*cond))
			cond++;
		c = *cond;
		if (c == 0)
		{
			checktype = CT_UNEQUAL;
			value = "";
		}
		else
		{
			*cond = 0;
			value = cond+1;
			if (c == '>')
			{
				if (cond[1] == '=')
					value++, checktype = CT_GEQUAL;
				else
					checktype = CT_GREATER;
			}
			else if (c == '<')
			{
				if (cond[1] == '=')
					value++, checktype = CT_LEQUAL;
				else
					checktype = CT_LESS;
			}
			else if (c == '=')
			{
				if (cond[1] == '=')
					value++;	//allow = or ==
				checktype = CT_EQUAL;
			}
			else if (c == '!')
			{
				if (cond[1] == '=')
					value++, checktype = CT_EQUAL;
				else
					key="", value="", checktype = CT_INVALID1;
			}
			else
			{
				Con_Printf ("Unknown comparison type\n");
				key="", value="", checktype = CT_INVALID1;
			}
		}
		if (not)
			checktype ^= 1;

#ifndef SVNREVISION
#define SVNREVISION -
#endif
#define SVNREVISIONSTR STRINGIFY(SVNREVISION)
//		if (!strcmp(key, "$GAME"))	//FIXME: need to reload rulesets on gamedir changes for that, which we can't reliably do.
//			key = FS_GetGamedir(true);
		if (!strcmp(key, "$FTEQW"))
			key = SVNREVISIONSTR;	//"-" is smaller than 0, but not empty. yay for private builds.
		if (!strcmp(key, "$QW"))
			key = "2.40";	//we're originally derived from the 2.40 source release.
		if (*key == '$')
			key = "";	//don't know what this is. some engine-specific key for another engine? treat it as false.
		switch(checktype)
		{
		case CT_EQUAL:	truth =  ! strcmp(key, value);	break;
		case CT_UNEQUAL:truth = !! strcmp(key, value);	break;
		case CT_GEQUAL:	truth = 0<=strcmp(key, value);	break;
		case CT_GREATER:truth = 0< strcmp(key, value);	break;
		case CT_LEQUAL:	truth = 0>=strcmp(key, value);	break;
		case CT_LESS:	truth = 0> strcmp(key, value);	break;
		case CT_INVALID1:
		case CT_INVALID2:	truth = -1;
		}
		*cond = c;
		return truth;
	}
	return true;
}

void Validation_Apply_Ruleset(void)
{	//rulesets are applied when the client first gets a connection to the server
	char b64[64];
	ruleset_t *rs = NULL;
	int i;
	char *rulesetname = ruleset.string;
	cvar_t **vars, *var;
	unsigned int latches = 0;
	int okay;

	if (!*rulesetname)
		rulesetname = "default";
	rs = Ruleset_Find(rulesetname);

	if (ruleset_current == rs)
		return;	//ruleset is already applied. no work needed (don't disconnect!).

	//the worker can poke the current ruleset for file hash checks. make sure none are active.
	COM_WorkerFullSync();

	Validation_DelatchRulesets();
	ruleset_current = NULL;
	InfoBuf_SetStarKey(&cls.userinfo[0], RULESET_USERINFO, "");

	if (!rs)
	{
		if (strcmp(rulesetname, "default") && strcmp(rulesetname, "none"))
		{
			Con_Printf("Cannot apply ruleset \"%s\" - not recognised\n", rulesetname);
			if (ruleset_list)
			{
				Con_Printf("Known rulesets:\n");
				for (rs = ruleset_list; rs; rs = rs->next)
					Con_Printf("\t%s\n", rs->rulesetname);
			}
		}

		Con_DPrintf("Ruleset set to %s\n", "default");
		if (cls.state && !cls.demoplayback)
		{	//changing a ruleset while on-server MUST disconnect(+reconnect) you, to make it obvious you just tried to cheat (wiping any scores).
			//note that this can often happen on initial connection (gamedir changes execing configs from different gamedirs).
			Con_Printf("Reconnecting to enforce change to ruleset \"%s\"\n", rulesetname);

#ifdef HAVE_SERVER
			if (sv.state)
				Cbuf_AddText("disconnect;map_restart\n", RESTRICT_LOCAL);
			else
#endif
				Cbuf_AddText("disconnect;reconnect\n", RESTRICT_LOCAL);
		}
		return;
	}

	ruleset_current = NULL;
	vars = Z_Malloc(sizeof(*vars)*rs->rules);
	for (i = 0; i < rs->rules; i++)
	{	//make sure we're actually allowed to make these changes.
		vars[i] = NULL;

		okay = Ruleset_CheckRuleConditionIsOkay(rs->rule[i].rulecond);
		if (okay < 0)
			break;	//error!
		if (!okay)
			continue;	//condition is false, this rule does not apply to us.

		if (!strcmp(rs->rule[i].rulename, "ruleset_unsupported"))
		{	//special pseudo-setting to cause the ruleset to fail entirely, with a reason for failure (eg 'Engine version too old').
			if (*rs->rule[i].rulevalue)
				Con_Printf("Ruleset %s is unsupported: %s\n", rs->rulesetname, rs->rule[i].rulevalue);
			else
				Con_Printf("Ruleset %s is unsupported\n", rs->rulesetname);
			break;
		}

		var = Cvar_FindVar(rs->rule[i].rulename);
		if (!var)
		{
			if (!rs->rule[i].rulecond)
				continue;	//doesn't exist... assume it was for some other engine
			//they gave a condition. it doesn't exist... wtf.
			Con_Printf("Ruleset %s requires cvar %s\n", rs->rulesetname, rs->rule[i].rulename);
			break;
		}
		//FIXME: should cvars need to be engine-defined? it would break mods...
		//       oh well, default.cfg should have set/seta commands for any mod cvars to make sure they're set in advance.
		if (var->flags & CVAR_NOSET)
		{
			Con_Printf("Ruleset %s requires change to read-only cvar %s\n", rs->rulesetname, var->name);
			break;
		}

		if ((var->flags & CVAR_NOTFROMSERVER) && Cmd_IsInsecure())
		{
			Con_Printf ("Server tried setting %s cvar\n", var->name);
			break;
		}

		vars[i] = var;
		if (strcmp(var->string, rs->rule[i].rulevalue))
			latches |= var->flags;
	}

	if (i == rs->rules)
	{
		//lock down the cvars.
		for (i = 0; i < rs->rules; i++)
		{
			cvar_t *var = vars[i];
			if (!var)	//for some other engine
				continue;

			if (!Cvar_ApplyLatchFlag(var, rs->rule[i].rulevalue, CVAR_RULESETLATCH,
					CVAR_VIDEOLATCH|CVAR_RENDERERLATCH|	//ignore these, we'll vid_restart as required anyway.
					CVAR_SERVEROVERRIDE|CVAR_MAPLATCH|	//we're going to reconnect/restart anyway.
					CVAR_CHEAT|CVAR_SEMICHEAT))			//ignore these too,
			{
				Con_Printf("Failed to apply ruleset %s due to cvar %s\n", rs->rulesetname, var->name);
				break;
			}
		}

		ruleset_current = rs;
		Base64_EncodeBlock(rs->hash, sizeof(rs->hash), b64, sizeof(b64));
		if (rs->external)	//include a hash, so it can be validated
			InfoBuf_SetStarKey(&cls.userinfo[0], RULESET_USERINFO, va("%s:%s", rulesetname, b64));
		else	//internals don't bother including a hash, to reduce issues with old servers/clients...
			InfoBuf_SetStarKey(&cls.userinfo[0], RULESET_USERINFO, rulesetname);

		Con_DPrintf("Ruleset set to %s\n", rs->rulesetname);

		//force video restart, if required.
		if (latches & CVAR_VIDEOLATCH)
			Cbuf_AddText("vid_restart\n", RESTRICT_LOCAL);
		else if (latches & CVAR_RENDERERLATCH)
			Cbuf_AddText("vid_reload\n", RESTRICT_LOCAL);
		else
			Cbuf_AddText("flush\n", RESTRICT_LOCAL);	//make sure file hashes take effect.

		if ((cls.state && !cls.demoplayback) || (latches&CVAR_MAPLATCH))
		{	//changing a ruleset while on-server MUST disconnect(+reconnect) you, to make it obvious you just tried to cheat (wiping any scores).
			//note that this can often happen on initial connection (gamedir changes execing configs from different gamedirs).
			Con_Printf("Reconnecting to enforce change to ruleset \"%s\"\n", rulesetname);

#ifdef HAVE_SERVER
			if (sv.state)
				Cbuf_AddText("disconnect;map_restart\n", RESTRICT_LOCAL);
			else
#endif
				Cbuf_AddText("disconnect;reconnect\n", RESTRICT_LOCAL);
		}
	}
	Z_Free(vars);
}

//////////////////////

void Validation_Auto_Response(int playernum, char *s)
{
	static float versionresponsetime;
#ifdef HAVE_LEGACY
	static float modifiedresponsetime;
	static float fakeshaftresponsetime;
#endif
	static float skinsresponsetime;
	static float serverresponsetime;
	static float rulesetresponsetime;
	static float systemresponsetime;
	static float cmdlineresponsetime;
	static float scriptsresponsetime;

	if (cls.demoplayback)
		return;	//noone gives a shit about qtv spectator versions that can't even play without reconnecting.

	//quakeworld tends to use f_*
	//netquake uses the slightly more guessable q_* form
	if (!strncmp(s, "f_", 2))
		s+=2;
	else if (!strncmp(s, "q_", 2))
		s+=2;
	else
		return;

	if (!strncmp(s, "version", 7) && versionresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_Version();
		versionresponsetime = Sys_DoubleTime() + 5;
	}
	else if (cl.playerview[0].spectator)
		return;
	else if (!strncmp(s, "server", 6) && serverresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_Server();
		serverresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "system", 6) && systemresponsetime < Sys_DoubleTime())
	{
		Validation_System();
		systemresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "cmdline", 7) && cmdlineresponsetime < Sys_DoubleTime())
	{
		Validation_CmdLine();
		cmdlineresponsetime = Sys_DoubleTime() + 5;
	}
#ifdef HAVE_LEGACY
	else if (!strncmp(s, "fakeshaft", 9) && fakeshaftresponsetime < Sys_DoubleTime())
	{
		Validation_FakeShaft();
		fakeshaftresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "modified", 8) && modifiedresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_FilesModified();
		modifiedresponsetime = Sys_DoubleTime() + 5;
	}
#endif
	else if (!strncmp(s, "scripts", 7) && scriptsresponsetime < Sys_DoubleTime())
	{
		Validation_Scripts();
		scriptsresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "skins", 5) && skinsresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_Skins();
		skinsresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "ruleset", 7) && rulesetresponsetime < Sys_DoubleTime())
	{
		if (1)
			Validation_AllChecks();
		else
			Validation_OldRuleset();
		rulesetresponsetime = Sys_DoubleTime() + 5;
	}
}


