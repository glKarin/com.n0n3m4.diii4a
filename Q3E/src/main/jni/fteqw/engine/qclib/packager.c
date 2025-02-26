#include "qcc.h"
#if !defined(MINIMAL) && !defined(OMIT_QCC)
#include <time.h>
#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif
void QCC_JoinPaths(char *fullname, size_t fullnamesize, const char *newfile, const char *base);

//package formats:
//pakzip - files are uncompressed, with both a pak header and a zip trailer, allowing it to be read as either type of file.
//zip - standard zips
//spanned zip - the full list of files is written into a separate central-directory-only zip, the actual file data comes from regular zips named foo.z##/.p## instead of foo.zip/foo.pk3

/*
dataset common
{
	output default data.pk3
	output logic textures.pk3
}
dataset desktop
{
	output tex textures_pc.pk3
}
dataset mobile
{
	output tex textures_mobile.pk3
}
input pak0.pk3

rule dxt1 {
 dataset desktop
 output tex
 newext dds
 command "@\"c:/program files/Compressonator/CompressonatorCLI\" -fd DXT1 $input $output"
}

rule etc2 {
 dataset mobile
 output tex
 newext ktx
 command "@\"c:/program files/Compressonator/CompressonatorCLI\" -fd ETC2 $input $output"
}

logic {
	progs.dat
}
class texa0 {
	output tex
	desktop: dxt1
	mobile: etc2
}
texa0
{
	gfx/conback.txt 
}
*/

#define quint64_t long long
#define qofs_t size_t

#define countof(x) (sizeof(x)/sizeof((x)[0]))

struct pkgctx_s
{
	void (*messagecallback)(void *userctx, const char *message, ...);
	void *userctx;

	char *listfile;

	pbool test;
	pbool readoldpacks;
	char gamepath[MAX_OSPATH];
	char sourcepath[MAX_OSPATH];
	time_t buildtime;

	//skips the file if its listed in one of these packages, unless the modification time on disk is newer.
	struct oldpack_s
	{
		struct oldpack_s *next;
		char filename[128];
		size_t numfiles;
		unsigned int part;
		struct
		{
			char name[128];

			unsigned short zmethod;
			unsigned int zcrc;
			qofs_t zhdrofs;
			qofs_t rawsize;
			qofs_t zipsize;
			unsigned short dostime;
			unsigned short dosdate;
		} *file;
	} *oldpacks;

	struct dataset_s
	{
		struct dataset_s *next;

		//these are the output pk3s from this package.
		struct output_s
		{
			struct output_s *next;
			char code[128];
			char filename[128];
			struct file_s *files;

			pbool usediffs;
			unsigned int numparts;
			struct oldpack_s *oldparts;
		} *outputs;

		char name[1];
	} *datasets;
	struct rule_s
	{
		struct rule_s *next;
		char name[128];

		int dropfile:1;

		char *newext;
		char *command;
	} *rules;

	struct class_s
	{
		char name[128];
		struct class_s *next;

		//the output package codename to write to. class is skipped if the dataset doesn't include that name.
		char outname[128];

		struct
		{
			struct dataset_s *set;
			struct rule_s *rule;
		} dataset[8];
		struct rule_s *defaultrule;

		struct file_s
		{
			struct file_s *next;
			char name[128];

			//temp data for tracking what's getting written.
			struct
			{
				char name[128];
				struct file_s *nextwrite;
				struct rule_s *rule;
				unsigned int zdisk;
				unsigned short zmethod;
				unsigned int zcrc;
				qofs_t zhdrofs;
				qofs_t pakofs;
				qofs_t rawsize;
				qofs_t zipsize;
				unsigned short dostime;
				unsigned short dosdate;
				time_t	timestamp;
			} write;
		} *files;
	} *classes;
};

#ifdef _WIN32
static time_t filetime_to_timet(FILETIME ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
#endif

static struct rule_s *PKG_FindRule(struct pkgctx_s *ctx, char *code)
{
	struct rule_s *o;
	for (o = ctx->rules; o; o = o->next)
	{
		if (!strcmp(o->name, code))
			return o;
	}
	return NULL;
}
static struct class_s *PKG_FindClass(struct pkgctx_s *ctx, char *code)
{
	struct class_s *c;
	for (c = ctx->classes; c; c = c->next)
	{
		if (!strcmp(c->name, code))
			return c;
	}
	return NULL;
}
static struct dataset_s *PKG_FindDataset(struct pkgctx_s *ctx, const char *code)
{
	struct dataset_s *o;
	for (o = ctx->datasets; o; o = o->next)
	{
		if (!strcmp(o->name, code))
			return o;
	}
	return NULL;
}
static struct dataset_s *PKG_GetDataset(struct pkgctx_s *ctx, const char *code)
{
	struct dataset_s *s = PKG_FindDataset(ctx, code);
	if (!s)
	{
		s = malloc(sizeof(*s)+strlen(code));
		strcpy(s->name, code);
		s->outputs = NULL;
		s->next = ctx->datasets;
		ctx->datasets = s;
	}
	return s;
}

static pbool PKG_SkipWhite(struct pkgctx_s *ctx, pbool linebreak)
{
	for(;;)
	{
		if (qcc_iswhite(*ctx->listfile))
		{
			if (qcc_islineending(ctx->listfile[0], ctx->listfile[1]) && !linebreak)
				return false;
			ctx->listfile++;
			continue;
		}
		if (ctx->listfile[0] == '/' && ctx->listfile[1] == '/')
		{
			while (!qcc_islineending(ctx->listfile[0], ctx->listfile[1]))
				ctx->listfile++;
			continue;
		}
		if (ctx->listfile[0] == '/' && ctx->listfile[1] == '*')
		{
			ctx->listfile+=2;
			while (*ctx->listfile)
			{
				if (ctx->listfile[0]=='*' && ctx->listfile[1]=='/')
				{
					ctx->listfile+=2;
					break;
				}
				ctx->listfile++;
			}
			continue;
		}
		break;
	}
	return true;
}
static pbool PKG_GetToken(struct pkgctx_s *ctx, char *token, size_t sizeoftoken, pbool linebreak)
{
	if (!PKG_SkipWhite(ctx, linebreak))
		return false;
	if (*ctx->listfile)
	{
		while(*ctx->listfile)
		{
			if (qcc_iswhite(*ctx->listfile))
				break;
			*token = *ctx->listfile++;
			if (sizeoftoken > 1)
			{
				token++;
				sizeoftoken--;
			}
		}
		*token = 0;
		return true;
	}
	return false;
}

static pbool PKG_GetStringToken(struct pkgctx_s *ctx, char *token, size_t sizeoftoken)
{
	if (!PKG_SkipWhite(ctx, false))
		return false;
	if (*ctx->listfile == '\"')
	{
		ctx->listfile++;
		while(*ctx->listfile)
		{
			if (*ctx->listfile == '\"')
			{
				ctx->listfile++;
				break;
			}
			else if (*ctx->listfile == '\\')
			{
				ctx->listfile++;
				switch(*ctx->listfile++)
				{
				case '\"':	*token = '\"'; break;
				case '\\':	*token = '\\'; break;
				case '\r':	*token = '\r'; break;
				case '\n':	*token = '\n'; break;
				case '\t':	*token = '\t'; break;
				default:	*token = '?'; break;
				}
				sizeoftoken--;
			}
			else
				*token = *ctx->listfile++;
			if (sizeoftoken > 1)
			{
				token++;
				sizeoftoken--;
			}
		}
		*token = 0;
		return true;
	}
	return false;
}

static pbool PKG_Expect(struct pkgctx_s *ctx, char *token)
{
	char tok[128];
	if (PKG_GetToken(ctx, tok, sizeof(tok), true))
	{
		if (!strcmp(tok, token))
			return true;
	}
	ctx->messagecallback(ctx->userctx, "Expected '%s', found '%s'\n", token, tok);
	return false;
}

static void PKG_ReplaceString(char *str, char *find, char *newpart)
{
	char *oldpart;
	size_t oldlen = strlen(find);
	size_t nlen = strlen(newpart);
	while((oldpart = strstr(str, find)))
	{
		memmove(oldpart+nlen, oldpart+oldlen, strlen(oldpart+oldlen)+1);
		memmove(oldpart, newpart, nlen);
		str = oldpart+nlen;
	}
}
static void PKG_CreateOutput(struct pkgctx_s *ctx, struct dataset_s *s, const char *code, const char *filename, pbool diff)
{
	char path[MAX_OSPATH];
	char date[64];
	struct output_s *o;
	for (o = s->outputs; o; o = o->next)
	{
		if (!strcmp(o->code, code))
		{
			ctx->messagecallback(ctx->userctx, "Dataset '%s' defined with dupe output\n", s->name, code);
			return;
		}
	}

	if (strlen(code) >= sizeof(o->code))
	{
		ctx->messagecallback(ctx->userctx, "Output '%s' name too long\n", code);
		return;
	}

	strcpy(path, filename);
	strftime(date, sizeof(date), "%Y%m%d", localtime(&ctx->buildtime));
	PKG_ReplaceString(path, "$date", date);

	o = malloc(sizeof(*o));
	memset(o, 0, sizeof(*o));
	strcpy(o->code, code);
	o->usediffs = diff;
	QCC_JoinPaths(o->filename, sizeof(o->filename), path, ctx->gamepath);
	o->next = s->outputs;
	s->outputs = o;


	if (diff)
	{
		char *end = path + strlen(path)-2;
		unsigned int i;
		for (i = 0; i <= 99; i++)
		{
#ifdef _WIN32
			struct _stat statbuf;
#else
			struct stat statbuf;
#endif
			sprintf(end, "%02u", i+1);
#ifdef _WIN32
			//FIXME: use the utf16 version because microsoft suck and don't allow utf-8
			if (_stat(path, &statbuf) == 0)
#else
			if (stat(path, &statbuf) == 0)
#endif
			{
				struct oldpack_s *span = malloc(sizeof(*span));
				strcpy(span->filename, path);
				span->numfiles = 0;
				span->file = NULL;
				span->next = o->oldparts;
				span->part = i;
				o->oldparts = span;
			}
		}
	}
}

static void PKG_ParseOutput(struct pkgctx_s *ctx, pbool diff)
{
	struct dataset_s *s;
	char name[128];
	char prop[128];
	char fname[128];

	if (!PKG_GetToken(ctx, name, sizeof(name), false))
	{
		ctx->messagecallback(ctx->userctx, "Output: Expected name\n");
		return;
	}

	if (PKG_GetStringToken(ctx, prop, sizeof(prop)))
	{
		s = PKG_GetDataset(ctx, "core");
		PKG_CreateOutput(ctx, s, name, prop, diff);
	}
	else
	{
		if (!PKG_Expect(ctx, "{"))
			return;
		while(PKG_GetToken(ctx, prop, sizeof(prop), true))
		{
			if (!strcmp(prop, "}"))
				break;
			else
			{
				char *e = strchr(prop, ':');
				if (e && !e[1])
				{
					*e = 0;
					s = PKG_GetDataset(ctx, prop);
					if (PKG_GetStringToken(ctx, fname, sizeof(fname)))
						PKG_CreateOutput(ctx, s, name, fname, diff);
					else
						ctx->messagecallback(ctx->userctx, "Output '%s[%s]' filename omitted\n", name, prop);
				}
				else
					ctx->messagecallback(ctx->userctx, "Output '%s' has unknown property '%s'\n", name, prop);
			}

			//skip any junk
			while(PKG_GetToken(ctx, prop, sizeof(prop), false))
			{
				if (!strcmp(prop, ";"))
					break;
			}
		}
	}
}

#ifdef _WIN32
static void PKG_AddOldPack(struct pkgctx_s *ctx, const char *fname)
{
	struct oldpack_s *pack;

	pack = malloc(sizeof(*pack));
	strcpy(pack->filename, fname);
	pack->numfiles = 0;
	pack->file = NULL;
	pack->next = ctx->oldpacks;
	ctx->oldpacks = pack;
}
#endif

static void PKG_ParseOldPack(struct pkgctx_s *ctx)
{
	char token[MAX_OSPATH];

	if (!PKG_GetStringToken(ctx, token, sizeof(token)))
		return;

#ifdef _WIN32
	{
		char oldpack[MAX_OSPATH];
		WIN32_FIND_DATA fd;
		HANDLE h;
		QCC_JoinPaths(oldpack, sizeof(oldpack), token, ctx->gamepath);
		h = FindFirstFile(oldpack, &fd);
		if (h == INVALID_HANDLE_VALUE)
			ctx->messagecallback(ctx->userctx, "wildcard string '%s' found no files\n", token);
		else
		{
			do
			{
				QCC_JoinPaths(token, sizeof(token), fd.cFileName, oldpack);
				PKG_AddOldPack(ctx, token);
			} while(FindNextFile(h, &fd));
		}
	}
#else
	ctx->messagecallback(ctx->userctx, "no wildcard support, sorry\n");
#endif
}
/*
static void PKG_ParseDataset(struct pkgctx_s *ctx)
{
	struct dataset_s *s;
	char name[128];
	char prop[128];

	if (!PKG_GetToken(ctx, name, sizeof(name), false))
	{
		ctx->messagecallback(ctx->userctx, "Dataset: Expected name\n");
		return;
	}

	if (strlen(name) >= sizeof(s->name))
	{
		ctx->messagecallback(ctx->userctx, "Dataset '%s' name too long\n", name);
		return;
	}

	s = malloc(sizeof(*s));
	memset(s, 0, sizeof(*s));
	strcpy(s->name, name);

	if (PKG_Expect(ctx, "{"))
	{
		while(PKG_GetToken(ctx, prop, sizeof(prop), true))
		{
			if (!strcmp(prop, "}"))
				break;
			else if (!strcmp(prop, "output"))
			{
				if (PKG_GetToken(ctx, name, sizeof(name), false))
					if (PKG_GetStringToken(ctx, prop, sizeof(prop)))
					{
						PKG_CreateOutput(ctx, s, name, prop);
					}
			}
			else if (!strcmp(prop, "base"))
				PKG_GetStringToken(ctx, prop, sizeof(prop));
			else
				ctx->messagecallback(ctx->userctx, "Dataset '%s' has unknown property '%s'\n", name, prop);

			//skip any junk
			while(PKG_GetToken(ctx, prop, sizeof(prop), false))
			{
				if (!strcmp(prop, ";"))
					break;
			}
		}
	}

	if (PKG_FindDataset(ctx, name))
		ctx->messagecallback(ctx->userctx, "Dataset '%s' is already defined\n", name);
	else
	{	//link it in!
		s->next = ctx->datasets;
		ctx->datasets = s;
		return;
	}
	PKG_DestroyDataset(s);
	return;
}*/

static void PKG_ParseRule(struct pkgctx_s *ctx)
{
	struct rule_s *r;
	char name[128];
	char prop[128];
	char newext[128];
	char command[4096];
	int dropfile = false;

	if (!PKG_GetToken(ctx, name, sizeof(name), false))
		return;

	if (strlen(name) >= sizeof(r->name))
	{
		ctx->messagecallback(ctx->userctx, "Rule '%s' name too long\n", name);
		return;
	}

	*newext = *command = 0;
	if (PKG_Expect(ctx, "{"))
	{
		while(PKG_GetToken(ctx, prop, sizeof(prop), true))
		{
			if (!strcmp(prop, "}"))
				break;
			else if (!strcmp(prop, "newext"))
				PKG_GetToken(ctx, newext, sizeof(newext), false);
			else if (!strcmp(prop, "skip"))
			{
				if (PKG_GetToken(ctx, prop, sizeof(prop), false))
					dropfile = atoi(prop);
				else
					dropfile = true;
			}
			else if (!strcmp(prop, "command"))
				PKG_GetStringToken(ctx, command, sizeof(command));
			else
				ctx->messagecallback(ctx->userctx, "Rule '%s' has unknown property '%s'\n", name, prop);

			//skip any junk
			while(PKG_GetToken(ctx, prop, sizeof(prop), false))
			{
				if (!strcmp(prop, ";"))
					break;
			}
		}
	}

	r = PKG_FindRule(ctx, name);
	if (r)
	{
		ctx->messagecallback(ctx->userctx, "Rule %s is already defined\n", name);
		return;
	}

	r = malloc(sizeof(*r));
	memset(r, 0, sizeof(*r));
	strcpy(r->name, name);
	r->newext = strdup(newext);
	r->command = strdup(command);
	r->dropfile = dropfile;
	r->next = ctx->rules;
	ctx->rules = r;
}
static void PKG_AddClassFile(struct pkgctx_s *ctx, struct class_s *c, const char *fname, time_t mtime)
{
	struct file_s *f;
	struct tm *t;

	if (strlen(fname) >= sizeof(f->name))
	{
		ctx->messagecallback(ctx->userctx, "File name '%s' too long in class %s\n", fname, c->name);
		return;
	}

	f = malloc(sizeof(*f));
	memset(f, 0, sizeof(*f));
	strcpy(f->name, fname);
	f->write.timestamp = mtime;
	t = localtime(&f->write.timestamp);
	f->write.dostime = (t->tm_sec>>1)|(t->tm_min<<5)|(t->tm_hour<<11);
	f->write.dosdate = (t->tm_mday<<0)|(t->tm_mon<<5)|((t->tm_year+1900-1980)<<9);
	f->next = c->files;
	c->files = f;
}
static void PKG_AddClassFiles(struct pkgctx_s *ctx, struct class_s *c, const char *fname)
{
#ifdef _WIN32
	WIN32_FIND_DATA fd;
	HANDLE h;
	char basepath[MAX_PATH];
	QCC_JoinPaths(basepath, sizeof(basepath), fname, ctx->sourcepath);
	h = FindFirstFile(basepath, &fd);
	if (h == INVALID_HANDLE_VALUE)
		ctx->messagecallback(ctx->userctx, "wildcard string '%s' found no files\n", fname);
	else
	{
		do
		{
			QCC_JoinPaths(basepath, sizeof(basepath), fd.cFileName, fname);
			PKG_AddClassFile(ctx, c, basepath, filetime_to_timet(fd.ftLastWriteTime));
		} while(FindNextFile(h, &fd));
	}
#else
	DIR *dir;
	struct dirent *ent;
	char basepath[MAX_OSPATH], tmppath[MAX_OSPATH];
	struct stat statbuf;

	QCC_JoinPaths(basepath, sizeof(basepath), fname, ctx->sourcepath);
	QC_strlcat(basepath, "/", sizeof(basepath));
	dir = opendir(basepath);
	if (!dir)
	{
		ctx->messagecallback(ctx->userctx, "unable to open dir %s\n", basepath);
		return;
	}
	while ((ent = readdir(dir)))
	{
		if (*ent->d_name == '.')
			continue;
		QCC_JoinPaths(basepath, sizeof(basepath), ent->d_name, fname);
		QCC_JoinPaths(tmppath, sizeof(tmppath), basepath, ctx->sourcepath);
		if (stat(tmppath, &statbuf)!=0)
			continue;

		switch (statbuf.st_mode & S_IFMT)
		{
		default:	//some weird file type. shouldn't be a symlink sadly.
//			ctx->messagecallback(ctx->userctx, "found weird %s\n", basepath);
			break;
		case S_IFDIR:
			QC_strlcat(basepath, "/", sizeof(basepath));
//			ctx->messagecallback(ctx->userctx, "found dir %s\n", basepath);
			PKG_AddClassFiles(ctx, c, basepath);
			break;
		case S_IFREG:
//			ctx->messagecallback(ctx->userctx, "found file %s\n", basepath);
			PKG_AddClassFile(ctx, c, basepath, statbuf.st_mtime);
			break;
		}
	}
	closedir(dir);
#endif
}
static void PKG_ParseClass(struct pkgctx_s *ctx, char *output)
{
	struct class_s *c;
	struct rule_s *r;
	struct dataset_s *s;
	char *e;
	char name[128];
	char prop[128];
	size_t u;

	if (output)
	{
		if (!PKG_Expect(ctx, "{"))
			return;
		*name = 0;
	}
	else if (!PKG_GetToken(ctx, name, sizeof(name), false))
		return;

	if (output || !strcmp(name, "{"))
	{
		c = malloc(sizeof(*c));
		memset(c, 0, sizeof(*c));
		strcpy(c->name, "");
		strcpy(c->outname, (output && *output)?output:"default");
		c->next = ctx->classes;
		ctx->classes = c;
	}
	else
	{
		if (strlen(name) >= sizeof(c->name))
		{
			ctx->messagecallback(ctx->userctx, "Class '%s' name too long\n", name);
			return;
		}

		c = PKG_FindClass(ctx, name);
		if (!c)
		{
			c = malloc(sizeof(*c));
			memset(c, 0, sizeof(*c));
			strcpy(c->name, name);
			strcpy(c->outname, (output && *output)?output:"default");
			c->next = ctx->classes;
			ctx->classes = c;
		}

		if (!PKG_Expect(ctx, "{"))
			return;
	}

	{
		while(PKG_GetToken(ctx, prop, sizeof(prop), true))
		{
			if (!strcmp(prop, "}"))
				break;
			else if (!strcmp(prop, "output"))
				PKG_GetToken(ctx, c->outname, sizeof(c->outname), false);
			else if (!strcmp(prop, "rule"))
			{
				if (PKG_GetToken(ctx, prop, sizeof(prop), false))
				{
					if (c->defaultrule)
						ctx->messagecallback(ctx->userctx, "Class '%s' already has a default rule\n", name);
					c->defaultrule = PKG_FindRule(ctx, prop);
					if (!c->defaultrule)
						ctx->messagecallback(ctx->userctx, "Class '%s' specifies unknown rule %s\n", name, prop);
				}
			}
			else
			{
				e = strchr(prop, ':');
				if (e && !e[1])
				{
					*e = 0;
					s = PKG_FindDataset(ctx, prop);
					PKG_GetToken(ctx, prop, sizeof(prop), false);
					if (s)
					{
						r = PKG_FindRule(ctx, prop);
						for (u = 0; ; u++)
						{
							if (u == countof(c->dataset))
							{
								ctx->messagecallback(ctx->userctx, "Class '%s' specialises for too many datasets\n", c->name, s->name);
								break;
							}
							if (c->dataset[u].set == s)
								ctx->messagecallback(ctx->userctx, "Class '%s' already defines a rule for dataset '%s'\n", c->name, s->name);
							else if (!c->dataset[u].set)
							{
								c->dataset[u].set = s;
								c->dataset[u].rule = r;
								break;
							}
						}
					}
				}
				else if (strchr(prop, '.'))
				{
//					if (strchr(prop, '*') || strchr(prop, '?'))
						PKG_AddClassFiles(ctx, c, prop);
//					else
//						PKG_AddClassFile(ctx, c, prop);
				}
				else
					ctx->messagecallback(ctx->userctx, "Class '%s' has unknown property '%s'\n", name, prop);
			}

			//skip any junk
			while(PKG_GetToken(ctx, prop, sizeof(prop), false))
			{
				if (!strcmp(prop, ";"))
					break;
			}
		}
	}
}
static void PKG_ParseClassFiles(struct pkgctx_s *ctx, struct class_s *c)
{
	char prop[128];

	if (PKG_Expect(ctx, "{"))
	{
		while(PKG_GetToken(ctx, prop, sizeof(prop), true))
		{
			if (!strcmp(prop, "}"))
				break;
			if (!strcmp(prop, ";"))
				continue;

//			if (strchr(prop, '*') || strchr(prop, '?'))
				PKG_AddClassFiles(ctx, c, prop);
//			else
//				PKG_AddClassFile(ctx, c, prop);
		}
	}
}




#ifdef AVAIL_ZLIB
#include <zlib.h>
static unsigned int PKG_DeflateToFile(FILE *f, unsigned int rawsize, void *in, int method)
{
	char out[8192];
	int i=0;

	z_stream strm = {
		(char *)in,
		rawsize,
		0,

		out,
		sizeof(out),
		0,

		NULL,
		NULL,

		NULL,
		NULL,
		NULL,

		Z_BINARY,
		0,
		0
	};

	if (method == 8)
		deflateInit2(&strm, 9, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);		//zip deflate compression
	else
		deflateInit(&strm, Z_BEST_COMPRESSION);	//zlib compression
	while(deflate(&strm, Z_FINISH) == Z_OK)
	{
		fwrite(out, 1, sizeof(out) - strm.avail_out, f);	//compress in chunks of 8192. Saves having to allocate a huge-mega-big buffer
		i+=sizeof(out) - strm.avail_out;
		strm.next_out = out;
		strm.avail_out = sizeof(out);
	}
	deflateEnd(&strm);
	fwrite(out, 1, sizeof(out) - strm.avail_out, f);
	i+=sizeof(out) - strm.avail_out;
	return i;
}
#endif

#ifdef _WIN32
static void StupidWindowsPopenAlternativeCrap(struct pkgctx_s *ctx, char *commandline)
{
	PROCESS_INFORMATION piProcInfo = {0}; 
	SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	STARTUPINFO siStartInfo = {sizeof(STARTUPINFO)};
	HANDLE readpipe = INVALID_HANDLE_VALUE;
	HANDLE writepipe = INVALID_HANDLE_VALUE;
	siStartInfo.hStdError = siStartInfo.hStdOutput = siStartInfo.hStdInput = INVALID_HANDLE_VALUE;
	if (CreatePipe(&readpipe, &siStartInfo.hStdOutput, &saAttr, 0))
	{
		if (CreatePipe(&siStartInfo.hStdInput, &writepipe, &saAttr, 0))
		{
			SetHandleInformation(readpipe, HANDLE_FLAG_INHERIT, 0);
			SetHandleInformation(writepipe, HANDLE_FLAG_INHERIT, 0);
			siStartInfo.hStdError = siStartInfo.hStdOutput;
			siStartInfo.dwFlags |= STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW/*ZOMGWTFBBQ*/;
			if (!CreateProcess(NULL, (*commandline=='@')?commandline+1:commandline,  NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) 
				ctx->messagecallback(ctx->userctx, "Unable to execute command %s\n", commandline);
			else 
			{
				CloseHandle(piProcInfo.hProcess);
				CloseHandle(piProcInfo.hThread);
			}
		}
	}
	CloseHandle(siStartInfo.hStdOutput);
	CloseHandle(siStartInfo.hStdInput);

	CloseHandle(writepipe);
	for (;;) 
	{ 
		char buf[64];
		DWORD SHOUTY;
		if (!ReadFile(readpipe, buf, sizeof(buf)-1, &SHOUTY, NULL) || SHOUTY == 0)
			break;
		if (*commandline == '@')
			continue;
		buf[SHOUTY] = 0;
		ctx->messagecallback(ctx->userctx, "%s", buf);
	}
	CloseHandle(readpipe);
}
#endif

static void *PKG_OpenSourceFile(struct pkgctx_s *ctx, struct file_s *file, size_t *fsize)
{
	char fullname[1024];
	FILE *f;
	char *data;
	size_t size;
	struct rule_s *rule = file->write.rule;

	*fsize = 0;

	QCC_JoinPaths(fullname, sizeof(fullname), file->name, ctx->sourcepath);
	strcpy(file->write.name, file->name);

	//WIN32 FIXME: use the utf16 version because microsoft suck and don't allow utf-8
	f = fopen(fullname, "rb");
	if (!f)
		return NULL;

	if (rule)
		ctx->messagecallback(ctx->userctx, "\t\tProcessing %s (%s)\n", file->name, rule->name);
	else
		ctx->messagecallback(ctx->userctx, "\t\tCompressing %s\n", file->name);

	if (rule)
	{
		data = strrchr(file->write.name, '.');
		if (!data)
			data = file->write.name+strlen(file->write.name);
		if (strchr(rule->newext, '.'))
			strcpy(data, rule->newext);	//note: this allows weird _foo.tga postfixes.
		else
		{
			*data = '.';
			strcpy(data+1, rule->newext);
		}

		if (rule->command)
		{
			int i;
			char commandline[4096];
			char *cmd;
			char tempname[1024]; 
			//generate a sequenced temp filename
			//run the external tool to write that file
			//read the temp file.
			//delete temp file...
			fclose(f);

			QCC_JoinPaths(tempname, sizeof(tempname), file->write.name, ctx->sourcepath);
			f = fopen(tempname, "rb");
			if (f)
			{
				fclose(f);
				ctx->messagecallback(ctx->userctx, "Temp file %s already exists... not replacing+deleting\n", tempname);
				return NULL;
			}
			
			for (i = 0, cmd = rule->command; *cmd && i < countof(commandline)-1; )
			{
				if (!strncmp(cmd, "$input", 6))
				{
					strcpy(&commandline[i], fullname);
					i += strlen(&commandline[i]);
					cmd += 6;
				}
				else if (!strncmp(cmd, "$output", 7))
				{
					strcpy(&commandline[i], tempname);
					i += strlen(&commandline[i]);
					cmd += 7;
				}
				else
					commandline[i++] = *cmd++;
			}
			commandline[i] = 0;
//			ctx->messagecallback(ctx->userctx, "Commandline is %s\n", commandline);


#ifdef _WIN32		//windows is so fucking useless sometimes. sure, _popen 'works'... its just perverse enough that its not an option, forcing system-specific crap in anything that isn't originally from unix... maybe it is just incompetence? still feels like malice to me.
			StupidWindowsPopenAlternativeCrap(ctx, commandline);
#else
			{
				FILE *p;
				p = popen((*commandline=='@')?commandline+1:commandline, "rt");
				if (!p)
				{
					ctx->messagecallback(ctx->userctx, "Unable to execute command\n", tempname);
					return NULL;
				}
				while(fgets(commandline, sizeof(commandline), p))
					ctx->messagecallback(ctx->userctx, "%s", commandline);
				if (feof(p))
					ctx->messagecallback(ctx->userctx, "Process returned %d\n", pclose( p ));
				else
				{
					fprintf(stderr, "Error: Failed to read the pipe to the end.\n");
					pclose(p);
				}
			}
#endif

			f = fopen(tempname, "rb");
			if (!f)
			{
				ctx->messagecallback(ctx->userctx, "Temp file %s wasn't created\n", tempname);
				return NULL;
			}

			fseek(f, 0, SEEK_END);
			size = ftell(f);
			fseek(f, 0, SEEK_SET);
			data = malloc(size+1);
			fread(data, 1, size, f);
			fclose(f);
			*fsize = size;

#ifdef _WIN32
			_unlink(tempname);
#else
			unlink(tempname);
#endif
			return data;
		}
	}


	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = malloc(size+1);
	fread(data, 1, size, f);
	fclose(f);
	*fsize = size;

	return data;
}

static pbool PKG_WritePackageData(struct pkgctx_s *ctx, struct output_s *out, unsigned int index, pbool directoryonly)
{
	//helpers to deal with misaligned data. writes little-endian.
#define misbyte(ptr,ofs,data)  ((unsigned char*)(ptr))[ofs] = (data)&0xff
#define misshort(ptr,ofs,data) do{misbyte((ptr),(ofs),(data));misbyte((ptr),(ofs)+1,(data)>>8);}while(0)
#define misint(ptr,ofs,data)   do{misshort((ptr),(ofs),(data));misshort((ptr),(ofs)+2,(data)>>16);}while(0)
#define misint64(ptr,ofs,data) do{misint((ptr),(ofs),(data));misint((ptr),(ofs)+4,((quint64_t)(data))>>32);}while(0)
	qofs_t num=0;
	pbool pak = false;

	struct file_s *f;
	char centralheader[46+sizeof(f->write.name)];
	qofs_t centraldirsize;
	qofs_t centraldirofs;
	qofs_t z64eocdofs;

	char *filedata;

	FILE *outf;
	struct
	{
		char magic[4];
		unsigned int tabofs;
		unsigned int tabbytes;
	} pakheader = {"PACK", 0, 0};
	char *ext;

#define GPF_TRAILINGSIZE (1u<<3)
#define GPF_UTF8 (1u<<11)
#ifdef AVAIL_ZLIB
	#define compmethod (pak?0:8)/*Z_DEFLATED*/
#else
	#define compmethod 0/*Z_RAW*/
#endif
	if (!compmethod && !directoryonly && !index)
		pak = true; //might as well boost compat...
	ext = strrchr(out->filename, '.');
	if (ext && !QC_strcasecmp(ext, ".pak") && !index)
		pak = true;

	if (!directoryonly)
	{
		for (f = out->files; f ; f=f->write.nextwrite)
		{
			if (index != f->write.zdisk)
				continue;	//not in this disk...
			break;
		}
		if (!f)
		{
			ctx->messagecallback(ctx->userctx, "\t\tNo files to write to %s\n", out->filename);
			return false;
		}
	}

	if (out->usediffs && !directoryonly)
	{
		char newname[MAX_OSPATH];
		memcpy(newname, out->filename, sizeof(newname));
		if (ext)
		{
			ext = newname+(ext-out->filename);
			ext+=1;
			if (*ext)
				ext++;
			QC_snprintfz(ext, sizeof(newname)-(ext-newname), "%02u", index+1);
		}
		outf = fopen(newname, "wb");
	}
	else
		outf = fopen(out->filename, "wb");
	if (!outf)
	{
		ctx->messagecallback(ctx->userctx, "\t\tUnable to open %s\n", out->filename);
		return false;
	}

	if (pak)	//reserve space for the pak header
		fwrite(&pakheader, 1, sizeof(pakheader), outf);

	if (!directoryonly)
	{
		for (f = out->files; f ; f=f->write.nextwrite)
		{
			char header[32+sizeof(f->write.name)];
			size_t fnamelen;
			size_t hofs;
			unsigned short gpflags = GPF_UTF8;

			if (index != f->write.zdisk)
				continue;	//not in this disk...

			filedata = PKG_OpenSourceFile(ctx, f, &f->write.rawsize);
			if (!filedata)
			{
				ctx->messagecallback(ctx->userctx, "\t\tUnable to open %s\n", f->name);
			}
			fnamelen = strlen(f->write.name);

			f->write.zcrc = QC_encodecrc(f->write.rawsize, filedata);
			misint  (header, 0, 0x04034b50);
			misshort(header, 4, 45);//minver
			misshort(header, 6, gpflags);//general purpose flags
			misshort(header, 8, 0);//compression method, 0=store, 8=deflate
			misshort(header, 10, f->write.dostime);//lastmodfiletime
			misshort(header, 12, f->write.dosdate);//lastmodfiledate
			misint  (header, 14, f->write.zcrc);//crc32
			misint  (header, 18, f->write.rawsize);//compressed size
			misint  (header, 22, f->write.rawsize);//uncompressed size
			misshort(header, 26, fnamelen);//filename length
			misshort(header, 28, 0);//extradata length (filled in later)
			memcpy(header+30, f->write.name, fnamelen);
			hofs = 30+fnamelen;
			//Write extra data here...
			misshort(header, 28, hofs-(30+fnamelen));//extradata length
			f->write.zhdrofs = ftell(outf);
			fwrite(header, 1, hofs, outf);

#ifdef AVAIL_ZLIB
			if (f->write.rawsize && (compmethod == 2 || compmethod == 8))
			{
				gpflags |= 1u<<1;
				f->write.pakofs = 0;

				f->write.zmethod = compmethod;
				f->write.zipsize = PKG_DeflateToFile(outf, f->write.rawsize, filedata, compmethod);
			}
			else
#endif
			{
				f->write.zmethod = 0;
				f->write.pakofs = ftell(outf);
				f->write.zipsize = fwrite(filedata, 1, f->write.rawsize, outf);
			}

			//update the header
			misshort(header, 8, f->write.zmethod);//compression method, 0=store, 8=deflate
			if (f->write.zipsize > 0xffffffff)
			{
				misint  (header, 18, 0xffffffff);//compressed size
				gpflags |= GPF_TRAILINGSIZE;
			}
			else
				misint  (header, 18, f->write.zipsize);//compressed size
			if (f->write.rawsize > 0xffffffff)
			{
				misint  (header, 22, 0xffffffff);//compressed size
				gpflags |= GPF_TRAILINGSIZE;
			}
			else
				misint  (header, 22, f->write.rawsize);//compressed size
			misshort(header, 6, gpflags);//general purpose flags

			fseek(outf, f->write.zhdrofs, SEEK_SET);
			fwrite(header, 1, hofs, outf);
			fseek(outf, 0, SEEK_END);

			if (gpflags & GPF_TRAILINGSIZE) //if (gpflags & GPF_TRAILINGSIZE)
			{
				misint  (header, 0, 0x08074b50);
				misint  (header, 4, f->write.zcrc);
				misint64(header, 8, f->write.zipsize);
				misint64(header, 16, f->write.rawsize);
				fwrite(header, 1, 24, outf);
			}

			free(filedata);
			num++;
		}
	}

	if (pak)
	{
		struct 
		{
			char name[56];
			unsigned int offset;
			unsigned int size;
		} pakentry;
		pakheader.tabofs = ftell(outf);

		//write the pak file table.
		for (f = out->files,num=0; f ; f=f->write.nextwrite)
		{
			if (index != f->write.zdisk)
				continue;	//not in this disk...

			memset(&pakentry, 0, sizeof(pakentry));
			QC_strlcpy(pakentry.name, f->write.name, sizeof(pakentry.name));
			pakentry.size = (f->write.pakofs==0)?0:f->write.rawsize;
			pakentry.offset = f->write.pakofs;
			fwrite(&pakentry, 1, sizeof(pakentry), outf);
			num++;
		}

		//replace the pak header, then return to the end of the file for the zip end-of-central-directory
		pakheader.tabbytes = num * sizeof(pakentry);
		fseek(outf, 0, SEEK_SET);
		fwrite(&pakheader, 1, sizeof(pakheader), outf);
		fseek(outf, 0, SEEK_END);
	}

	centraldirofs = ftell(outf);
	for (f = out->files,num=0; f ; f=f->write.nextwrite)
	{
		size_t hofs;
		size_t fnamelen;
		if (!directoryonly && index != f->write.zdisk)
			continue;

		fnamelen = strlen(f->write.name);
		misint  (centralheader, 0, 0x02014b50);
		misshort(centralheader, 4, (3<<8)|63);//ourver
		misshort(centralheader, 6, 45);//minver
		misshort(centralheader, 8, GPF_UTF8);//general purpose flags
		misshort(centralheader, 10, f->write.rawsize?compmethod:0);//compression method, 0=store, 8=deflate
		misshort(centralheader, 12, f->write.dostime);//lastmodfiletime
		misshort(centralheader, 14, f->write.dosdate);//lastmodfiledate
		misint  (centralheader, 16, f->write.zcrc);//crc32
		misint  (centralheader, 20, f->write.zipsize);//compressed size
		misint  (centralheader, 24, f->write.rawsize);//uncompressed size
		misshort(centralheader, 28, fnamelen);//filename length
		misshort(centralheader, 30, 0);//extradata length (filled in later)
		misshort(centralheader, 32, 0);//comment length
		misshort(centralheader, 34, f->write.zdisk);//first disk number
		misshort(centralheader, 36, 0);//internal file attribs
		misint  (centralheader, 38, 0);//external file attribs
		misint  (centralheader, 42, f->write.zhdrofs);//local header offset
		strcpy(centralheader+46, f->write.name);

		hofs = 46+fnamelen;
		if (f->write.zdisk >= 0xffff || f->write.zhdrofs >= 0xffffffff || f->write.rawsize >= 0xffffffff || f->write.zipsize >= 0xffffffff)
		{
			misshort(centralheader, hofs, 0x0001);//zip64 tagid
			misshort(centralheader, hofs+2, 0x0001);//zip64 tag size
			hofs+=4;
			if (f->write.rawsize >= 0xffffffff)
			{
				misint64(centralheader, hofs, f->write.rawsize);//uncompressed size
				hofs += 8;
			}
			if (f->write.zipsize >= 0xffffffff)
			{
				misint64(centralheader, hofs, f->write.zipsize);//compressed size
				hofs += 8;
			}
			if (f->write.zhdrofs >= 0xffffffff)
			{
				misint64(centralheader, hofs, f->write.zhdrofs);//localheader offset
				hofs += 8;
			}
			if (f->write.zdisk >= 0xffff)
			{
				misint  (centralheader, hofs, f->write.zdisk);//compressed size
				hofs += 4;
			}
		}
		misshort(centralheader, 30, hofs-(46+fnamelen));//extradata length

		fwrite(centralheader, 1, hofs, outf);
		num++;
	}
	centraldirsize = ftell(outf)-centraldirofs;

	//zip64 end of central dir 
	z64eocdofs = ftell(outf);
	misint  (centralheader, 0, 0x06064b50);
	misint64(centralheader, 4, (qofs_t)(56-16));
	misshort(centralheader, 12, (3<<8)|63);	//ver made by = unix|appnote ver
	misshort(centralheader, 14, 45);	//ver needed
	misint  (centralheader, 16, index);	//thisdisk number
	misint  (centralheader, 20, index);	//centraldir start disk
    misint64(centralheader, 24, num);	//centraldir entry count (disk)
	misint64(centralheader, 32, num);	//centraldir entry count (total)
	misint64(centralheader, 40, centraldirsize);//centraldir entry bytes
	misint64(centralheader, 48, centraldirofs);	//centraldir start offset
	fwrite(centralheader, 1, 56, outf);

	//zip64 end of central dir locator 
	misint  (centralheader, 0, 0x07064b50);
	misint  (centralheader, 4, index);		//centraldir first disk
	misint64(centralheader, 8, z64eocdofs);
	misint  (centralheader, 16, index+1);		//total disk count
	fwrite(centralheader, 1, 20, outf);

//	centraldirofs = ftell(outf) - centraldirofs;
	//write zip end-of-central-directory
	misint  (centralheader, 0, 0x06054b50);
	misshort(centralheader, 4,  (index         >    0xffff)?    0xffff:index);	//this disk number
	misshort(centralheader, 6,  (index         >    0xffff)?    0xffff:index);	//centraldir first disk
	misshort(centralheader, 8,  (num           >    0xffff)?    0xffff:num);	//centraldir entries
	misshort(centralheader, 10, (num           >    0xffff)?    0xffff:num);	//total centraldir entries
	misint  (centralheader, 12, (centraldirsize>0xffffffff)?0xffffffff:centraldirsize);	//centraldir size
	misint  (centralheader, 16, (centraldirofs >0xffffffff)?0xffffffff:centraldirofs);	//centraldir offset
	misshort(centralheader, 20, 0);	//comment length
	fwrite(centralheader, 1, 22, outf);

	fclose(outf);

	return true;
}

/*
#include <sys/stat.h>
static time_t PKG_GetFileTime(const char *filename)
{
	struct stat s;
	if (stat(filename, &s) != -1)
		return s.st_mtime;
}
*/

static void PKG_ReadPackContents(struct pkgctx_s *ctx, struct oldpack_s *old)
{
#define longfromptr(p) (((p)[0]<<0)|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))
#define shortfromptr(p) (((p)[0]<<0)|((p)[1]<<8))
	size_t u, namelen;
	unsigned int foffset;
	unsigned char header[46];
	int i;
	FILE *f;

	//ignore packages if we're going to be overwritten.
	struct dataset_s *set;
	struct output_s *out;
	for (set = ctx->datasets; set; set = set->next)
	{
		for (out = set->outputs; out; out = out->next)
		{
			if (!strcmp(out->filename, old->filename))
				return;
		}
	}


	f = fopen(old->filename, "rb");
	if (f)
	{
		//find end-of-central-dir
		//assume no comment
		fseek(f, -22, SEEK_END);
		fread(header, 1, 22, f);

		if (header[0] == 'P' && header[1] == 'K' && header[2] == 5 && header[3] == 6)
		{
			old->part = shortfromptr(header+4);
			//centraldirstart = shortfromptr(header+6);
			old->numfiles = shortfromptr(header+8);
			//numfiles_all = shortfromptr(header+10);
			//centaldirsize = shortfromptr(header+12);
			foffset = longfromptr(header+16);
			//commentength = shortfromptr(header+20);

			old->file = malloc(sizeof(*old->file)*old->numfiles);


			fseek(f, foffset, SEEK_SET);
			for(u = 0; u < old->numfiles; u++)
			{
				unsigned int extra_len, comment_len;
				fread(header, 1, 46, f);
				//zcrc @ 16

				//version_madeby = shortfromptr(header+4);
				//version_needed = shortfromptr(header+6);
				//gflags = shortfromptr(header+8);
				old->file[u].zmethod = shortfromptr(header+10);
				old->file[u].dostime = shortfromptr(header+12);
				old->file[u].dosdate = shortfromptr(header+14);
				old->file[u].zcrc = longfromptr(header+16);
				old->file[u].zipsize = longfromptr(header+20);
				old->file[u].rawsize = longfromptr(header+24);
				namelen = shortfromptr(header+28);
				extra_len = shortfromptr(header+30);
				comment_len = shortfromptr(header+32);
				//disknum = shortfromptr(header+34);
				//iattributes = shortfromptr(header+36);
				//eattributes = longfromptr(header+38);
				//localheaderoffset = longfromptr(header+42);

				fread(old->file[u].name, 1, namelen, f);
				old->file[u].name[namelen] = 0;
				i = extra_len+comment_len;
				if (i)
					fseek(f, i, SEEK_CUR);
			}
		}
		else
		{
			fseek(f, 0, SEEK_SET);
			fread(header, 1, 12, f);
			if (header[0] == 'P' && header[1] == 'A' && header[2] == 'C' && header[3] == 'K')
			{
				unsigned int ofs = longfromptr(header+4);
				unsigned int dsz = longfromptr(header+8);
				struct 
				{
					char name[56];
					unsigned int size;
					unsigned int offset;
				} *files;
				files = malloc(dsz);
				fseek(f, ofs, SEEK_SET);
				fread(files, 1, dsz, f);
				old->numfiles = dsz / sizeof(*files);
				old->file = malloc(sizeof(*old->file)*old->numfiles);
				for (u = 0; u < old->numfiles; u++)
				{
					strcpy(old->file[u].name, files[u].name);
					old->file[u].rawsize = files[u].size;
				}
				free(files);
			}
			else
				ctx->messagecallback(ctx->userctx, "%s does not appear to be a package\n", old->filename);
		}

		//walk central directory
		fclose(f);
	}
}

static pbool PKG_FileIsModified(struct pkgctx_s *ctx, struct oldpack_s *old, struct file_s *file)
{
	size_t u;

	for (u = 0; u < old->numfiles; u++)
	{
		//should check filesize etc, but rules and extension changes make that messy
		if (!strcmp(old->file[u].name, file->name))
		{
			if(file->write.dosdate < old->file[u].dosdate || (file->write.dosdate == old->file[u].dosdate && file->write.dostime <= old->file[u].dostime))
			{
				file->write.zmethod = old->file[u].zmethod;
				//char name[128];
				file->write.zcrc = old->file[u].zcrc;
				file->write.zhdrofs = old->file[u].zhdrofs;
				file->write.pakofs = 0;
				file->write.rawsize = old->file[u].rawsize;
				file->write.zipsize = old->file[u].zipsize;
				file->write.dostime = old->file[u].dostime;
				file->write.dosdate = old->file[u].dosdate;
				return false;
			}
		}
	}
	return true;
}

static void PKG_WriteDataset(struct pkgctx_s *ctx, struct dataset_s *set)
{
	struct class_s *cls;
	struct output_s *out;
	struct file_s *file;
	struct rule_s *rule;
	struct oldpack_s *old;
	size_t u;

	if (!ctx->readoldpacks)
	{
		ctx->readoldpacks = true;

		for(old = ctx->oldpacks; old; old = old->next)
		{	//fixme: strip any wildcarded paks that match an output, to avoid weirdness.
			PKG_ReadPackContents(ctx, old);
		}

		for (out = set->outputs; out; out = out->next)
		{
			if(out->usediffs)
			{
				for (old = out->oldparts; old; old = old->next)
				{
					PKG_ReadPackContents(ctx, old);
					if (out->numparts <= old->part)
						out->numparts = old->part + 1;
				}
			}
		}
	}

	ctx->messagecallback(ctx->userctx, "Building dataset %s\n", set->name);

	for (cls = ctx->classes; cls; cls = cls->next)
	{
		for (out = set->outputs; out; out = out->next)
		{
			if (!strcmp(out->code, cls->outname))
				break;
		}
		if (!out)	//dataset doesn't name this.
			continue;

		rule = cls->defaultrule;
		for (u = 0; u < countof(cls->dataset); u++)
		{
			if (cls->dataset[u].set == set)
			{
				rule = cls->dataset[u].rule;
				break;
			}
		}

		if (rule && rule->dropfile)
			continue;

		for (file = cls->files; file; file = file->next)
		{
			for (old = ctx->oldpacks; old; old = old->next)
			{
				if (!PKG_FileIsModified(ctx, old, file))
					break;
			}
			if (old)
			{
				ctx->messagecallback(ctx->userctx, "\t\tFile %s found inside %s\n", file->name, old->filename);
				file->write.zdisk = ~0u;
			}
			else
			{
//				ctx->messagecallback(ctx->userctx, "\t\tFile %s, rule %s\n", file->name, rule?rule->name:"");

				file->write.zdisk = out->numparts;

				for (old = out->oldparts; old; old = old->next)
				{
					if (!PKG_FileIsModified(ctx, old, file))
					{
						file->write.zdisk = old->part;
						break;
					}
				}

				file->write.nextwrite = out->files;
				file->write.rule = rule;
				out->files = file;
			}
		}
	}

	for (out = set->outputs; out; out = out->next)
	{
		if (!out->files)
		{
			ctx->messagecallback(ctx->userctx, "\tOutput %s[%s] \"%s\" has no files\n", out->code, set->name, out->filename);
			continue;
		}

		if (ctx->test)
		{
			for (file = out->files; file; file = file->write.nextwrite)
			{
				if (file->write.rule)
					ctx->messagecallback(ctx->userctx, "\t\tFile %s has changed (rule %s)\n", file->name, file->write.rule->name);
				else
					ctx->messagecallback(ctx->userctx, "\t\tFile %s has changed\n", file->name);
			}
		}
		else
		{
			ctx->messagecallback(ctx->userctx, "\tGenerating %s[%s] \"%s\"\n", out->code, set->name, out->filename);
			if (PKG_WritePackageData(ctx, out, out->numparts, false))
			{
				if(out->usediffs)
					PKG_WritePackageData(ctx, out, out->numparts+1, true);
			}
		}
	}
}
void Packager_WriteDataset(struct pkgctx_s *ctx, char *setname)
{
	struct dataset_s *dataset;
	if (setname && strcmp(setname, "*"))
	{
		dataset = PKG_FindDataset(ctx, setname);
		if (dataset)
			PKG_WriteDataset(ctx, dataset);
		else
			ctx->messagecallback(ctx->userctx, "Dataset %s not known\n", setname);
	}
	else
	{
		for (dataset = ctx->datasets; dataset; dataset = dataset->next)
			PKG_WriteDataset(ctx, dataset);
	}
}
struct pkgctx_s *Packager_Create(void (*messagecallback)(void *userctx, const char *message, ...), void *userctx)
{
	struct pkgctx_s *ctx;
	ctx = malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	ctx->messagecallback = messagecallback;
	ctx->userctx = userctx;
	ctx->test = false;
	time(&ctx->buildtime);
	return ctx;
}
void Packager_ParseText(struct pkgctx_s *ctx, char *scripttext)
{
	char cmd[128];

	ctx->listfile = scripttext;
	while (PKG_GetToken(ctx, cmd, sizeof(cmd), true))
	{
//		if (!strcmp(cmd, "dataset"))
//			PKG_ParseDataset(ctx);
		if (!strcmp(cmd, "output"))
			PKG_ParseOutput(ctx, false);
		else if (!strcmp(cmd, "diffoutput") || !strcmp(cmd, "splitoutput"))
			PKG_ParseOutput(ctx, true);
		else if (!strcmp(cmd, "inputdir"))
		{
			char old[MAX_OSPATH];
			memcpy(old, ctx->sourcepath, sizeof(old));
			if (PKG_GetStringToken(ctx, cmd, sizeof(cmd)))
			{
				QC_strlcat(cmd, "/", sizeof(cmd));
				QCC_JoinPaths(ctx->sourcepath, sizeof(ctx->sourcepath), cmd, old);
			}
		}
		else if (!strcmp(cmd, "rule"))
			PKG_ParseRule(ctx);
		else if (!strcmp(cmd, "class"))
			PKG_ParseClass(ctx, NULL);
		else if (!strcmp(cmd, "ignore")||!strcmp(cmd, "oldpack"))
			PKG_ParseOldPack(ctx);
		else
		{
			char *e = strchr(cmd, ':');
			if (e && !e[1])
			{
				*e = 0;
				PKG_ParseClass(ctx, cmd);
			}
			else
			{
				struct class_s *c = PKG_FindClass(ctx, cmd);
				if (c)
					PKG_ParseClassFiles(ctx, c);
				else
					ctx->messagecallback(ctx->userctx, "Unrecognised token at global scope '%s'\n", cmd);
			}
		}
		//skip any junk
		while(PKG_GetToken(ctx, cmd, sizeof(cmd), false))
		{
			if (!strcmp(cmd, ";"))
				break;
		}
	}
}

void Packager_ParseFile(struct pkgctx_s *ctx, char *scriptname)
{
	size_t remaining = 0;
	char *file = qccprogfuncs->funcs.parms->ReadFile(scriptname, NULL, NULL, &remaining, true);
	strcpy(ctx->gamepath, scriptname);
	strcpy(ctx->sourcepath, scriptname);
	Packager_ParseText(ctx, file);
	free(file);
}

void Packager_Destroy(struct pkgctx_s *ctx)
{
	free(ctx);
}

pbool			Packager_CompressDir(const char *dirname, enum pkgtype_e type, void (*messagecallback)(void *userctx, const char *message, ...), void *userctx)
{
	char *ext;
	char filename[MAX_QPATH];
	struct pkgctx_s *ctx = Packager_Create(messagecallback, userctx);
	struct dataset_s *s;
	struct class_s *c;
	QC_strlcpy(ctx->sourcepath, dirname, sizeof(ctx->sourcepath));
	ext = strrchr(ctx->sourcepath, '/');
	if (*ctx->sourcepath && (!ext || ext[1]))
		QC_strlcat(ctx->sourcepath, "/", sizeof(ctx->sourcepath));

	QC_strlcpy(filename, dirname, sizeof(filename));
	for (;(ext = strrchr(filename, '/')) && !ext[1]; *ext = 0)
		;
	ext = strrchr(filename, '.');
	if (ext)
		*ext = 0;
	if (type == PACKAGER_PAK)
		QC_strlcat(filename, ".pak", sizeof(filename));
	else
		QC_strlcat(filename, ".pk3", sizeof(filename));

	s = PKG_GetDataset(ctx, "default");
	PKG_CreateOutput(ctx, s, "default", filename, type == PACKAGER_PK3_SPANNED);

	c = malloc(sizeof(*c));
	memset(c, 0, sizeof(*c));
	strcpy(c->name, "file");
	strcpy(c->outname, "default");
	c->next = ctx->classes;
	ctx->classes = c;
	PKG_AddClassFiles(ctx, c, "");

	Packager_WriteDataset(ctx, NULL);
	Packager_Destroy(ctx);

	return true;
}
#endif
