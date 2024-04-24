
#ifdef WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "darkplaces.h"

#ifdef WIN32
#include "utf8lib.h"
#endif

// LadyHavoc: some portable directory listing code I wrote for lmp2pcx, now used in darkplaces to load id1/*.pak and such...

int matchpattern(const char *in, const char *pattern, int caseinsensitive)
{
	return matchpattern_with_separator(in, pattern, caseinsensitive, "/\\:", false);
}

// wildcard_least_one: if true * matches 1 or more characters
//                     if false * matches 0 or more characters
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, qbool wildcard_least_one)
{
	int c1, c2;
	while (*pattern)
	{
		switch (*pattern)
		{
		case 0:
			return 1; // end of pattern
		case '?': // match any single character
			if (*in == 0 || strchr(separators, *in))
				return 0; // no match
			in++;
			pattern++;
			break;
		case '*': // match anything until following string
			if(wildcard_least_one)
			{
				if (*in == 0 || strchr(separators, *in))
					return 0; // no match
				in++;
			}
			pattern++;
			while (*in)
			{
				if (strchr(separators, *in))
					break;
				// see if pattern matches at this offset
				if (matchpattern_with_separator(in, pattern, caseinsensitive, separators, wildcard_least_one))
					return 1;
				// nope, advance to next offset
				in++;
			}
			break;
		default:
			if (*in != *pattern)
			{
				if (!caseinsensitive)
					return 0; // no match
				c1 = *in;
				if (c1 >= 'A' && c1 <= 'Z')
					c1 += 'a' - 'A';
				c2 = *pattern;
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
				if (c1 != c2)
					return 0; // no match
			}
			in++;
			pattern++;
			break;
		}
	}
	if (*in)
		return 0; // reached end of pattern but not end of input
	return 1; // success
}

// a little strings system
void stringlistinit(stringlist_t *list)
{
	memset(list, 0, sizeof(*list));
}

void stringlistfreecontents(stringlist_t *list)
{
	int i;
	for (i = 0;i < list->numstrings;i++)
	{
		if (list->strings[i])
			Z_Free(list->strings[i]);
		list->strings[i] = NULL;
	}
	list->numstrings = 0;
	list->maxstrings = 0;
	if (list->strings)
		Z_Free(list->strings);
	list->strings = NULL;
}

void stringlistappend(stringlist_t *list, const char *text)
{
	size_t textlen;
	char **oldstrings;

	if (list->numstrings >= list->maxstrings)
	{
		oldstrings = list->strings;
		list->maxstrings += 4096;
		list->strings = (char **) Z_Malloc(list->maxstrings * sizeof(*list->strings));
		if (list->numstrings)
			memcpy(list->strings, oldstrings, list->numstrings * sizeof(*list->strings));
		if (oldstrings)
			Z_Free(oldstrings);
	}
	textlen = strlen(text) + 1;
	list->strings[list->numstrings] = (char *) Z_Malloc(textlen);
	memcpy(list->strings[list->numstrings], text, textlen);
	list->numstrings++;
}

static int stringlistsort_cmp(const void *a, const void *b)
{
	return strcasecmp(*(const char **)a, *(const char **)b);
}

void stringlistsort(stringlist_t *list, qbool uniq)
{
	int i, j;
	if(list->numstrings < 1)
		return;
	qsort(&list->strings[0], list->numstrings, sizeof(list->strings[0]), stringlistsort_cmp);
	if(uniq)
	{
		// i: the item to read
		// j: the item last written
		for (i = 1, j = 0; i < list->numstrings; ++i)
		{
			char *save;
			if(!strcasecmp(list->strings[i], list->strings[j]))
				continue;
			++j;
			save = list->strings[j];
			list->strings[j] = list->strings[i];
			list->strings[i] = save;
		}
		for(i = j+1; i < list->numstrings; ++i)
		{
			if (list->strings[i])
				Z_Free(list->strings[i]);
		}
		list->numstrings = j+1;
	}
}

// operating system specific code
static void adddirentry(stringlist_t *list, const char *path, const char *name)
{
	if (strcmp(name, ".") && strcmp(name, ".."))
	{
		char temp[MAX_OSPATH];
		dpsnprintf( temp, sizeof( temp ), "%s%s", path, name );
		stringlistappend(list, temp);
	}
}
#ifdef WIN32
void listdirectory(stringlist_t *list, const char *basepath, const char *path)
{
	#define BUFSIZE 4096
	char pattern[BUFSIZE] = {0};
	wchar patternw[BUFSIZE] = {0};
	char filename[BUFSIZE] = {0};
	wchar *filenamew;
	int lenw = 0;
	WIN32_FIND_DATAW n_file;
	HANDLE hFile;
	dp_strlcpy(pattern, basepath, sizeof(pattern));
	dp_strlcat(pattern, path, sizeof (pattern));
	dp_strlcat(pattern, "*", sizeof (pattern));
	fromwtf8(pattern, strlen(pattern), patternw, BUFSIZE);
	// ask for the directory listing handle
	hFile = FindFirstFileW(patternw, &n_file);
	if(hFile == INVALID_HANDLE_VALUE)
		return;
	do {
		filenamew = n_file.cFileName;
		lenw = 0;
		while(filenamew[lenw] != 0) ++lenw;
		towtf8(filenamew, lenw, filename, BUFSIZE);
		adddirentry(list, path, filename);
	} while (FindNextFileW(hFile, &n_file) != 0);
	FindClose(hFile);
	#undef BUFSIZE
}
#else
void listdirectory(stringlist_t *list, const char *basepath, const char *path)
{
	char fullpath[MAX_OSPATH];
	DIR *dir;
	struct dirent *ent;
	dpsnprintf(fullpath, sizeof(fullpath), "%s%s", basepath, path);
#ifdef __ANDROID__
	// SDL currently does not support listing assets, so we have to emulate
	// it. We're using relative paths for assets, so that will do.
	if (basepath[0] != '/')
	{
		char listpath[MAX_OSPATH];
		qfile_t *listfile;
		dpsnprintf(listpath, sizeof(listpath), "%sls.txt", fullpath);
		char *buf = (char *) FS_SysLoadFile(listpath, tempmempool, true, NULL);
		if (!buf)
			return;
		char *p = buf;
		for (;;)
		{
			char *q = strchr(p, '\n');
			if (q == NULL)
				break;
			*q = 0;
			adddirentry(list, path, p);
			p = q + 1;
		}
		Mem_Free(buf);
		return;
	}
#endif
	dir = opendir(fullpath);
	if (!dir)
		return;

	while ((ent = readdir(dir)))
		adddirentry(list, path, ent->d_name);
	closedir(dir);
}
#endif

