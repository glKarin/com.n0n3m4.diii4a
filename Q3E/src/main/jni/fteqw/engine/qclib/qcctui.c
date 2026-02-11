#include "qcc.h"

#include <stdarg.h>
#include <stdio.h>

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

/*
==============
LoadFile
==============
*/
static void *QCC_ReadFile(const char *fname, unsigned char *(*buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile)
//unsigned char *PDECL QCC_ReadFile (const char *fname, void *buffer, int len, size_t *sz)
{
	size_t len;
	FILE *f;
	char *buffer;

	f = fopen(fname, "rb");
	if (!f)
	{
		if (out_size)
			*out_size = 0;
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (buf_get)
		buffer = buf_get(buf_ctx, len+1);
	else
		buffer = malloc(len+1);
	((char*)buffer)[len] = 0;
	if (len != fread(buffer, 1, len, f))
	{
		if (!buf_get)
			free(buffer);
		buffer = NULL;
	}
	fclose(f);

	if (out_size)
		*out_size = len;
	return buffer;
}
static int PDECL QCC_FileSize (const char *fname)
{
	long    length;
	FILE *f;
	f = fopen(fname, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fclose(f);

	return length;
}

static pbool PDECL QCC_WriteFile (const char *name, void *data, int len)
{
	long    length;
	FILE *f;
	f = fopen(name, "wb");
	if (!f)
		return false;
	length = fwrite(data, 1, len, f);
	fclose(f);

	if (length != len)
		return false;

	return true;
}

#undef printf
#undef Sys_Error

static void PDECL Sys_Error(const char *text, ...)
{
	va_list argptr;
	static char msg[2048];	

	va_start (argptr,text);
	QC_vsnprintf (msg,sizeof(msg)-1, text,argptr);
	va_end (argptr);

	QCC_Error(ERR_INTERNAL, "%s", msg);
}


static FILE *logfile;
static int logprintf(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	va_start (argptr, format);
#ifdef _WIN32
	_vsnprintf (string,sizeof(string)-1, format,argptr);
#else
	vsnprintf (string,sizeof(string), format,argptr);
#endif
	va_end (argptr);

	fprintf(stderr, "%s", string);
//	fputs(string, stderr);
	if (logfile)
		fputs(string, logfile);

	return 0;
}

static size_t totalsize, filecount;
static void QCC_FileList(const char *name, const void *compdata, size_t compsize, int method, size_t plainsize)
{
	totalsize += plainsize;
	filecount += 1;
	if (method < 0)
	{
		if (method == -1-9)
			externs->Printf("%s%8u DF64 %s%s\n", col_error, (unsigned)plainsize, name, col_none);
		else if (method == -1)	//general error
			externs->Printf("%s%8u ERR %s%s\n", col_error, (unsigned)plainsize, name, col_none);
		else
			externs->Printf("%s%8u m%-3i %s%s\n", col_error, (unsigned)plainsize, -1-method, name, col_none);
	}
	else if (!method && compsize==plainsize)
		externs->Printf("%8u      %s\n", (unsigned)plainsize, name);
	else
		externs->Printf("%8u %3u%% %s\n", (unsigned)plainsize, plainsize?(unsigned)((100*compsize)/plainsize):100u, name);
}
#include <limits.h>
#ifdef __unix__
#include <sys/stat.h>
void QCC_Mkdir(const char *path)
{
	char buf[MAX_OSPATH], *sl;
	if (!strchr(path, '/'))
		return;	//no need to create anything
	memcpy(buf, path, MAX_OSPATH);
	while((sl=strrchr(buf, '/')))
	{
		*sl = 0;
		mkdir(buf, 0777);
	}
}
#else
void QCC_Mkdir(const char *path)
{
	//unsupported.
}
#endif

static char qcc_tolower(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c-'A'+'a';
	return c;
}
int qcc_wildcmp(const char *wild, const char *string)
{
	while (*string)
	{
		if (*wild == '*')
		{
			if (*string == '/' || *string == '\\')
			{
				//* terminates if we get a match on the char following it, or if its a \ or / char
				wild++;
				continue;
			}
			if (qcc_wildcmp(wild+1, string))
				return true;
			string++;
		}
		else if ((qcc_tolower(*wild) == qcc_tolower(*string)) || (*wild == '?'))
		{
			//this char matches
			wild++;
			string++;
		}
		else
		{
			//failure
			return false;
		}
	}

	while (*wild == '*')
	{
		wild++;
	}
	return !*wild;
}

void AddSourceFile(const char *parentpath, const char *filename){}	//used in the gui to insert extra stuff into the file list. irrelevant here.
void compilecb(void){}	//... used in the gui to repaint things while busy, so a pointless stub here.
static void DoDecompileProgsDat(const char *name, void *blob, size_t blobsize)
{
//	extern pbool qcc_vfiles_changed;
	extern vfile_t *qcc_vfiles;
	vfile_t *f;
	DecompileProgsDat(name, blob, blobsize);

	for (f = qcc_vfiles; f; f = f->next)
	{
		int h;
		h = SafeOpenWrite(f->filename, -1);
		if (h >= 0)
		{
			SafeWrite(h, f->file, f->size);
			SafeClose(h);
		}
		else
			externs->Printf("%s: write failure\n", f->filename);
	}
}

static const char *extractonly;	//the file we're looking for
static pbool extractonlyfound;	//for errors.
static pbool extractecho;	//print the file to stdout instead of writing it.
static pbool extractdecomp;	//print the file to stdout instead of writing it.
static void QCC_FileExtract(const char *name, const void *compdata, size_t compsize, int method, size_t plainsize)
{
	if (method < 0)
		return;	//QC_decode will fail. provided for enumeration reasons.
	if (extractonly)
	{
		const char *sl = strrchr(extractonly, '/');
		if (sl && !sl[1])
		{	//trailing / - extract the entire dir.
			if (!strcmp(name, extractonly))
				return;	//ignore the dir itself...
			if (strncmp(name, extractonly, strlen(extractonly)))
				return;
		}
		else
			if (!qcc_wildcmp(extractonly, name))
				return;	//ignore it if its not the one we're going for.
	}
	extractonlyfound = true;
	externs->Printf("Extracting %s...", name);
	if (plainsize <= INT_MAX)
	{
		void *buffer = malloc(plainsize);
		if (buffer && QC_decode(progfuncs, compsize, plainsize, method, compdata, buffer))
		{
			if (extractdecomp)
				DoDecompileProgsDat(name, buffer, plainsize);
			else if (extractecho)
			{
				externs->Printf("\n");
				fwrite(buffer, 1, plainsize, stdout);
			}
			else
			{
				QCC_Mkdir(name);
				if (!QCC_WriteFile(name, buffer, plainsize))
					externs->Printf(" write failure\n");
				else
					externs->Printf(" done\n");
			}
		}
		else
			externs->Printf(" read failure\n");

		free(buffer);
	}
	else
		externs->Printf(" too large\n");
}

static void QCC_PR_PackagerMessage(void *userctx, const char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,message);
	QC_vsnprintf (string,sizeof(string)-1,message,argptr);
	va_end (argptr);

	externs->Printf ("%s", string);
}

int main (int argc, const char **argv)
{
	unsigned int i;
	pbool sucess;
#if 0//def _WIN32
	pbool writelog = true;	//spew log files on windows. windows often closes the window as soon as the program ends making its output otherwise unreadable.
#else
	pbool writelog = false;	//other systems are sane.
#endif
	pbool halp = false;
	int colours = 2;	//auto
	int ziparg = -1;
	progexterns_t ext;
	progfuncs_t funcs;
	progfuncs = &funcs;
	memset(&funcs, 0, sizeof(funcs));
	funcs.funcs.parms = &ext;
	memset(&ext, 0, sizeof(progexterns_t));
	funcs.funcs.parms->ReadFile = QCC_ReadFile;
	funcs.funcs.parms->FileSize = QCC_FileSize;
	funcs.funcs.parms->WriteFile = QCC_WriteFile;
	funcs.funcs.parms->Printf = logprintf;
	funcs.funcs.parms->Sys_Error = Sys_Error;

	for (i = 0; i < argc; i++)
	{
		if (!argv[i])
			continue;
		if (!strcmp(argv[i], "-log"))
			writelog = true;
		else if (!strcmp(argv[i], "-nolog"))
			writelog = false;
		else if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "--help"))
			halp = true;

		//arg consistency with ls
		else if (!strcmp(argv[i], "--color=always") || !strcmp(argv[i], "--color"))
			colours = 1;
		else if (!strcmp(argv[i], "--color=never"))
			colours = 0;
		else if (!strcmp(argv[i], "--color=auto"))
			colours = 2;
		else if (!strcmp(argv[i], "-d") || //o.O
				 !strcmp(argv[i], "-l") ||
				 !strcmp(argv[i], "-x") ||
				 !strcmp(argv[i], "-p") ||
				 !strcmp(argv[i], "-z") ||
				 !strcmp(argv[i], "-0") ||
				 !strcmp(argv[i], "-9"))
		{
			ziparg = i;
			break;	//other args are all filenames. don't misinterpret stuff.
		}
	}

	for (i = 0; i < COL_MAX; i++)
		qcccol[i] = "";
#if defined(__linux__) || defined(__unix__)
	if (colours == 2)
		colours = isatty(STDOUT_FILENO);
	if (colours)
	{	//only use colours if its a tty, and not if we're redirected.
		col_none = "\e[0;m";			//reset to white
		col_error = "\e[0;31m";			//red
		col_symbol = "\e[0;32m";		//green
		col_warning = "\e[0;33m";		//yellow
		//col_ = "\e[0;34m";			//blue
		col_name = "\e[0;35m";			//magenta
		col_type = "\e[0;36m";			//cyan
		col_location = "\e[0;1;37m";	//bright white
	}
#else
	(void)colours;
#endif

	if (halp)
	{
		logprintf("Archiving args:\n");
		logprintf(" -l PACKAGE : List files within a pak or pk3\n");
		logprintf(" -x PACKAGE [FILENAMES]: Extract files from pak or pk3\n");
		logprintf(" -p PACKAGE FILENAME: Pipe files from a pak or pk3 to the stdout\n");
		logprintf(" -z DIRECTORY : Create a spanned pk3 from a 'foo.pk3dir' subdir.\n");
		logprintf("     the pk3 itself contains just the file table, actual data will reside in external .p## files which will NOT be overwritten and can be referenced by future revisions to reduce redundancy on future updates\n");
		logprintf(" -0 DIRECTORY : Create a hybrid pak (uncompressed)\n");
		logprintf("     such pak files can also be read with any zip tool without needing special tools to extract (but should not be edited)\n");
		logprintf(" -9 DIRECTORY : Create a standard pk3\n");
		logprintf("     regular compressed zip with limited feature set for greater engine compat\n");
		logprintf("Decompiling args:\n");
		logprintf(" -d FILENAME : decompile a progs (into working directory)\n");
	}
	else if (ziparg >= 0)
	{
		if (ziparg+1 >= argc)
		{
			logprintf("archive name not specified\n");
			return EXIT_FAILURE;
		}
		switch(argv[ziparg][1])
		{
		case 'd':	//decompile...
			{
				size_t blobsize;
				void *blob = QCC_ReadFile(argv[ziparg+1], NULL, NULL, &blobsize, false);
				if (!blob)
					logprintf("Unable to read %s\n", argv[ziparg+1]);
				else if (!strncmp(blob, "PACK", 4) || !strncmp(blob, "PK", 2))
				{	//.pak or .pk3... probably.
					extractonly = (ziparg+2 < argc)?argv[ziparg+2]:"progs.dat";
					extractdecomp = true;
					extractonlyfound = false;
					QC_EnumerateFilesFromBlob(blob, blobsize, QCC_FileExtract);
					if (!extractonlyfound)
						externs->Printf("Unable to find file %s inside %s\n", extractonly, argv[ziparg+1]);
					else
						return EXIT_SUCCESS;
					extractonly = NULL;
				}
				else if (blob)
				{	//directly a .dat
					DoDecompileProgsDat(argv[ziparg+1], blob, blobsize);
					free(blob);
					return EXIT_SUCCESS;
				}
				return EXIT_FAILURE;
			}
			break;
		case 'l':	//list all files.
			{
				size_t blobsize;
				void *blob = QCC_ReadFile(argv[ziparg+1], NULL, NULL, &blobsize, false);
				if (blob)
				{
					QC_EnumerateFilesFromBlob(blob, blobsize, QCC_FileList);
					externs->Printf("Total size %lu bytes, %u files\n", (unsigned long)totalsize, (unsigned)filecount);
					free(blob);
					return EXIT_SUCCESS;
				}
				logprintf("Unable to read %s\n", argv[ziparg+1]);
			}
			break;
		case 'p':	//print (named) files to stdout.
			extractecho = true;
			//fall through
		case 'x':	//extract (named) files to working directory.
			{	//list/extract/view
				size_t blobsize;
				void *blob = QCC_ReadFile(argv[ziparg+1], NULL, NULL, &blobsize, false);
				int ret = EXIT_FAILURE;
				if (!blob)
					logprintf("Unable to read %s\n", argv[ziparg+1]);
				else if (ziparg+2 < argc)
				{
					for (i = ziparg+2; i < argc; i++)
					{
						extractonly = argv[i];
						extractonlyfound = false;
						QC_EnumerateFilesFromBlob(blob, blobsize, QCC_FileExtract);
						if (!extractonlyfound)
							externs->Printf("Unable to find file %s\n", extractonly);
						else
							ret = EXIT_SUCCESS;
					}
					extractonly = NULL;
				}
				else
				{
					QC_EnumerateFilesFromBlob(blob, blobsize, QCC_FileExtract);
					ret = EXIT_SUCCESS;
				}
				free(blob);
				return ret;
			}
		case 'z':	//fancy spanned stuff
		case '0':	//store-only (pak)
		case '9':	//best compression (pk3)

			{	//exe -0 foo.pk3dir
				enum pkgtype_e t;
				if (argv[ziparg][1] == '9')
					t = PACKAGER_PK3;
				else if (argv[ziparg][1] == '0')
					t = PACKAGER_PAK;	//not really any difference but oh well
				else
					t = PACKAGER_PK3_SPANNED;

				if (Packager_CompressDir(argv[ziparg+1], t, QCC_PR_PackagerMessage, NULL))
					return EXIT_SUCCESS;
			}
			break;
		default:
			//should be unreachable.
			break;
		}
		return EXIT_FAILURE;
	}


	logfile = writelog?fopen("fteqcc.log", "wt"):false;

	if (logfile)
	{
		fputs("Args:", logfile);
		for (i = 0; i < argc; i++)
		{
			if (!argv[i])
				continue;
			if (strchr(argv[i], ' '))
				fprintf(logfile, " \"%s\"", argv[i]);
			else
				fprintf(logfile, " %s", argv[i]);
		}
		fprintf(logfile, "\n");
	}
	sucess = CompileParams(&funcs, NULL, argc, argv);
	qccClearHunk();
	if (logfile)
		fclose(logfile);

#ifdef _WIN32
//	fgetc(stdin);	//wait for keypress
#endif
	return sucess?EXIT_SUCCESS:EXIT_FAILURE;
}
