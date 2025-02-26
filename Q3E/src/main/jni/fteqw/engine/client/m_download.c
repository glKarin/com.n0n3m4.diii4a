//copyright 'Spike', license gplv2+
//provides both a package manager and downloads menu.
//FIXME: block downloads of exe/dll/so/etc if not an https url (even if inside zips). also block such files from package lists over http.

//Note: for a while we didn't have strong hashes, nor signing, so we depended upon known-self-signed tls certificates to prove authenticity
//we now have sha256 hashes(and sizes) to ensure that the file we wanted hasn't been changed in transit.
//and we have signature info, to prove that the hash specified was released by a known authority. This means that we should now be able to download such things over http without worries (or at least that we can use an untrustworthy CA that's trusted by insecurity-mafia browsers).
//WARNING: paks/pk3s may still be installed without signatures, without allowing dlls/exes/etc to be installed.
//signaturedata+hashes can be generated with 'fteqw -privkey key.priv -pubkey key.pub -certhost MyAuthority -sign pathtofile', but Auth_GetKnownCertificate will need to be updated for any allowed authorities.

#include "quakedef.h"
#include "shader.h"
#include "netinc.h"

#define ENABLEPLUGINSBYDEFAULT	//auto-enable plugins that the user installs. this risks other programs installing dlls (but we can't really protect our listing file so this is probably not any worse in practise).

#ifdef PACKAGEMANAGER
	#if !defined(NOBUILTINMENUS) && defined(HAVE_CLIENT)
		#define DOWNLOADMENU
	#endif
#endif

#ifdef PACKAGEMANAGER
#include "fs.h"

//some extra args for the downloads menu (for the downloads menu to handle engine updates).
#ifndef SVNREVISION
#define SVNREVISION -
#endif
static char enginerevision[256] = STRINGIFY(SVNREVISION);



#ifdef ENABLEPLUGINSBYDEFAULT
cvar_t	pkg_autoupdate = CVARFD("pkg_autoupdate", "1", CVAR_NOTFROMSERVER|CVAR_NOSAVE|CVAR_NOSET|CVAR_NORESET, "Controls autoupdates, can only be changed via the downloads menu.\n0: off.\n1: enabled (stable only).\n2: enabled (unstable).\nNote that autoupdate will still prompt the user to actually apply the changes."); //read from the package list only.
#else
cvar_t	pkg_autoupdate = CVARFD("pkg_autoupdate", "-1", CVAR_NOTFROMSERVER|CVAR_NOSAVE|CVAR_NOSET|CVAR_NORESET, "Controls autoupdates, can only be changed via the downloads menu.\n0: off.\n1: enabled (stable only).\n2: enabled (unstable).\nNote that autoupdate will still prompt the user to actually apply the changes."); //read from the package list only.
#endif

#define INSTALLEDFILES	"installed.lst"	//the file that resides in the quakedir (saying what's installed).

//installed native okay [previously manually installed, or has no a qhash]
//installed cached okay [had a qhash]
//installed native corrupt [they overwrote it manually]
//installed cached corrupt [we fucked up, probably]
//installed native missing (becomes not installed) [deleted]
//installed cached missing (becomes not installed) [deleted]
//installed none [meta package with no files]

//!installed native okay [was manually installed, flag as installed now]
//!installed cached okay [they got it from some other source / previously installed]
//!installed native corrupt [manually installed conflict]
//!installed cached corrupt [we fucked up, probably]

//!installed * missing [simply not installed]

#define DPF_ENABLED					(1u<<0)
#define DPF_NATIVE					(1u<<1)	//appears to be installed properly
#define DPF_CACHED					(1u<<2)	//appears to be installed in their dlcache dir (and has a qhash)
#define DPF_CORRUPT					(1u<<3)	//will be deleted before it can be changed

#define DPF_USERMARKED				(1u<<4)	//user selected it
#define DPF_AUTOMARKED				(1u<<5)	//selected only to satisfy a dependancy
#define DPF_MANIMARKED				(1u<<6)	//legacy. selected to satisfy packages listed directly in manifests. the filesystem code will load the packages itself, we just need to download (but not enable).
#define DPF_DISPLAYVERSION			(1u<<7)	//some sort of conflict, the package is listed twice, so show versions so the user knows what's old.

#define DPF_FORGETONUNINSTALL		(1u<<8)	//for previously installed packages, remove them from the list if there's no current version any more (should really be automatic if there's no known mirrors)
#define DPF_HIDDEN					(1u<<9)	//wrong arch, file conflicts, etc. still listed if actually installed.
#define DPF_PURGE					(1u<<10)	//package should be completely removed (ie: the dlcache dir too). if its still marked then it should be reinstalled anew. available on cached or corrupt packages, implied by native.
#define DPF_MANIFEST				(1u<<11)	//package was named by the manifest, and should only be uninstalled after a warning.

#define DPF_TESTING					(1u<<12)	//package is provided on a testing/trial basis, and will only be selected/listed if autoupdates are configured to allow it.
#define DPF_GUESSED					(1u<<13)	//package data was guessed from basically just filename+qhash+url. merge aggressively.
#define DPF_ENGINE					(1u<<14)	//engine update. replaces old autoupdate mechanism
#define DPF_PLUGIN					(1u<<15)	//this is a plugin package, with a dll

#define DPF_TRUSTED					(1u<<16)	//package was trusted when installed. any pk3s can be flagged appropriately.
#define DPF_SIGNATUREREJECTED		(1u<<17)	//signature is bad
#define DPF_SIGNATUREACCEPTED		(1u<<18)	//signature is good (required for dll/so/exe files)
#define DPF_SIGNATUREUNKNOWN		(1u<<19)	//signature is unknown
#define DPF_HIDEUNLESSPRESENT		(1u<<20)	//hidden in the ui, unless present.
#define DPF_PENDING					(1u<<21)	//started a download (required due to threading sillyness).

#define DPF_MARKED					(DPF_USERMARKED|DPF_AUTOMARKED)	//flags that will enable it
#define DPF_ALLMARKED				(DPF_USERMARKED|DPF_AUTOMARKED|DPF_MANIMARKED)	//flags that will download it without necessarily enabling it.
#define DPF_PRESENT					(DPF_NATIVE|DPF_CACHED)
#define DPF_DISABLEDINSTALLED		(DPF_ENGINE|DPF_PLUGIN)	//engines+plugins can be installed without being enabled.
//pak.lst
//priories <0
//pakX
//manifest packages
//priority 0-999
//*.pak
//priority >=1000
#define PM_DEFAULTPRIORITY		1000

void CL_StartCinematicOrMenu(void);

#if defined(SERVERONLY)
#	define ENGINE_RENDERER "sv"
#elif defined(GLQUAKE) && (defined(VKQUAKE) || defined(D3DQUAKE) || defined(SWQUAKE))
#	define ENGINE_RENDERER "m"
#elif defined(GLQUAKE)
#	define ENGINE_RENDERER "gl"
#elif defined(VKQUAKE)
#	define ENGINE_RENDERER "vk"
#elif defined(D3DQUAKE)
#	define ENGINE_RENDERER "d3d"
#else
#	define ENGINE_RENDERER "none"
#endif
#if defined(NOCOMPAT)
#	define ENGINE_CLIENT "-nc"
#elif defined(MINIMAL)
#	define ENGINE_CLIENT "-min"
#elif defined(CLIENTONLY)
#	define ENGINE_CLIENT "-cl"
#else
#	define ENGINE_CLIENT
#endif

#define THISARCH PLATFORM "_" ARCH_CPU_POSTFIX
#define THISENGINE THISARCH "-" DISTRIBUTION "-" ENGINE_RENDERER ENGINE_CLIENT

#define MAXMIRRORS 8
typedef struct package_s {
	char *name;
	char *category;	//in path form

	struct package_s *alternative;	//alternative (hidden) forms of this package.

	char *mirror[MAXMIRRORS];	//FIXME: move to two types of dep...
	char gamedir[16];
	enum fs_relative fsroot;	//FS_BINARYPATH or FS_ROOT _ONLY_
	char version[24];
	char *arch;
	char *qhash;
	char *packprefix;	//extra weirdness to skip embedded gamedirs or force extra maps/ nesting

	quint64_t filesize;	//in bytes, as part of verifying the hash.
	char *filesha1;		//this is the hash of the _download_, not the individual files
	char *filesha512;	//this is the hash of the _download_, not the individual files
	char *signature;	//signature of the [prefix:]sha512

	char *title;
	char *description;
	char *license;
	char *author;
	char *website;
	char *previewimage;
	enum
	{
		EXTRACT_COPY,	//just copy the download over
		EXTRACT_XZ,		//give the download code a write filter so that it automatically decompresses on the fly
		EXTRACT_GZ,		//give the download code a write filter so that it automatically decompresses on the fly
		EXTRACT_EXPLICITZIP,//extract an explicit file list once it completes. kinda sucky.
		EXTRACT_ZIP,	//extract stuff once it completes. kinda sucky.
	} extract;

	struct packagedep_s
	{
		struct packagedep_s *next;
		enum
		{
			DEP_CONFLICT,		//don't install if we have the named package installed.
			DEP_REPLACE,		//obsoletes the specified package (or just acts as a conflict marker for now).
			DEP_FILECONFLICT,	//don't install if this file already exists.
			DEP_REQUIRE,		//don't install unless we have the named package installed.
			DEP_RECOMMEND,		//like depend, but uninstalling will not bubble.
			DEP_SUGGEST,		//like recommend, but will not force install (ie: only prevents auto-uninstall)
			DEP_NEEDFEATURE,	//requires a specific feature to be available (typically controlled via a cvar)
//			DEP_MIRROR,
//			DEP_FAILEDMIRROR,
			DEP_MAP,			//This package contains this map. woo.

			DEP_SOURCE,			//which source url we found this package from
			DEP_EXTRACTNAME,	//a file that will be installed
			DEP_FILE,			//a file that will be installed. will be loaded as a package, where appropriate.
			DEP_CACHEFILE		//an installed file that's relative to the downloads/ subdir
		} dtype;
		char name[1];
	} *deps;

#ifdef WEBCLIENT
	struct dl_download *curdownload;
	unsigned int trymirrors;
#endif

	int flags;
	int priority;
	struct package_s **link;
	struct package_s *next;
} package_t;

static qboolean loadedinstalled;
static package_t *availablepackages;
static int numpackages;
static char *declinedpackages;	//metapackage named by the manicfest.

#ifdef WEBCLIENT
static struct
{
	char *package;	//package to load. don't forget its dependancies too.
	char *map;		//the map to load.
} pm_onload;

static int allowphonehome = -1;	//if autoupdates are disabled, make sure we get (temporary) permission before phoning home for available updates. (-1=unknown, 0=no, 1=yes)
static int doautoupdate;	//updates will be marked (but not applied without the user's actions)
static int pm_pendingprompts; //number of prompts that are pending. don't show apply prompts until these are all cleared.
static qboolean pkg_updating;	//when flagged, further changes are blocked until completion.
#else
static const qboolean pkg_updating = false;
#endif
static qboolean pm_packagesinstalled;

//FIXME: these are allocated for the life of the exe. changing basedir should purge the list.
static size_t pm_numsources = 0;
static struct pm_source_s
{
	char *url;					//url to query. unique.
	char *prefix;				//category prefix for packages from this source.
	enum
	{
		SRCSTAT_UNTRIED,		//not tried to connect at all.
		SRCSTAT_FAILED_DNS,		//tried but failed, unresolvable.
		SRCSTAT_FAILED_NORESP,	//tried but failed, no response.
		SRCSTAT_FAILED_REFUSED,	//tried but failed, refused (no precess).
		SRCSTAT_FAILED_EOF,		//tried but failed, abrupt termination.
		SRCSTAT_FAILED_MITM,	//tried but failed. misc cert problems.
		SRCSTAT_FAILED_HTTP,	//tried but failed, misc http failure.
		SRCSTAT_PENDING,		//waiting for response (or queued). don't show package list yet.
		SRCSTAT_OBTAINED,		//we got a response.
	} status;
	#define SRCFL_HISTORIC	(1u<<0)	//aka hidden. replaced by the others... used for its enablement. must be parsed first so its enabled-state wins.
	#define SRCFL_NESTED	(1u<<1)	//discovered from a different source. always disabled.
	#define SRCFL_MANIFEST	(1u<<2)	//not saved. often default to enabled.
	#define SRCFL_USER		(1u<<3)	//user explicitly added it. included into installed.lst. enabled (if trusted).
	#define SRCFL_PLUGIN	(1u<<4)	//user explicitly added it. included into installed.lst. enabled (if trusted).
	#define SRCFLMASK_FROM	(SRCFL_HISTORIC|SRCFL_NESTED|SRCFL_MANIFEST|SRCFL_USER|SRCFL_PLUGIN) //mask of flags, forming priority for replacements.
	#define SRCFL_DISABLED	(1u<<5)	//source was explicitly disabled.
	#define SRCFL_ENABLED	(1u<<6)	//source was explicitly enabled.
	#define SRCFL_PROMPTED	(1u<<7)	//source was explicitly enabled.
	#define SRCFL_ONCE		(1u<<8)	//autoupdates are disabled, but the user is viewing packages anyway. enabled via a prompt.
	#define SRCFL_UNSAFE	(1u<<9)	//ignore signing requirements.
	unsigned int flags;
	struct dl_download *curdl;	//the download context

	void *module;	//plugins
	plugupdatesourcefuncs_t *funcs;
} *pm_source/*[pm_maxsources]*/;
static int pm_sequence;	//bumped any time any package is purged

static void PM_WriteInstalledPackages(void);
static void PM_PreparePackageList(void);
static void PM_UpdatePackageList(qboolean autoupdate);
static void PM_PromptApplyChanges(void);
qboolean PM_AreSourcesNew(qboolean doprompt); //prompts to enable sources, or just queries(to do the flashing thing)
#ifdef DOWNLOADMENU
static qboolean PM_DeclinedPackages(char *out, size_t outsize);
#endif
#ifdef WEBCLIENT
static qboolean PM_SignatureOkay(package_t *p);
#endif

static void PM_FreePackage(package_t *p)
{
	struct packagedep_s *d;
#ifdef WEBCLIENT
	int i;
#endif

	COM_AssertMainThread("PM_FreePackage");

	if (p->link)
	{
		if (p->alternative)
		{	//replace it with its alternative package
			*p->link = p->alternative;
			p->alternative->alternative = p->alternative->next;
			if (p->alternative->alternative)
				p->alternative->alternative->link = &p->alternative->alternative;
			p->alternative->next = p->next;
			p->alternative->link = p->link;
		}
		else
		{	//just remove it from the list.
			*p->link = p->next;
			if (p->next)
				p->next->link = p->link;
		}
	}

#ifdef WEBCLIENT
	if (p->curdownload)
	{	//if its currently downloading, cancel it.
		DL_Close(p->curdownload);
		p->curdownload = NULL;
	}

	for (i = 0; i < countof(p->mirror); i++)
		Z_Free(p->mirror[i]);
#endif

	//free its data.
	while(p->deps)
	{
		d = p->deps;
		p->deps = d->next;
		Z_Free(d);
	}

	Z_Free(p->name);
	Z_Free(p->category);
	Z_Free(p->title);
	Z_Free(p->description);
	Z_Free(p->author);
	Z_Free(p->website);
	Z_Free(p->license);
	Z_Free(p->previewimage);
	Z_Free(p->qhash);
	Z_Free(p->arch);
	Z_Free(p->packprefix);
	Z_Free(p->filesha1);
	Z_Free(p->filesha512);
	Z_Free(p->signature);
	Z_Free(p);
}

static void PM_AddDep(package_t *p, int deptype, const char *depname)
{
	struct packagedep_s *nd, **link;

	//no dupes.
	for (link = &p->deps; (nd=*link) ; link = &nd->next)
	{
		if (nd->dtype == deptype && !strcmp(nd->name, depname))
			return;
	}

	//add it on the end, preserving order.
	nd = Z_Malloc(sizeof(*nd) + strlen(depname));
	nd->dtype = deptype;
	strcpy(nd->name, depname);
	nd->next = *link;
	*link = nd;
}
static const char *PM_GetDepSingle(package_t *p, int deptype)
{
	struct packagedep_s *d;
	const char *val = NULL;

	//no dupes.
	for (d = p->deps; d ; d = d->next)
	{
		if (d->dtype == deptype)
		{
			if (val)
				return NULL;	//found a second. give up in confusion.
			else
				val = d->name;	//found the first, but continue to make sure there's no second.
		}
	}
	return val;
}
static qboolean PM_HasDep(package_t *p, int deptype, const char *depname)
{
	struct packagedep_s *d;

	//no dupes.
	for (d = p->deps; d ; d = d->next)
	{
		if (d->dtype == deptype && (!depname || !strcmp(d->name, depname)))
			return true;
	}
	return false;
}

static qboolean PM_PurgeOnDisable(package_t *p)
{
	//corrupt packages must be purged
	if (p->flags & DPF_CORRUPT)
		return true;
	//certain updates can be present and not enabled
	if (p->flags & DPF_DISABLEDINSTALLED)
		return false;
	//hashed packages can also be present and not enabled, but only if they're in the cache and not native
	if (*p->gamedir && (p->flags & DPF_CACHED))
		return false;
	//all other packages must be deleted to disable them
	return true;
}

static void PM_ValidateAuthenticity(package_t *p, enum hashvalidation_e validated)
{
	qbyte hashdata[512+MAX_QPATH];
	size_t hashsize = 0;
	qbyte signdata[1024];
	size_t signsize = 0;
	int r, i;
	char authority[MAX_QPATH], *sig;

	const qbyte *pubkey;
	size_t pubkeysize;

#ifndef _DEBUG
#pragma message("Temporary code.")
	//this is temporary code and should be removed once everything else has been fixed.
	//ignore the signature (flag as accepted) for any packages with all mirrors on our own update site.
	//we can get away with this because we enforce a known certificate for the download.
	if (!COM_CheckParm("-notlstrust") && p->mirror[0])
	{
		conchar_t musite[256], *e;
		char site[256];
		char *oldprefix = "http://fte.";
		char *newprefix = "https://updates.";
		int m;
		e = COM_ParseFunString(CON_WHITEMASK, ENGINEWEBSITE, musite, sizeof(musite), false);
		COM_DeFunString(musite, e, site, sizeof(site)-1, true, true);
		if (!strncmp(site, oldprefix, strlen(oldprefix)))
		{
			memmove(site+strlen(newprefix), site+strlen(oldprefix), strlen(site)-strlen(oldprefix)+1);
			memcpy(site, newprefix, strlen(newprefix));
		}
		Q_strncatz(site, "/", sizeof(site));
		for (m = 0; m < countof(p->mirror); m++)
		{
			if (p->mirror[m] && strncmp(p->mirror[m], site, strlen(site)))
				break;	//some other host
		}
		if (m == countof(p->mirror))
		{
			p->flags |= DPF_SIGNATUREACCEPTED;
			return;
		}
	}
#endif

	*authority = 0;
	if (!p->signature)
		r = VH_AUTHORITY_UNKNOWN;
	else if (!p->filesha512)
		r = VH_INCORRECT;
	else
	{
		sig = strchr(p->signature, ':');
		if (sig && sig-p->signature<countof(authority)-1)
		{
			memcpy(authority, p->signature, sig-p->signature);
			authority[sig-p->signature] = 0;
			sig++;
		}
		else
		{
			strcpy(authority, "Spike");	//legacy bollocks
			sig = p->signature;
		}

		//to validate it, we need a single blob that composits all the parts that might need to be validated. mostly just prefix and file hash.
		hashsize = Base16_DecodeBlock(p->filesha512, hashdata, sizeof(hashdata));

		if (p->packprefix && *p->packprefix)
		{	//need to a bit of extra hashing when we have extra data, to get it to fit.
			hashfunc_t *h = &hash_sha2_512;
			void *ctx = alloca(h->contextsize);
			h->init(ctx);
			h->process(ctx, p->packprefix, strlen(p->packprefix));
			h->process(ctx, "\0", 1);
			h->process(ctx, hashdata, hashsize);
			h->terminate(hashdata, ctx);
			hashsize = h->digestsize;
		}

		//and the proof...
		signsize = Base64_DecodeBlock(sig, NULL, signdata, sizeof(signdata));
		r = VH_UNSUPPORTED;//preliminary
	}

	pubkey = Auth_GetKnownCertificate(authority, &pubkeysize);
	if (!pubkey)
		r = VH_AUTHORITY_UNKNOWN;

	//try and get one of our providers to verify it...
	for (i = 0; r==VH_UNSUPPORTED && i < cryptolib_count; i++)
	{
		if (cryptolib[i] && cryptolib[i]->VerifyHash)
			r = cryptolib[i]->VerifyHash(hashdata, hashsize, pubkey, pubkeysize, signdata, signsize);
	}

	p->flags &= ~(DPF_SIGNATUREACCEPTED|DPF_SIGNATUREREJECTED|DPF_SIGNATUREUNKNOWN);
	if (r == VH_CORRECT)
		p->flags |= DPF_SIGNATUREACCEPTED;
	else if (r == VH_INCORRECT)
	{
		Con_Printf("Signature verification failed\n");
		p->flags |= DPF_SIGNATUREREJECTED;
	}
	else if (validated == VH_CORRECT && p->filesize && (p->filesha1||p->filesha512))
		p->flags |= DPF_SIGNATUREACCEPTED;	//parent validation was okay, expand that to individual packages too.
	else if (p->signature)
		p->flags |= DPF_SIGNATUREUNKNOWN;
}

static qboolean PM_TryGenCachedName(const char *pname, package_t *p, char *local, int llen)
{
	if (!*p->gamedir)
		return false;
	if (!p->qhash)
	{	//we'll throw paks+pk3s into the cache dir even without a qhash
		const char *ext = COM_GetFileExtension(pname, NULL);
		if (strcmp(ext, ".pak") && strcmp(ext, ".pk3"))
			return false;
	}
	return FS_GenCachedPakName(pname, p->qhash, local, llen);
}

//checks the status of each package
static void PM_ValidatePackage(package_t *p)
{
	package_t *o;
	struct packagedep_s *dep;
	vfsfile_t *pf;
	char temp[MAX_OSPATH];
	p->flags &=~ (DPF_NATIVE|DPF_CACHED|DPF_CORRUPT|DPF_HIDEUNLESSPRESENT);
	if (p->flags & DPF_ENABLED)
	{
		for (dep = p->deps; dep; dep = dep->next)
		{
			char *n;
			if (dep->dtype == DEP_CACHEFILE)
			{
//				p->flags |= DPF_HIDEUNLESSPRESENT;
				n = va("downloads/%s", dep->name);
				pf = FS_OpenVFS(n, "rb", p->fsroot);
				if (pf)
				{
					VFS_CLOSE(pf);
					p->flags |= DPF_CACHED;
				}
			}
			else if (dep->dtype == DEP_FILE)
			{
				if (*p->gamedir)
					n = va("%s/%s", p->gamedir, dep->name);
				else
					n = dep->name;
				pf = FS_OpenVFS(n, "rb", p->fsroot);
				if (pf)
				{
					VFS_CLOSE(pf);
					p->flags |= DPF_NATIVE;
				}
				else if (PM_TryGenCachedName(n, p, temp, sizeof(temp)))
				{
					pf = FS_OpenVFS(temp, "rb", p->fsroot);
					if (pf)
					{
						VFS_CLOSE(pf);
						p->flags |= DPF_CACHED;
					}
				}
			}
			else
				continue;
			if (!(p->flags & (DPF_NATIVE|DPF_CACHED)))
				Con_Printf("WARNING: (%i) %s (%s) no longer exists\n", dep->dtype, p->name, n);
		}

		//if no files were present, unmark it.
		if (!(p->flags & (DPF_NATIVE|DPF_CACHED)))
			p->flags &= ~DPF_ENABLED;
	}
	else
	{
		for (dep = p->deps; dep; dep = dep->next)
		{
			char *n;
			struct packagedep_s *odep;
			unsigned int fl = DPF_NATIVE;
			if (dep->dtype == DEP_FILE)
			{
				if (*p->gamedir)
					n = va("%s/%s", p->gamedir, dep->name);
				else
					n = dep->name;
				pf = FS_OpenVFS(n, "rb", p->fsroot);
				if (!pf && PM_TryGenCachedName(n, p, temp, sizeof(temp)))
				{
					pf = FS_OpenVFS(temp, "rb", p->fsroot);
					fl = DPF_CACHED;
					//fixme: skip any archive checks
				}
			}
			else if (dep->dtype == DEP_CACHEFILE)
			{
//				p->flags |= DPF_HIDEUNLESSPRESENT;
				fl = DPF_CACHED;
				n = va("downloads/%s", dep->name);
				pf = FS_OpenVFS(n, "rb", p->fsroot);
			}
			else
				continue;

			if (pf)
			{
				for (o = availablepackages; o; o = o->next)
				{
					if (o == p)
						continue;
					if (o->flags & DPF_ENABLED)
					{
						if (!strcmp(p->gamedir, o->gamedir) && p->fsroot == o->fsroot)
							if (strcmp(p->name, o->name) || strcmp(p->version, o->version))
							{
								for (odep = o->deps; odep; odep = odep->next)
								{
									if (!strcmp(dep->name, odep->name))
										break;
								}
								if (odep)
									break;
							}
					}
				}
				if ((o && o->qhash && p->qhash && (o->flags & DPF_CACHED)) || fl == DPF_CACHED)
					p->flags |= DPF_CACHED;
				else if (!o)
				{
					if (!PM_PurgeOnDisable(p) || !strcmp(p->gamedir, "downloads"))
					{
						p->flags |= fl;
						VFS_CLOSE(pf);
					}
					else if (p->qhash)
					{
						searchpathfuncs_t *archive = FS_OpenPackByExtension(pf, NULL, n, n, p->packprefix);

						if (archive)
						{
							unsigned int fqhash;
							pf = NULL;
							fqhash = archive->GeneratePureCRC(archive, NULL);
							archive->ClosePath(archive);

							if (fqhash == (unsigned int)strtoul(p->qhash, NULL, 0))
							{
								p->flags |= fl;
								if (fl&DPF_NATIVE)
									p->flags |= DPF_MARKED|DPF_ENABLED;
								break;
							}
							else
								pf = NULL;
						}
						else
							VFS_CLOSE(pf);
					}
					else
					{
						p->flags |= DPF_CORRUPT|fl;
						VFS_CLOSE(pf);
					}
					break;
				}
				VFS_CLOSE(pf);
			}
		}
	}
}

static qboolean PM_MergePackage(package_t *oldp, package_t *newp)
{
	//we don't track mirrors for previously-installed packages.
	//use the file list of the installed package, zips ignore the file list of the remote package but otherwise they must match to be mergeable
	//local installed copies of the package may lack some information, like mirrors.
	//the old package *might* be installed, the new won't be. this means we need to use the old's file list rather than the new
	if (!oldp->qhash || !strcmp(oldp->qhash?oldp->qhash:"", newp->qhash?newp->qhash:""))
	{
		unsigned int om, nm;
		struct packagedep_s *od, *nd;
		qboolean ignorefiles;
		for (om = 0; om < countof(oldp->mirror) && oldp->mirror[om]; om++)
			;
		for (nm = 0; nm < countof(newp->mirror) && newp->mirror[nm]; nm++)
			;
//		if (oldp->priority != newp->priority)
//			return false;

		ignorefiles = (oldp->extract==EXTRACT_ZIP);	//zips ignore the remote file list, its only important if its already installed (so just keep the old file list and its fine).
		if (oldp->extract != newp->extract)
		{	//if both have mirrors of different types then we have some sort of conflict
			if (ignorefiles || (om && nm))
				return false;
		}

		//packages are merged according to how it should look when its actually installed.
		//this means we might be merging two packages from different sources with different installation methods/files/etc...
		if ((newp->signature && oldp->signature && strcmp(newp->signature, oldp->signature)) ||
			(newp->filesha512 && oldp->filesha512 && strcmp(newp->filesha512, oldp->filesha512)) ||
			(newp->filesha1 && oldp->filesha1 && strcmp(newp->filesha1, oldp->filesha1)))
			return false;

		for (od = oldp->deps, nd = newp->deps; od && nd; )
		{
			//if its a zip then the 'remote' file list will be blank while the local list is not (we can just keep the local list).
			//if the file list DOES change, then bump the version.
			if ((od->dtype == DEP_FILE && ignorefiles) || od->dtype == DEP_SOURCE)
			{
				od = od->next;
				continue;
			}
			if ((nd->dtype == DEP_FILE && ignorefiles) || nd->dtype == DEP_SOURCE)
			{
				nd = nd->next;
				continue;
			}

			if (od->dtype != nd->dtype)
				return false;	//deps don't match
			if (strcmp(od->name, nd->name))
				return false;
			od = od->next;
			nd = nd->next;
		}

		//overwrite these. use the 'new' / remote values for each of them
		//the versions of the two packages will be the same, so the texts should be the same. still favour the new one so that things can be corrected serverside without needing people to redownload everything.
		if (newp->qhash){Z_Free(oldp->qhash); oldp->qhash = Z_StrDup(newp->qhash);}
		if (newp->description){Z_Free(oldp->description); oldp->description = Z_StrDup(newp->description);}
		if (newp->license){Z_Free(oldp->license); oldp->license = Z_StrDup(newp->license);}
		if (newp->author){Z_Free(oldp->author); oldp->author = Z_StrDup(newp->author);}
		if (newp->website){Z_Free(oldp->website); oldp->website = Z_StrDup(newp->website);}
		if (newp->previewimage){Z_Free(oldp->previewimage); oldp->previewimage = Z_StrDup(newp->previewimage);}

		//use the new package's auth settings. this affects downloading rather than reactivation.
		if (newp->signature || newp->filesha1 || newp->filesha512)
		{
			Z_Free(oldp->signature); oldp->signature = newp->signature?Z_StrDup(newp->signature):NULL;
			Z_Free(oldp->filesha1); oldp->filesha1 = newp->filesha1?Z_StrDup(newp->filesha1):NULL;
			Z_Free(oldp->filesha512); oldp->filesha512 = newp->filesha512?Z_StrDup(newp->filesha512):NULL;
			oldp->filesize = newp->filesize;
			oldp->flags &=            ~(DPF_SIGNATUREACCEPTED|DPF_SIGNATUREREJECTED|DPF_SIGNATUREUNKNOWN);
			oldp->flags |= newp->flags&(DPF_SIGNATUREACCEPTED|DPF_SIGNATUREREJECTED|DPF_SIGNATUREUNKNOWN);
		}
		else
			oldp->flags &= ~DPF_SIGNATUREACCEPTED;

		oldp->priority = newp->priority;

		if (nm)
		{	//copy over the mirrors
			oldp->extract = newp->extract;
			for (; nm --> 0 && om < countof(oldp->mirror); )
			{
				//skip it if this mirror was already known.
				unsigned int u;
				for (u = 0; u < om; u++)
				{
					if (!strcmp(oldp->mirror[u], newp->mirror[nm]))
						break;
				}
				if (u < om)
					continue;

				//new mirror! copy it over
				oldp->mirror[om++] = newp->mirror[nm];
				newp->mirror[nm] = NULL;
			}
		}
		//these flags should only remain set if set in both.
		oldp->flags &= ~(DPF_FORGETONUNINSTALL|DPF_TESTING|DPF_MANIFEST|DPF_GUESSED) | (newp->flags & (DPF_FORGETONUNINSTALL|DPF_TESTING|DPF_MANIFEST|DPF_GUESSED));

		for (nd = newp->deps; nd ; nd = nd->next)
		{
			if (nd->dtype == DEP_SOURCE)
			{
				if (!PM_HasDep(oldp, DEP_SOURCE, nd->name))
					PM_AddDep(oldp, DEP_SOURCE, nd->name);
			}
		}

		PM_FreePackage(newp);
		return true;
	}
	return false;
}

static int QDECL PM_PackageSortOrdering(const void *l, const void *r)
{	//for qsort.
	const package_t *a=*(package_t*const*)l, *b=*(package_t*const*)r;
	const char *ac, *bc;
	int order;

	//sort by categories
	ac = a->category?a->category:"";
	bc = b->category?b->category:"";
	order = Q_strcasecmp(ac,bc);
	if (order)
		return order;

	//otherwise sort by title.
	ac = a->title?a->title:a->name;
	bc = b->title?b->title:b->name;
	order = Q_strcasecmp(ac,bc);
	return order;
}
static void PM_ResortPackages(void)
{
	int i, count;
	package_t **sorted;
	package_t *p;
	for (count = 0, p = availablepackages; p; p=p->next)
		count++;
	if (!count)
		return;
	sorted = Z_Malloc(sizeof(*sorted)*count);
	for (count = 0, p = availablepackages; p; p=p->next)
		sorted[count++] = p;
	qsort(sorted, count, sizeof(*sorted), PM_PackageSortOrdering);
	availablepackages = NULL;
	for (i = count; i --> 0; )
	{
		sorted[i]->next = availablepackages;
		sorted[i]->link = &availablepackages;

		if (availablepackages)
			availablepackages->link = &sorted[i]->next;
		availablepackages = sorted[i];
	}
	Z_Free(sorted);
}

static package_t *PM_InsertPackage(package_t *p)
{
	package_t **link;
	int v;

	for (link = &availablepackages; *link; )
	{
		package_t *prev = *link;
		if (((prev->flags|p->flags) & DPF_GUESSED) && prev->extract == p->extract && !strcmp(prev->gamedir, p->gamedir) && prev->fsroot == p->fsroot && !strcmp(prev->qhash?prev->qhash:"", p->qhash?p->qhash:""))
		{	//if one of the packages was guessed then match according to the qhash and file names.
			struct packagedep_s	*a = prev->deps, *b = p->deps;
			qboolean differs = false;
			for (;;)
			{
				while (a && a->dtype != DEP_FILE)
					a = a->next;
				while (b && b->dtype != DEP_FILE)
					b = b->next;
				if (!a && !b)
					break;
				if (a && b && !strcmp(a->name, b->name))
				{
					a = a->next;
					b = b->next;
					continue;	//matches...
				}
				differs = true;
				break;
			}
			if (differs)
			{
				link = &(*link)->next;
				continue;
			}
			else
			{
				if (((p->flags & DPF_GUESSED) && (prev->flags & (DPF_USERMARKED|DPF_MANIMARKED|DPF_PENDING))) || prev->curdownload)
				{	//the new one was also guessed. just return the existing package instead.
					prev->flags |= p->flags&(DPF_PRESENT|DPF_ALLMARKED|DPF_ENABLED);
					PM_FreePackage(p);
					return prev;
				}
				PM_FreePackage(prev);
				link = &availablepackages;
				continue;
			}
		}
		v = PM_PackageSortOrdering(&prev, &p);
		if (v > 0)
			break;	//insert before this one
		else if (v == 0)
		{	//name matches.
			//if (!strcmp(p->fullname),prev->fullname)
			if (!strcmp(p->version, prev->version) && !strcmp(prev->qhash?prev->qhash:"", p->qhash?p->qhash:""))
			if (!strcmp(p->gamedir, prev->gamedir))
			if (!strcmp(p->arch?p->arch:"", prev->arch?prev->arch:""))
			{ /*package matches, merge them somehow, don't add*/
				package_t *a;
				if (PM_MergePackage(prev, p))
					return prev;
				for (a = p->alternative; a; a = a->next)
				{
					if (PM_MergePackage(a, p))
						return prev;
				}
				p->next = prev->alternative;
				prev->alternative = p;
				p->link = &prev->alternative;
				if (p->next)
					p->next->link = &p->next;
				return p;
			}

			//something major differs, display both independantly.
			p->flags |= DPF_DISPLAYVERSION;
			prev->flags |= DPF_DISPLAYVERSION;

			//there might be more such dupes.
		}

		link = &(*link)->next;
	}
	PM_ValidatePackage(p);

	if (p->flags & DPF_MANIFEST)
		if (!(p->flags & (DPF_PRESENT|DPF_CACHED)))
		{	//if a manifest wants it then there's only any point listing it if there's an actual mirror listed. otherwise its a hint to the filesystem for ordering and not something that's actually present.
			int i;
			for (i = 0; i < countof(p->mirror); i++)
				if (p->mirror[i])
					break;
			if (i == countof(p->mirror))
			{
				PM_FreePackage(p);
				return NULL;
			}
		}

	p->next = *link;
	p->link = link;
	*link = p;
	numpackages++;

	return p;
}

static qboolean PM_CheckFeature(const char *feature, const char **featurename, const char **concommand)
{
#ifdef HAVE_CLIENT
	extern cvar_t r_replacemodels;
#endif
	*featurename = NULL;
	*concommand = NULL;
#ifdef HAVE_CLIENT
	//check for compressed texture formats, to warn when not supported.
	if (!strcmp(feature, "bc1") || !strcmp(feature, "bc2") || !strcmp(feature, "bc3") || !strcmp(feature, "s3tc"))
		return *featurename="S3 Texture Compression", sh_config.hw_bc>=1;
	if (!strcmp(feature, "bc4") || !strcmp(feature, "bc5") || !strcmp(feature, "rgtc"))
		return *featurename="Red/Green Texture Compression", sh_config.hw_bc>=2;
	if (!strcmp(feature, "bc6") || !strcmp(feature, "bc7") || !strcmp(feature, "bptc"))
		return *featurename="Block Partitioned Texture Compression", sh_config.hw_bc>=3;
	if (!strcmp(feature, "etc1"))
		return *featurename="Ericson Texture Compression, Original", sh_config.hw_etc>=1;
	if (!strcmp(feature, "etc2") || !strcmp(feature, "eac"))
		return *featurename="Ericson Texture Compression, Revision 2", sh_config.hw_etc>=2;
	if (!strcmp(feature, "astcldr") || !strcmp(feature, "astc"))
		return *featurename="Adaptive Scalable Texture Compression (LDR)", sh_config.hw_astc>=1;
	if (!strcmp(feature, "astchdr"))
		return *featurename="Adaptive Scalable Texture Compression (HDR)", sh_config.hw_astc>=2;

	if (!strcmp(feature, "24bit"))
		return *featurename="24bit Textures", *concommand="seta gl_load24bit 1\n", gl_load24bit.ival;
	if (!strcmp(feature, "md3"))
		return *featurename="Replacement Models", *concommand="seta r_replacemodels md3 md2 md5mesh\n", !!strstr(r_replacemodels.string, "md3");
	if (!strcmp(feature, "rtlights"))
		return *featurename="Realtime Dynamic Lights", *concommand="seta r_shadow_realtime_dlight 1\n", r_shadow_realtime_dlight.ival||r_shadow_realtime_world.ival;
	if (!strcmp(feature, "rtworld"))
		return *featurename="Realtime World Lights", *concommand="seta r_shadow_realtime_dlight 1\nseta r_shadow_realtime_world 1\n", r_shadow_realtime_world.ival;
#endif

	return false;
}
static qboolean PM_CheckPackageFeatures(package_t *p)
{
	struct packagedep_s *dep;
	const char *featname, *enablecmd;

	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_NEEDFEATURE)
		{
			if (!PM_CheckFeature(dep->name, &featname, &enablecmd))
				return false;
		}
	}
	return true;
}

static qboolean PM_CheckFile(const char *filename, enum fs_relative base)
{
	vfsfile_t *f = FS_OpenVFS(filename, "rb", base);
	if (f)
	{
		VFS_CLOSE(f);
		return true;
	}
	return false;
}

static void PM_Plugin_Source_CacheFinished(void *ctx, vfsfile_t *f);
static void PM_AddSubListModule(void *module, plugupdatesourcefuncs_t *funcs, const char *url, const char *prefix, unsigned int flags)
{
	size_t i;
	if (!prefix)
		prefix = "";
	if (!*url)
		return;
	if (strchr(url, '\"') || strchr(url, '\n'))
		return;
	if (strchr(prefix, '\"') || strchr(prefix, '\n'))
		return;

	for (i = 0; i < pm_numsources; i++)
	{
		if (!strcmp(pm_source[i].url, url))
		{
			unsigned int newpri = flags&SRCFLMASK_FROM;
			unsigned int oldpri = pm_source[i].flags&SRCFLMASK_FROM;
			if (module)
			{
				pm_source[i].module = module;
				pm_source[i].funcs = funcs;
			}
			if (newpri > oldpri)
			{	//replacing an historic package should stomp on most of it, retaining only its enablement status.
				pm_source[i].flags &= ~SRCFLMASK_FROM;
				pm_source[i].flags |= flags&(SRCFLMASK_FROM);

				Z_Free(pm_source[i].prefix);
				pm_source[i].prefix = Z_StrDup(prefix);
			}
			pm_source[i].flags &= ~SRCFL_HISTORIC;
			pm_source[i].flags |= flags & SRCFL_UNSAFE;
			if ((flags & SRCFL_USER) && (flags & (SRCFL_ENABLED|SRCFL_DISABLED)))
			{
				pm_source[i].flags &= ~SRCFL_ENABLED|SRCFL_DISABLED;
				pm_source[i].flags |= (SRCFL_ENABLED|SRCFL_DISABLED) & flags;
			}
			break;
		}
	}
	if (i == pm_numsources)
	{
		Z_ReallocElements((void*)&pm_source, &pm_numsources, i+1, sizeof(*pm_source));

		pm_source[i].module = module;
		pm_source[i].funcs = funcs;
		pm_source[i].status = SRCSTAT_UNTRIED;
		pm_source[i].flags = flags;

		pm_source[i].url = BZ_Malloc(strlen(url)+1);
		strcpy(pm_source[i].url, url);

		pm_source[i].prefix = BZ_Malloc(strlen(prefix)+1);
		strcpy(pm_source[i].prefix, prefix);

		pm_sequence++;
	}

	if (pm_source[i].funcs && (pm_source[i].status == SRCSTAT_UNTRIED) && (pm_source[i].flags&SRCFL_ENABLED))	//cache only!
		pm_source[i].funcs->Update(pm_source[i].url, VFS_OpenPipeCallback(PM_Plugin_Source_CacheFinished, &pm_source[i]), true);
}
static void PM_AddSubList(const char *url, const char *prefix, unsigned int flags)
{
	PM_AddSubListModule(NULL, NULL, url, prefix, flags);
}
#ifdef WEBCLIENT
static void PM_RemSubList(const char *url)
{
	int i;
	for (i = 0; i < pm_numsources; )
	{
		if (!strcmp(pm_source[i].url, url))
		{
			if (pm_source[i].curdl)
			{
				DL_Close(pm_source[i].curdl);
				pm_source[i].curdl = NULL;
			}
			//don't actually remove it, that'd mess up the indexes which could break stuff like PM_ListDownloaded callbacks. :(
			pm_source[i].flags = SRCFL_HISTORIC;	//forget enablement state etc. we won't bother writing it.
			pm_sequence++;	//make sure any menus hide it.

			Con_Printf("Source %s removed\n", url);
			return;
		}
		else
			i++;
	}

	Con_Printf("Source %s not known, cannot remove\n", url);
}
#endif

struct packagesourceinfo_s
{
	unsigned int parseflags;	//0 for a downloadable ones, or DPF_FORGETONUNINSTALL|DPF_ENABLED for the installed package list.
	const char *url;
	const char *categoryprefix;

	enum hashvalidation_e validated;

	int version;
	char gamedir[64];	//when not overridden...
	char mirror[MAXMIRRORS][MAX_OSPATH];
	int nummirrors;
};
static const char *PM_ParsePackage(struct packagesourceinfo_s *source, const char *tokstart, package_t **outpackage, int wantvariation, const char *defaultpkgname)
{
	package_t *p;
	struct packagedep_s *dep;
	qboolean isauto = false;
	const char *start = tokstart;
	int variation = 0;	//variation we're currently parsing (or skipping...).
	qboolean invariation = false;


	{
		char pathname[256];
		char *fullname = defaultpkgname?Z_StrDup(defaultpkgname):NULL;
		char *file = NULL;
		char *url = NULL;
		char *category = NULL;
		unsigned int flags = source->parseflags;

		if (source->version > 2)	//v3 has an 'installed' token to say its enabled. v2 has a 'stale' token to say its old, which was weird and to try to avoid conflicts.
			flags &= ~DPF_ENABLED;

		p = Z_Malloc(sizeof(*p));
		p->extract = EXTRACT_COPY;
		p->priority = PM_DEFAULTPRIORITY;
		p->fsroot = FS_ROOT;
		Q_strncpyz(p->gamedir, source->gamedir, sizeof(p->gamedir));
		while (tokstart)
		{
			char *eq;
			char key[8192];
			char val[8192];

			//skip leading whitespace
			while (*tokstart>0 && *tokstart <= ' ')
				tokstart++;

			if (source->version >= 3)
			{
				tokstart = COM_ParseCString (tokstart, key, sizeof(key), NULL);
				if (!strcmp(key, "}"))
				{
					if (invariation)
					{
						invariation = false;
						variation++;
						continue;
					}
					else
						break;	//end of package.
				}
				else if (!strcmp(key, "{"))
				{
					if (invariation)	//some sort of corruption? stop here.
						break;
					invariation = true;
					continue;
				}
				tokstart = COM_ParseCString (tokstart, val, sizeof(val), NULL);

				if (invariation && variation != wantvariation)
					continue;	//we're parsing one of the other variations. ignore this.
			}
			else
			{
				//the following are [\]["]key=["]value["] parameters, which is definitely messy, yes.
				*val = 0;
				if (*tokstart == '\\' || *tokstart == '\"')
				{	//legacy quoting
					tokstart = COM_StringParse (tokstart, key, sizeof(key), false, false);
					eq = strchr(key, '=');
					if (eq)
					{
						*eq = 0;
						Q_strncpyz(val, eq+1, sizeof(val));
					}
				}
				else
				{
					tokstart = COM_ParseTokenOut(tokstart, "=", key, sizeof(key), NULL);
					if (!*key)
						continue;
					if (tokstart && *tokstart == '=')
					{
						tokstart++;
						if (!(*tokstart >= 0 && *tokstart <= ' '))
							tokstart = COM_ParseCString(tokstart, val, sizeof(val), NULL);
					}
				}
			}

			if (!strcmp(key, "package"))
				Z_StrDupPtr(&fullname, val);
			else if (!strcmp(key, "url"))
				Z_StrDupPtr(&url, val);
			else if (!strcmp(key, "category"))
				Z_StrDupPtr(&category, val);
			else if (!strcmp(key, "title"))
				Z_StrDupPtr(&p->title, val);
			else if (!strcmp(key, "gamedir"))
				Q_strncpyz(p->gamedir, val, sizeof(p->gamedir));
			else if (!strcmp(key, "ver") || !strcmp(key, "v"))
				Q_strncpyz(p->version, val, sizeof(p->version));
			else if (!strcmp(key, "arch"))
				Z_StrDupPtr(&p->arch, val);
			else if (!strcmp(key, "priority"))
				p->priority = atoi(val);
			else if (!strcmp(key, "qhash"))
				Z_StrDupPtr(&p->qhash, val);
			else if (!strcmp(key, "packprefix"))
				Z_StrDupPtr(&p->packprefix, val);
			else if (!strcmp(key, "desc") || !strcmp(key, "description"))
			{
				if (p->description)
					Z_StrCat(&p->description, "\n");
				Z_StrCat(&p->description, val);
			}
			else if (!strcmp(key, "license"))
				Z_StrDupPtr(&p->license, val);
			else if (!strcmp(key, "author"))
				Z_StrDupPtr(&p->author, val);
			else if (!strcmp(key, "preview"))
			{
				if (source->nummirrors && !(!strncmp(val, "http://", 7) || !strncmp(val, "https://", 8)))
				{
					Z_Free(p->previewimage);
					p->previewimage = Z_StrDupf("%s%s", source->mirror[0], val);
				}
				else
					Z_StrDupPtr(&p->previewimage, val);
			}
			else if (!strcmp(key, "website"))
				Z_StrDupPtr(&p->website, val);
			else if (!strcmp(key, "unzipfile"))
			{	//filename extracted from zip.
				p->extract = EXTRACT_EXPLICITZIP;
				PM_AddDep(p, DEP_EXTRACTNAME, val);
			}
			else if (!strcmp(key, "file"))
			{	//installed file
				if (!file)
					Z_StrDupPtr(&file, val);
				PM_AddDep(p, DEP_FILE, val);
			}
			else if (!strcmp(key, "cachefile"))
			{	//installed file
				if (!file)
					Z_StrDupPtr(&file, val);
				PM_AddDep(p, DEP_CACHEFILE, val);
			}
			else if (!strcmp(key, "extract"))
			{
				if (!strcmp(val, "xz"))
					p->extract = EXTRACT_XZ;
				else if (!strcmp(val, "gz"))
					p->extract = EXTRACT_GZ;
				else if (!strcmp(val, "zip"))
					p->extract = EXTRACT_ZIP;
				else if (!strcmp(val, "zip_explicit"))
					p->extract = EXTRACT_EXPLICITZIP;
				else
					Con_Printf("Unknown decompression method: %s\n", val);
			}
			else if (!strcmp(key, "map"))
				PM_AddDep(p, DEP_MAP, val);
			else if (!strcmp(key, "depend") || !strcmp(key, "depends"))
				PM_AddDep(p, DEP_REQUIRE, val);
			else if (!strcmp(key, "conflict"))
				PM_AddDep(p, DEP_CONFLICT, val);
			else if (!strcmp(key, "replace"))
				PM_AddDep(p, DEP_REPLACE, val);
			else if (!strcmp(key, "fileconflict"))
				PM_AddDep(p, DEP_FILECONFLICT, val);
			else if (!strcmp(key, "recommend"))
				PM_AddDep(p, DEP_RECOMMEND, val);
			else if (!strcmp(key, "suggest"))
				PM_AddDep(p, DEP_SUGGEST, val);
			else if (!strcmp(key, "need"))
				PM_AddDep(p, DEP_NEEDFEATURE, val);
			else if (!strcmp(key, "test"))
				flags |= DPF_TESTING;
			else if (!strcmp(key, "guessed"))
				flags |= DPF_GUESSED;
			else if (!strcmp(key, "trusted") && (source->parseflags&DPF_ENABLED))
				flags |= DPF_TRUSTED;
			else if (!strcmp(key, "stale") && source->version==2)
				flags &= ~DPF_ENABLED;	//known about, (probably) cached, but not actually enabled.
			else if (!strcmp(key, "enabled") && source->version>2)
				flags |= source->parseflags & DPF_ENABLED;
			else if (!strcmp(key, "auto"))
				isauto = true;	//autoinstalled and NOT user-installed
			else if (!strcmp(key, "root") && (source->parseflags&DPF_ENABLED))
			{
				if (!Q_strcasecmp(val, "bin"))
					p->fsroot = FS_BINARYPATH;
				else if (!Q_strcasecmp(val, "lib"))
					p->fsroot = FS_LIBRARYPATH;
				else
					p->fsroot = FS_ROOT;
			}
			else if (!strcmp(key, "dlsize"))
				p->filesize = strtoull(val, NULL, 0);
			else if (!strcmp(key, "sha1"))
				Z_StrDupPtr(&p->filesha1, val);
			else if (!strcmp(key, "sha512"))
				Z_StrDupPtr(&p->filesha512, val);
			else if (!strcmp(key, "sign"))
				Z_StrDupPtr(&p->signature, val);
			else
			{
				Con_DPrintf("Unknown package property \"%s\"\n", key);
			}
		}

		if (!fullname)
			fullname = Z_StrDup("UNKNOWN");
//		Con_Printf("%s - %s\n", source->url, fullname);

		if (category)
		{
			p->name = fullname;

			if (*source->categoryprefix)
				Q_snprintfz(pathname, sizeof(pathname), "%s/%s", source->categoryprefix, category);
			else
				Q_snprintfz(pathname, sizeof(pathname), "%s", category);
			if (*pathname)
			{
				if (pathname[strlen(pathname)-1] != '/')
					Q_strncatz(pathname, "/", sizeof(pathname));
			}
			p->category = Z_StrDup(pathname);
		}
		else
		{
			if (*source->categoryprefix)
				Q_snprintfz(pathname, sizeof(pathname), "%s/%s", source->categoryprefix, fullname);
			else
				Q_snprintfz(pathname, sizeof(pathname), "%s", fullname);
			Z_Free(fullname);
			p->name = Z_StrDup(COM_SkipPath(pathname));
			*COM_SkipPath(pathname) = 0;
			p->category = Z_StrDup(pathname);
		}

		if (!p->title)
			p->title = Z_StrDup(p->name);

		p->flags = flags;

		if (url && (!strncmp(url, "http://", 7) || !strncmp(url, "https://", 8)))
		{
			p->mirror[0] = Z_StrDup(url);

			if (!file && p->extract != EXTRACT_ZIP)
			{
				FS_PathURLCache(url, pathname, sizeof(pathname));
				PM_AddDep(p, DEP_CACHEFILE, pathname+10);
			}
		}
		else
		{
			int m;
			char *ext = "";
			char *relurl = url;
			if (!relurl)
			{
				if (p->extract == EXTRACT_XZ)
					ext = ".xz";
				else if (p->extract == EXTRACT_GZ)
					ext = ".gz";
				else if (p->extract == EXTRACT_ZIP || p->extract == EXTRACT_EXPLICITZIP)
					ext = ".zip";
				relurl = file;
			}
			if (relurl)
			{
				for (m = 0; m < source->nummirrors; m++)
					p->mirror[m] = Z_StrDupf("%s%s%s", source->mirror[m], relurl, ext);

				if (!file && p->extract != EXTRACT_ZIP && source->nummirrors)
				{
					FS_PathURLCache(p->mirror[0], pathname, sizeof(pathname));
					PM_AddDep(p, DEP_CACHEFILE, pathname+10);
				}
			}
		}

		PM_ValidateAuthenticity(p, source->validated);

		Z_Free(file);
		Z_Free(url);
		Z_Free(category);
	}
	if (p->arch)
	{
		if (!Q_strcasecmp(p->arch, THISENGINE))
		{
			if (!Sys_EngineMayUpdate())
				p->flags |= DPF_HIDDEN;
			else
				p->flags |= DPF_ENGINE;
		}
		else if (!Q_strcasecmp(p->arch, THISARCH))
		{
			if ((p->fsroot == FS_ROOT || p->fsroot == FS_BINARYPATH || p->fsroot == FS_LIBRARYPATH) && !*p->gamedir && p->priority == PM_DEFAULTPRIORITY)
				p->flags |= DPF_PLUGIN;
		}
		else
			p->flags |= DPF_HIDDEN;	//other engine builds or other cpus are all hidden
	}
	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_FILECONFLICT)
		{
			const char *n;
			if (*p->gamedir)
				n = va("%s/%s", p->gamedir, dep->name);
			else
				n = dep->name;
			if (PM_CheckFile(n, p->fsroot))
				p->flags |= DPF_HIDDEN;
		}
	}
	if (p->flags & DPF_ENABLED)
	{
		if (isauto)
			p->flags |= DPF_AUTOMARKED;	//FIXME: we don't know if this was manual or auto
		else
			p->flags |= DPF_USERMARKED;	//FIXME: we don't know if this was manual or auto
	}

	if (source->url)
		PM_AddDep(p, DEP_SOURCE, source->url);

	if (outpackage)
		*outpackage = p;
	else
	{
		PM_InsertPackage(p);

		if (wantvariation == 0)	//only the first!
		{
			while (++wantvariation < variation)
				if (tokstart != PM_ParsePackage(source, start, NULL, wantvariation, defaultpkgname))
				{
					Con_Printf(CON_ERROR"%s: Unable to parse package variation...\n", source->url);
					break;	//erk?
				}
		}
	}

	return tokstart;
}

static qboolean PM_ParsePackageList(const char *f, unsigned int parseflags, const char *url, const char *prefix)
{
	char line[65536];
	size_t l;
	char *sl;

	struct packagesourceinfo_s source = {parseflags, url, prefix};
	char *tokstart;
	qboolean forcewrite = false;

	const char *filestart = f, *linestart;

	if (!f)
		return forcewrite;

	source.validated = (parseflags & (DPF_ENABLED|DPF_SIGNATUREACCEPTED))?VH_CORRECT/*FIXME*/:VH_UNSUPPORTED;
	Q_strncpyz(source.gamedir, FS_GetGamedir(false), sizeof(source.gamedir));

	if (url)
	{
		Q_strncpyz(source.mirror[source.nummirrors], url, sizeof(source.mirror[source.nummirrors]));
		sl = COM_SkipPath(source.mirror[source.nummirrors]);
		*sl = 0;
		source.nummirrors++;
	}

	do
	{
		for (l = 0;*f;)
		{
			if (*f == '\r' || *f == '\n')
			{
				if (f[0] == '\r' && f[1] == '\n')
					f++;
				f++;
				break;
			}
			if (l < sizeof(line)-1)
				line[l++] = *f;
			else
				line[0] = 0;
			f++;
		}
		line[l] = 0;

		Cmd_TokenizeString (line, false, false);
	} while (!Cmd_Argc() && *f);

	if (strcmp(Cmd_Argv(0), "version"))
		return forcewrite;	//it's not the right format.

	source.version = atoi(Cmd_Argv(1));
	if (
#ifdef HAVE_LEGACY
		source.version != 0 && source.version != 1 &&
#endif
		source.version != 2 && source.version != 3)
	{
		Con_Printf("Packagelist is of a future or incompatible version\n");
		return forcewrite;	//it's not the right version.
	}

	pm_sequence++;

	while(*f)
	{
		linestart = f;
		for (l = 0;*f;)
		{
			if (*f == '\r' || *f == '\n')
			{
				if (f[0] == '\r' && f[1] == '\n')
					f++;
				f++;
				break;
			}
			if (l < sizeof(line)-1)
				line[l++] = *f;
			else
				line[0] = 0;
			f++;
		}
		line[l] = 0;

		tokstart = COM_StringParse (line, com_token, sizeof(com_token), false, false);
		if (*com_token)
		{
			if (!strcmp(com_token, "signature"))
			{	//FIXME: local file should hash /etc/machine-id with something, to avoid file-dropping dll-loading hacks.
				//signature "authority" "signdata"
#if 0
				(void)filestart;
				(void)linestart;
#else
				if (source.validated == VH_UNSUPPORTED)
				{	//only allow one valid signature line...
					char authority[MAX_OSPATH];
					char signdata[MAX_OSPATH];
					char signature_base64[MAX_OSPATH];
					qbyte *pubkey;
					size_t pubkeysize;
					size_t signsize;
					enum hashvalidation_e r;
					int i;
					hashfunc_t *hf = &hash_sha2_512;
					void *hashdata = Z_Malloc(hf->digestsize);
					void *hashctx = Z_Malloc(hf->contextsize);
					tokstart = COM_StringParse (tokstart, authority, sizeof(authority), false, false);
					tokstart = COM_StringParse (tokstart, signature_base64, sizeof(signature_base64), false, false);

					signsize = Base64_DecodeBlock(signature_base64, NULL, signdata, sizeof(signdata));
					hf->init(hashctx);
					hf->process(hashctx, filestart, linestart-filestart);	//hash the text leading up to the signature line
					hf->process(hashctx, f, strlen(f));						//and hash the data after it. so the only bit not hashed is the signature itself.
					hf->terminate(hashdata, hashctx);
					Z_Free(hashctx);
					r = VH_UNSUPPORTED;//preliminary

					pubkey = Auth_GetKnownCertificate(authority, &pubkeysize);
					if (!pubkey)
					{
						r = VH_AUTHORITY_UNKNOWN;
						Con_Printf(CON_ERROR"%s: Unknown signature authority %s\n", url, authority);
					}

					//try and get one of our providers to verify it...
					for (i = 0; r==VH_UNSUPPORTED && i < cryptolib_count; i++)
					{
						if (cryptolib[i] && cryptolib[i]->VerifyHash)
							r = cryptolib[i]->VerifyHash(hashdata, hf->digestsize, pubkey, pubkeysize, signdata, signsize);
					}

					Z_Free(hashdata);
					source.validated = r;
				}
#endif
				continue;
			}

			if (!strcmp(com_token, "sublist"))
			{
				char *subprefix;
				char url[MAX_OSPATH];
				char enablement[MAX_OSPATH];
				tokstart = COM_StringParse (tokstart, url, sizeof(url), false, false);
				tokstart = COM_StringParse (tokstart, com_token, sizeof(com_token), false, false);
				if (*prefix)
					subprefix = va("%s/%s", prefix, com_token);
				else
					subprefix = com_token;

				if (parseflags & DPF_ENABLED)
				{	//local file. a user-defined source that was previously registered (but may have been disabled)
					tokstart = COM_StringParse (tokstart, enablement, sizeof(enablement), false, false);
					if (!Q_strcasecmp(enablement, "enabled"))
						PM_AddSubList(url, subprefix, SRCFL_USER|SRCFL_ENABLED);
					else
						PM_AddSubList(url, subprefix, SRCFL_USER|SRCFL_DISABLED);
				}
				else	//a nested source. will need to inherit enablement the long way.
					PM_AddSubList(url, subprefix, SRCFL_NESTED);	//nested sources should be disabled by default.
				continue;
			}
			if (!strcmp(com_token, "source"))
			{	//`source URL ENABLED|DISABLED` -- valid ONLY in an installed.lst file
				char url[MAX_OSPATH];
				char enablement[MAX_OSPATH];
				tokstart = COM_StringParse (tokstart, url, sizeof(url), false, false);
				tokstart = COM_StringParse (tokstart, enablement, sizeof(enablement), false, false);
				if (parseflags & DPF_ENABLED)
				{
					if (!Q_strcasecmp(enablement, "enabled"))
						PM_AddSubList(url, NULL, SRCFL_HISTORIC|SRCFL_ENABLED);
					else
						PM_AddSubList(url, NULL, SRCFL_HISTORIC|SRCFL_DISABLED);
				}
				//else ignore possible exploits with sources trying to force-enable themselves.

				continue;
			}
			if (!strcmp(com_token, "set"))
			{
				tokstart = COM_StringParse (tokstart, com_token, sizeof(com_token), false, false);
				if (!strcmp(com_token, "gamedir"))
				{
					tokstart = COM_StringParse (tokstart, com_token, sizeof(com_token), false, false);
					if (!*com_token)
						Q_strncpyz(source.gamedir, FS_GetGamedir(false), sizeof(source.gamedir));
					else
						Q_strncpyz(source.gamedir, com_token, sizeof(source.gamedir));
				}
				else if (!strcmp(com_token, "mirrors"))
				{
					source.nummirrors = 0;
					while (source.nummirrors < countof(source.mirror) && tokstart)
					{
						tokstart = COM_StringParse (tokstart, com_token, sizeof(com_token), false, false);
						if (*com_token)
						{
							Q_strncpyz(source.mirror[source.nummirrors], com_token, sizeof(source.mirror[source.nummirrors]));
							source.nummirrors++;
						}
					}
				}
				else if (!strcmp(com_token, "updatemode"))
				{
					tokstart = COM_StringParse (tokstart, com_token, sizeof(com_token), false, false);
					if (parseflags & DPF_ENABLED)	//don't use a downloaded file's version of this, only use the local version of it.
						Cvar_ForceSet(&pkg_autoupdate, com_token);
				}
				else if (!strcmp(com_token, "declined"))
				{
					if (parseflags & DPF_ENABLED)	//don't use a downloaded file's version of this, only use the local version of it.
					{
						tokstart = COM_StringParse (tokstart, com_token, sizeof(com_token), false, false);
						Z_Free(declinedpackages);
						if (*com_token)
							declinedpackages = Z_StrDup(com_token);
						else
							declinedpackages = NULL;
					}
				}
				else
				{
					//erk
					Con_Printf("%s: unrecognised command - set %s\n", source.url, com_token);
				}
				continue;
			}

			if (!strcmp(com_token, "{"))
			{
				linestart = COM_StringParse (linestart, com_token, sizeof(com_token), false, false);
				f = PM_ParsePackage(&source, linestart, NULL, 0, NULL);
				if (!f)
					break;	//erk!
			}
			else if (source.version < 3)
			{	//old single-line gibberish
				PM_ParsePackage(&source, tokstart, NULL, -1, com_token);
			}
			else
			{
				Con_Printf("%s: unrecognised command - %s\n", source.url, com_token);
			}
		}
	}

	return forcewrite;
}

static qboolean PM_NameIsInStrings(const char *strings, const char *match)
{
	char tok[1024];
	while (strings && *strings)
	{
		strings = COM_ParseStringSetSep(strings, ';', tok, sizeof(tok));
		if (!Q_strcasecmp(tok, match))	//okay its here.
			return true;
	}
	return false;
}
#ifdef PLUGINS
void PM_EnumeratePlugins(void (*callback)(const char *name, qboolean blocked))
{
	package_t *p;
	struct packagedep_s *d;

	PM_PreparePackageList();

	for (p = availablepackages; p; p = p->next)
	{
		if ((p->flags & DPF_ENABLED) && (p->flags & DPF_PLUGIN))
		{
			for (d = p->deps; d; d = d->next)
			{
				if (d->dtype == DEP_FILE)
				{
					if (!Q_strncasecmp(d->name, PLUGINPREFIX, strlen(PLUGINPREFIX)))
					{
						qboolean blocked = PM_NameIsInStrings(fs_manifest?fs_manifest->installupd:NULL, va("!%s", p->name));
						callback(d->name, blocked);
					}
				}
			}
		}
	}
}
#endif
void PM_EnumerateMaps(const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	package_t *p;
	struct packagedep_s *d;
	size_t partiallen = strlen(partial);
	char mname[256];
	const char *sep = strchr(partial, ':');
	size_t pkgpartiallen = sep?sep-partial:partiallen;

	PM_PreparePackageList();

	for (p = availablepackages; p; p = p->next)
	{
		if (!Q_strncasecmp(p->name, partial, pkgpartiallen))
		{
			for (d = p->deps; d; d = d->next)
			{
				if (d->dtype == DEP_MAP)
				{
					/*if (!strchr(partial, ':'))
					{	//try to expand to only one...
						Q_snprintfz(mname, sizeof(mname), "%s:", p->name);
						ctx->cb(mname, NULL, NULL, ctx);
						break;
					}
					else*/
					{
						Q_snprintfz(mname, sizeof(mname), "%s:%s", p->name, d->name);
						if (!Q_strncasecmp(mname, partial, partiallen))
							ctx->cb(mname, p->description, NULL, ctx);
					}
				}
			}
		}
	}
}

static qboolean QDECL Host_StubClose (struct vfsfile_s *file)
{
	return true;
}
static char *PM_GetMetaTextFromFile(vfsfile_t *file, const char *filename, char *qhash, size_t hashsize)	//seeks, but does not close.
{
	qboolean (QDECL *OriginalClose) (struct vfsfile_s *file) = file->Close;	//evilness
	searchpathfuncs_t *archive;
	char *ret = NULL, *line;
	*qhash = 0;

	file->Close = Host_StubClose;	//so it doesn't go away without our say
	archive = FS_OpenPackByExtension(file, NULL, filename, filename, "");
	if (archive)
	{
		int crc;
		flocation_t loc;
		vfsfile_t *metafile = NULL;
		if (archive->FindFile(archive, &loc, "fte.meta", NULL))
			metafile = archive->OpenVFS(archive, &loc, "rb");
		else if (archive->FindFile(archive, &loc, "-", NULL))	//lame.
			metafile = archive->OpenVFS(archive, &loc, "rb");
		if (metafile)
		{
			size_t sz = VFS_GETLEN(metafile);
			ret = BZ_Malloc(sz+1);
			if (ret)
			{
				VFS_READ(metafile, ret, sz);
				ret[sz] = 0;
			}
			VFS_CLOSE(metafile);
		}
		crc = archive->GeneratePureCRC(archive, NULL);

		if (!ret)
		{	//it'd be really nice if creators were embedding metadata into their packages... but creators suck... especially when their creations predate our engine...
			//anyway, we kinda need this stuff in order to get the gamedirs right when drag+dropping. we also get nicer titles out of it.
#ifdef HAVE_LEGACY
			if (0)
				;
			//quake
			else if (crc == 0x92c8b832 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_shareware\ntitle \"Quake Shareware Data\"\ngamedir id1\nver 1.01\n}\n");
			else if (crc == 0x4f069cac && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_shareware\ntitle \"Quake Shareware Data\"\ngamedir id1\nver 1.06\n}\n");
			else if (crc == 0x329050a6 && !Q_strcasecmp(filename, "pak1.pak"))	ret = Z_StrDup("{\npackage quake_registered\ntitle \"Quake Registered Data\"\ngamedir id1\n}\n");
			else if (crc == 0x31bef951 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_hipnotic\ntitle \"Scourge of Armagon\"\ngamedir hipnotic\n}\n");
			else if (crc == 0x799d60c0 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rogue\ntitle \"Dissolution of Eternity\"\ngamedir rogue\n}\n");
			//quake rerelease
			else if (crc == 0xefdb8a92 && !Q_strcasecmp(filename, "QuakeEX.kpf"))ret= Z_StrDup("{\npackage quakeex_data\ntitle \"Quake Rerelease Misc Data\"\ngamedir \"\"\n}\n");	//p4
			else if (crc == 0xbb51cd43 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rerel\ntitle \"Quake Rerelease Data\"\ngamedir id1\nrequire quakeex_data\n}\n");	//p4
			else if (crc == 0xda8e9bb8 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rerel_hipnotic\ntitle \"Scourge of Armagon (Rerelease)\"\ngamedir hipnotic\nrequire quakeex_data\n}\n");
			else if (crc == 0x016eac0c && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rerel_rogue\ntitle \"Dissolution of Eternity (Rerelease)\"\ngamedir rogue\nrequire quakeex_data\n}\n");
			else if (crc == 0xbdfb74e2 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rerel_ctf\ntitle \"Capture The Flag (Rerelease)\"\ngamedir ctf\nrequire quakeex_data\n}\n");
			else if (crc == 0x8751abb3 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rerel_dopa\ntitle \"Dimensions Of The Past (Rerelease)\"\ngamedir dopa\nrequire quakeex_data\n}\n");
			else if (crc == 0x73f34d24 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake_rerel_mg1\ntitle \"Dimensions Of The Machine\"\ngamedir mg1\nrequire quakeex_data\n}\n");
			//hexen2
			else if (crc == 0xae67217a && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage hexen2_registered_p0\ntitle \"Quake2 Data (pak0)\"\ngamedir data1\n}\n");
			else if (crc == 0xee6fabd1 && !Q_strcasecmp(filename, "pak1.pak"))	ret = Z_StrDup("{\npackage hexen2_registered_p1\ntitle \"Quake2 Data (pak1)\"\ngamedir data1\n}\n");
			else if (crc == 0x400848b4 && !Q_strcasecmp(filename, "pak3.pak"))	ret = Z_StrDup("{\npackage hexen2_portals\ntitle \"Portals Data\"\ngamedir portals\n}\n");
			//quake2
			else if (crc == 0x03ffdce0 && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake2_p0\ntitle \"Quake2 Data (pak0)\"\ngamedir baseq2\n}\n");
			else if (crc == 0x7d40ac55 && !Q_strcasecmp(filename, "pak1.pak"))	ret = Z_StrDup("{\npackage quake2_p1\ntitle \"Quake2 Data (pak1)\"\ngamedir baseq2\n}\n");
			else if (crc == 0xdc6fabf9 && !Q_strcasecmp(filename, "pak2.pak"))	ret = Z_StrDup("{\npackage quake2_p2\ntitle \"Quake2 Data (pak2)\"\ngamedir baseq2\n}\n");
//			else if (crc == 0x && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake2_xatrixp2\ntitle \"The Reckoning\"\ngamedir baseq2\nconflict quake2_release\n}\n");
//			else if (crc == 0x && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake2_rogue\ntitle \"Ground Zero\"\ngamedir baseq2\nconflict quake2_release\n}\n");
//			else if (crc == 0x && !Q_strcasecmp(filename, "pak0.pak"))	ret = Z_StrDup("{\npackage quake2_release\ntitle \"Quake2 Rerelease Data\"\ngamedir baseq2\nrequire quake2ex_data\n}\n");
//			else if (crc == 0x && !Q_strcasecmp(filename, "Q2Game.kpf"))ret = Z_StrDup("{\npackage quake2ex_data\ntitle \"Quake2 Rerelease Misc Data\"\ngamedir \"\"\n}\n");
			//quake3
			else if (crc == 0xb1f4d354 && !Q_strcasecmp(filename, "pak0.pk3"))	ret = Z_StrDup("{\npackage quake3_demo\ntitle \"Quake3 Demo Data\"\ngamedir baseq3\n}\n");
			else if (crc == 0x5d626b5f && !Q_strcasecmp(filename, "pak0.pk3"))	ret = Z_StrDup("{\npackage quake3_p0\ntitle \"Quake3 Data (pak0)\"\ngamedir baseq3\n}\n");
			else if (crc == 0x11c4fe9b && !Q_strcasecmp(filename, "pak1.pk3"))	ret = Z_StrDup("{\npackage quake3_p1\ntitle \"Quake3 Data (pak1)\"\ngamedir baseq3\n}\n");
			else if (crc == 0x18912474 && !Q_strcasecmp(filename, "pak2.pk3"))	ret = Z_StrDup("{\npackage quake3_p2\ntitle \"Quake3 Data (pak2)\"\ngamedir baseq3\n}\n");
			else if (crc == 0xb24e9894 && !Q_strcasecmp(filename, "pak3.pk3"))	ret = Z_StrDup("{\npackage quake3_p3\ntitle \"Quake3 Data (pak3)\"\ngamedir baseq3\n}\n");
			else if (crc == 0x476700a6 && !Q_strcasecmp(filename, "pak4.pk3"))	ret = Z_StrDup("{\npackage quake3_p4\ntitle \"Quake3 Data (pak4)\"\ngamedir baseq3\n}\n");
			else if (crc == 0xf39bc355 && !Q_strcasecmp(filename, "pak5.pk3"))	ret = Z_StrDup("{\npackage quake3_p5\ntitle \"Quake3 Data (pak5)\"\ngamedir baseq3\n}\n");
			else if (crc == 0xdd13d69b && !Q_strcasecmp(filename, "pak6.pk3"))	ret = Z_StrDup("{\npackage quake3_p6\ntitle \"Quake3 Data (pak6)\"\ngamedir baseq3\n}\n");
			else if (crc == 0x362c0725 && !Q_strcasecmp(filename, "pak7.pk3"))	ret = Z_StrDup("{\npackage quake3_p7\ntitle \"Quake3 Data (pak7)\"\ngamedir baseq3\n}\n");
			else if (crc == 0x3a3dc1a6 && !Q_strcasecmp(filename, "pak8.pk3"))	ret = Z_StrDup("{\npackage quake3_p8\ntitle \"Quake3 Data (pak8)\"\ngamedir baseq3\n}\n");
			else if (crc == 0x90dc1501 && !Q_strcasecmp(filename, "pak0.pk3"))	ret = Z_StrDup("{\npackage quake3_ta_p0\ntitle \"TeamArena Data (pak0)\"\ngamedir missionpack\n}\n");
			else if (crc == 0x1e757510 && !Q_strcasecmp(filename, "pak1.pk3"))	ret = Z_StrDup("{\npackage quake3_ta_p1\ntitle \"TeamArena Data (pak1)\"\ngamedir missionpack\n}\n");
			else if (crc == 0x9eb4a591 && !Q_strcasecmp(filename, "pak2.pk3"))	ret = Z_StrDup("{\npackage quake3_ta_p2\ntitle \"TeamArena Data (pak2)\"\ngamedir missionpack\n}\n");
			else if (crc == 0x55c0476a && !Q_strcasecmp(filename, "pak3.pk3"))	ret = Z_StrDup("{\npackage quake3_ta_p3\ntitle \"TeamArena Data (pak3)\"\ngamedir missionpack\n}\n");
			else
#endif
				ret = Z_StrDup("{\nguessed 1\n}\n");
		}

		//get the archive's qhash.
		Q_snprintfz(qhash, hashsize, "0x%x", crc);

		line = ret;
		do
		{
			line = (char*)Cmd_TokenizeString (line, false, false);
			if (!strcmp(com_token, "{"))
				break;	//okay, found the start of it.
		} while (*line);
		//and leave it pointing there for easier parsing later. single package only.
		line = va("qhash %s\n%s", qhash, line);
		line = Z_StrDup(line);
		Z_Free(ret);
		ret = line;

		archive->ClosePath(archive);
	}
	else
		Con_Printf("No archive in %s\n", filename);
	file->Close = OriginalClose;
	return ret;
}

void *PM_GeneratePackageFromMeta(vfsfile_t *file, char *fname, size_t fnamesize, enum fs_relative *fsroot)
{
	package_t *p = NULL;
	char pkgname[MAX_QPATH];
	char qhash[64];
	char *pkgdata = PM_GetMetaTextFromFile(file, fname, qhash, sizeof(qhash));

	struct packagesourceinfo_s pkgsrc = {DPF_ENABLED};
	pkgsrc.version = 3;
	pkgsrc.categoryprefix = "";
	Q_strncpyz(pkgsrc.gamedir, FS_GetGamedir(true), sizeof(pkgsrc.gamedir));

	COM_StripAllExtensions(COM_SkipPath(fname), pkgname,sizeof(pkgname));

	PM_PreparePackageList();	//just in case.

	//see if we can make any sense of it.
	PM_ParsePackage(&pkgsrc, pkgdata, &p, 1, fname);
	if (p)
	{
		/*if (!(p->fsroot == FS_ROOT && *p->gamedir) && FS_RELATIVE_ISSPECIAL(p->fsroot))
		{	//the metadata is useless if it says to put it somewhere unsafe, though signed packages mean this is at least mostly safe...
			Z_Free(pkgdata);
			pkgdata = NULL;
			Menu_PromptOrPrint(va(localtext("Refusing to install to requested location\n%s\n"), fname), "Cancel", true);
		}
		else*/ if (!PM_SignatureOkay(p))
		{	//only allow if its trustworthy (or paks/pk3s)
			Z_Free(pkgdata);
			pkgdata = NULL;
			Menu_PromptOrPrint(va(localtext("Bad MetaData Signature\n%s\n"), fname), "Cancel", true);
		}
		else if (p->extract == EXTRACT_COPY)
		{
			vfsfile_t *vfs;
			const char *f = PM_GetDepSingle(p, DEP_FILE);
			if (!f)
				f = va("%s", fname);	//erk?

			if (*p->gamedir)
				f = va("%s/%s", p->gamedir, f);

			Z_StrDupPtr(&p->qhash, qhash);

			vfs = FS_OpenVFS(f, "rb", p->fsroot);	//if they're dropping pak0.pak, don't overwrite anything.
			if (vfs)
			{
				VFS_CLOSE(vfs);
				Z_Free(pkgdata);
				pkgdata = NULL;
				Menu_PromptOrPrint(va(localtext("File is already installed\n%s\n"), fname), "Cancel", true);
			}
			if (!vfs && PM_TryGenCachedName(f, p, fname, fnamesize))
				;
			else
				Q_strncpyz(fname, f, fnamesize);
			*fsroot = p->fsroot;
		}
		else
		{	//we're doing drag+drop single-file logic here, so don't want to handle extracting everything with the risk of overwriting.
			Z_Free(pkgdata);
			pkgdata = NULL;
			Menu_PromptOrPrint(va(localtext("Package installation too complex\n%s\n"), fname), "Cancel", true);
		}

		//okay, seems there's something in it.
		PM_FreePackage(p);
	}

	return pkgdata;
}

static qboolean PM_FileInstalled_Internal(const char *package, const char *category, const char *title, const char *filename, enum fs_relative fsroot, unsigned pkgflags, void *metainfo, qboolean enable)
{
	package_t *p;
	char *dlcache, *end;

	if (!*filename)
		return false;	//wtf?

	if (metainfo)
	{
		struct packagesourceinfo_s pkgsrc = {DPF_ENABLED};
		pkgsrc.version = 3;
		pkgsrc.categoryprefix = "";

		PM_ParsePackage(&pkgsrc, metainfo, &p, 1, package);
		if (!p)
			return false;
	}
	else
	{
		p = Z_Malloc(sizeof(*p));
		p->priority = PM_DEFAULTPRIORITY;
		strcpy(p->version, "?" "?" "?" "?");
	}

	if (PM_HasDep(p, DEP_FILE, filename))
		;	//no dupes
	else if (PM_GetDepSingle(p, DEP_FILE))
	{	//a filename was specified, but it differs from what we expected...
		Con_Printf(CON_WARNING"PM_FileInstalled: filename does not match metadata info\n");
		PM_FreePackage(p);
		return false;
	}
	else
		PM_AddDep(p, DEP_FILE, filename);

	dlcache = strstr(p->deps->name+1, "/dlcache/");
	if (dlcache)
	{
		memmove(dlcache+1, dlcache+9, strlen(dlcache+9)+1);
		dlcache = (char*)(COM_GetFileExtension(p->deps->name, NULL));
		if (dlcache[0] == '.' && dlcache[1] && *COM_GetFileExtension(p->deps->name, dlcache))
		{
			unsigned int qhash = strtoul(dlcache+1, &end, 16);
			if (!*end)	//reached the end of the name?...
			{
				p->qhash = Z_StrDupf("%#x", qhash);	//that looks like a qhash!
				*dlcache = 0;	//strip it from the name.
			}
		 }
	}

	if (fsroot == FS_PUBGAMEONLY)
	{
		Q_strncpyz(p->gamedir, FS_GetGamedir(true), sizeof(p->gamedir));
		p->fsroot = FS_ROOT;
	}
	else if (fsroot == FS_ROOT && pkgflags)
	{
		dlcache = strchr(p->deps->name+1, '/');
		if (dlcache)
		{
			*dlcache++ = 0;
			Q_strncpyz(p->gamedir, p->deps->name, sizeof(p->gamedir));
			memmove(p->deps->name, dlcache, strlen(dlcache)+1);
		}
		p->fsroot = FS_ROOT;
	}
	else
		p->fsroot = fsroot;

	if (pkgflags&DPF_PLUGIN)
	{
		if (strcmp(p->version, STRINGIFY(SVNREVISION)))
			enable = false;
		if (!p->arch)
			p->arch = Z_StrDup(THISARCH);
		Z_Free(p->qhash);	//don't get confused.
		p->qhash = NULL;
	}
	else
		pkgflags |= DPF_FORGETONUNINSTALL;
	if (!p->name || !*p->name)
		p->name = Z_StrDup(package);
	if (!p->title || !*p->title)
		p->title = Z_StrDup(title);
	if (!p->category || !*p->category)
		p->category = Z_StrDup(category);
	p->flags = pkgflags|DPF_NATIVE;
	if (enable)
		p->flags |= DPF_USERMARKED|DPF_ENABLED;

	p = PM_InsertPackage(p);
	if (p)
	{
		if (enable && !(p->flags&DPF_ENABLED))
			Con_Printf(CON_WARNING"PM_FileInstalled: package failed to enable\n");
//			Cmd_ExecuteString("menu_download\n", RESTRICT_LOCAL);
		PM_ResortPackages();
		PM_WriteInstalledPackages();
	}

	return true;
}
void PM_FileInstalled(const char *filename, enum fs_relative fsroot, void *metainfo, qboolean enable)
{
	char pkgname[MAX_QPATH];
	COM_StripAllExtensions(COM_SkipPath(filename), pkgname,sizeof(pkgname));
	PM_FileInstalled_Internal(pkgname, "", pkgname, filename, fsroot, /*metainfo?0:*/DPF_GUESSED,  metainfo, enable);
}

#ifdef PLUGINS
static package_t *PM_FindExactPackage(const char *packagename, const char *arch, const char *version, unsigned int flags);
static package_t *PM_FindPackage(const char *packagename);
static int QDECL PM_EnumeratedPlugin (const char *name, qofs_t size, time_t mtime, void *param, searchpathfuncs_t *spath)
{
	enum fs_relative fsroot = (qintptr_t)param;
	static const char *knownarch[] =
	{
		"x32", "x64", "amd64", "x86",	//various x86 ABIs
		"arm", "arm64", "armhf",		//various arm ABIs
		"ppc", "unk",					//various misc ABIs
	};
	package_t *p;
	struct packagedep_s *dep;
	char vmname[MAX_QPATH];
	int len, l, a;
	char *dot;
	char *pkgname;
	char *metainfo = NULL;
	vfsfile_t *f;
	if (!strncmp(name, PLUGINPREFIX, strlen(PLUGINPREFIX)))
		Q_strncpyz(vmname, name+strlen(PLUGINPREFIX), sizeof(vmname));
	else
		Q_strncpyz(vmname, name, sizeof(vmname));
	len = strlen(vmname);
	l = strlen(ARCH_CPU_POSTFIX ARCH_DL_POSTFIX);
	if (len > l && !strcmp(vmname+len-l, ARCH_CPU_POSTFIX ARCH_DL_POSTFIX))
	{
		len -= l;
		vmname[len] = 0;
	}
	else
	{
		dot = strchr(vmname, '.');
		if (dot)
		{
			*dot = 0;
			len = strlen(vmname);

			//if we can find a known cpu arch there then ignore it - its a different cpu arch
			for (a = 0; a < countof(knownarch); a++)
			{
				l = strlen(knownarch[a]);
				if (len > l && !Q_strcasecmp(vmname + len-l, knownarch[a]))
					return true;	//wrong arch! ignore it.
			}
		}
	}
	if (len > 0 && vmname[len-1] == '_')
		vmname[len-1] = 0;

	for (p = availablepackages; p; p = p->next)
	{
		if (!(p->flags & DPF_PLUGIN))
			continue;
		if (p->fsroot != fsroot)
			continue;
		for (dep = p->deps; dep; dep = dep->next)
		{
			if (dep->dtype != DEP_FILE)
				continue;
			if (!Q_strcasecmp(dep->name, name))
				return true;
		}
	}

	pkgname = va("fteplug_%s", vmname);

	if (PM_FindExactPackage(pkgname, NULL, NULL, 0))
		return true;	//don't include it if its a dupe anyway.
	if (PM_FindExactPackage(vmname, NULL, NULL, 0))
		return true;	//don't include it if its a dupe anyway.
	//FIXME: should be checking whether there's a package that provides the file...


	f = FS_OpenVFS(name, "rb", fsroot);
	if (f)
	{
		char qhash[16];
		metainfo = PM_GetMetaTextFromFile(f, name, qhash, sizeof(qhash));
		VFS_CLOSE(f);
	}

	return PM_FileInstalled_Internal(pkgname, "Plugins/", vmname, name, fsroot, DPF_PLUGIN|DPF_TRUSTED, metainfo,
#ifdef ENABLEPLUGINSBYDEFAULT
			true
#else
			false
#endif
		);
}
#ifndef SERVERONLY
#ifndef ENABLEPLUGINSBYDEFAULT
static void PM_PluginDetected(void *ctx, int status)
{
	if (status != PROMPT_CANCEL)
		PM_WriteInstalledPackages();
	if (status == PROMPT_YES)	//'view'...
	{
		Cmd_ExecuteString("menu_download\n", RESTRICT_LOCAL);
		Cmd_ExecuteString("menu_download \"Plugins/\"\n", RESTRICT_LOCAL);
	}
}
#endif
#endif
#endif

/*#ifndef SERVERONLY
static void PM_AutoUpdateQuery(void *ctx, promptbutton_t status)
{
	if (status == PROMPT_CANCEL)
		return; //'Later'
	if (status == PROMPT_YES)
		Cmd_ExecuteString("menu_download\n", RESTRICT_LOCAL);
	Menu_Download_Update();
}
#endif*/

static void PM_PreparePackageList(void)
{
	//figure out what we've previously installed.
	if (fs_manifest && !loadedinstalled)
	{
		int parm;
		qofs_t sz = 0;
		char *f = FS_MallocFile(INSTALLEDFILES, FS_ROOT, &sz);
		loadedinstalled = true;
		if (f)
		{
			if (PM_ParsePackageList(f, DPF_FORGETONUNINSTALL|DPF_ENABLED, NULL, ""))
				PM_WriteInstalledPackages();
			BZ_Free(f);
		}

		parm = COM_CheckParm ("-updatesrc");
		if (parm)
		{
			unsigned int fl = SRCFL_USER;
			if (COM_CheckParm ("-unsafe"))
				fl |= SRCFL_UNSAFE;
			do
			{
				PM_AddSubList(com_argv[parm+1], NULL, fl);	//enable it by default. functionality is kinda broken otherwise.
				parm = COM_CheckNextParm ("-updatesrc", parm);
			} while (parm && parm < com_argc-1);
		}
		else if (fs_manifest && fs_manifest->downloadsurl && *fs_manifest->downloadsurl)
		{
			unsigned int fl = SRCFL_MANIFEST;
			char *s = fs_manifest->downloadsurl;

			if (fs_manifest->security==MANIFEST_SECURITY_NOT)
				fl |= SRCFL_DISABLED;	//don't trust it, don't even prompt.

			while ((s = COM_Parse(s)))
			{
#ifdef FTE_TARGET_WEB
				if (*com_token == '/' && !(fl&SRCFL_DISABLED))
					PM_AddSubList(com_token, NULL, fl|SRCFL_ENABLED);	//enable it by default, its too annoying otherwise.
				else
#endif
					PM_AddSubList(com_token, NULL, fl);
			}
		}

#ifdef PLUGINS
		{
			char sys[MAX_OSPATH];
			char disp[MAX_OSPATH];
			if (FS_DisplayPath("", FS_BINARYPATH, disp, sizeof(disp))&&FS_SystemPath("", FS_BINARYPATH, sys, sizeof(sys)))
			{
				Con_DPrintf("Loading plugins from \"%s\"\n", disp);
				Sys_EnumerateFiles(sys, PLUGINPREFIX"*" ARCH_DL_POSTFIX, PM_EnumeratedPlugin, (void*)FS_BINARYPATH, NULL);
			}
			if (FS_DisplayPath("", FS_LIBRARYPATH, disp, sizeof(disp))&&FS_SystemPath("", FS_LIBRARYPATH, sys, sizeof(sys)))
			{
				Con_DPrintf("Loading plugins from \"%s\"\n", disp);
				Sys_EnumerateFiles(sys, PLUGINPREFIX"*" ARCH_DL_POSTFIX, PM_EnumeratedPlugin, (void*)FS_LIBRARYPATH, NULL);
			}
		}
#endif
	}
}

void PM_ManifestChanged(ftemanifest_t *man)
{	//gamedir or something changed. reset the package manager stuff.
	if (!man)
	{	//shutting down... don't reload anything.
		PM_Shutdown(false);
		return;
	}
	PM_Shutdown(true);
	PM_UpdatePackageList(false);
}

qboolean PM_HandleRedirect(const char *package, /*char *cachename, size_t cachesize,*/ char *url, size_t urlsize)
{	//given a "GAMEDIR/PACKAGEFILE", swap it out for a real path/url that a client can download.
	//eg "package/GAMEDIR/dlcache/PACKAGEFILE.xxxxxxxx" or http://whereeverweoriginallydownloadeditfrom"
	char gamedir[16];
	char *sep;
	package_t *p;
char *cachename = NULL; size_t cachesize=0;
	sep = strchr(package, '/');
	*cachename = 0;
	if (!sep)
		return false;	//no gamedir? that doesn't seem right. tell em to fuck off.
	if (sep-package >= sizeof(gamedir))
		return false;	//overflow
	memcpy(gamedir, package, sep-package);
	gamedir[sep-package] = 0;
	sep++;
	for (p = availablepackages; p; p = p->next)
	{
		if (!(p->flags & DPF_PRESENT))
			continue;	//dunno, shouldn't really be trying to download it if its not even on the server.
		if (strcmp(p->gamedir, gamedir))
			continue;	//nope, some other gamedir perhaps.
		if (PM_HasDep(p, DEP_FILE, sep))
		{
			if (!(p->flags & DPF_CACHED) || !PM_TryGenCachedName(package, p, cachename, cachesize))
				*cachename = 0;
			if (p->mirror[0])
			{
				Q_strncpyz(url, p->mirror[0], urlsize);
				return true;
			}
			if (p->flags & DPF_CACHED)
			{
				Q_snprintfz(url, urlsize, "package/%s", cachename);
				return true;
			}
			return false;	//native? don't redirect it locally.
		}
		/*if (PM_HasDep(p, DEP_CACHEFILE, sep))
		{
			Q_snprintfz(cachename, sizeof(cachename), "downloads/%s", d->name);
		}*/
	}
	return false;
}
void PM_LoadPackages(searchpath_t **oldpaths, const char *parent_pure, const char *parent_logical, searchpath_t *search, unsigned int loadstuff, int minpri, int maxpri)
{
	package_t *p;
	struct packagedep_s *d;
	char temp[MAX_OSPATH];
	int pri;

	do
	{
		//find the lowest used priority above the previous
		pri = maxpri;
		for (p = availablepackages; p; p = p->next)
		{
			if ((p->flags & (DPF_ENABLED|DPF_MANIMARKED)) && p->priority>=minpri&&p->priority<pri && !Q_strcasecmp(parent_pure, p->gamedir))
				pri = p->priority;
		}
		minpri = pri+1;

		for (p = availablepackages; p; p = p->next)
		{
			if ((p->flags & (DPF_ENABLED|DPF_MANIMARKED)) && p->priority==pri && !Q_strcasecmp(parent_pure, p->gamedir))
			{
				char *qhash = (p->qhash&&*p->qhash)?p->qhash:NULL;
				unsigned int fsfl = 0;
				if (p->flags & DPF_CACHED)
					fsfl |= 0;	//package is in the dlcache dir... if its downloaded from somewhere then its probably a bit rich to block it from downloading from elsewhere.
				else
					fsfl |= SPF_COPYPROTECTED;
				if (qhash && (p->flags&DPF_TRUSTED))
					;
				else
					fsfl |= SPF_UNTRUSTED;	//never trust it if we can't provide it

				for (d = p->deps; d; d = d->next)
				{
					if (d->dtype == DEP_FILE)
					{
						Q_snprintfz(temp, sizeof(temp), "%s/%s", p->gamedir, d->name);
						FS_AddHashedPackage(oldpaths, parent_pure, parent_logical, search, loadstuff, temp, qhash, p->packprefix, fsfl);
					}
					else if (d->dtype == DEP_CACHEFILE)
					{
						Q_snprintfz(temp, sizeof(temp), "downloads/%s", d->name);
						FS_AddHashedPackage(oldpaths, parent_pure, parent_logical, NULL, loadstuff, temp, qhash, p->packprefix, fsfl);
					}
				}
			}
		}
	} while (pri < maxpri);
}

void PM_Shutdown(qboolean soft)
{
	size_t i, pm_numoldsources = pm_numsources;
	//free everything...

	pm_sequence++;

	pm_numsources = 0;
	for (i = 0; i < pm_numoldsources; i++)
	{
#ifdef WEBCLIENT
		if (pm_source[i].curdl)
		{
			DL_Close(pm_source[i].curdl);
			pm_source[i].curdl = NULL;
		}
#endif
		pm_source[i].status = SRCSTAT_UNTRIED;

		if (pm_source[i].module && soft)
		{	//added via a plugin. reset rather than forget.
			pm_source[pm_numsources++] = pm_source[i];
		}
		else
		{	//forget it, oh noes.
			Z_Free(pm_source[i].url);
			pm_source[i].url = NULL;
			Z_Free(pm_source[i].prefix);
			pm_source[i].prefix = NULL;
		}
	}
	if (!pm_numsources)
	{
		Z_Free(pm_source);
		pm_source = NULL;
	}

	if (!soft)
	{
		while (availablepackages)
			PM_FreePackage(availablepackages);
	}
	loadedinstalled = false;
}

//finds the newest version
static package_t *PM_FindExactPackage(const char *packagename, const char *arch, const char *version, unsigned int flags)
{
	package_t *p, *r = NULL;
	if (arch && !*arch) arch = NULL;
	if (version && !*version) version = NULL;

	for (p = availablepackages; p; p = p->next)
	{
		if (!strcmp(p->name, packagename))
		{
			if (arch && (!p->arch||strcmp(p->arch, arch)))
				continue;	//wrong arch.
			if (!arch && p->arch && *p->arch && Q_strcasecmp(p->arch, THISARCH))
				continue;	//wrong arch.
			if (flags && !(p->flags & flags))
				continue;
			if (version)
			{	//versions are a bit more complex.
				if (*version == '=' && strcmp(p->version, version+1))
					continue;
				if (*version == '>' && strcmp(p->version, version+1)<=0)
					continue;
				if (*version == '<' && strcmp(p->version, version+1)>=0)
					continue;
			}
			if (!r || strcmp(p->version, r->version)>0)
				r = p;
		}
	}
	return r;
}
static package_t *PM_FindPackage(const char *packagename)
{
	char *t = strcpy(alloca(strlen(packagename)+2), packagename);
	char *arch = strchr(t, ':');
	char *ver = strchr(t, '=');
	if (!ver)
		ver = strchr(t, '>');
	if (!ver)
		ver = strchr(t, '<');
	if (arch)
		*arch++ = 0;
	if (ver)
	{
		*ver = 0;
		return PM_FindExactPackage(t, arch, packagename + (ver-t), 0);	//weirdness is because the leading char of the version is important.
	}
	else
		return PM_FindExactPackage(t, arch, NULL, 0);
}
//returns the marked version of a package, if any.
static package_t *PM_MarkedPackage(const char *packagename, int markflag)
{
	char *t = strcpy(alloca(strlen(packagename)+2), packagename);
	char *arch = strchr(t, ':');
	char *ver = strchr(t, '=');
	if (!markflag)
		return NULL;
	if (!ver)
		ver = strchr(t, '>');
	if (!ver)
		ver = strchr(t, '<');
	if (arch)
		*arch++ = 0;
	if (ver)
	{
		*ver = 0;
		return PM_FindExactPackage(t, arch, packagename + (ver-t), markflag);	//weirdness is because the leading char of the version is important.
	}
	else
		return PM_FindExactPackage(t, arch, NULL, markflag);
}

//just resets all actions, so that a following apply won't do anything.
static void PM_RevertChanges(void)
{
	package_t *p;
	int us = parse_revision_number(enginerevision, true), them;

	if (pkg_updating)
		return;

	for (p = availablepackages; p; p = p->next)
	{
		if (p->flags & DPF_ENGINE)
		{
			them = parse_revision_number(p->version, true);
			if (!(p->flags & DPF_HIDDEN) && us && us==them && (p->flags & DPF_PRESENT))
				p->flags |= DPF_AUTOMARKED;
			else
				p->flags &= ~DPF_MARKED;
		}
		else
		{
			if (p->flags & DPF_ENABLED)
				p->flags |= DPF_USERMARKED;
			else
				p->flags &= ~DPF_MARKED;
		}
		p->flags &= ~DPF_PURGE;
	}
}

static qboolean PM_HasDependant(package_t *package, unsigned int markflag)
{
	package_t *o;
	struct packagedep_s *dep;
	for (o = availablepackages; o; o = o->next)
	{
		if (o->flags & markflag)
			for (dep = o->deps; dep; dep = dep->next)
				if (dep->dtype == DEP_REQUIRE || dep->dtype == DEP_RECOMMEND || dep->dtype == DEP_SUGGEST)
					if (!strcmp(package->name, dep->name))
						return true;
	}
	return false;
}

//just flags, doesn't delete (yet)
//markflag should be DPF_AUTOMARKED or DPF_USERMARKED
static void PM_UnmarkPackage(package_t *package, unsigned int markflag)
{
	package_t *o;
	struct packagedep_s *dep;

	COM_AssertMainThread("PM_UnmarkPackage");

	if (pkg_updating)
		return;

	if (!(package->flags & markflag))
		return;	//looks like its already deselected.
	package->flags &= ~(markflag);

	if (!(package->flags & DPF_MARKED))
	{
#ifdef WEBCLIENT
		//Is this safe?
		package->trymirrors = 0;	//if its enqueued, cancel that quickly...
		if (package->curdownload)
		{					//if its currently downloading, cancel it.
			DL_Close(package->curdownload);
			package->curdownload = NULL;
		}
		package->flags &= ~DPF_PENDING;
#endif

		//remove stuff that depends on us
		for (o = availablepackages; o; o = o->next)
		{
			for (dep = o->deps; dep; dep = dep->next)
				if (dep->dtype == DEP_REQUIRE)
					if (!strcmp(dep->name, package->name))
						PM_UnmarkPackage(o, DPF_MARKED);
		}
	}
	if (!(package->flags & DPF_USERMARKED))
	{
		//go through dependancies and unmark any automarked packages if nothing else depends upon them
		for (dep = package->deps; dep; dep = dep->next)
		{
			if (dep->dtype == DEP_REQUIRE || dep->dtype == DEP_RECOMMEND)
			{
				package_t *d = PM_MarkedPackage(dep->name, DPF_AUTOMARKED);
				if (d && !(d->flags & DPF_USERMARKED))
				{
					if (!PM_HasDependant(d, DPF_MARKED))
						PM_UnmarkPackage(d, DPF_AUTOMARKED);
				}
			}
		}
	}
}

static package_t *PM_RootForAlt(package_t *package)
{
	package_t *p, *a;
	for (p = availablepackages; p; p = p->next)
	{
		if (p == package)
			return p;
		for (a = p->alternative; a; a = a->next)
		{
			if (a == package)
				return p;
		}
	}
	return NULL;	//some kind of error
}
//just flags, doesn't install
//returns true if it was marked (or already enabled etc), false if we're not allowed.
static qboolean PM_MarkPackage(package_t *package, unsigned int markflag)
{
	package_t *o;
	struct packagedep_s *dep, *dep2;
	qboolean replacing = false;
	package_t *root;	//if its an alt package, we want to do some switcheroos

	if (pkg_updating)
	{
		Con_Printf("PM_MarkPackage: busy...\n");
		return false;
	}

	if (package->flags & DPF_MARKED)
	{
		package->flags |= markflag;
		return true;	//looks like its already picked. marking it again will do no harm.
	}

#ifndef WEBCLIENT
	//can't mark for download if we cannot download.
	if (!(package->flags & DPF_PRESENT))
	{	//though we can at least unmark it for deletion...
		package->flags &= ~DPF_PURGE;
		Con_Printf("PM_MarkPackage: unable to download, and not already present.\n");
		return false;
	}
#else
	if (!PM_SignatureOkay(package))
	{
		Con_Printf(CON_WARNING"%s: package not trusted, refusing to mark.\n", package->name);
		return false;
	}
#endif

	//any file-conflicts prevent the package from being installable.
	//this is mostly for pak1.pak
	for (dep = package->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_FILECONFLICT)
		{
			const char *n;
			if (*package->gamedir)
				n = va("%s/%s", package->gamedir, dep->name);
			else
				n = dep->name;
			if (PM_CheckFile(n, package->fsroot))
			{
				char tmp[MAX_OSPATH];
				FS_DisplayPath(n, package->fsroot, tmp,sizeof(tmp));
				Con_Printf(CON_WARNING"%s: conflicts with existing file \"%s\"\n", package->name, tmp);
				return false;
			}
		}
	}

	root = PM_RootForAlt(package);
	if (root != package && (package->flags&DPF_TRUSTED) >= (root->flags&DPF_TRUSTED))
	{
		//unlink the new.
		*package->link = package->next;
		if (package->next)
			package->next->link = package->link;

		//replace the old root with the new
		*root->link = package;
		package->next = root->next;
		if (package->next)
			package->next->link = &package->next;

		//insert the old into the new's alt
		root->next = package->alternative;
		if (root->next)
			root->next->link = &root->next;
		root->link = &package->alternative;
		*root->link = root;

		root->flags &= ~DPF_ALLMARKED;
	}
	else if (root != package)
		return false;

	package->flags |= markflag;

	//first check to see if we're replacing a different version of the same package
	for (o = availablepackages; o; o = o->next)
	{
		if (o == package)
			continue;

		if (o->flags & DPF_MARKED)
		{
			if (!strcmp(o->name, package->name))
			{	//replaces this package
				o->flags &= ~DPF_MARKED;
				replacing = true;
			}
			else if ((package->flags & DPF_ENGINE) && (o->flags & DPF_ENGINE))
				PM_UnmarkPackage(o, DPF_MARKED);	//engine updates are mutually exclusive, unmark the existing one (you might have old ones cached, but they shouldn't be enabled).
			else
			{	//two packages with the same filename are always mutually incompatible, but with totally separate dependancies etc.
				qboolean remove = false;
				for (dep = package->deps; dep; dep = dep->next)
				{
					if (dep->dtype == DEP_FILE)
					for (dep2 = o->deps; dep2; dep2 = dep2->next)
					{
						if (dep2->dtype == DEP_FILE)
						if (!strcmp(dep->name, dep2->name))
						{
							PM_UnmarkPackage(o, DPF_MARKED);
							remove = true;
							break;
						}
					}
					if (remove)
						break;
				}
				//fixme: zip content conflicts
			}
		}
	}

	//if we are replacing an existing one, then dependancies are already settled (only because we don't do version deps)
	if (replacing)
		return true;

	//satisfy our dependancies.
	for (dep = package->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_REQUIRE || dep->dtype == DEP_RECOMMEND)
		{
			package_t *d = PM_MarkedPackage(dep->name, DPF_MARKED);
			if (!d)
			{
				d = PM_FindPackage(dep->name);
				if (d)
				{
					if (dep->dtype == DEP_RECOMMEND && !PM_CheckPackageFeatures(d))
						Con_DPrintf("Skipping recommendation \"%s\"\n", dep->name);
					else
						PM_MarkPackage(d, DPF_AUTOMARKED);
				}
				else if (dep->dtype == DEP_REQUIRE)
					Con_Printf(CON_WARNING"Couldn't find dependancy \"%s\"\n", dep->name);
				else
					Con_DPrintf("Couldn't find dependancy \"%s\"\n", dep->name);
			}
		}
		if (dep->dtype == DEP_CONFLICT || dep->dtype == DEP_REPLACE)
		{
			for (;;)
			{
				package_t *d = PM_MarkedPackage(dep->name, DPF_MARKED);
				if (!d)
					break;
				PM_UnmarkPackage(d, DPF_MARKED);
			}
		}
	}

	//remove any packages that conflict with us.
	for (o = availablepackages; o; o = o->next)
	{
		for (dep = o->deps; dep; dep = dep->next)
			if (dep->dtype == DEP_CONFLICT || dep->dtype == DEP_REPLACE)
				if (!strcmp(dep->name, package->name))
					PM_UnmarkPackage(o, DPF_MARKED);
	}
	return true;
}

//just flag stuff as needing updating
#define UPDAVAIL_UPDATE		1	//there's regular updates available. show the updates menu.
#define UPDAVAIL_REQUIRED	2	//important updates that require the user to confirm
#define UPDAVAIL_FORCED		4	//trusted updates that are insisted on by the manifest.
unsigned int PM_MarkUpdates (void)
{
	unsigned int ret = 0;
	package_t *p, *o, *b;
#ifdef WEBCLIENT
	package_t *e = NULL;
	int bestengine = parse_revision_number(enginerevision, true);
	int them;
#endif

	doautoupdate = 0;

	if (fs_manifest && fs_manifest->installupd)
	{
		char tok[1024];
		char *strings = fs_manifest->installupd;
		while (strings && *strings)
		{
			qboolean isunwanted = (*strings=='!');
			strings = COM_ParseStringSetSep(strings+isunwanted, ';', tok, sizeof(tok));

			p = PM_MarkedPackage(tok, DPF_MARKED);
			if (!p)
			{
				if (PM_NameIsInStrings(declinedpackages, tok))
					continue;
				p = PM_FindPackage(tok);
				if (p)
				{
					if (PM_MarkPackage(p, DPF_AUTOMARKED))
						ret |= UPDAVAIL_UPDATE;
				}
			}
			else if (isunwanted)
			{
				PM_UnmarkPackage(p, DPF_AUTOMARKED);	//try and unmark it.
				ret |= UPDAVAIL_UPDATE;
			}
			else if (!(p->flags & DPF_ENABLED))
				ret |= UPDAVAIL_UPDATE;
		}

		if (ret)
			ret = (fs_manifest->security==MANIFEST_SECURITY_INSTALLER)?UPDAVAIL_FORCED:UPDAVAIL_REQUIRED;
	}

	for (p = availablepackages; p; p = p->next)
	{
#ifdef WEBCLIENT
		if ((p->flags & DPF_ENGINE) && !(p->flags & DPF_HIDDEN) && bestengine>0 && PM_SignatureOkay(p))
		{
			them = parse_revision_number(p->version, true);
			if (!(p->flags & DPF_TESTING) || pkg_autoupdate.ival >= UPD_TESTING)
				if (them > bestengine)
				{
					e = p;
					bestengine = them;
				}
		}
#endif
		if (p->flags & DPF_MARKED)
		{
			b = NULL;
			for (o = availablepackages; o; o = o->next)
			{
				if (p == o || (o->flags & DPF_HIDDEN))
					continue;
				if (!(o->flags & DPF_TESTING) || pkg_autoupdate.ival >= UPD_TESTING)
					if (!strcmp(o->name, p->name) && !strcmp(o->arch?o->arch:"", p->arch?p->arch:"") && strcmp(o->version, p->version) > 0)
					{
						if (!b || strcmp(b->version, o->version) < 0)
							b = o;
					}
			}

			if (b)
			{
				if (PM_MarkPackage(b, p->flags&DPF_MARKED))
				{
					ret |= UPDAVAIL_UPDATE;
					PM_UnmarkPackage(p, DPF_MARKED);
				}
			}
		}
	}
#ifdef WEBCLIENT
	if (e && !(e->flags & DPF_MARKED))
	{
		if (pkg_autoupdate.ival >= UPD_STABLE)
		{
			if (PM_MarkPackage(e, DPF_AUTOMARKED))
				ret |= UPDAVAIL_UPDATE;
		}
	}
#endif

	return ret;
}

#if defined(M_Menu_Prompt) || defined(SERVERONLY)
#else
static unsigned int PM_ChangeList(char *out, size_t outsize)
{
	unsigned int changes = 0;
	const char *change;
	package_t *p;
	size_t l;
	size_t ofs = 0;
	if (!outsize)
		out = NULL;
	else
		*out = 0;
	for (p = availablepackages; p; p=p->next)
	{
		if (!(p->flags & DPF_MARKED) != !(p->flags & DPF_ENABLED) || (p->flags & DPF_PURGE))
		{
			changes++;
			if (!out)
				continue;

			if (p->flags & DPF_MARKED)
			{
				if (p->flags & DPF_PURGE)
					change = va(" reinstall %s\n", p->name);
				else if (p->flags & DPF_PRESENT)
					change = va(" enable %s\n", p->name);
				else
					change = va(" install %s\n", p->name);
			}
			else if ((p->flags & DPF_PURGE) || !(p->qhash && (p->flags & DPF_PRESENT)))
				change = va(" uninstall %s\n", p->name);
			else
				change = va(" disable %s\n", p->name);

			l = strlen(change);
			if (ofs+l >= outsize)
			{
				Q_strncpyz(out, "Too many changes\n", outsize);
				out = NULL;

				break;
			}
			else
			{
				memcpy(out+ofs, change, l);
				ofs += l;
				out[ofs] = 0;
			}
		}
	}
	return changes;
}
#endif

static void PM_PrintChanges(void)
{
	qboolean changes = 0;
	package_t *p;
	for (p = availablepackages; p; p=p->next)
	{
		if (!(p->flags & DPF_MARKED) != !(p->flags & DPF_ENABLED) || (p->flags & DPF_PURGE))
		{
			changes++;
			if (p->flags & DPF_MARKED)
			{
				if (p->flags & DPF_PURGE)
					Con_Printf(" reinstall %s\n", p->name);
				else
					Con_Printf(" install %s\n", p->name);
			}
			else if ((p->flags & DPF_PURGE) || !(p->qhash && (p->flags & DPF_CACHED)))
				Con_Printf(" uninstall %s\n", p->name);
			else
				Con_Printf(" disable %s\n", p->name);
		}
	}
	if (!changes)
		Con_Printf("<no changes>\n");
	else
		Con_Printf("<%i package(s) changed>\n", changes);
}

#ifdef WEBCLIENT
static void PM_ListDownloaded(struct dl_download *dl)
{
	size_t listidx = dl->user_num;
	size_t sz = 0;
	char *f = NULL;
	if (dl->file && dl->status != DL_FAILED)
	{
		sz = VFS_GETLEN(dl->file);
		f = BZ_Malloc(sz+1);
		if (f)
		{
			f[sz] = 0;
			if (sz != VFS_READ(dl->file, f, sz))
			{	//err... weird...
				BZ_Free(f);
				f = NULL;
			}
			if (strlen(f) != sz)
			{	//don't allow mid-file nulls.
				BZ_Free(f);
				f = NULL;
			}
		}
	}

	if (dl != pm_source[listidx].curdl)
	{
		//this request looks stale.
		BZ_Free(f);
		return;
	}
	pm_source[listidx].curdl = NULL;

	//FIXME: validate a signature!

	if (f)
	{
		if (dl->replycode != 100)
			pm_source[listidx].status = SRCSTAT_OBTAINED;
		pm_sequence++;
		if (pm_source[listidx].flags & SRCFL_UNSAFE)
			PM_ParsePackageList(f, DPF_SIGNATUREACCEPTED, dl->url, pm_source[listidx].prefix);
		else
			PM_ParsePackageList(f, 0, dl->url, pm_source[listidx].prefix);
		PM_ResortPackages();
	}
	else if (dl->replycode == HTTP_DNSFAILURE)
		pm_source[listidx].status = SRCSTAT_FAILED_DNS;
	else if (dl->replycode == HTTP_NORESPONSE)
		pm_source[listidx].status = SRCSTAT_FAILED_NORESP;
	else if (dl->replycode == HTTP_REFUSED)
		pm_source[listidx].status = SRCSTAT_FAILED_REFUSED;
	else if (dl->replycode == HTTP_EOF)
		pm_source[listidx].status = SRCSTAT_FAILED_EOF;
	else if (dl->replycode == HTTP_MITM || dl->replycode == HTTP_UNTRUSTED)
		pm_source[listidx].status = SRCSTAT_FAILED_MITM;
	else if (dl->replycode && dl->replycode < 900)
		pm_source[listidx].status = SRCSTAT_FAILED_HTTP;
	else
		pm_source[listidx].status = SRCSTAT_FAILED_EOF;
	BZ_Free(f);

	if (!doautoupdate)
		return;	//don't spam this.
	if (pm_pendingprompts)
		return;
	//check if we're still waiting
	for (listidx = 0; listidx < pm_numsources; listidx++)
	{
		if (pm_source[listidx].status == SRCSTAT_PENDING)
			return;
	}

	//if our downloads finished and we want to shove it in the user's face then do so now.
	if (doautoupdate && listidx == pm_numsources)
	{
		int updates = PM_MarkUpdates();
		if (updates == UPDAVAIL_FORCED)
			PM_PromptApplyChanges();
		else if (updates)
		{
#ifdef DOWNLOADMENU
			if (!isDedicated)
			{
				Menu_PopAll();
				Cmd_ExecuteString("menu_download\n", RESTRICT_LOCAL);
			}
			else
#endif
				PM_PrintChanges();
		}
	}
}
static void PM_Plugin_Source_Finished(void *ctx, vfsfile_t *f)
{	//plugin closed its write end.
	struct pm_source_s *src = ctx;
	size_t idx = src-pm_source;
	if (idx < pm_numsources && ctx == &pm_source[idx])
	{
		COM_AssertMainThread("PM_Plugin_Source_Finished");
		if (!src->curdl)
		{
			struct dl_download dl;
			dl.file = f;
			dl.status = DL_FINISHED;
			dl.user_num = src-pm_source;
			dl.url = src->url;
			dl.replycode = 200;	//okay
			src->curdl = &dl;
			PM_ListDownloaded(&dl);
		}
	}
	VFS_CLOSE(f);
}
static void PM_Plugin_Source_CacheFinished(void *ctx, vfsfile_t *f)
{	//plugin closed its write end.
	struct pm_source_s *src = ctx;
	size_t idx = src-pm_source;
	if (idx < pm_numsources && ctx == &pm_source[idx])
	{
		COM_AssertMainThread("PM_Plugin_Source_CacheFinished");
		if (!src->curdl)
		{
			struct dl_download dl;
			dl.file = f;
			dl.status = DL_FINISHED;
			dl.user_num = src-pm_source;
			dl.url = src->url;
			dl.replycode = 100;
			src->curdl = &dl;
			PM_ListDownloaded(&dl);
		}
	}
	VFS_CLOSE(f);
}
#endif
#if defined(HAVE_CLIENT) && defined(WEBCLIENT)
static void PM_AllowPackageListQuery_Callback(void *ctx, promptbutton_t opt)
{
	unsigned int i;
	//something changed, let it download now.
	if (opt!=PROMPT_CANCEL)
	{
		allowphonehome = (opt==PROMPT_YES);

		for (i = 0; i < pm_numsources; i++)
		{
			if ((pm_source[i].flags & SRCFL_MANIFEST) && !(pm_source[i].flags & SRCFL_DISABLED))
				pm_source[i].flags |= SRCFL_ONCE;
		}
	}
	PM_UpdatePackageList(false);
}
#endif
//retry 1==
static void PM_UpdatePackageList(qboolean autoupdate)
{
	unsigned int i;

	PM_PreparePackageList();

#ifndef WEBCLIENT
	for (i = 0; i < pm_numsources; i++)
	{
		if (pm_source[i].status == SRCSTAT_PENDING)
			pm_source[i].status = SRCSTAT_FAILED_DNS;
	}
#else
	doautoupdate |= autoupdate;

	if (COM_CheckParm("-noupdate") || COM_CheckParm("-noupdates"))
		allowphonehome = false;
	#ifdef HAVE_CLIENT
	else if (pkg_autoupdate.ival >= 1)
		allowphonehome = true;
	else if (allowphonehome == -1)
	{
		if (doautoupdate)
			Menu_Prompt(PM_AllowPackageListQuery_Callback, NULL, localtext("Query updates list?\n"), "Okay", NULL, "Nope", true);
		return;
	}

	if (allowphonehome && PM_AreSourcesNew(true))
		return;
	#else
		allowphonehome = true; //erk.
	#endif

	if (pm_pendingprompts)
		return;
	autoupdate = doautoupdate;

	//kick off the initial tier of list-downloads.
	for (i = 0; i < pm_numsources; i++)
	{
		if (pm_source[i].flags & SRCFL_HISTORIC)
			continue;
		if (!(pm_source[i].flags & (SRCFL_ENABLED|SRCFL_ONCE)))
			continue;	//is not explicitly enabled. might be pending for user confirmation.
		autoupdate = false;
		if (pm_source[i].curdl)
			continue;

		if (allowphonehome<=0)
		{
			pm_source[i].status = SRCSTAT_UNTRIED;
			continue;
		}
		if (pm_source[i].status == SRCSTAT_OBTAINED)
			continue;	//already successful once. no need to do it again.
		pm_source[i].flags &= ~SRCFL_ONCE;

		if (pm_source[i].funcs)
		{
			pm_source[i].funcs->Update(pm_source[i].url, VFS_OpenPipeCallback(PM_Plugin_Source_Finished, &pm_source[i]), false);
		}
		else
		{
			pm_source[i].curdl = HTTP_CL_Get(pm_source[i].url, NULL, PM_ListDownloaded);
			if (pm_source[i].curdl)
			{
				pm_source[i].curdl->user_num = i;

				pm_source[i].curdl->file = VFSPIPE_Open(1, false);
				pm_source[i].curdl->isquery = true;
				DL_CreateThread(pm_source[i].curdl, NULL, NULL);
			}
			else
			{
				Con_Printf("Could not contact updates server - %s\n", pm_source[i].url);
				pm_source[i].status = SRCSTAT_FAILED_DNS;
			}
		}
	}
#endif

	if (autoupdate)
	{
		if (PM_MarkUpdates())
		{
#ifdef DOWNLOADMENU
			if (!isDedicated)
				Cbuf_AddText("menu_download\n", RESTRICT_LOCAL);
			else
#endif
				PM_PrintChanges();
		}
	}
}

qboolean PM_RegisterUpdateSource(void *module, plugupdatesourcefuncs_t *funcs)
{
	size_t i;
	if (!funcs)
	{
		for (i = 0; i < pm_numsources; i++)
		{
			if (pm_source[i].module == module)
			{
				pm_source[i].module = NULL;
				pm_source[i].funcs = NULL;
			}
		}
	}
	else
		PM_AddSubListModule(module, funcs, va("plug:%s", funcs->description), NULL, SRCFL_PLUGIN|SRCFL_ENABLED);	//plugin sources default to enabled as you can always just disable the plugin that provides it.
	return true;
}

static void COM_QuotedConcat(const char *cat, char *buf, size_t bufsize)
{
	const unsigned char *gah;
	for (gah = (const unsigned char*)cat; *gah; gah++)
	{
		if (*gah <= ' ' || *gah == '$' || *gah == '\"' || *gah == '\n' || *gah == '\r')
			break;
	}
	if (*gah || *cat == '\\' ||
		strstr(cat, "//") || strstr(cat, "/*"))
	{	//contains some dodgy stuff.
		size_t curlen = strlen(buf);
		buf += curlen;
		bufsize -= curlen;
		COM_QuotedString(cat, buf, bufsize, false);
	}
	else
	{	//okay, no need for quotes.
		Q_strncatz(buf, cat, bufsize);
	}
}
static void COM_QuotedKeyVal(const char *key, const char *val, char *buf, size_t bufsize)
{
	size_t curlen;

	Q_strncatz(buf, "\t\"", bufsize);

	curlen = strlen(buf);
	buf += curlen;
	bufsize -= curlen;
	COM_QuotedString(key, buf, bufsize, true);

	if (strlen(key) <= 5)
		Q_strncatz(buf, "\"\t\t\"", bufsize);
	else
		Q_strncatz(buf, "\"\t\"", bufsize);

	curlen = strlen(buf);
	buf += curlen;
	bufsize -= curlen;
	COM_QuotedString(val, buf, bufsize, true);

	Q_strncatz(buf, "\"\n", bufsize);
}
static void PM_WriteInstalledPackage_v3(package_t *p, char *buf, size_t bufsize)
{
	struct packagedep_s *dep;

	buf[0] = '{';
	buf[1] = '\n';
	buf[2] = 0;
	COM_QuotedKeyVal("package", p->name, buf, bufsize);
	COM_QuotedKeyVal("category", p->category, buf, bufsize);
	if (p->flags & DPF_ENABLED)
		COM_QuotedKeyVal("enabled", "1", buf, bufsize);
	if (p->flags & DPF_GUESSED)
		COM_QuotedKeyVal("guessed", "1", buf, bufsize);
	if (p->flags & DPF_TRUSTED)
		COM_QuotedKeyVal("trusted", "1", buf, bufsize);
	if (*p->title && strcmp(p->title, p->name))
		COM_QuotedKeyVal("title", p->title, buf, bufsize);
	if (*p->version)
		COM_QuotedKeyVal("ver", p->version, buf, bufsize);
	COM_QuotedKeyVal("gamedir", p->gamedir, buf, bufsize);
	if (p->qhash)
		COM_QuotedKeyVal("qhash", p->qhash, buf, bufsize);
	if (p->priority!=PM_DEFAULTPRIORITY)
		COM_QuotedKeyVal("priority", va("%i", p->priority), buf, bufsize);
	if (p->arch)
		COM_QuotedKeyVal("arch", p->arch, buf, bufsize);

	if (p->license)
		COM_QuotedKeyVal("license", p->license, buf, bufsize);
	if (p->website)
		COM_QuotedKeyVal("website", p->website, buf, bufsize);
	if (p->author)
		COM_QuotedKeyVal("author", p->author, buf, bufsize);
	if (p->description)
		COM_QuotedKeyVal("desc", p->description, buf, bufsize);
	if (p->previewimage)
		COM_QuotedKeyVal("preview", p->previewimage, buf, bufsize);
	if (p->filesize)
		COM_QuotedKeyVal("filesize", va("%"PRIu64, p->filesize), buf, bufsize);

	if (p->fsroot == FS_BINARYPATH)
		COM_QuotedKeyVal("root", "bin", buf, bufsize);
	else if (p->fsroot == FS_LIBRARYPATH)
		COM_QuotedKeyVal("root", "lib", buf, bufsize);
	if (p->packprefix)
		COM_QuotedKeyVal("packprefix", p->packprefix, buf, bufsize);

	for (dep = p->deps; dep; dep = dep->next)
	{
		safeswitch(dep->dtype)
		{
		case DEP_FILE:			COM_QuotedKeyVal("file",		dep->name, buf, bufsize); break;
		case DEP_CACHEFILE:		COM_QuotedKeyVal("cachefile",	dep->name, buf, bufsize); break;
		case DEP_MAP:			COM_QuotedKeyVal("map",			dep->name, buf, bufsize); break;
		case DEP_REQUIRE:		COM_QuotedKeyVal("depend",		dep->name, buf, bufsize); break;
		case DEP_CONFLICT:		COM_QuotedKeyVal("conflict",	dep->name, buf, bufsize); break;
		case DEP_REPLACE:		COM_QuotedKeyVal("replace",		dep->name, buf, bufsize); break;
		case DEP_FILECONFLICT:	COM_QuotedKeyVal("fileconflict",dep->name, buf, bufsize); break;
		case DEP_RECOMMEND:		COM_QuotedKeyVal("recommend",	dep->name, buf, bufsize); break;
		case DEP_NEEDFEATURE:	COM_QuotedKeyVal("need",		dep->name, buf, bufsize); break;
		case DEP_SUGGEST:		COM_QuotedKeyVal("suggest",		dep->name, buf, bufsize); break;
		case DEP_SOURCE:		COM_QuotedKeyVal("source",		dep->name, buf, bufsize); break;
		case DEP_EXTRACTNAME:	COM_QuotedKeyVal("unzipfile",	dep->name, buf, bufsize); break;
		safedefault:
			break;
		}
	}

	if (p->flags & DPF_TESTING)
		COM_QuotedKeyVal("test", "1", buf, bufsize);

	if ((p->flags & DPF_AUTOMARKED) && !(p->flags & DPF_USERMARKED))
		COM_QuotedKeyVal("auto", "1", buf, bufsize);

	Q_strncatz(buf, "}", bufsize);
	Q_strncatz(buf, "\n", bufsize);
}
static void PM_WriteInstalledPackage_v2(package_t *p, char *buf, size_t bufsize)
{
	struct packagedep_s *dep;

	buf[0] = 0;
	COM_QuotedString(va("%s%s", p->category, p->name), buf, bufsize, false);
	if (p->flags & DPF_ENABLED)
	{	//v3+
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("enabled=1"), buf, bufsize);
	}
	else
	{	//v2
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("stale=1"), buf, bufsize);
	}
	if (p->flags & DPF_TRUSTED)
	{	//v3+. was signed when installed.
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("trusted=1"), buf, bufsize);
	}
	if (p->flags & DPF_GUESSED)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("guessed=1"), buf, bufsize);
	}
	if (*p->title && strcmp(p->title, p->name))
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("title=%s", p->title), buf, bufsize);
	}
	if (*p->version)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("ver=%s", p->version), buf, bufsize);
	}
	//if (*p->gamedir)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("gamedir=%s", p->gamedir), buf, bufsize);
	}
	if (p->qhash)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("qhash=%s", p->qhash), buf, bufsize);
	}
	if (p->priority!=PM_DEFAULTPRIORITY)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("priority=%i", p->priority), buf, bufsize);
	}
	if (p->arch)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("arch=%s", p->arch), buf, bufsize);
	}

	if (p->license)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("license=%s", p->license), buf, bufsize);
	}
	if (p->website)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("website=%s", p->website), buf, bufsize);
	}
	if (p->author)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("author=%s", p->author), buf, bufsize);
	}
	if (p->description)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("desc=%s", p->description), buf, bufsize);
	}
	if (p->previewimage)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("preview=%s", p->previewimage), buf, bufsize);
	}
	if (p->filesize)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("filesize=%"PRIu64, p->filesize), buf, bufsize);
	}

	if (p->fsroot == FS_BINARYPATH)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat("root=bin", buf, bufsize);
	}
	if (p->packprefix)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat(va("packprefix=%s", p->packprefix), buf, bufsize);
	}

	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_FILE)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("file=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_CACHEFILE)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("cachefile=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_MAP)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("map=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_REQUIRE)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("depend=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_CONFLICT)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("conflict=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_REPLACE)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("replace=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_FILECONFLICT)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("fileconflict=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_RECOMMEND)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("recommend=%s", dep->name), buf, bufsize);
		}
		else if (dep->dtype == DEP_NEEDFEATURE)
		{
			Q_strncatz(buf, " ", bufsize);
			COM_QuotedConcat(va("need=%s", dep->name), buf, bufsize);
		}
	}

	if (p->flags & DPF_TESTING)
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat("test=1", buf, bufsize);
	}

	if ((p->flags & DPF_AUTOMARKED) && !(p->flags & DPF_USERMARKED))
	{
		Q_strncatz(buf, " ", bufsize);
		COM_QuotedConcat("auto", buf, bufsize);
	}

	buf[bufsize-2] = 0;	//just in case.
	Q_strncatz(buf, "\n", bufsize);
}
static void PM_WriteInstalledPackages(void)
{
	char buf[65536];
	int i;
	char *s;
	package_t *p;
	vfsfile_t *f = FS_OpenVFS(INSTALLEDFILES, "wbp", FS_ROOT);
	qboolean v3 = false;
	if (!f)
	{
		if (FS_DisplayPath(INSTALLEDFILES, FS_ROOT, buf, sizeof(buf)))
			Con_Printf("package manager: Can't write %s\n", buf);
		else
			Con_Printf("package manager: Can't update installed list\n");
		return;
	}

	if (v3)
		s = "version 3\n";
	else
		s = "version 2\n";
	VFS_WRITE(f, s, strlen(s));

	s = va("set updatemode %s\n", COM_QuotedString(pkg_autoupdate.string, buf, sizeof(buf), false));
	VFS_WRITE(f, s, strlen(s));
	s = va("set declined %s\n", COM_QuotedString(declinedpackages?declinedpackages:"", buf, sizeof(buf), false));
	VFS_WRITE(f, s, strlen(s));

	for (i = 0; i < pm_numsources; i++)
	{
		char *status;
		if (!(pm_source[i].flags & (SRCFL_DISABLED|SRCFL_ENABLED)))
			continue;	//don't bother saving sources which the user has neither confirmed nor denied.

		if (pm_source[i].flags & SRCFL_ENABLED)
			status = "enabled";	//anything else is enabled
		else
			status = "disabled";

		if (pm_source[i].flags & SRCFL_USER)	//make sure it works.
			s = va("sublist \"%s\" \"%s\" \"%s\"\n", pm_source[i].url, pm_source[i].prefix, status);
		else	//will be 'historic' when loaded
			s = va("source \"%s\" \"%s\"\n", pm_source[i].url, status);
		VFS_WRITE(f, s, strlen(s));
	}

	for (p = availablepackages; p ; p=p->next)
	{
		if (p->flags & (DPF_PRESENT|DPF_ENABLED))
		{
			if (v3)
				PM_WriteInstalledPackage_v3(p, buf, sizeof(buf));
			else
				PM_WriteInstalledPackage_v2(p, buf, sizeof(buf));

			VFS_WRITE(f, buf, strlen(buf));
		}
	}

	VFS_CLOSE(f);
}

//package has been downloaded and installed, but some packages need to be enabled
//(plugins might have other dll dependancies, so this can only happen AFTER the entire package was extracted)
static void PM_PackageEnabled(package_t *p)
{
	char ext[8];
	struct packagedep_s *dep;
#ifdef HAVEAUTOUPDATE
	struct packagedep_s *ef = NULL;
#endif
	FS_FlushFSHashFull();
	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype != DEP_FILE && dep->dtype != DEP_CACHEFILE)
			continue;
		COM_FileExtension(dep->name, ext, sizeof(ext));
/*this is now done after all downloads completed, so we don't keep re-execing configs nor forgetting that they changed.
		if (!pm_packagesinstalled)
		if (!stricmp(ext, "pak") || !stricmp(ext, "pk3") || !stricmp(ext, "zip"))
		{
			if (pm_packagesinstalled)
			{
				pm_packagesinstalled = false;
				FS_ChangeGame(fs_manifest, true, false);
			}
			else
				FS_ReloadPackFiles();
		}*/
#ifdef PLUGINS
		if ((p->flags & DPF_PLUGIN) && !Q_strncasecmp(dep->name, PLUGINPREFIX, strlen(PLUGINPREFIX)))
			Cmd_ExecuteString(va("plug_load %s\n", dep->name), RESTRICT_LOCAL);
#endif
#ifdef MENU_DAT
		if (!Q_strcasecmp(dep->name, "menu.dat"))
			Cmd_ExecuteString("menu_restart\n", RESTRICT_LOCAL);
#endif
#ifdef HAVEAUTOUPDATE
		if (p->flags & DPF_ENGINE)
			ef = dep;
#endif
	}

#ifdef HAVEAUTOUPDATE
	//this is an engine update (with installed file) and marked.
	if (ef && (p->flags & DPF_MARKED))
	{
		char native[MAX_OSPATH];
		package_t *othr;
		//make sure there's no more recent build that's also enabled...
		for (othr = availablepackages; othr ; othr=othr->next)
		{
			if ((othr->flags & DPF_ENGINE) && (othr->flags & DPF_MARKED) && othr->flags & (DPF_PRESENT|DPF_ENABLED) && othr != p)
				if (strcmp(p->version, othr->version) >= 0)
					return;
		}

#ifndef HAVE_CLIENT
#define Menu_Prompt(cb,ctx,msg,yes,no,cancel,highpri) Con_Printf(CON_WARNING "%s\n", msg)
#endif

		if (FS_SystemPath(ef->name, p->fsroot, native, sizeof(native)) && Sys_SetUpdatedBinary(native))
		{
			Q_strncpyz(enginerevision, p->version, sizeof(enginerevision));	//make sure 'revert' picks up the new binary...
			Menu_Prompt(NULL, NULL, localtext("Engine binary updated.\nRestart to use."), NULL, NULL, NULL, true);
		}
		else
			Menu_Prompt(NULL, NULL, localtext("Engine update failed.\nManual update required."), NULL, NULL, NULL, true);
	}
#endif
}

#ifdef WEBCLIENT
//callback from PM_Download_Got, extracts each file from an archive
static int QDECL PM_ExtractFiles(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{	//this is gonna suck. threading would help, but gah.
	package_t *p = parm;
	flocation_t loc;
	if (fname[strlen(fname)-1] == '/')
	{	//directory.

	}
	else if (spath->FindFile(spath, &loc, fname, NULL) && loc.len < 0x80000000u)
	{
		char *f = malloc(loc.len);
		const char *n;
		if (f)
		{
			spath->ReadFile(spath, &loc, f);
			if (*p->gamedir)
				n = va("%s/%s", p->gamedir, fname);
			else
				n = fname;
			if (FS_WriteFile(n, f, loc.len, p->fsroot))
				p->flags |= DPF_NATIVE|DPF_ENABLED;

			//keep track of the installed files, so we can delete them properly after.
			PM_AddDep(p, DEP_FILE, fname);
		}
		free(f);
	}
	return 1;
}

static qboolean PM_Download_Got_Extract(package_t *p, searchpathfuncs_t *archive, const char *tempname, enum fs_relative temproot)
{	//EXTRACT_EXPLICITZIP is very special.
	qboolean success = false;
	char displayname[MAX_OSPATH];
	char ext[8];
	char *destname;
	struct packagedep_s *dep, *srcname = p->deps;

	for (dep = p->deps; dep; dep = dep->next)
	{
		unsigned int nfl;
		if (dep->dtype != DEP_FILE && dep->dtype != DEP_CACHEFILE)
			continue;

		COM_FileExtension(dep->name, ext, sizeof(ext));
		if (!stricmp(ext, "pak") || !stricmp(ext, "pk3") || !stricmp(ext, "zip"))
			FS_UnloadPackFiles();	//we reload them after
#ifdef PLUGINS
		if ((!stricmp(ext, "dll") || !stricmp(ext, "so")) && !Q_strncmp(dep->name, PLUGINPREFIX, strlen(PLUGINPREFIX)))
			Cmd_ExecuteString(va("plug_close %s\n", dep->name), RESTRICT_LOCAL);	//try to purge plugins so there's no files left open
#endif

		if (dep->dtype == DEP_CACHEFILE)
		{
			nfl = DPF_CACHED;
			destname = va("downloads/%s", dep->name);
		}
		else
		{
			nfl = DPF_NATIVE;
			if (!*p->gamedir)	//basedir
				destname = dep->name;
			else
			{
				char temp[MAX_OSPATH];
				destname = va("%s/%s", p->gamedir, dep->name);
				if (PM_TryGenCachedName(destname, p, temp, sizeof(temp)))
				{
					nfl = DPF_CACHED;
					destname = va("%s", temp);
				}
			}
		}
		if (p->flags & DPF_MARKED)
			nfl |= DPF_ENABLED;
		nfl |= (p->flags & ~(DPF_CACHED|DPF_NATIVE|DPF_CORRUPT));
		FS_CreatePath(destname, p->fsroot);
		if (FS_Remove(destname, p->fsroot))
			;
		if (p->extract == EXTRACT_EXPLICITZIP)
		{
			while (srcname && srcname->dtype != DEP_EXTRACTNAME)
				srcname = srcname->next;
			if (!archive)
				Con_Printf(CON_ERROR"archive %s is not an archive!\n", tempname);
			else if (!srcname)
				Con_Printf(CON_ERROR"archive %s is EXTRACT_EXPLICITZIP but filename not specified!\n", tempname);
			else
			{
				flocation_t loc;

				if (archive->FindFile(archive, &loc, srcname->name, NULL)==FF_FOUND && loc.len < 0x80000000u)
				{
					char *f = malloc(loc.len);
					if (f)
					{
						archive->ReadFile(archive, &loc, f);
						if (FS_WriteFile(destname, f, loc.len, p->fsroot))
						{
							if (!FS_DisplayPath(destname, p->fsroot, displayname, sizeof(displayname)))
								Q_strncpyz(displayname, destname, sizeof(displayname));
							Con_Printf(CON_WARNING"Extracted %s (to %s)\n", p->name, displayname);

							p->flags = nfl;
							success = true;
							continue;
						}
						else
						{
							if (!FS_DisplayPath(destname, p->fsroot, displayname, sizeof(displayname)))
								Q_strncpyz(displayname, destname, sizeof(displayname));
							Con_Printf(CON_ERROR"Failed to write %s (extracting %s)\n", displayname, p->name);
						}
					}
				}
				else
					Con_Printf(CON_ERROR"Failed to find %s in archive %s\n", srcname->name, tempname);
			}

			if (!FS_DisplayPath(destname, p->fsroot, displayname, sizeof(displayname)))
				Q_strncpyz(displayname, destname, sizeof(displayname));
			Con_Printf(CON_ERROR"Couldn't extract %s/%s to %s. Removed instead.\n", tempname, dep->name, displayname);

			success = false;
		}
		else if (!FS_Rename2(tempname, destname, temproot, p->fsroot))
		{
			//error!
			if (!FS_DisplayPath(destname, p->fsroot, displayname, sizeof(displayname)))
				Q_strncpyz(displayname, destname, sizeof(displayname));
			Con_Printf(CON_ERROR"Couldn't rename %s to %s. Removed instead.\n", tempname, displayname);

			success = false;
		}
		else
		{	//success!
			if (!FS_DisplayPath(destname, p->fsroot, displayname, sizeof(displayname)))
				Q_strncpyz(displayname, destname, sizeof(displayname));
			Con_Printf("Downloaded %s (to %s)\n", p->name, displayname);

			p->flags = nfl;

			success = true;
		}
	}

	return success;
}

static qboolean PM_DownloadSharesSource(package_t *p1, package_t *p2)
{
	if (p1 == p2)
		return false;	//well yes... but no. just no.
	if (p1->extract != p2->extract)
		return false;
	if (p1->extract != EXTRACT_EXPLICITZIP)
		return false;	//others can't handle it, or probably don't make much sense. don't try, too unsafe/special-case.
	//these are on the download, not the individual files to extract, awkwardly enough.
	if (p1->filesize != p2->filesize)
		return false;
	if (!p1->filesha1 != !p2->filesha1)
		return false;
	if (p1->filesha1 && strcmp(p1->filesha1, p2->filesha1))
		return false;
	if (!p1->filesha512 != !p2->filesha512)
		return false;
	if (p1->filesha512 && strcmp(p1->filesha512, p2->filesha512))
		return false;

	return true;
}

static void PM_StartADownload(void);
typedef struct
{
	package_t *p;
	qboolean successful;
	char *tempname;	//z_strduped string, so needs freeing.
	enum fs_relative temproot;
	char localname[256];
	char url[256];
} pmdownloadedinfo_t;
//callback from PM_StartADownload
static void PM_Download_Got(int iarg, void *data)
{
	pmdownloadedinfo_t *info = data;
	qboolean successful = info->successful;
	package_t *p, *p2;
	char *tempname = info->tempname;
	const enum fs_relative temproot = info->temproot;

	for (p = availablepackages; p ; p=p->next)
	{
		if (p == info->p)
			break;
	}
	pm_packagesinstalled=true;

	if (p)
	{
		p->flags &= ~DPF_PENDING;
		p->curdownload = NULL;	//just in case.

		if (!successful)
		{
			Con_Printf(CON_ERROR"Couldn't download %s (from %s)\n", p->name, info->url);
			FS_Remove (tempname, temproot);
			Z_Free(tempname);
			PM_StartADownload();
			return;
		}

		if (p->extract == EXTRACT_ZIP)
		{
			searchpathfuncs_t *archive = NULL;
#ifdef PACKAGE_PK3
			vfsfile_t *f = FS_OpenVFS(tempname, "rb", temproot);
			if (f)
			{
				archive = FSZIP_LoadArchive(f, NULL, tempname, tempname, NULL);
				if (!archive)
					VFS_CLOSE(f);
			}
#else
			Con_Printf("zip format not supported in this build - %s (from %s)\n", p->name, dl->url);
#endif
			if (archive)
			{
				p->flags &= ~(DPF_NATIVE|DPF_CACHED|DPF_CORRUPT|DPF_ENABLED);
				//FIXME: add an EnumerateAllFiles function or something.
				//can't scan for directories - paks don't have em, zips don't guarentee em either.
				archive->EnumerateFiles(archive, "*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*/*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*/*/*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*/*/*/*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*/*/*/*/*/*", PM_ExtractFiles, p);
				archive->EnumerateFiles(archive, "*/*/*/*/*/*/*/*/*", PM_ExtractFiles, p);
				archive->ClosePath(archive);

				PM_WriteInstalledPackages();

//					if (!stricmp(ext, "pak") || !stricmp(ext, "pk3"))
//						FS_ReloadPackFiles();
			}
			PM_ValidatePackage(p);
			FS_Remove (tempname, temproot);
			Z_Free(tempname);
			p->trymirrors = 0;	//we're done... don't download it again!
			PM_StartADownload();
			return;
		}
		else
		{
			qboolean success = false;
			searchpathfuncs_t *archive = NULL;
#ifdef PACKAGE_PK3
			if (p->extract == EXTRACT_EXPLICITZIP)
			{
				vfsfile_t *f = FS_OpenVFS(tempname, "rb", temproot);
				if (f)
				{
					archive = FSZIP_LoadArchive(f, NULL, tempname, tempname, NULL);
					if (!archive)
						VFS_CLOSE(f);
				}
				success = PM_Download_Got_Extract(p, archive, tempname, temproot);

				if (success)
				for (p2 = availablepackages; p2; p2=p2->next)
				{
					if ((p2->flags&DPF_PENDING) ||	//only if they've not already started downloading separately...
						!p2->trymirrors	//ignore ones that are not pending.
						)
						continue;

					if (PM_DownloadSharesSource(p, p2))
						if (PM_Download_Got_Extract(p2, archive, tempname, temproot))
						{
							p2->trymirrors = false;	//already did it. mwahaha.
							PM_ValidatePackage(p2);
							PM_PackageEnabled(p2);
						}
				}
			}
			else
#endif
				success = PM_Download_Got_Extract(p, archive, tempname, temproot);
			if (archive)
				archive->ClosePath(archive);
			if (p->extract == EXTRACT_EXPLICITZIP || !success)
				FS_Remove (tempname, temproot);
			if (success)
			{
				PM_ValidatePackage(p);

				PM_PackageEnabled(p);
				PM_WriteInstalledPackages();

				Z_Free(tempname);

				p->trymirrors = 0;	//we're done with this one... don't download it from another mirror!
				PM_StartADownload();
				return;
			}
		}
		Con_Printf(CON_ERROR"PM_Download_Got: %s has no filename info\n", p->name);
	}
	else
		Con_Printf(CON_ERROR"PM_Download_Got: Can't figure out where \"%s\"(%s) came from (url: %s)\n", info->localname, tempname, info->url);

	FS_Remove (tempname, temproot);
	Z_Free(tempname);
	PM_StartADownload();
}
static void PM_Download_PreliminaryGot(struct dl_download *dl)
{	//this function is annoying.
	//we're on the mainthread, but we might still be waiting for some other thread to complete
	//there could be loads of stuff on the callstack. lots of stuff that could get annoyed if we're restarting the entire filesystem, for instance.
	//so set up a SECOND callback using a different mechanism...

	pmdownloadedinfo_t info;
	info.tempname = dl->user_ctx;
	info.temproot = dl->user_num;

	Q_strncpyz(info.url, dl->url, sizeof(info.url));
	Q_strncpyz(info.localname, dl->localname, sizeof(info.localname));

	for (info.p = availablepackages; info.p ; info.p=info.p->next)
	{
		if (info.p->curdownload == dl)
		{
			info.p->curdownload = NULL;	//have to clear it here, because this pointer will no longer be valid after (which could result in segfaults)
			break;
		}
	}

	info.successful = (dl->status == DL_FINISHED);
	if (dl->file)
	{
		if (!VFS_CLOSE(dl->file))
			info.successful = false;
		dl->file = NULL;
	}
	else
		info.successful = false;

	Cmd_AddTimer(0, PM_Download_Got, 0, &info, sizeof(info));
}

static char *PM_GetTempName(package_t *p)
{
	struct packagedep_s *dep, *fdep;
	char *destname, *t, *ts;
	//always favour the file so that we can rename safely without needing a copy.
	for (dep = p->deps, fdep = NULL; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_FILE || dep->dtype == DEP_CACHEFILE)
		{
			if (fdep)
			{
				fdep = NULL;
				break;
			}
			fdep = dep;
		}
	}
	if (fdep)
	{
		if (fdep->dtype == DEP_CACHEFILE)
			destname = va("downloads/%s.tmp", fdep->name);
		else if (*p->gamedir)
			destname = va("%s/%s.tmp", p->gamedir, fdep->name);
		else
			destname = va("%s.tmp", fdep->name);
		return Z_StrDup(destname);
	}
	ts = Z_StrDup(p->name);
	for (t = ts; *t; t++)
	{
		switch(*t)
		{
		case '/':
		case '?':
		case '<':
		case '>':
		case '\\':
		case ':':
		case '*':
		case '|':
		case '\"':
		case '.':
			*t = '_';
			break;
		default:
			break;
		}
	}
	if (*ts)
	{
		if (*p->gamedir)
			destname = va("%s/%s.tmp", p->gamedir, ts);
		else
			destname = va("%s.tmp", ts);
	}
	else
		destname = va("%x.tmp", (unsigned int)(quintptr_t)p);
	Z_Free(ts);
	return Z_StrDup(destname);
}


typedef struct {
	vfsfile_t pub;
	vfsfile_t *f;
	hashfunc_t *hashfunc;
	qofs_t sz;
	qofs_t needsize;
	qboolean fail;
	qbyte need[DIGEST_MAXSIZE];
	char *fname;
	qbyte ctx[1];
} hashfile_t;
static int QDECL HashFile_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestowrite)
{
	hashfile_t *f = (hashfile_t*)file;
	f->hashfunc->process(f->ctx, buffer, bytestowrite);
	if (bytestowrite != VFS_WRITE(f->f, buffer, bytestowrite))
		f->fail = true;	//something went wrong.
	if (f->fail)
		return -1;	//error! abort! fail! give up!
	f->sz += bytestowrite;
	return bytestowrite;
}
static void QDECL HashFile_Flush (struct vfsfile_s *file)
{
	hashfile_t *f = (hashfile_t*)file;
	VFS_FLUSH(f->f);
}
static qboolean QDECL HashFile_Close (struct vfsfile_s *file)
{
	qbyte digest[DIGEST_MAXSIZE];
	hashfile_t *f = (hashfile_t*)file;
	if (!VFS_CLOSE(f->f))
		f->fail = true;	//something went wrong.
	f->f = NULL;

	f->hashfunc->terminate(digest, f->ctx);
	if (f->fail)
		Con_Printf("Filesystem problem saving %s during download\n", f->fname);	//don't error if we failed on actual disk problems
	else if (f->sz != f->needsize)
	{
		Con_Printf("Download truncated: %s\n", f->fname);	//don't error if we failed on actual disk problems
		f->fail = true;
	}
	else if (memcmp(digest, f->need, f->hashfunc->digestsize))
	{
		qbyte base64[(DIGEST_MAXSIZE*2)+16];
		Con_Printf("Invalid hash for downloaded file %s, try again later?\n", f->fname);

		if (f->hashfunc == &hash_sha1)
		{
			base64[Base16_EncodeBlock(digest, f->hashfunc->digestsize, base64, sizeof(base64)-1)] = 0;
			Con_Printf("%s vs ", base64);
			base64[Base16_EncodeBlock(f->need, f->hashfunc->digestsize, base64, sizeof(base64)-1)] = 0;
			Con_Printf("%s\n", base64);
		}
		else
		{
			base64[Base64_EncodeBlock(digest, f->hashfunc->digestsize, base64, sizeof(base64)-1)] = 0;
			Con_Printf("%s vs ", base64);
			base64[Base64_EncodeBlock(f->need, f->hashfunc->digestsize, base64, sizeof(base64)-1)] = 0;
			Con_Printf("%s\n", base64);
		}
		f->fail = true;
	}

	return !f->fail;	//true if all okay!
}
static vfsfile_t *FS_Hash_ValidateWrites(vfsfile_t *f, const char *fname, qofs_t needsize, hashfunc_t *hashfunc, const char *hash)
{	//wraps a writable file with a layer that'll cause failures when the hash differs from what we expect.
	if (f)
	{
		hashfile_t *n = Z_Malloc(sizeof(*n) + hashfunc->contextsize + strlen(fname));
		n->pub.WriteBytes = HashFile_WriteBytes;
		n->pub.Flush = HashFile_Flush;
		n->pub.Close = HashFile_Close;
		n->pub.seekstyle = SS_UNSEEKABLE;
		n->f = f;
		n->hashfunc = hashfunc;
		n->fname = n->ctx+hashfunc->contextsize;
		strcpy(n->fname, fname);
		n->needsize = needsize;
		Base16_DecodeBlock(hash, n->need, sizeof(n->need));
		n->fail = false;

		n->hashfunc->init(n->ctx);

		f = &n->pub;
	}
	return f;
}

//function that returns true if the package doesn't look exploity.
//so either its a versioned package, or its got a trusted signature.
static qboolean PM_SignatureOkay(package_t *p)
{
	struct packagedep_s *dep;
	const char *ext;

	if (p->flags & DPF_PRESENT)
		return true;	//we don't know where it came from, but someone manually installed it...
	if (p->flags & (DPF_SIGNATUREREJECTED|DPF_SIGNATUREUNKNOWN))	//the sign key didn't match its sha512 hash
		return false;	//just block it entirely.
	if (p->flags & DPF_SIGNATUREACCEPTED)	//sign value is present and correct
		return true;	//go for it.

	if (p->flags & DPF_TRUSTED)
		return true;

	//packages without a signature are only allowed under some limited conditions.
	//basically we only allow meta packages, pk3s, and paks.

	//metadata doesn't specify all file names for zips.
	if (p->extract == EXTRACT_ZIP)
		return false;
	if (!*p->gamedir)
		return false;

	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype != DEP_FILE && dep->dtype != DEP_CACHEFILE)
			continue;

		//only allow .pak/.pk3/.zip without a signature, and only when they have a qhash specified (or the .fmf specified it without a qhash...).
		ext = COM_GetFileExtension(dep->name, NULL);
		if ((!stricmp(ext, ".kpf") || !stricmp(ext, ".pak") || !stricmp(ext, ".pk3") || !stricmp(ext, ".zip")) && (p->qhash || dep->dtype == DEP_CACHEFILE || (p->flags&DPF_MANIFEST)))
			;
		else
			return false;
	}
	return true;
}
#endif

/*static void PM_AddDownloadedPackage(const char *filename)
{
	char pathname[1024];
	package_t *p;
	Q_snprintfz(pathname, sizeof(pathname), "%s/%s", "Cached", filename);
	p->name = Z_StrDup(COM_SkipPath(pathname));
	*COM_SkipPath(pathname) = 0;
	p->category = Z_StrDup(pathname);

	Q_strncpyz(p->version, "", sizeof(p->version));

	Q_snprintfz(p->gamedir, sizeof(p->gamedir), "%s", "");
	p->fsroot = FS_ROOT;
	p->extract = EXTRACT_COPY;
	p->priority = 0;
	p->flags = DPF_INSTALLED;

	p->title = Z_StrDup(p->name);
	p->arch = NULL;
	p->qhash = NULL; //FIXME
	p->description = NULL;
	p->license = NULL;
	p->author = NULL;
	p->previewimage = NULL;
}*/

#ifdef WEBCLIENT
static size_t PM_AddFilePackage(const char *packagename, struct gamepacks *gp, size_t numgp)
{
	size_t found = 0;
	struct packagedep_s *dep;
	package_t *p = PM_FindPackage(packagename);
	if (!p)
		return 0;

	if (found < numgp)
	{
		if (p->arch)
			gp[found].package = Z_StrDupf("%s:%s=%s", p->name, p->arch, p->version);
		else
			gp[found].package = Z_StrDupf("%s=%s", p->name, p->version);
		gp[found].path = NULL;
		gp[found].url = p->mirror[0];	//FIXME: random mirror.
		for (dep = p->deps; dep; dep = dep->next)
		{
			if (dep->dtype == DEP_CACHEFILE)
				gp[found].path = Z_StrDupf("downloads/%s", dep->name);
		}
		if (gp[found].path)
		{
			gp[found].subpath = p->packprefix;
			found++;
		}
	}
	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_REQUIRE)
			found += PM_AddFilePackage(dep->name, gp+found, numgp);
	}
	return found;
}
static void PM_DownloadsCompleted(int iarg, void *data)
{	//if something installed, then make sure everything is reconfigured properly.
	if (pm_packagesinstalled || pm_onload.package || pm_onload.map)
	{
		pm_packagesinstalled = false;

		if (pm_onload.package)
		{
			package_t *p = PM_FindPackage(pm_onload.package);
			char *map = pm_onload.map;

			struct gamepacks packs[64];
			size_t usedpacks;
			pm_onload.map = NULL;

			usedpacks = PM_AddFilePackage(pm_onload.package, packs, countof(packs)-1);
			memset(&packs[usedpacks], 0, sizeof(packs[usedpacks]));

			Z_Free(pm_onload.package);
			pm_onload.package = NULL;
			COM_Gamedir(p->gamedir, packs);
			Cbuf_InsertText(map, RESTRICT_LOCAL, false);
			Z_Free(map);
		}
		else
			FS_ChangeGame(fs_manifest, true, false);
	}
}

static unsigned int PM_DownloadingCount(void)
{
	package_t *p;
	unsigned int count = 0;
	for (p = availablepackages; p ; p=p->next)
	{
		if (p->flags&DPF_PENDING)
			count++;
	}
	return count;
}

//looks for the next package that needs downloading, and grabs it
static void PM_StartADownload(void)
{
	vfsfile_t *tmpfile;
	char *temp;
	enum fs_relative temproot;
	package_t *p;
	const int simultaneous = 2;
	int i;
	qboolean downloading = false;

	for (p = availablepackages; p && simultaneous > PM_DownloadingCount(); p=p->next)
	{
		if (p->curdownload)
			downloading = true;
		else if (p->trymirrors)
		{	//flagged for a (re?)download
			char *mirror = NULL;
			for (i = 0; i < countof(p->mirror); i++)
			{
				if (p->mirror[i] && (p->trymirrors & (1u<<i)))
				{
					mirror = p->mirror[i];
					p->trymirrors &= ~(1u<<i);
					break;
				}
			}
			if (!mirror)
			{	//erk...
				p->trymirrors = 0;

				for (i = 0; i < countof(p->mirror); i++)
					if (p->mirror[i])
						break;
				if (i == countof(p->mirror))
				{	//this appears to be a meta package with no download
					//just directly install it.
					if (PM_HasDep(p, DEP_FILE, NULL))
						Con_Printf("Unable to install package %s due to lack of any mirrors\n", p->name);
					else
					{
						p->flags &= ~(DPF_NATIVE|DPF_CACHED|DPF_CORRUPT);
						if (p->flags & DPF_MARKED)
							p->flags |= DPF_ENABLED;

						Con_Printf("Enabled meta package %s\n", p->name);
						PM_WriteInstalledPackages();
						PM_PackageEnabled(p);
					}
				}
				continue;
			}

			if ((p->flags & DPF_PRESENT) && !PM_PurgeOnDisable(p))
			{	//its in our cache directory, so lets just use that
				p->trymirrors = 0;
				if (p->flags & DPF_MARKED)
					p->flags |= DPF_ENABLED;

				Con_Printf("Enabled cached package %s\n", p->name);
				PM_WriteInstalledPackages();
				PM_PackageEnabled(p);
				continue;
			}

			if (!PM_SignatureOkay(p) || (qofs_t)p->filesize != p->filesize)
			{
				p->flags &= ~DPF_MARKED;	//refusing to do it.
				continue;
			}

			if (p->extract == EXTRACT_EXPLICITZIP)
			{	//don't allow multiple of these at a time... so we can download a single file and extract two packages from it.
				package_t *p2;
				for (p2 = availablepackages; p2; p2=p2->next)
					if (p2->flags&DPF_PENDING)	//only skip if the other one is already downloading.
						if (PM_DownloadSharesSource(p2, p))
							break;
				if (p2)
					continue;	//skip downloading it. we'll extract this one when the other is extracted.
			}

			temp = PM_GetTempName(p);
			temproot = p->fsroot;

			//FIXME: we should lock in the temp path, in case the user foolishly tries to change gamedirs.

			FS_CreatePath(temp, temproot);
			switch (p->extract)
			{
			case EXTRACT_ZIP:
			case EXTRACT_EXPLICITZIP:
			case EXTRACT_COPY:
				tmpfile = FS_OpenVFS(temp, "wb", temproot);
				break;
#ifdef AVAIL_XZDEC
			case EXTRACT_XZ:
				{
					vfsfile_t *raw = FS_OpenVFS(temp, "wb", temproot);
					tmpfile = raw?FS_XZ_DecompressWriteFilter(raw):NULL;
					if (!tmpfile && raw)
						VFS_CLOSE(raw);
				}
				break;
#endif
#ifdef AVAIL_GZDEC
			case EXTRACT_GZ:
				{
					vfsfile_t *raw = FS_OpenVFS(temp, "wb", temproot);
					tmpfile = raw?FS_GZ_WriteFilter(raw, true, false):NULL;
					if (!tmpfile && raw)
						VFS_CLOSE(raw);
				}
				break;
#endif
			default:
				Con_Printf("decompression method not supported\n");
				continue;
			}

			if (p->filesha512 && tmpfile)
				tmpfile = FS_Hash_ValidateWrites(tmpfile, p->name, p->filesize, &hash_sha2_512, p->filesha512);
			else if (p->filesha1 && tmpfile)
				tmpfile = FS_Hash_ValidateWrites(tmpfile, p->name, p->filesize, &hash_sha1, p->filesha1);

			if (tmpfile)
			{
				p->curdownload = HTTP_CL_Get(mirror, NULL, PM_Download_PreliminaryGot);
				if (!p->curdownload)
					Con_Printf("Unable to download %s (%s)\n", p->name, mirror);
			}
			else
			{
				char displaypath[MAX_OSPATH];
				Con_Printf("Unable to write %s. Fix permissions before trying to download %s\n", FS_DisplayPath(temp, temproot, displaypath, sizeof(displaypath))?displaypath:p->name, p->name);
				p->trymirrors = 0;	//don't bother trying other mirrors if we can't write the file or understand its type.
			}
			if (p->curdownload)
			{
				Con_Printf("Downloading %s\n", p->name);
				p->flags |= DPF_PENDING;
				p->curdownload->file = tmpfile;
				p->curdownload->user_ctx = temp;
				p->curdownload->user_num = temproot;

				DL_CreateThread(p->curdownload, NULL, NULL);
				downloading = true;
			}
			else
			{
				p->flags &= ~DPF_PENDING;
				p->flags &= ~DPF_MARKED;	//can't do it.
				if (tmpfile)
					VFS_CLOSE(tmpfile);
				FS_Remove(temp, temproot);
			}
		}
	}

	if (pkg_updating && !downloading)
		Cmd_AddTimer(0, PM_DownloadsCompleted, 0, NULL, 0);

	//clear the updating flag once there's no more activity needed
	pkg_updating = downloading;
}
qboolean PM_LoadMapPackage(const char *package)
{
	struct packagedep_s *dep;
	package_t *p = PM_FindPackage(package);
	if (!p)
		return false;

	if (p->flags & DPF_PRESENT)
		return true;	//okay, already installed.
	if (p->extract == EXTRACT_ZIP)
		return false;

	//refuse to use weird packages.
	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_FILE)
			return false;
	}

	//make sure its downloaded. fs code will be asked to explicitly use it, so no need to activate it.
	p->trymirrors = ~0u;
	return true;
}
void PM_LoadMap(const char *package, const char *map)
{
	if (!PM_LoadMapPackage(package))
		return;
	pm_onload.package = Z_StrDup(package);
	pm_onload.map = Z_StrDup(map);

	pkg_updating = true;
	PM_StartADownload();
}
#else
void PM_LoadMap(const char *package, const char *map)
{	//not supported, which is a shame because it might have been downloaded via other means.
}
#endif
unsigned int PM_IsApplying(void)
{
	int ret = 0;
#ifdef WEBCLIENT
	size_t i;
	if (PM_DownloadingCount())
		ret |= 1;
	for (i = 0; i < pm_numsources; i++)
		if (pm_source[i].curdl)
			ret |= 1;	//some source is downloading.
#endif
	if (pm_pendingprompts)
		ret |= 2;	//waiting for user action to complete.
	if (doautoupdate)
		ret |= 4;	//will want to trigger a prompt...

	return ret;
}
//'just' starts doing all the things needed to remove/install selected packages
void PM_ApplyChanges(void)
{
	package_t *p, **link;
	char temp[MAX_OSPATH];

#ifdef WEBCLIENT
	if (pkg_updating)
		return;
	pkg_updating = true;
#endif

//delete any that don't exist
	for (link = &availablepackages; *link ; )
	{
		p = *link;
#ifdef WEBCLIENT
		if (p->flags&DPF_PENDING)
			; //erk, dude, don't do two!
		else
#endif
			if ((p->flags & DPF_PURGE) || (!(p->flags&DPF_MARKED) && (p->flags&DPF_ENABLED)))
		{	//if we don't want it but we have it anyway. don't bother to follow this logic when reinstalling
			qboolean reloadpacks = false;
			struct packagedep_s *dep;


			for (dep = p->deps; dep; dep = dep->next)
			{
				if (dep->dtype == DEP_FILE || dep->dtype == DEP_CACHEFILE)
				{
					char ext[8];
					COM_FileExtension(dep->name, ext, sizeof(ext));
					if (!stricmp(ext, "pak") || !stricmp(ext, "pk3") || !stricmp(ext, "zip"))
						reloadpacks = true;

#ifdef PLUGINS		//when disabling/purging plugins, be sure to unload them first (unfortunately there might be some latency before this can actually happen).
					if ((p->flags & DPF_PLUGIN) && !Q_strncasecmp(dep->name, PLUGINPREFIX, strlen(PLUGINPREFIX)))
						Cmd_ExecuteString(va("plug_close %s\n", dep->name), RESTRICT_LOCAL);	//try to purge plugins so there's no files left open
#endif

				}
			}
			if (reloadpacks)	//okay, some package was removed, unload all, do the deletions/disables, then reload them. This is kinda shit. Would be better to remove individual packages, which would avoid unnecessary config execs.
				FS_UnloadPackFiles();

			if ((p->flags & DPF_PURGE) || PM_PurgeOnDisable(p))
			{
				Con_Printf("Purging package %s\n", p->name);
				for (dep = p->deps; dep; dep = dep->next)
				{
					if (dep->dtype == DEP_CACHEFILE)
					{
						char *f = va("downloads/%s", dep->name);
						if (!FS_Remove(f, p->fsroot))
							p->flags |= DPF_CACHED;
					}
					else if (dep->dtype == DEP_FILE)
					{
						if (*p->gamedir)
						{
							char *f = va("%s/%s", p->gamedir, dep->name);
							if (PM_TryGenCachedName(f, p, temp, sizeof(temp)) && PM_CheckFile(temp, p->fsroot))
							{
								if (!FS_Remove(temp, p->fsroot))
									p->flags |= DPF_CACHED;
							}
							else if (!FS_Remove(va("%s/%s", p->gamedir, dep->name), p->fsroot))
								p->flags |= DPF_NATIVE;
						}
						else if (!FS_Remove(dep->name, p->fsroot))
							p->flags |= DPF_NATIVE;
					}
				}
			}
			else
			{
				for (dep = p->deps; dep; dep = dep->next)
				{
					if (dep->dtype == DEP_FILE)
					{
						if (*p->gamedir)
						{
							char *f = va("%s/%s", p->gamedir, dep->name);
							if ((p->flags & DPF_NATIVE) && PM_TryGenCachedName(f, p, temp, sizeof(temp)))
								FS_Rename(f, temp, p->fsroot);
						}
					}
				}
				Con_Printf("Disabling package %s\n", p->name);
			}
			p->flags &= ~(DPF_PURGE|DPF_ENABLED);

			/* FIXME: windows bug:
			** deleting an exe might 'succeed' but leave the file on disk for a while anyway.
			** the file will eventually disappear, but until then we'll still see it as present,
			**  be unable to delete it again, and trying to open it to see if it still exists
			**  will fail.
			** there's nothing we can do other than wait until whatever part of
			**  windows that's fucking up releases its handles.
			** thankfully this only affects reinstalling exes/engines.
			*/

			PM_ValidatePackage(p);
			PM_WriteInstalledPackages();

			if (reloadpacks)
				FS_ReloadPackFiles();

			if ((p->flags & DPF_FORGETONUNINSTALL) && !(p->flags & DPF_PRESENT))
			{	//packages that have no source to redownload
#if 1
				pm_sequence++;
				PM_FreePackage(p);
#else
				if (p->alternative)
				{	//replace it with its alternative package
					*p->link = p->alternative;
					p->alternative->alternative = p->alternative->next;
					if (p->alternative->alternative)
						p->alternative->alternative->link = &p->alternative->alternative;
					p->alternative->next = p->next;
				}
				else
				{	//just remove it from the list.
					*p->link = p->next;
					if (p->next)
						p->next->link = p->link;
				}

//FIXME: the menu(s) hold references to packages, so its not safe to purge them
				p->flags |= DPF_HIDDEN;
//					BZ_Free(p);
#endif

				continue;
			}
		}

		link = &(*link)->next;
	}

#ifdef WEBCLIENT
	//and flag any new/updated ones for a download
	for (p = availablepackages; p ; p=p->next)
	{
		if (!(p->flags & DPF_PENDING))
			if (((p->flags & DPF_MANIMARKED) && !(p->flags&DPF_PRESENT)) ||	//satisfying a manifest merely requires that it be present, not actually enabled.
				((p->flags&DPF_MARKED) && !(p->flags&DPF_ENABLED)))			//actually enabled stuff requires actual enablement
			{
				p->trymirrors = ~0u;
				if (p->flags & DPF_SIGNATUREACCEPTED)
					p->flags |= DPF_TRUSTED;	//user confirmed it, engine trusts it, we're all okay with any exploits it may have...
			}
	}
	PM_StartADownload();	//and try to do those downloads.
#else
	for (p = availablepackages; p; p=p->next)
	{
		if ((p->flags&DPF_MARKED) && !(p->flags&DPF_ENABLED))
		{	//flagged for a (re?)download
			int i;
			struct packagedep_s *dep;
			for (i = 0; i < countof(p->mirror); i++)
				if (p->mirror[i])
					break;
			for (dep = p->deps; dep; dep=dep->next)
				if (dep->dtype == DEP_FILE||dep->dtype == DEP_CACHEFILE)
					break;
			if (!dep && i == countof(p->mirror))
			{	//this appears to be a meta package with no download
				//just directly install it.
				p->flags &= ~(DPF_NATIVE|DPF_CACHED|DPF_CORRUPT);
				p->flags |= DPF_ENABLED;

				Con_Printf("Enabled meta package %s\n", p->name);
				PM_WriteInstalledPackages();
				PM_PackageEnabled(p);
			}

			if ((p->flags & DPF_PRESENT) && !PM_PurgeOnDisable(p))
			{	//its in our cache directory, so lets just use that
				p->flags |= DPF_ENABLED;

				Con_Printf("Enabled cached package %s\n", p->name);
				PM_WriteInstalledPackages();
				PM_PackageEnabled(p);
				continue;
			}
			else
				p->flags &= ~DPF_MARKED;
		}
	}
#endif
}

#ifdef DOWNLOADMENU
static qboolean PM_DeclinedPackages(char *out, size_t outsize)
{
	size_t ofs = 0;
	package_t *p;
	qboolean ret = false;
	if (fs_manifest)
	{
		char tok[1024];
		char *strings = fs_manifest->installupd;
		while (strings && *strings)
		{
			strings = COM_ParseStringSetSep(strings, ';', tok, sizeof(tok));

			//already in the list
			if (PM_NameIsInStrings(declinedpackages, tok))
				continue;

			p = PM_MarkedPackage(tok, DPF_MARKED);
			if (p)	//don't mark it as declined if it wasn't
				continue;

			p = PM_FindPackage(tok);
			if (p)
			{	//okay, it was declined
				ret = true;
				if (!out)
				{	//we're confirming that they should be flagged as declined
					if (declinedpackages)
					{
						char *t = declinedpackages;
						declinedpackages = Z_StrDupf("%s;%s", declinedpackages, tok);
						Z_Free(t);
					}
					else
						declinedpackages = Z_StrDup(tok);
				}
				else
				{	//we're collecting a list of package names
					char *change = va("%s\n", p->name);
					size_t l = strlen(change);
					if (ofs+l >= outsize)
					{
						Q_strncpyz(out, "Too many changes\n", outsize);
						out = NULL;

						break;
					}
					else
					{
						memcpy(out+ofs, change, l);
						ofs += l;
						out[ofs] = 0;
					}
					break;
				}
			}
		}
	}
	if (!out && ret)
		PM_WriteInstalledPackages();
	return ret;
}
#endif
#if defined(M_Menu_Prompt) || defined(SERVERONLY)
//if M_Menu_Prompt is a define, then its a stub...
static void PM_PromptApplyChanges(void)
{
	PM_ApplyChanges();
}
#else
static void PM_PromptApplyChanges_Callback(void *ctx, promptbutton_t opt)
{
#ifdef WEBCLIENT
	pkg_updating = false;
#endif
	if (opt == PROMPT_YES)
		PM_ApplyChanges();
}
static void PM_PromptApplyDecline_Callback(void *ctx, promptbutton_t opt)
{
#ifdef WEBCLIENT
	pkg_updating = false;
#endif
	if (opt == PROMPT_NO)
	{
		PM_DeclinedPackages(NULL, 0);
		PM_PromptApplyChanges();
	}
}
static void PM_PromptApplyChanges(void)
{
	unsigned int changes;
	char text[8192];
#ifdef WEBCLIENT
	//lock it down, so noone can make any changes while this prompt is still displayed
	if (pkg_updating)
	{
		Menu_Prompt(PM_PromptApplyChanges_Callback, NULL, localtext("An update is already in progress\nPlease wait\n"), NULL, NULL, "Cancel", true);
		return;
	}
	pkg_updating = true;
#endif

	strcpy(text, localtext("Really decline the following\nrecommended packages?\n\n"));
	if (PM_DeclinedPackages(text+strlen(text), sizeof(text)-strlen(text)))
		Menu_Prompt(PM_PromptApplyDecline_Callback, NULL, text, NULL, "Confirm", "Cancel", true);
	else
	{
		strcpy(text, localtext("Apply the following changes?\n\n"));
		changes = PM_ChangeList(text+strlen(text), sizeof(text)-strlen(text));
		if (!changes)
		{
#ifdef WEBCLIENT
			pkg_updating = false;//no changes...
#endif
		}
		else
			Menu_Prompt(PM_PromptApplyChanges_Callback, NULL, text, "Apply", NULL, "Cancel", true);
	}
}
#endif
#if defined(HAVE_CLIENT) && defined(WEBCLIENT)
static void PM_AddSubList_Callback(void *ctx, promptbutton_t opt)
{
	if (opt == PROMPT_YES)
	{
		PM_AddSubList(ctx, "", SRCFL_USER|SRCFL_ENABLED);
		PM_WriteInstalledPackages();
		PM_UpdatePackageList(false);
	}
	Z_Free(ctx);
}
#endif

qboolean PM_CanInstall(const char *packagename)
{
	int i;
	package_t *p = PM_FindPackage(packagename);
	if (p && !(p->flags&(DPF_ENABLED|DPF_CORRUPT|DPF_HIDDEN)))
	{
		for (i = 0; i < countof(p->mirror); i++)
			if (p->mirror[i])
				return true;
	}
	return false;
}

static void PM_Pkg_ListSources(qboolean onlyenabled)
{
	size_t i, c=0, e=0;
	for (i = 0; i < pm_numsources; i++)
	{
		if ((pm_source[i].flags & SRCFL_HISTORIC) && !developer.ival)
			continue;	//hidden ones were historically enabled/disabled. remember the state even when using a different fmf, but don't confuse the user.

		c++;
		if (pm_source[i].flags & SRCFL_ENABLED)
			e++;
		else if (onlyenabled)
			continue;

		if (pm_source[i].flags & SRCFL_ENABLED)
			Con_Printf("^&02 ");
		else if (pm_source[i].flags & SRCFL_DISABLED)
			Con_Printf("^&04 ");
		else
			Con_Printf("^&0E ");

		if (pm_source[i].flags & SRCFL_DISABLED)
			Con_Printf("%s ", pm_source[i].url);	//enable
		else
			Con_Printf("%s ", pm_source[i].url);	//disable

		if (pm_source[i].flags & SRCFL_USER)
			Con_Printf("- ^[[Delete]\\type\\pkg remsource \"%s\"^]\n", pm_source[i].url);
		else
			Con_Printf("(implicit)\n");
	}
	if (c != e)
		Con_Printf("<%u sources, %u enabled>\n", (unsigned)c, (unsigned)e);
	else
		Con_Printf("<%u sources>\n", (unsigned)c);
}
static void PM_Pkg_ListPackage(package_t *p, const char **category)
{
	const char *newcat;
	char quoted[8192];
	const char *status;
	char *markup;
	struct packagedep_s *dep;

	if (p->flags & DPF_ENABLED)
		markup = S_COLOR_GREEN;
	else if (p->flags & DPF_CORRUPT)
		markup = S_COLOR_RED;
	else if (p->flags & (DPF_CACHED))
		markup = S_COLOR_YELLOW;	//downloaded but not active
	else
		markup = S_COLOR_WHITE;

	if (!(p->flags & DPF_MARKED) != !(p->flags & DPF_ENABLED) || (p->flags & DPF_PURGE))
	{
		if (p->flags & DPF_MARKED)
		{
			if (p->flags & DPF_PURGE)
				status = S_COLOR_CYAN"<to reinstall>";
			else
				status = S_COLOR_CYAN"<to install>";
		}
		else if ((p->flags & DPF_PURGE) || !(p->qhash && (p->flags & DPF_CACHED)))
			status = S_COLOR_CYAN"<to uninstall>";
		else
			status = S_COLOR_CYAN"<to disable>";
	}
	else if ((p->flags & (DPF_ENABLED|DPF_CACHED)) == DPF_CACHED)
		status = S_COLOR_CYAN"<disabled>";
	else if (p->flags & DPF_USERMARKED)
		status = S_COLOR_GRAY"<manual>";
	else if (p->flags & DPF_AUTOMARKED)
		status = S_COLOR_GRAY"<auto>";
	else
		status = "";

	//show category banner
	newcat = p->category?p->category:"";
	if (strcmp(*category, newcat))
	{
		*category = newcat;
		Con_Printf("%s\n", *category);
	}

	//show quick status display
	if (p->flags & DPF_ENABLED)
		Con_Printf("^&02 ");
	else if (p->flags & DPF_PRESENT)
		Con_Printf("^&0E ");
	else
		Con_Printf("^&04 ");
	if (p->flags & DPF_MARKED)
		Con_Printf("^&02 ");
	else if (!(p->flags & DPF_PURGE) && (p->flags&DPF_PRESENT))
		Con_Printf("^&0E ");
	else
		Con_Printf("^&04 ");

	//show the package details.
	Con_Printf("\t^["S_COLOR_GRAY"%s%s%s%s^] %s"S_COLOR_GRAY" %s (%s%s)", markup, p->name, p->arch?":":"", p->arch?p->arch:"", status, strcmp(p->name, p->title)?p->title:"", p->version, (p->flags&DPF_TESTING)?"-testing":"");

	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_SOURCE)
			Con_Printf(S_COLOR_MAGENTA" %s", dep->name);
	}

	if (!(p->flags&DPF_MARKED) && p == PM_FindPackage(p->name))
		Con_Printf(" ^[[Add]\\type\\pkg add %s;pkg apply^]", COM_QuotedString(p->name, quoted, sizeof(quoted), false));
	if ((p->flags&DPF_MARKED) && p == PM_MarkedPackage(p->name, DPF_MARKED))
		Con_Printf(" ^[[Remove]\\type\\pkg rem %s;pkg apply^]", COM_QuotedString(p->name, quoted, sizeof(quoted), false));


	if (p->flags & DPF_SIGNATUREACCEPTED)
		Con_Printf(" ^&02Trusted");
	else if (p->flags & DPF_SIGNATUREREJECTED)
		Con_Printf(" ^&04Untrusted");
	else if (p->flags & DPF_SIGNATUREUNKNOWN)
		Con_Printf(" ^&0EUnverified");
	else
		Con_Printf(" ^&0EUnsigned");

	if (p->curdownload)
	{
		if (p->curdownload->totalsize==0)
			Con_Printf(" Downloading %s/unknown", FS_AbbreviateSize(quoted,sizeof(quoted), p->curdownload->completed));
		else
			Con_Printf(" Downloading %i%% (%s total)", (int)(100*(double)p->curdownload->completed/(double)p->curdownload->totalsize), FS_AbbreviateSize(quoted,sizeof(quoted), p->curdownload->totalsize));
	}
	else if (p->flags & DPF_PENDING)
		Con_Printf(" Finalising...");
	else if (p->trymirrors)
		Con_Printf(" Pending...");

	Con_Printf("\n");
}
static void PM_Pkg_ListPackages(qboolean onlyenabled)
{
	const char *category = "";
	int i, count;
	package_t **sorted, *p, *a;

	for (count = 0, p = availablepackages; p; p=p->next)
		count++;
	sorted = Z_Malloc(sizeof(*sorted)*count);
	for (count = 0, p = availablepackages; p; p=p->next)
	{
		if ((p->flags & DPF_HIDDEN) && !(p->flags & (DPF_MARKED|DPF_ENABLED|DPF_PURGE|DPF_CACHED)))
			continue;

		if (p->flags & DPF_ALLMARKED)
			;
		else if (onlyenabled)
			continue;

		sorted[count++] = p;
	}
	qsort(sorted, count, sizeof(*sorted), PM_PackageSortOrdering);
	for (i = 0; i < count; i++)
	{
		PM_Pkg_ListPackage(sorted[i], &category);
		for(a=sorted[i]->alternative; a; a = a->next)
		{
			Con_Printf(" ");
			PM_Pkg_ListPackage(a, &category);
		}
	}
	Z_Free(sorted);
	Con_Printf("<end of list>\n");
}

void PM_Command_f(void)
{
	package_t *p;
	const char *act = Cmd_Argv(1);
	const char *key;
	qboolean quiet = false;

	if (Cmd_FromGamecode())
	{
		Con_Printf("%s may not be used from gamecode\n", Cmd_Argv(0));
		return;
	}

	if (!strncmp(act, "quiet_", 6))
	{
		quiet = true;	//don't spam so much. for menus to (ab)use.
		act += 6;
	}

	if (!strcmp(act, "listenabledsources"))
		PM_Pkg_ListSources(true);
	else if (!strcmp(act, "sources") || !strcmp(act, "addsource"))
	{
		#ifdef WEBCLIENT
			if (Cmd_Argc() == 2)
				PM_Pkg_ListSources(false);
			else
			{
				#ifdef HAVE_CLIENT
					Menu_Prompt(PM_AddSubList_Callback, Z_StrDup(Cmd_Argv(2)), va(localtext("Add updates source?\n%s"), Cmd_Argv(2)), "Confirm", NULL, "Cancel", true);
				#else
					PM_AddSubList(Cmd_Argv(2), "", SRCFL_USER|SRCFL_ENABLED);
					PM_WriteInstalledPackages();
				#endif
			}
		#endif
	}
	else if (!strcmp(act, "remsource"))
	{
	#ifdef WEBCLIENT
		PM_RemSubList(Cmd_Argv(2));
		PM_WriteInstalledPackages();
	#endif
	}
	else
	{
		if (!loadedinstalled)
			PM_UpdatePackageList(false);

		if (!strcmp(act, "list"))
		{
			PM_Pkg_ListSources(false);
			PM_Pkg_ListPackages(false);
		}
		else if (!strcmp(act, "listmarked"))
			PM_Pkg_ListPackages(true);
		else if (!strcmp(act, "internal"))
		{
			char buf[65536];
			key = Cmd_Argv(2);
			for (p = availablepackages; p; p=p->next)
			{
				if (Q_strcasecmp(p->name, key))
					continue;
				PM_WriteInstalledPackage_v3(p, buf, sizeof(buf));
				Con_Printf("%s", buf);
			}
		}
		else if (!strcmp(act, "show"))
		{
			struct packagedep_s *dep;
			int found = 0;
			key = Cmd_Argv(2);
			for (p = availablepackages; p; p=p->next)
			{
				if (Q_strcasecmp(p->name, key))
					continue;

				if (p->previewimage)
					Con_Printf("^[%s (%s)\\tipimg\\%s\\tip\\%s^]\n", p->name, p->version, p->previewimage, "");
				else
					Con_Printf("%s (%s)\n", p->name, p->version);
				if (p->title)
					Con_Printf("	^mtitle: ^m%s\n", p->title);
				if (p->license)
					Con_Printf("	^mlicense: ^m%s\n", p->license);
				if (p->author)
					Con_Printf("	^mauthor: ^m%s\n", p->author);
				if (p->website)
					Con_Printf("	^mwebsite: ^m%s\n", p->website);
				for (dep = p->deps; dep; dep = dep->next)
				{
					if (dep->dtype == DEP_SOURCE)
						Con_Printf("	^msource: ^m%s\n", dep->name);
				}
				if (p->description)
					Con_Printf("%s\n", p->description);
				for (dep = p->deps; dep; dep = dep->next)
				{
					if (dep->dtype == DEP_MAP)
						Con_Printf("	^mmap: ^[[%s]\\map\\%s^]\n", dep->name, dep->name);
				}

				if (p->flags & DPF_MARKED)
				{
					if (p->flags & DPF_ENABLED)
					{
						if (p->flags & DPF_PURGE)
							Con_Printf("	package is flagged to be re-installed\n");
						else
							Con_Printf("	package is currently installed\n");
					}
					else
						Con_Printf("	package is flagged to be installed\n");
				}
				else
				{
					if (p->flags & DPF_ENABLED)
					{
						if (p->flags & DPF_PURGE)
							Con_Printf("	package is flagged to be purged\n");
						else
							Con_Printf("	package is flagged to be disabled\n");
					}
					else
						Con_Printf("	package is not installed\n");
				}
				if (p->flags & DPF_NATIVE)
					Con_Printf("	package is native\n");
				if (p->flags & DPF_CACHED)
					Con_Printf("	package is cached\n");
				if (p->flags & DPF_CORRUPT)
					Con_Printf("	package is corrupt\n");
				if (p->flags & DPF_DISPLAYVERSION)
					Con_Printf("	package has a version conflict\n");
				if (p->flags & DPF_FORGETONUNINSTALL)
					Con_Printf("	package is obsolete\n");
				if (p->flags & DPF_HIDDEN)
					Con_Printf("	package is hidden\n");
				if (p->flags & DPF_ENGINE)
					Con_Printf("	package is an engine update\n");
				if (p->flags & DPF_TESTING)
					Con_Printf(S_COLOR_YELLOW"	package is untested\n");
				if (p->flags & DPF_TRUSTED)
					Con_Printf("	package is trusted\n");
#ifdef WEBCLIENT
				if (!PM_SignatureOkay(p))
				{
					if (!p->signature)
						Con_Printf(CON_ERROR"	Signature missing"CON_DEFAULT"\n");			//some idiot forgot to include a signature
					else if (p->flags & DPF_SIGNATUREREJECTED)
						Con_Printf(CON_ERROR"	Signature invalid"CON_DEFAULT"\n");			//some idiot got the wrong auth/sig/hash
					else if (p->flags & DPF_SIGNATUREUNKNOWN)
						Con_Printf(S_COLOR_RED"	Signature is not trusted"CON_DEFAULT"\n");	//clientside permission.
					else
						Con_Printf(CON_ERROR"	Unable to verify signature"CON_DEFAULT"\n");	//clientside problem.
				}
#endif
				found++;
			}
			if (!found)
				Con_Printf("<package not found>\n");
		}
		else if (!strcmp(act, "search") || !strcmp(act, "find"))
		{
			const char *key = Cmd_Argv(2);
			for (p = availablepackages; p; p=p->next)
			{
				if (Q_strcasestr(p->name, key) || (p->title && Q_strcasestr(p->title, key)) || (p->description && Q_strcasestr(p->description, key)))
				{
					Con_Printf("%s\n", p->name);
				}
			}
			Con_Printf("<end of list>\n");
		}
		else if (!strcmp(act, "apply"))
		{
			Con_Printf("Applying package changes\n");
			if (qrenderer != QR_NONE)
				PM_PromptApplyChanges();
			else if (Cmd_ExecLevel == RESTRICT_LOCAL)
				PM_ApplyChanges();
		}
		else if (!strcmp(act, "changes"))
		{
			PM_PrintChanges();
		}
		else if (!strcmp(act, "reset") || !strcmp(act, "revert"))
		{
			PM_RevertChanges();
		}
#ifdef WEBCLIENT
		else if (!strcmp(act, "update"))
		{	//query the servers if we've not already done so.
			//FIXME: regrab if more than an hour ago?
			if (!allowphonehome)
				allowphonehome = -1;	//trigger a prompt, instead of ignoring it.
			PM_UpdatePackageList(false);
		}
		else if (!strcmp(act, "refresh"))
		{	//flush package cache, make a new request even if we already got a response from the server.
			int i;
			for (i = 0; i < pm_numsources; i++)
			{
				if (!(pm_source[i].flags & SRCFL_ENABLED))
				{
					if (isDedicated && !(pm_source[i].flags & SRCFL_DISABLED))
						pm_source[i].flags |= SRCFL_ONCE;
					else
						continue;
				}
				pm_source[i].status = SRCSTAT_PENDING;
			}
			if (!allowphonehome)
				allowphonehome = -1;	//trigger a prompt, instead of ignoring it.
			PM_UpdatePackageList(false);
		}
		else if (!strcmp(act, "upgrade"))
		{	//auto-mark any updated packages.
			unsigned int changes = PM_MarkUpdates();
			if (changes)
			{
				if (!quiet)
					Con_Printf("%u packages flagged\n", changes);
				PM_PromptApplyChanges();
			}
			else if (!quiet)
				Con_Printf("Already using latest versions of all packages\n");
		}
#endif
		else if (!strcmp(act, "add") || !strcmp(act, "get") || !strcmp(act, "install") || !strcmp(act, "enable"))
		{	//FIXME: make sure this updates.
			int arg = 2;
			for (arg = 2; arg < Cmd_Argc(); arg++)
			{
				const char *key = Cmd_Argv(arg);
				p = PM_FindPackage(key);
				if (p)
				{
					if (!PM_MarkPackage(p, DPF_USERMARKED))
						Con_Printf("%s: unable to activate/mark %s\n", Cmd_Argv(0), key);
					p->flags &= ~DPF_PURGE;
				}
				else
					Con_Printf("%s: package %s not known\n", Cmd_Argv(0), key);
			}
			if (!quiet)
				PM_PrintChanges();
		}
#ifdef WEBCLIENT
		else if (!strcmp(act, "reinstall"))
		{	//fixme: favour the current verson.
			int arg = 2;
			for (arg = 2; arg < Cmd_Argc(); arg++)
			{
				const char *key = Cmd_Argv(arg);
				p = PM_FindPackage(key);
				if (p)
				{
					PM_MarkPackage(p, DPF_USERMARKED);
					p->flags |= DPF_PURGE;
				}
				else
					Con_Printf("%s: package %s not known\n", Cmd_Argv(0), key);
			}
			if (!quiet)
				PM_PrintChanges();
		}
#endif
		else if (!strcmp(act, "disable") || !strcmp(act, "rem") || !strcmp(act, "remove"))
		{
			int arg = 2;
			for (arg = 2; arg < Cmd_Argc(); arg++)
			{
				const char *key = Cmd_Argv(arg);
				p = PM_MarkedPackage(key, DPF_MARKED);
				if (!p)
					p = PM_FindPackage(key);
				if (p)
					PM_UnmarkPackage(p, DPF_MARKED);
				else
					Con_Printf("%s: package %s not known\n", Cmd_Argv(0), key);
			}
			if (!quiet)
				PM_PrintChanges();
		}
		else if (!strcmp(act, "del") || !strcmp(act, "purge") || !strcmp(act, "delete") || !strcmp(act, "uninstall"))
		{
			int arg = 2;
			for (arg = 2; arg < Cmd_Argc(); arg++)
			{
				const char *key = Cmd_Argv(arg);
				p = PM_MarkedPackage(key, DPF_MARKED);
				if (!p)
					p = PM_FindPackage(key);
				if (p)
				{
					PM_UnmarkPackage(p, DPF_MARKED);
					if (p->flags & (DPF_PRESENT|DPF_CORRUPT))
						p->flags |=	DPF_PURGE;
				}
				else
					Con_Printf("%s: package %s not known\n", Cmd_Argv(0), key);
			}
			if (!quiet)
				PM_PrintChanges();
		}
		else
			Con_Printf("%s: Unknown action %s\nShould be one of list, show, search, upgrade, revert, add, rem, del, changes, apply, sources, addsource, remsource\n", Cmd_Argv(0), act);
	}
}

qboolean PM_FindUpdatedEngine(char *syspath, size_t syspathsize)
{
	struct packagedep_s *dep;
	package_t *e = NULL, *p;
	char *pfname;

	int best = parse_revision_number(enginerevision, true);
	int them;
	if (best <= 0)
		return false;	//no idea what revision we are, we might be more recent.
	//figure out what we've previously installed.
	PM_PreparePackageList();

	for (p = availablepackages; p; p = p->next)
	{
		if ((p->flags & DPF_ENGINE) && !(p->flags & DPF_HIDDEN) && p->fsroot == FS_ROOT)
		{
			them = parse_revision_number(p->version, true);
			if ((p->flags & DPF_ENABLED) && them>best)
			{
				for (dep = p->deps, pfname = NULL; dep; dep = dep->next)
				{
					if (dep->dtype != DEP_FILE)
						continue;
					if (pfname)
					{
						pfname = NULL;
						break;
					}
					pfname = dep->name;
				}

				if (pfname && PM_CheckFile(pfname, p->fsroot))
				{
					if (FS_SystemPath(pfname, p->fsroot, syspath, syspathsize))
					{
						e = p;
						best = them;
					}
				}
			}
		}
	}

	if (e)
		return true;
	return false;
}






//called by the filesystem code to make sure needed packages are in the updates system
static const char *FS_RelativeURL(const char *base, const char *file, char *buffer, int bufferlen)
{
	//fixme: cope with windows paths
	qboolean baseisurl = base?!!strchr(base, ':'):false;
	qboolean fileisurl = !!strchr(file, ':');
	//qboolean baseisabsolute = (*base == '/' || *base == '\\');
	qboolean fileisabsolute = (*file == '/' || *file == '\\');
	const char *ebase;

	if (fileisurl || !base)
		return file;
	if (fileisabsolute)
	{
		if (baseisurl)
		{
			ebase = strchr(base, ':');
			ebase++;
			while(*ebase == '/')
				ebase++;
			while(*ebase && *ebase != '/')
				ebase++;
		}
		else
			ebase = base;
	}
	else
		ebase = COM_SkipPath(base);
	memcpy(buffer, base, ebase-base);
	strcpy(buffer+(ebase-base), file);

	return buffer;
}

//this function is called by the filesystem code to start downloading the packages listed by each manifest.
void PM_AddManifestPackages(ftemanifest_t *man, qboolean mayapply)
{
	package_t *p, *m;
	size_t i;
	const char *path, *url;

	char buffer[MAX_OSPATH];
	int idx;
	struct manpack_s *pack;
	const char *baseurl = man->updateurl;	//this is a url for updated versions of the fmf itself.
	int dtype;

	for (p = availablepackages; p; p = p->next)
		p->flags &= ~DPF_MANIMARKED;

	PM_RevertChanges();	//we'll be force-applying, so make sure there's no unconfirmed pending stuff.

	for (idx = 0; idx < countof(man->package); idx++)
	{
		pack = &man->package[idx];
		if (!pack->type)
			continue;

		//check this package's conditional
		if (pack->condition)
		{
			if (!If_EvaluateBoolean(pack->condition, RESTRICT_LOCAL))
				continue;	//ignore it
		}

		if (pack->packagename)
		{
			p = PM_FindPackage(pack->packagename);
			if (p)
			{
				PM_MarkPackage(p, DPF_MANIMARKED);
				continue;
			}
		}

		/*if (pack->mirrors[0])
		{
			for (p = availablepackages; p; p = p->next)
			{
				if (p->mirror[0] && !strcmp(p->mirror[0], pack->mirrors[0]))
					break;
			}
			if (p)
			{
				PM_MarkPackage(p, DPF_MANIMARKED);
				continue;
			}
		}*/

		p = Z_Malloc(sizeof(*p));
		strcpy(p->version, "");
		if (pack->packagename)
		{
			char *arch;
			char *ver;
			p->name = Z_StrDup(pack->packagename);
			ver = strchr(p->name, '=');
			arch = strchr(p->name, ':');
			if (*ver)
			{
				*ver++ = 0;
				Q_strncpyz(p->version, ver, sizeof(p->version));
			}
			if (*arch)
			{
				*arch++ = 0;
				p->arch = Z_StrDup(arch);
			}
		}
		else
		{
			p->name = Z_StrDup(pack->path);
			strcpy(p->version, "");
		}
		p->title = Z_StrDup(pack->path);
		p->category = Z_StrDupf("%s/", man->formalname);
		p->priority = PM_DEFAULTPRIORITY;
		p->fsroot = FS_ROOT;
		p->flags = DPF_FORGETONUNINSTALL|DPF_MANIFEST|DPF_GUESSED;
		p->qhash = pack->crcknown?Z_StrDupf("%#x", pack->crc):NULL;
		dtype = DEP_FILE;

		p->packprefix = pack->prefix?Z_StrDup(pack->prefix):NULL;

		//note that this signs the hash(validated with size) with an separately trusted authority and is thus not dependant upon trusting the manifest itself...
		//that said, we can't necessarily trust any overrides the manifest might include - those parts do not form part of the signature.
		if (pack->crcknown && strchr(p->name, '/'))
		{
			p->signature = pack->signature?Z_StrDup(pack->signature):NULL;
		}
		else if (pack->signature)
			Con_Printf(CON_WARNING"Ignoring signature for %s\n", p->name);
		p->filesha512 = pack->sha512?Z_StrDup(pack->sha512):NULL;
		p->filesize = pack->filesize;

		{
			char *c = p->name;
			for (c=p->name; *c; c++)	//don't get confused.
				if (*c == '/') *c = '_';
		}

		path = pack->path;
		if (pack->type != mdt_installation)
		{
			char *s = strchr(path, '/');
			if (!s)
			{
				PM_FreePackage(p);
				continue;
			}
			*s = 0;
			Q_strncpyz(p->gamedir, path, sizeof(p->gamedir));
			*s = '/';
			path=s+1;
		}

		p->extract = EXTRACT_COPY;
		for (i = 0; i < countof(pack->mirrors) && i < countof(p->mirror); i++)
		{
			url = pack->mirrors[i];
			if (!url)
				continue;
			if (!strncmp(url, "gz:", 3))
			{
				url+=3;
				p->extract = EXTRACT_GZ;
			}
			else if (!strncmp(url, "xz:", 3))
			{
				url+=3;
				p->extract = EXTRACT_XZ;
			}
			else if (!strncmp(url, "unzip:", 6))
			{
				char *comma;
				url+=6;
				comma = strchr(url, ',');
				if (comma)
				{
					p->extract = EXTRACT_EXPLICITZIP;
					*comma = 0;
					PM_AddDep(p, DEP_EXTRACTNAME, url);
					*comma = ',';
					url = comma+1;
				}
				else
					p->extract = EXTRACT_ZIP;
			}
			/*else if (!strncmp(url, "prompt:", 7))
			{
				url+=7;
				fspdl_extracttype = X_COPY;
			}*/

			url = FS_RelativeURL(baseurl, url, buffer, sizeof(buffer));
			if (url && *url)
				p->mirror[i] = Z_StrDup(url);
		}
		PM_AddDep(p, dtype, path);

		PM_ValidateAuthenticity(p, (man->security == MANIFEST_SECURITY_INSTALLER)?VH_CORRECT:VH_UNSUPPORTED);
		if (p->flags & DPF_SIGNATUREACCEPTED)
			p->flags |= DPF_TRUSTED;	//user confirmed it, engine trusts it, we're all okay with any exploits it may have...

		m = PM_InsertPackage(p);
		if (!m)
			continue;

		PM_MarkPackage(m, DPF_MANIMARKED);

/*		//okay, so we merged it into m. I guess we already had a copy!
		if (!(m->flags & DPF_PRESENT))
			if (PM_SignatureOkay(m))
				m->trymirrors = ~0;	//FIXME: we should probably mark+prompt...
*/
		continue;
	}

	PM_ResortPackages();
	if (mayapply)
		PM_ApplyChanges();	//we reverted changes earlier, so the only packages added should have been from here.
}

#ifdef HAVE_CLIENT
#include "pr_common.h"
void QCBUILTIN PF_cl_getpackagemanagerinfo(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int packageidx = G_INT(OFS_PARM0);
	enum packagemanagerinfo_e fieldidx = G_INT(OFS_PARM1);
	package_t *p;
	G_INT(OFS_RETURN) = 0;
	if (packageidx < 0)
		return;
	if (packageidx == 0)
		PM_AreSourcesNew(true);

	for (p = availablepackages; p; p = p->next)
	{
		if ((p->flags & DPF_HIDDEN) && !(p->flags & (DPF_MARKED|DPF_ENABLED|DPF_PURGE|DPF_CACHED)))
			continue;
		if (packageidx--)
			continue;
		switch(fieldidx)
		{
		case GPMI_NAME:
			if (p->arch)
				RETURN_TSTRING(va("%s:%s=%s", p->name, p->arch, p->version));
			else
				RETURN_TSTRING(va("%s=%s", p->name, p->version));
			break;
		case GPMI_CATEGORY:
			RETURN_TSTRING(p->category);
			break;
		case GPMI_TITLE:
			if (p->flags & DPF_DISPLAYVERSION)
				RETURN_TSTRING(va("%s (%s)", p->title, p->version));
			else
				RETURN_TSTRING(p->title);
			break;
		case GPMI_VERSION:
			RETURN_TSTRING(p->version);
			break;
		case GPMI_DESCRIPTION:
			RETURN_TSTRING(p->description);
			break;
		case GPMI_LICENSE:
			RETURN_TSTRING(p->license);
			break;
		case GPMI_AUTHOR:
			RETURN_TSTRING(p->author);
			break;
		case GPMI_WEBSITE:
			RETURN_TSTRING(p->website);
			break;
		case GPMI_FILESIZE:
			if (p->filesize)
				RETURN_TSTRING(va("%lu", (unsigned long)p->filesize));
			break;
		case GPMI_AVAILABLE:
#ifdef WEBCLIENT
			if (PM_SignatureOkay(p) && !(p->flags&DPF_FORGETONUNINSTALL))
				RETURN_SSTRING("1");
#endif
			break;

		case GPMI_INSTALLED:
			if (p->flags & DPF_CORRUPT)
				RETURN_SSTRING("corrupt");	//some sort of error
			else if (p->flags & DPF_ENABLED)
				RETURN_SSTRING("enabled");	//its there (and in use)
			else if (p->flags & DPF_PRESENT)
				RETURN_SSTRING("present");	//its there (but ignored)
#ifdef WEBCLIENT
			else if (p->curdownload)
			{	//we're downloading it
				if (p->curdownload->qdownload.sizeunknown&&cls.download->size==0 && p->filesize>0 && p->extract==EXTRACT_COPY)
					RETURN_TSTRING(va("%i%%", (int)((100*p->curdownload->qdownload.completedbytes)/p->filesize)));	//server didn't report total size, but we know how big its meant to be.
				else
					RETURN_TSTRING(va("%i%%", (int)p->curdownload->qdownload.percent));	//we're downloading it.
			}
			else if (p->flags & DPF_PENDING)
				RETURN_TSTRING("100%");	//finalising shouldn't stay in this state long...
			else if (p->trymirrors)
				RETURN_SSTRING("pending");	//its queued.
#endif
			break;
		case GPMI_GAMEDIR:
			RETURN_TSTRING(p->gamedir);
			break;

		case GPMI_ACTION:
			if (p->flags & DPF_PURGE)
			{
				if (p->flags & DPF_MARKED)
					RETURN_SSTRING("reinstall");	//user wants to install it
				else
					RETURN_SSTRING("purge");	//wiping it out
			}
			else if (p->flags & DPF_USERMARKED)
				RETURN_SSTRING("user");	//user wants to install it
			else if (p->flags & (DPF_AUTOMARKED|DPF_MANIMARKED))
				RETURN_SSTRING("auto");	//enabled to satisfy a dependancy
			else if (p->flags & DPF_ENABLED)
				RETURN_SSTRING("disable");	//change from enabled to cached.
			else if (p->flags & DPF_PRESENT)
				RETURN_SSTRING("retain");	//keep it in cache
			//else not installed and don't want it.
			break;
		case GPMI_MAPS:
			{
				char buf[256];
				char *maps = NULL;
				struct packagedep_s *d;
				for (d = p->deps; d; d = d->next)
				{
					if (d->dtype == DEP_MAP)
					{
						*buf = ' ';
						COM_QuotedString(d->name, buf+1, sizeof(buf)-1, false);
						if (maps)
							Z_StrCat(&maps, buf);
						else
							Z_StrCat(&maps, buf+1);
					}
				}
				if (maps)
				{	//at least one map found.
					RETURN_TSTRING(maps);
					Z_Free(maps);
				}
			}
			break;
		case GPMI_PREVIEWIMG:
			if (p->previewimage)
				RETURN_TSTRING(p->previewimage);
			break;
		}
		return;
	}
}
#endif

#else
qboolean PM_CanInstall(const char *packagename)
{
	return false;
}
void PM_EnumeratePlugins(void (*callback)(const char *name, qboolean blocked))
{
}
int PM_IsApplying(qboolean listsonly)
{
	return false;
}
qboolean PM_FindUpdatedEngine(char *syspath, size_t syspathsize)
{
	return false;
}
#endif

#ifdef DOWNLOADMENU
typedef struct {
	menucustom_t *list;
	char intermediatefilename[MAX_QPATH];
	char pathprefix[MAX_QPATH];
	int downloadablessequence;
	char titletext[128];
	char applymessage[128];	//so we can change its text to give it focus
	char filtertext[128];
	qboolean filtering;
	const void *expandedpackage;	//which package we're currently viewing maps for.
	qboolean populated;
} dlmenu_t;

static void MD_Draw (int x, int y, struct menucustom_s *c, struct emenu_s *m)
{
	package_t *p;
	char *n;
	struct packagedep_s *dep;

	if (y + 8 < 0 || y >= vid.height)	//small optimisation.
		return;

	if (c->dint != pm_sequence)
		return;	//probably stale

#ifdef WEBCLIENT
	if (allowphonehome == -2)
	{
		allowphonehome = false;
#ifdef HAVE_CLIENT
		Menu_Prompt(PM_AllowPackageListQuery_Callback, NULL, localtext("Query updates list?\n"), "Okay", NULL, "Nope", true);
#endif
	}
#endif

	p = c->dptr;
	if (p)
	{
		if (p->alternative && (p->flags & DPF_HIDDEN))
			p = p->alternative;

		for (dep = p->deps; dep; dep = dep->next)
			if (dep->dtype == DEP_MAP)
				break;

		if (dep)
		{	//map packages are not marked, but cached on demand.
			if (p->flags & DPF_PRESENT)
			{
				if (p->flags & DPF_PURGE)
					Draw_FunStringWidth (x, y, "DEL", 48, 2, false);	//purge
				else
					Draw_FunStringWidth (x, y, "^&03  ", 48, 2, false);	//cyan
			}
			else
				Draw_FunStringWidth (x, y, "^&06  ", 48, 2, false);	//orange
		}
		else
#ifdef WEBCLIENT
		if (p->curdownload)
			Draw_FunStringWidth (x, y, va("%i%%", (int)p->curdownload->qdownload.percent), 48, 2, false);
		else if (p->trymirrors || (p->flags&DPF_PENDING))
			Draw_FunStringWidth (x, y, "PND", 48, 2, false);
		else
#endif
		{
			if (p->flags & DPF_USERMARKED)
			{
				if (!(p->flags & DPF_ENABLED))
				{	//DPF_MARKED|!DPF_ENABLED:
					if (p->flags & DPF_PURGE)
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"GET", 48, 2, false);
					else if (p->flags & (DPF_PRESENT))
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"USE", 48, 2, false);
					else
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"GET", 48, 2, false);
				}
				else
				{	//DPF_MARKED|DPF_ENABLED:
					if (p->flags & DPF_PURGE)
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"GET", 48, 2, false);	//purge and reinstall.
					else if (p->flags & DPF_CORRUPT)
						Draw_FunStringWidth (x, y, "?""?""?", 48, 2, false);
					else
					{
						Draw_FunStringWidth (x, y, "^&02  ", 48, 2, false);	//green
//						Draw_FunStringWidth (x, y, "^Ue080^Ue082", 48, 2, false);
//						Draw_FunStringWidth (x, y, "^Ue083", 48, 2, false);
					}
				}
			}
			else if (p->flags & DPF_MARKED)
			{	//auto-use options. draw with half alpha to darken them a little.
				if (!(p->flags & DPF_ENABLED))
				{	//DPF_MARKED|!DPF_ENABLED:
					if (p->flags & DPF_PURGE)
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"^hGET", 48, 2, false);
					else if (p->flags & (DPF_PRESENT))
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"^hUSE", 48, 2, false);
					else
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"^hGET", 48, 2, false);
				}
				else
				{	//DPF_MARKED|DPF_ENABLED:
					if (p->flags & DPF_PURGE)
						Draw_FunStringWidth (x, y, S_COLOR_GREEN"^hGET", 48, 2, false);	//purge and reinstall.
					else if (p->flags & DPF_CORRUPT)
						Draw_FunStringWidth (x, y, "?""?""?", 48, 2, false);
					else
					{
						Draw_FunStringWidth (x, y, "^&02  ", 48, 2, false);	//green
//						Draw_FunStringWidth (x, y, "^Ue080^Ue082", 48, 2, false);
//						Draw_FunStringWidth (x, y, "^Ue083", 48, 2, false);
					}
				}
			}
			else
			{
				if (!(p->flags & DPF_ENABLED))
				{	//!DPF_MARKED|!DPF_ENABLED:
					if (p->flags & DPF_PURGE)
						Draw_FunStringWidth (x, y, S_COLOR_RED"DEL", 48, 2, false);	//purge
					else if (p->flags & DPF_HIDDEN)
						Draw_FunStringWidth (x, y, "---", 48, 2, false);
					else if (p->flags & DPF_CORRUPT)
						Draw_FunStringWidth (x, y, "!!!", 48, 2, false);
					else
					{
						if (p->flags & DPF_PRESENT)
							Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
						else
							Draw_FunStringWidth (x, y, "^&04  ", 48, 2, false);	//red

//						Draw_FunStringWidth (x, y, "^Ue080^Ue082", 48, 2, false);
//						Draw_FunStringWidth (x, y, "^Ue081", 48, 2, false);
//						if (p->flags & DPF_PRESENT)
//							Draw_FunStringWidth (x, y, "-", 48, 2, false);
					}
				}
				else
				{	//!DPF_MARKED|DPF_ENABLED:
					if ((p->flags & DPF_PURGE) || PM_PurgeOnDisable(p))
						Draw_FunStringWidth (x, y, S_COLOR_RED"DEL", 48, 2, false);
					else
						Draw_FunStringWidth (x, y, S_COLOR_YELLOW"DIS", 48, 2, false);
				}
			}
		}

		n = p->title;
		if (p->flags & DPF_DISPLAYVERSION)
			n = va("%s (%s)", n, *p->version?p->version:"unversioned");

		if (p->flags & DPF_TESTING)	//hide testing updates 
			n = va("^h%s", n);

		if (!PM_CheckPackageFeatures(p))
			Draw_FunStringWidth(0, y, "!", x+8, true, true);
#ifdef WEBCLIENT
		if (!PM_SignatureOkay(p))
			Draw_FunStringWidth(0, y, "^b!", x+8, true, true);
#endif

//		if (!(p->flags & (DPF_ENABLED|DPF_MARKED|DPF_PRESENT))
//			continue;

//		if (&m->selecteditem->common == &c->common)
//			Draw_AltFunString (x+48, y, n);
//		else
			Draw_FunStringU8(CON_WHITEMASK, x+48, y, n);
	}
}

static qboolean MD_Key (struct menucustom_s *c, struct emenu_s *m, int key, unsigned int unicode)
{
	qboolean ctrl = keydown[K_LCTRL] || keydown[K_RCTRL];
	package_t *p, *p2;
	struct packagedep_s *dep, *dep2;
	dlmenu_t *info = m->data;
	if (c->dint != pm_sequence)
		return false;	//probably stale
	p = c->dptr;
	if (key == 'c' && ctrl)
		Sys_SaveClipboard(CBT_CLIPBOARD, p->website);
	else if (key == K_DEL || key == K_KP_DEL || key == K_BACKSPACE || key == K_GP_DIAMOND_ALTCONFIRM)
	{
		if (!(p->flags & DPF_MARKED))
			p->flags |= DPF_PURGE;	//purge it when its already not marked (ie: when pressed twice)
		PM_UnmarkPackage(p, DPF_MARKED);	//deactivate it
	}
	else if (key == K_ENTER || key == K_KP_ENTER || key == K_MOUSE1 || key == K_TOUCH || key == K_GP_DIAMOND_CONFIRM)
	{
		if (p->alternative && (p->flags & DPF_HIDDEN))
			p = p->alternative;

		if (info->expandedpackage == p)
		{	//close this submenu thing...
			info->expandedpackage = NULL;
			//remove the following map items.
			pm_sequence++;
			return true;
		}

		for (dep = p->deps; dep; dep = dep->next)
		{
			if (dep->dtype == DEP_MAP)
			{
				info->expandedpackage = p;
				pm_sequence++;
				//add the map items after (and shift everything)
				return true;
			}
		}

		if (p->flags & DPF_ENABLED)
		{
			switch (p->flags & (DPF_PURGE|DPF_MARKED))
			{
			case DPF_USERMARKED:
			case DPF_AUTOMARKED:
			case DPF_MARKED:
				PM_UnmarkPackage(p, DPF_MARKED);	//deactivate it
				break;
			case 0:
				p->flags |= DPF_PURGE;	//purge
				if (!PM_PurgeOnDisable(p))
					break;
				//fall through
			case DPF_PURGE:
				PM_MarkPackage(p, DPF_USERMARKED);		//reinstall
//				if (!(p->flags & DPF_HIDDEN) && !(p->flags & DPF_CACHED))
//					break;
				//fall through
			case DPF_USERMARKED|DPF_PURGE:
			case DPF_AUTOMARKED|DPF_PURGE:
			case DPF_MARKED|DPF_PURGE:
				p->flags &= ~DPF_PURGE;	//back to no-change
				break;
			}
		}
		else
		{
			switch (p->flags & (DPF_PURGE|DPF_MARKED))
			{
			case 0:
				PM_MarkPackage(p, DPF_USERMARKED);
				//now: try to install
				break;
			case DPF_AUTOMARKED:	//
				p->flags |= DPF_USERMARKED;
				break;
			case DPF_USERMARKED:
			case DPF_MARKED:
				p->flags |= DPF_PURGE;
#ifdef WEBCLIENT
				//now: re-get despite already having it.
				if ((p->flags & DPF_CORRUPT) || ((p->flags & DPF_PRESENT) && !PM_PurgeOnDisable(p)))
					break;	//only makes sense if we already have a cached copy that we're not going to use.
#endif
				//fallthrough
			case DPF_USERMARKED|DPF_PURGE:
			case DPF_AUTOMARKED|DPF_PURGE:
			case DPF_MARKED|DPF_PURGE:
				PM_UnmarkPackage(p, DPF_MARKED);
				//now: delete
				if ((p->flags & DPF_CORRUPT) || ((p->flags & DPF_PRESENT) && !PM_PurgeOnDisable(p)))
					break;	//only makes sense if we have a cached/corrupt copy of it already
				//fallthrough
			case DPF_PURGE:
				p->flags &= ~DPF_PURGE;
				//now: no change
				break;
			}
		}

		if (p->flags&DPF_MARKED)
		{
			//any other packages that conflict should be flagged for uninstall now that this one will replace it.
			for (p2 = availablepackages; p2; p2 = p2->next)
			{
				if (p == p2)
					continue;
				if (strcmp(p->gamedir, p2->gamedir))
					continue;	//different gamedirs. don't screw up.
				for (dep = p->deps; dep; dep = dep->next)
				{
					if (dep->dtype == DEP_FILE)
					{
						for (dep2 = p2->deps; dep2; dep2 = dep2->next)
						{
							if (dep2->dtype != DEP_FILE)
								continue;
							if (!strcmp(dep->name, dep2->name))
							{
								PM_UnmarkPackage(p2, DPF_MARKED);
								break;
							}
						}
					}
				}
			}
		}
#ifdef WEBCLIENT
		else
			p->trymirrors = 0;
#endif
		return true;
	}

	return false;
}

static void MD_MapDraw (int x, int y, struct menucustom_s *c, struct emenu_s *m)
{
	struct packagedep_s *map = c->dptr2;
#if defined(HAVE_SERVER)
	const package_t *p = c->dptr;
	struct packagedep_s *dep;
	float besttime, fulltime, bestkills, bestsecrets;
	char *package = NULL;
	const char *ext;
#endif

	if (y + 8 < 0 || y >= vid.height)	//small optimisation.
		return;

	if (c->dint != pm_sequence)
		return;	//probably stale

#if defined(HAVE_SERVER)
	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype == DEP_CACHEFILE)
		{
			package = va("downloads/%s", dep->name);
			break;
		}
		else if (dep->dtype == DEP_FILE && !package)
			package = dep->name;
	}
	ext = COM_GetFileExtension(map->name, NULL);
	if (package && Log_CheckMapCompletion(package, va("maps/%s%s", map->name, *ext?"":".bsp"), &besttime, &fulltime, &bestkills, &bestsecrets))
	{
		if (besttime != fulltime)
			Draw_FunStringU8(CON_WHITEMASK, x+48+8*4, y, va("^m%s^m (%g %g in %.1f secs, fastest in %.1f)", map->name, bestkills,bestsecrets,fulltime, besttime));
		else
			Draw_FunStringU8(CON_WHITEMASK, x+48+8*4, y, va("^m%s^m (%g %g in %.1f secs)", map->name, bestkills,bestsecrets,fulltime));
	}
	else
#endif
		Draw_FunStringU8(CON_WHITEMASK, x+48+8*4, y, map->name);
}
static qboolean MD_MapKey (struct menucustom_s *c, struct emenu_s *m, int key, unsigned int unicode)
{
	const package_t *p = c->dptr;
	struct packagedep_s *map = c->dptr2;
	struct packagedep_s *dep;

	if (c->dint != pm_sequence)
		return false;	//probably stale

	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_START || key == K_MOUSE1 || key == K_TOUCH || key == K_GP_DIAMOND_CONFIRM)
		for (dep = p->deps; dep; dep = dep->next)
		{
			if (dep == map)
			{
				char quoted[MAX_QPATH*2];
				Cbuf_AddText(va("map %s\n", COM_QuotedString(va("%s:%s", p->name, map->name), quoted, sizeof(quoted), false)), RESTRICT_LOCAL);
				return true;
			}
		}

	return false;
}

#ifdef WEBCLIENT
static void MD_Source_Draw (int x, int y, struct menucustom_s *c, struct emenu_s *m)
{
	char *text;
	if (!(pm_source[c->dint].flags & SRCFL_ENABLED))
	{
		if (!(pm_source[c->dint].flags & SRCFL_DISABLED))
			Draw_FunStringWidth (x, y, "??", 48, 2, false);
		else
			Draw_FunStringWidth (x, y, "^&04  ", 48, 2, false);	//red
	}
	else switch(pm_source[c->dint].status)
	{
	case SRCSTAT_OBTAINED:
		Draw_FunStringWidth (x, y, "^&02  ", 48, 2, false);	//green
		break;
	case SRCSTAT_PENDING:
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		Draw_FunStringWidth (x, y, "??", 48, 2, false);	//this should be fast... so if they get a chance to see the ?? then there's something bad happening, and the ?? is appropriate.
		break;
	case SRCSTAT_UNTRIED:	//waiting for a refresh.
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		break;
	case SRCSTAT_FAILED_DNS:
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
#ifdef WEBCLIENT
		Draw_FunStringWidth (x, y, "DNS", 48, 2, false);
#endif
		break;
	case SRCSTAT_FAILED_NORESP:
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		Draw_FunStringWidth (x, y, "NR", 48, 2, false);
		break;
	case SRCSTAT_FAILED_REFUSED:
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		Draw_FunStringWidth (x, y, "REFUSED", 48, 2, false);
		break;
	case SRCSTAT_FAILED_EOF:
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		Draw_FunStringWidth (x, y, "EOF", 48, 2, false);
		break;
	case SRCSTAT_FAILED_MITM:
		if ((int)(realtime*4) & 1)	//flash
			Draw_FunStringWidth (x, y, "^&04  ", 48, 2, false);	//red
		else
			Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		Draw_FunStringWidth (x, y, "^bMITM", 48, 2, false);
		break;
	case SRCSTAT_FAILED_HTTP:
		Draw_FunStringWidth (x, y, "^&0E  ", 48, 2, false);	//yellow
		Draw_FunStringWidth (x, y, "404", 48, 2, false);
		break;
	}

	text = va("%s%s",	(pm_source[c->dint].flags & (SRCFL_ENABLED|SRCFL_DISABLED))?"":"^b",
						pm_source[c->dint].url);
	Draw_FunStringU8 (CON_WHITEMASK, x+48, y, text);
}
static qboolean MD_Source_Key (struct menucustom_s *c, struct emenu_s *m, int key, unsigned int unicode)
{
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		if (pm_source[c->dint].flags & SRCFL_DISABLED)
		{
			pm_source[c->dint].flags = (pm_source[c->dint].flags&~SRCFL_DISABLED)|SRCFL_ENABLED;
			pm_source[c->dint].status = SRCSTAT_UNTRIED;
		}
		else
		{
			pm_source[c->dint].flags = (pm_source[c->dint].flags&~SRCFL_ENABLED)|SRCFL_DISABLED;
			pm_source[c->dint].status = SRCSTAT_PENDING;
		}
		PM_WriteInstalledPackages();
		if (pm_source[c->dint].flags & SRCFL_DISABLED)
			PM_Shutdown(true);
		PM_UpdatePackageList(true);
		return true;
	}
	if (key == K_DEL || key == K_BACKSPACE || key == K_GP_DIAMOND_ALTCONFIRM)
	{
		if (pm_source[c->dint].flags & SRCFL_ENABLED)
		{
			pm_source[c->dint].flags = (pm_source[c->dint].flags&~SRCFL_ENABLED)|SRCFL_DISABLED;
			pm_source[c->dint].status = SRCSTAT_PENDING;
		}
		else if (pm_source[c->dint].flags & SRCFL_USER)
		{
			pm_source[c->dint].flags &= ~(SRCFL_ENABLED|SRCFL_DISABLED);
			pm_source[c->dint].flags |= SRCFL_HISTORIC;
		}
		else
			return false;	//will just be re-added anyway... :(
		PM_WriteInstalledPackages();
		if (pm_source[c->dint].flags & SRCFL_DISABLED)
			PM_Shutdown(true);
		PM_UpdatePackageList(true);
		return true;
	}
	return false;
}

static void MD_AutoUpdate_Draw (int x, int y, struct menucustom_s *c, struct emenu_s *m)
{
	char *settings[] = 
	{
		"Off",
		"Stable Updates",
		"Test Updates"
	};
	char *text;
	int setting = bound(0, pkg_autoupdate.ival, 2);

	size_t i;
	for (i = 0; i < pm_numsources && !(pm_source[i].flags & SRCFL_ENABLED); i++);

	text = va("%sAuto Update: ^a%s", (i<pm_numsources)?"":"^h", settings[setting]);
//	if (&m->selecteditem->common == &c->common)
//		Draw_AltFunString (x, y, text);
//	else
		Draw_FunString (x, y, text);
}
static qboolean MD_AutoUpdate_Key (struct menucustom_s *c, struct emenu_s *m, int key, unsigned int unicode)
{
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		char nv[8] = "0";
		if (pkg_autoupdate.ival < UPD_TESTING && pkg_autoupdate.ival >= 0)
			Q_snprintfz(nv, sizeof(nv), "%i", pkg_autoupdate.ival+1);
		Cvar_ForceSet(&pkg_autoupdate, nv);
		PM_WriteInstalledPackages();

		PM_UpdatePackageList(true);
	}
	return false;
}

static qboolean MD_MarkUpdatesButton (union menuoption_s *mo,struct emenu_s *m,int key)
{
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		PM_MarkUpdates();
		return true;
	}
	return false;
}
#endif

qboolean MD_PopMenu (union menuoption_s *mo,struct emenu_s *m,int key)
{
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		M_RemoveMenu(m);
		return true;
	}
	return false;
}

static qboolean MD_ApplyDownloads (union menuoption_s *mo,struct emenu_s *m,int key)
{
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		PM_PromptApplyChanges();
		return true;
	}
	return false;
}

static qboolean MD_RevertUpdates (union menuoption_s *mo,struct emenu_s *m,int key)
{
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		PM_RevertChanges();
		return true;
	}
	return false;
}

static int MD_AddMapItems(emenu_t *m, package_t *p, int y, const char *filter)
{
	struct packagedep_s *dep;
	menucustom_t *c;
	for (dep = p->deps; dep; dep = dep->next)
	{
		if (dep->dtype != DEP_MAP)
			continue;
		if (filter)
			if (!strstr(dep->name, filter))
				continue;
		c = MC_AddCustom(m, 0, y, p, pm_sequence, NULL);
		c->dptr2 = dep;
		c->draw = MD_MapDraw;
		c->key = MD_MapKey;
		c->common.width = 320-16;
		c->common.height = 8;
		y += 8;
	}
	return y;
}
static int MD_DownloadMenuFiltered(package_t *p, const char *filter)
{
	struct packagedep_s *dep;
	if ((p->flags & DPF_HIDDEN) && (p->arch || !(p->flags & DPF_ENABLED)))
		return true;
	if ((p->flags & DPF_HIDEUNLESSPRESENT) && !(p->flags & DPF_PRESENT))
		return true;

	if (filter && *filter)
	{
		if (strstr(p->title, filter) ||
			strstr(p->name, filter) ||
			(p->description && strstr(p->description, filter)) ||
			(p->author && strstr(p->author, filter)))
			;
		else
		{
			for (dep = p->deps; dep; dep = dep->next)
			{
				if (dep->dtype == DEP_MAP)
					if (strstr(dep->name, filter))
						return -1;
			}
			if (!dep)
				return true;
		}
	}
	return false;
}

static int MD_AddItemsToDownloadMenu(emenu_t *m, int y, const char *pathprefix, const char *filter, void *selpackage)
{
	char path[MAX_QPATH];
	package_t *p;
	menucustom_t *c;
	char *slash;
	menuoption_t *mo;
	int prefixlen = strlen(pathprefix);
	struct packagedep_s *dep;
	dlmenu_t *info = m->data;
	int filtered;

	//add all packages in this dir
	for (p = availablepackages; p; p = p->next)
	{
		if (strncmp(p->category, pathprefix, prefixlen))
			continue;
		filtered = MD_DownloadMenuFiltered(p, filter);
		if (filtered==true)
			continue;
		slash = strchr(p->category+prefixlen, '/');
		if (!slash)
		{
			char *head;
			char *desc = p->description;
			if (p->author || p->license || p->website)
				head = va("^aauthor:^U00A0^a%s\n^alicense:^U00A0^a%s\n^awebsite:^U00A0^a%s\n", p->author?p->author:"^hUnknown^h", p->license?p->license:"^hUnknown^h", p->website?p->website:"^hUnknown^h");
			else
				head = NULL;
			if (p->filesize)
			{
				char buf[32];
				if (!head)
					head = "";
				head = va("%s^asize:^U00A0^a%s\n", head, FS_AbbreviateSize(buf,sizeof(buf), p->filesize));
			}

			for (dep = p->deps; dep; dep = dep->next)
			{
				if (dep->dtype == DEP_NEEDFEATURE)
				{
					const char *featname, *enablecmd;
					if (!PM_CheckFeature(dep->name, &featname, &enablecmd))
					{
						if (enablecmd)
							head = va("^aDisabled: ^a%s\n%s", featname, head?head:"");
						else
							head = va("^aUnavailable: ^a%s\n%s", featname, head?head:"");
					}
				}
			}
#ifdef WEBCLIENT
			if (!PM_SignatureOkay(p))
			{
				if (!p->signature)
					head = va(CON_ERROR"^bSignature missing^b"CON_DEFAULT"\n%s", head?head:"");			//some idiot forgot to include a signature
				else if (p->flags & DPF_SIGNATUREREJECTED)
					head = va(CON_ERROR"^bSignature invalid^b"CON_DEFAULT"\n%s", head?head:"");			//some idiot got the wrong auth/sig/hash
				else if (p->flags & DPF_SIGNATUREUNKNOWN)
					head = va(CON_ERROR"^bSignature is not trusted^b"CON_DEFAULT"\n%s", head?head:"");	//clientside permission.
				else
					head = va(CON_ERROR"^bUnable to verify signature^b"CON_DEFAULT"\n%s", head?head:"");	//clientside problem.
			}
#endif

			if (head && desc)
				desc = va(U8("%s\n%s"), head, desc);
			else if (head)
				desc = va(U8("%s"), head);

			if (developer.ival)
			{
				const char *root = "?/";
				switch(p->fsroot)
				{
				//valid roots
				case FS_ROOT:			root = "$BASEDIR/";				break;
				case FS_BINARYPATH:		root = "$BINDIR/";				break;
				case FS_LIBRARYPATH:	root = "$LIBDIR/";				break;

				//invalid roots... (we don't use fs_game etc for packages because that breaks when switching mods within the same basedir)
				case FS_GAME:			root = "$FS_GAME/";				break;
				case FS_GAMEONLY:		root = "$FS_GAMEONLY/";			break;
				case FS_BASEGAMEONLY:	root = "$FS_BASEGAMEONLY/";		break;
				case FS_PUBGAMEONLY:	root = "$FS_PUBGAMEONLY/";		break;
				case FS_PUBBASEGAMEONLY:root = "FS_PUBBASEGAMEONLY://";	break;
				case FS_SYSTEM:			root = "file://";				break;
				default:				root = "?/";					break;
				}
				if (!desc)
					desc = "";
				for (dep = p->deps; dep; dep = dep->next)
					if (dep->dtype == DEP_FILE)
					{
						if (p->qhash || p->packprefix)
							desc = va("%s\n%s%s%s%s.%s%s", desc, root, p->gamedir, *p->gamedir?"/":"", dep->name, p->qhash, p->packprefix);
						else
							desc = va("%s\n%s%s%s%s", desc, root, p->gamedir, *p->gamedir?"/":"", dep->name);
					}
			}

			c = MC_AddCustom(m, 0, y, p, pm_sequence, desc);
			c->draw = MD_Draw;
			c->key = MD_Key;
			c->common.width = 320-16;
			c->common.height = 8;
			y += 8;

			if (info->expandedpackage == p || filtered<0)
			{
				m->selecteditem = (menuoption_t*)c;

				y = MD_AddMapItems(m, p, y, (filtered==-1)?filter:NULL);
			}
			if (!m->selecteditem || p == selpackage)
				m->selecteditem = (menuoption_t*)c;
		}
	}

	//and then try to add any subdirs...
	for (p = availablepackages; p; p = p->next)
	{
		if (strncmp(p->category, pathprefix, prefixlen))
			continue;
		if (MD_DownloadMenuFiltered(p, filter)==true)
			continue;

		slash = strchr(p->category+prefixlen, '/');
		if (slash)
		{
			Q_strncpyz(path, p->category, MAX_QPATH);
			slash = strchr(path+prefixlen, '/');
			if (slash)
				*slash = '\0';

			for (mo = m->options; mo; mo = mo->common.next)
				if (mo->common.type == mt_text/*mt_button*/)
					if (!strcmp(mo->button.text, path + prefixlen))
						break;
			if (!mo)
			{
				y += 8;
				MC_AddBufferedText(m, 48, 320-16, y, path+prefixlen, false, true);
				y += 8;
				Q_strncatz(path, "/", sizeof(path));
				y = MD_AddItemsToDownloadMenu(m, y, path, filter, selpackage);
			}
		}
	}
	return y;
}

#include "shader.h"
static void MD_Download_UpdateStatus(struct emenu_s *m)
{
	dlmenu_t *info = m->data;
	int y;
	package_t *p;
	unsigned int totalpackages=0, selectedpackages=0, addpackages=0, rempackages=0;
	menuoption_t *si;
	menubutton_t *b, *d;
#ifdef WEBCLIENT
	int i;
	unsigned int downloads=0;
	menucustom_t *c;
	qboolean sources;
#endif
	float framefrac = 0;
	void *oldpackage = NULL;

	if (info->downloadablessequence != pm_sequence || !info->populated)
	{
		while(m->options)
		{
			menuoption_t *op = m->options;
			m->options = op->common.next;
			if (op->common.type == mt_frameend)
				framefrac = op->frame.frac;
			else if (m->selecteditem == op && op->common.type == mt_custom)
				oldpackage = op->custom.dptr;
			if (op->common.iszone)
				Z_Free(op);
		}
		m->cursoritem = m->selecteditem = m->mouseitem = NULL;
		info->downloadablessequence = pm_sequence;

		info->populated = false;
		MC_AddWhiteText(m, 24, 320, 8, "Downloads", false)->text = info->titletext;
		if (info->filtering || *info->filtertext)
			MC_AddWhiteText(m, 24, 320, 16, va("%sFilter: %s", info->filtering?"^m":"", info->filtertext), false);
		MC_AddWhiteText(m, 16, 320, 24, "^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f", false);

		//FIXME: should probably reselect the previous selected item. lets just assume everyone uses a mouse...
	}

	for (p = availablepackages; p; p = p->next)
	{
		if (p->alternative && (p->flags & DPF_HIDDEN))
			p = p->alternative;

		totalpackages++;
#ifdef WEBCLIENT
		if ((p->flags&DPF_PENDING) || p->trymirrors)
			downloads++;	//downloading or pending
#endif
		if (p->flags & DPF_MARKED)
		{
			if (p->flags & DPF_ENABLED)
			{
				selectedpackages++;
				if (p->flags & DPF_PURGE)
				{
					rempackages++;
					addpackages++;
				}
			}
			else
			{
				selectedpackages++;
				if (p->flags & DPF_PURGE)
					rempackages++;	//adding, but also deleting. how weird is that!
				addpackages++;
			}
		}
		else
		{
			if (p->flags & DPF_ENABLED)
				rempackages++;
			else
			{
				if (p->flags & DPF_PURGE)
					rempackages++;
			}
		}
	}

	//show status.
	if (cls.download)
	{	//we can actually download more than one thing at a time, but that makes the UI messy, so only show one active download here.
		if (cls.download->sizeunknown && cls.download->size == 0)
			Q_snprintfz(info->titletext, sizeof(info->titletext), "Downloads (%ukbps - %s)", CL_DownloadRate()/1000, cls.download->localname);
		else
			Q_snprintfz(info->titletext, sizeof(info->titletext), "Downloads (%u%% %ukbps - %s)", (int)cls.download->percent, CL_DownloadRate()/1000, cls.download->localname);
	}
	else if (!addpackages && !rempackages)
		Q_snprintfz(info->titletext, sizeof(info->titletext), "Downloads (%i of %i)", selectedpackages, totalpackages);
	else
		Q_snprintfz(info->titletext, sizeof(info->titletext), "Downloads (+%u -%u)", addpackages, rempackages);

	if (pkg_updating)
		Q_snprintfz(info->applymessage, sizeof(info->applymessage), "Apply (please wait)");
	else if (addpackages || rempackages)
		Q_snprintfz(info->applymessage, sizeof(info->applymessage), "%sApply (+%u -%u)", ((int)(realtime*4)&3)?"^a":"", addpackages, rempackages);
	else
		Q_snprintfz(info->applymessage, sizeof(info->applymessage), "Apply");

	if (!info->populated)
	{
		y = 48;

		info->populated = true;
		MC_AddFrameStart(m, 48);
		if (!info->filtering)
		{
#ifdef WEBCLIENT
			for (i = 0, sources=false; i < pm_numsources; i++)
			{
				if (pm_source[i].flags & SRCFL_HISTORIC)
					continue;	//historic... ignore it.
				if (!sources)
					MC_AddBufferedText(m, 48, 320-16, y, "Sources", false, true), y += 8;
				sources=true;
				c = MC_AddCustom(m, 0, y, p, i, NULL);
				c->draw = MD_Source_Draw;
				c->key = MD_Source_Key;
				c->common.width = 320-48-16;
				c->common.height = 8;

				if (!m->selecteditem)
					m->selecteditem = (menuoption_t*)c;
				y += 8;
			}
			y+=4;	//small gap
#endif
			MC_AddBufferedText(m, 48, 320-16, y, "Options", false, true), y += 8;
			b = MC_AddCommand(m, 48, 320-16, y, info->applymessage, MD_ApplyDownloads);
			b->rightalign = false;
			b->common.tooltip = "Enable/Disable/Download/Delete packages to match any changes made (you will be prompted with a list of the changes that will be made).";
			y+=8;
			d = b = MC_AddCommand(m, 48, 320-16, y, "Back", MD_PopMenu);
			b->rightalign = false;
			y+=8;
#ifdef WEBCLIENT
			if (pm_numsources)
			{
				b = MC_AddCommand(m, 48, 320-16, y, "Mark Updates", MD_MarkUpdatesButton);
				b->rightalign = false;
				b->common.tooltip = "Select any updated versions of packages that are already installed.";
				y+=8;
			}
#endif
			b = MC_AddCommand(m, 48, 320-16, y, "Undo Changes", MD_RevertUpdates);
			b->rightalign = false;
			b->common.tooltip = "Reset selection to only those packages that are currently installed.";
			y+=8;
#ifdef WEBCLIENT
			if (pm_numsources)
			{
				c = MC_AddCustom(m, 48, y, p, 0, NULL);
				c->draw = MD_AutoUpdate_Draw;
				c->key = MD_AutoUpdate_Key;
				c->common.width = 320-48-16;
				c->common.height = 8;
				y += 8;
			}
#endif
			y+=4;	//small gap
		}
		else d = NULL;
		MC_AddBufferedText(m, 48, 320-16, y, "Packages", false, true), y += 8;
		MD_AddItemsToDownloadMenu(m, y, info->pathprefix, info->filtertext, oldpackage);
		if (!m->selecteditem)
			m->selecteditem = (menuoption_t*)d;
		if (info->filtering)
			m->cursoritem = NULL;
		else
			m->cursoritem = (menuoption_t*)MC_AddWhiteText(m, 40, 0, m->selecteditem->common.posy, NULL, false);
		MC_AddFrameEnd(m, 48)->frac = framefrac;
	}

	si = m->mouseitem;
	if (!si)
		si = m->selecteditem;
	if (si && si->common.type == mt_custom && si->custom.dptr)
	{
		package_t *p = si->custom.dptr;
		if (p->previewimage)
		{
			shader_t *sh = R_RegisterPic(p->previewimage, NULL);
			if (R_GetShaderSizes(sh, NULL, NULL, false) > 0)
				R2D_Image(0, 0, vid.width, vid.height, 0, 0, 1, 1, sh);
		}
	}
}

static qboolean MD_Download_Key(struct emenu_s *m, int key, unsigned int unicode)
{
	dlmenu_t *info = m->data;

	if (info->filtering)
	{
		if (key == K_BACKSPACE)
		{
			unsigned int l = strlen(info->filtertext);
			if (l)
			{
				while (l > 0 && (info->filtertext[--l]&0xc0) == 0x80)
					;
				info->filtertext[l] = 0;
			}
			else
				info->filtering = false;
		}
		else if (key == K_ENTER || key == K_ESCAPE)
			info->filtering = false; //done...
		else if (key>=K_F1 && key!=K_RALT && key!=K_RCTRL && key!=K_RSHIFT)
		{	//some other action... don't swallow clicks.
			info->filtering = false;
			info->populated = false;
			return false;
		}
		else if (unicode)
		{
			unsigned int l = strlen(info->filtertext);
			l+=utf8_encode(info->filtertext+l, unicode, sizeof(info->filtertext)-1-l);
			info->filtertext[l] = 0;
		}

		info->populated = false;
		return true;
	}
	else if (key == '/')
	{
		info->filtering = true;
		info->filtertext[0] = 0;

		info->populated = false;
		return true;
	}
	else
	{
		info->filtering = false;
		return false;
	}
}

void Menu_DownloadStuff_f (void)
{
	emenu_t *menu;
	dlmenu_t *info;

	Key_Dest_Remove(kdm_console|kdm_cwindows);
	menu = M_CreateMenu(sizeof(dlmenu_t));
	info = menu->data;

	menu->menu.persist = true;
	menu->predraw = MD_Download_UpdateStatus;
	menu->key = MD_Download_Key;
	info->downloadablessequence = pm_sequence;


	Q_strncpyz(info->pathprefix, Cmd_Argv(1), sizeof(info->pathprefix));
	if (!*info->pathprefix || !loadedinstalled)
		PM_UpdatePackageList(false);

	info->populated = false;	//will add any headers as needed
}

#ifdef WEBCLIENT
static void PM_ConfirmSource(void *ctx, promptbutton_t button)
{ //yes='Enable', no='Configure', cancel='Later'...
	size_t i;
	pm_pendingprompts--;
	if (button == PROMPT_YES)
	{
		for (i = 0; i < pm_numsources; i++)
		{
			if (!strcmp(pm_source[i].url, ctx))
			{
				pm_source[i].flags |= (button == PROMPT_YES)?SRCFL_ENABLED:SRCFL_DISABLED;
				PM_WriteInstalledPackages();
				PM_UpdatePackageList(true);
				break;
			}
		}
	}
	else	//cancel or 'customize'
	{
		for (i = 0; i < pm_numsources; i++)
			pm_source[i].flags |= SRCFL_PROMPTED;

		if (button == PROMPT_NO)
			Cmd_ExecuteString("menu_download\n", RESTRICT_LOCAL);
		else
			doautoupdate = false; //try don't want updates... don't give them any.
	}
}

//given a url, try to chop it down to just a hostname
static const char *PrettyHostFromURL(const char *origurl)
{
	char *url, *end;
	url = va("%s", origurl);
	if (!strnicmp(url, "https://", 8))
		url+=8;
	else if (!strnicmp(url, "http://", 7))
		url+=7;
	else
		return origurl;

	url = va("%s", url);
	if (*url == '[')
	{	//ipv6 host
		end = strchr(url+1, ']');
		if (!end)
			return origurl;
		*end = 0;
	}
	else
	{	//strip any resource part.
		end = strchr(url, '/');
		if (end)
			*end = 0;
		//strip any explicit port number.
		end = strrchr(url, ':');
		if (end)
			*end = 0;
	}

	return url;
}
#endif

qboolean PM_AreSourcesNew(qboolean doprompt)
{
	qboolean ret = false;
#ifdef WEBCLIENT
	//only prompt if autoupdate is actually enabled.
	int i;
	for (i = 0; i < pm_numsources; i++)
	{
		if (pm_source[i].flags & SRCFL_HISTORIC)
			continue;	//hidden anyway
		if (!(pm_source[i].flags & (SRCFL_ENABLED|SRCFL_DISABLED|SRCFL_PROMPTED)) && (pkg_autoupdate.ival > 0 || (pm_source[i].flags&SRCFL_MANIFEST)))
		{
			ret = true;	//something is dirty.
			if (doprompt)
			{
				const char *msg = va(localtext("Enable update source\n\n^x66F%s"), (pm_source[i].flags&SRCFL_MANIFEST)?PrettyHostFromURL(pm_source[i].url):pm_source[i].url);
				pm_pendingprompts++;
				Menu_Prompt(PM_ConfirmSource, Z_StrDup(pm_source[i].url), msg, "Enable", "Configure", "Later", true);
				pm_source[i].flags |= SRCFL_PROMPTED;
			}
			break;
		}
	}
#endif
	return ret;
}

//should only be called AFTER the filesystem etc is inited.
void Menu_Download_Update(void)
{
	if (pkg_autoupdate.ival <= 0)
		return;

	PM_UpdatePackageList(true);
}
#else
void Menu_Download_Update(void)
{
#ifdef PACKAGEMANAGER
	PM_UpdatePackageList(true);
#endif
}
void Menu_DownloadStuff_f (void)
{
	Con_Printf("Download menu not implemented in this build\n");
}
#endif
