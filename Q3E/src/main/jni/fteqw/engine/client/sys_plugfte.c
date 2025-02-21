#include "quakedef.h"
#include "winquake.h"
#include "sys_plugfte.h"
#include "../http/iweb.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif

static void UnpackAndExtractPakFiles_Complete(struct dl_download *dl);
static void pscript_property_splash_sets(struct context *ctx, const char *val);
void *globalmutex;

#ifdef _MSC_VER
# ifdef _WIN64
# pragma comment(lib, MSVCLIBSPATH "zlib64.lib")
# else
# pragma comment(lib, MSVCLIBSPATH "zlib.lib")
# endif
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ~0
#endif

#ifdef _WIN32
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	return NULL;
}
#else
#include <dlfcn.h>
void Sys_CloseLibrary(dllhandle_t *lib)
{
	if (lib)
		dlclose(lib);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	char soname[MAX_OSPATH];
	int i;
	dllhandle_t *lib;

	lib = NULL;
	if (!lib)
	{
		Q_snprintfz(soname, sizeof(soname), "%s.so", name);
		lib = (dllhandle_t*)dlopen (soname, RTLD_LAZY);
	}
	if (!lib)
		lib = (dllhandle_t*)dlopen (name, RTLD_LAZY);
	if (!lib)
	{
		Con_Printf("%s\n", dlerror());
		return NULL;
	}

	if (funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = dlsym(lib, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Con_Printf("Unable to find symbol \"%s\" in \"%s\"\n", funcs[i].name, name);
			Sys_CloseLibrary((dllhandle_t*)lib);
			lib = NULL;
		}
	}

	return (dllhandle_t*)lib;
}
#endif

void BZ_Free(void *ptr)
{
	free(ptr);
}
void *BZF_Malloc(int size)
{
	return malloc(size);
}
//FIXME: we can't use this.
void *BZ_Malloc(int size)
{
	return BZF_Malloc(size);
}

void QDECL Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}
char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/' || *pathname == '\\')
			last = pathname+1;
		pathname++;
	}
	return (char *)last;
}
void VARGS Q_vsnprintfz (char *dest, size_t size, const char *fmt, va_list argptr)
{
	vsnprintf (dest, size, fmt, argptr);
	dest[size-1] = 0;
}
void VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr, fmt);
	Q_vsnprintfz(dest, size, fmt, argptr);
	va_end (argptr);
}

char *COM_TrimString(char *str, char *buffer, int buffersize)
{
	int i;
	while (*str <= ' ' && *str>'\0')
		str++;

	for (i = 0; i < buffersize-1; i++)
	{
		if (*str <= ' ')
			break;
		buffer[i] = *str++;
	}
	buffer[i] = '\0';
	return buffer;
}
void VARGS Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char dest[256];

	va_start (argptr, fmt);
	Q_vsnprintfz(dest, sizeof(dest), fmt, argptr);
	va_end (argptr);

#ifdef _WIN32
	OutputDebugString(dest);
#else
	FILE *f = fopen("/tmp/ftelog", "a");
	if (f)
	{
		fputs(dest, f);
		fclose(f);
	}
	write(2, dest, strlen(dest));
#endif
}
void VARGS Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char dest[256];

	va_start (argptr, fmt);
	Q_vsnprintfz(dest, sizeof(dest), fmt, argptr);
	va_end (argptr);

#ifdef _WIN32
	OutputDebugString(dest);
#else
	FILE *f = fopen("/tmp/ftelog", "a");
	if (f)
	{
		fputs(dest, f);
		fclose(f);
	}
	write(2, dest, strlen(dest));
#endif
}
#ifdef _WIN32
// don't use these functions in MSVC8
#if (_MSC_VER < 1400)
int VARGS linuxlike_snprintf(char *buffer, int size, const char *format, ...)
{
#undef _vsnprintf
	int ret;
	va_list		argptr;

	if (size <= 0)
		return 0;
	size--;

	va_start (argptr, format);
	ret = _vsnprintf (buffer,size, format,argptr);
	va_end (argptr);

	buffer[size] = '\0';

	return ret;
}

int VARGS linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr)
{
#undef _vsnprintf
	int ret;

	if (size <= 0)
		return 0;
	size--;

	ret = _vsnprintf (buffer,size, format,argptr);

	buffer[size] = '\0';

	return ret;
}
#else
int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...)
{
	int ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = vsnprintf_s (buffer,size, _TRUNCATE, format,argptr);
	va_end (argptr);

	return ret;
}
#endif

#endif

#include "netinc.h"
#ifndef _WIN32
#define pgetaddrinfo getaddrinfo
#define pfreeaddrinfo freeaddrinfo
#else
#if 0//def _MSC_VER
#include <wspiapi.h>
#define pgetaddrinfo getaddrinfo
#define pfreeaddrinfo freeaddrinfo
#else
#include <ws2tcpip.h>
#endif
#endif
size_t	NET_StringToSockaddr2 (const char *s, int defaultport, struct sockaddr_qstorage *sadr, int *addrfamily, int *addrsize, size_t addrcount)
{
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *pos;
	struct addrinfo udp6hint;
	int error;
	char *port;
	char dupbase[256];
	int len;
#ifndef pgetaddrinfo
	int (WINAPI *pgetaddrinfo)(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
	void (WINAPI *pfreeaddrinfo)(struct addrinfo *ai);
	void *lib = LoadLibrary("ws2_32.dll");
	if (!lib)
		return false;
	if (addrcount != 1)
		return false;
	pgetaddrinfo = (void*)GetProcAddress(lib, "getaddrinfo");
	pfreeaddrinfo = (void*)GetProcAddress(lib, "freeaddrinfo");
	if (!pgetaddrinfo || !pfreeaddrinfo)
		return 0;
#endif

	if (!(*s))
		return 0;

	memset (sadr, 0, sizeof(*sadr));


	memset(&udp6hint, 0, sizeof(udp6hint));
	udp6hint.ai_family = 0;//Any... we check for AF_INET6 or 4
	udp6hint.ai_socktype = SOCK_DGRAM;
	udp6hint.ai_protocol = IPPROTO_UDP;

	//handle parsing of ipv6 literal addresses
	if (*s == '[')
	{
		port = strstr(s, "]");
		if (!port)
			error = EAI_NONAME;
		else
		{
			len = port - (s+1);
			if (len >= sizeof(dupbase))
				len = sizeof(dupbase)-1;
			strncpy(dupbase, s+1, len);
			dupbase[len] = '\0';
			error = pgetaddrinfo(dupbase, (port[1] == ':')?port+2:NULL, &udp6hint, &addrinfo);
		}
	}
	else
	{
		port = strrchr(s, ':');

		if (port)
		{
			len = port - s;
			if (len >= sizeof(dupbase))
				len = sizeof(dupbase)-1;
			strncpy(dupbase, s, len);
			dupbase[len] = '\0';
			error = pgetaddrinfo(dupbase, port+1, &udp6hint, &addrinfo);
		}
		else
			error = EAI_NONAME;
		if (error)	//failed, try string with no port.
			error = pgetaddrinfo(s, NULL, &udp6hint, &addrinfo);	//remember, this func will return any address family that could be using the udp protocol... (ip4 or ip6)
	}
	if (error)
	{
		return 0;
	}
	((struct sockaddr*)sadr)->sa_family = 0;
	for (pos = addrinfo; pos; pos = pos->ai_next)
	{
		switch(pos->ai_family)
		{
		default:
			//unrecognised address families are ignored.
			break;
		case AF_INET6:
			if (((struct sockaddr_in *)sadr)->sin_family == AF_INET6)
				break;	//first one should be best...
			//fallthrough
		case AF_INET:
			memcpy(sadr, pos->ai_addr, pos->ai_addrlen);
			if (pos->ai_family == AF_INET)
				goto dblbreak;	//don't try finding any more, this is quake, they probably prefer ip4...
			break;
		}
	}
dblbreak:
	pfreeaddrinfo (addrinfo);
	if (!((struct sockaddr*)sadr)->sa_family)	//none suitablefound
		return 0;

	if (addrfamily)
		*addrfamily = ((struct sockaddr*)sadr)->sa_family;

	if (((struct sockaddr*)sadr)->sa_family == AF_INET)
	{
		if (!((struct sockaddr_in *)sadr)->sin_port)
			((struct sockaddr_in *)sadr)->sin_port = htons(defaultport);
		if (addrsize)
			*addrsize = sizeof(struct sockaddr_in);
	}
	else
	{
		if (!((struct sockaddr_in6 *)sadr)->sin6_port)
			((struct sockaddr_in6 *)sadr)->sin6_port = htons(defaultport);
		if (addrsize)
			*addrsize = sizeof(struct sockaddr_in6);
	}

	return 1;
}

char *COM_ParseType (const char *data, char *out, int outlen, com_tokentype_t *toktype)
{
	int		c;
	int		len;

//	if (out == com_token)
//		COM_AssertMainThread("COM_ParseOut: com_token");

	len = 0;
	out[0] = 0;
	if (toktype)
		*toktype = TTP_EOF;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*')
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		if (toktype)
			*toktype = TTP_STRING;

		data++;
		while (1)
		{
			if (len >= outlen-1)
			{
				out[len] = 0;
				return (char*)data;
			}

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

// parse a regular word
	if (toktype)
		*toktype = TTP_RAWTOKEN;
	do
	{
		if (len >= outlen-1)
		{
			out[len] = 0;
			return (char*)data;
		}

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	out[len] = 0;
	return (char*)data;
}

#if 0
typedef struct 
{
	vfsfile_t funcs;

	char *data;
	int maxlen;
	int writepos;
	int readpos;
} vfspipe_t;

void VFSPIPE_Close(vfsfile_t *f)
{
	vfspipe_t *p = (vfspipe_t*)f;
	free(p->data);
	free(p);
}
unsigned long VFSPIPE_GetLen(vfsfile_t *f)
{
	vfspipe_t *p = (vfspipe_t*)f;
	return p->writepos - p->readpos;
}
unsigned long VFSPIPE_Tell(vfsfile_t *f)
{
	return 0;
}
qboolean VFSPIPE_Seek(vfsfile_t *f, unsigned long offset)
{
	Con_Printf("Seeking is a bad plan, mmkay?\n");
	return false;
}
int VFSPIPE_ReadBytes(vfsfile_t *f, void *buffer, int len)
{
	vfspipe_t *p = (vfspipe_t*)f;
	if (len > p->writepos - p->readpos)
		len = p->writepos - p->readpos;
	memcpy(buffer, p->data+p->readpos, len);
	p->readpos += len;

	if (p->readpos > 8192)
	{
		//shift the memory down periodically
		//fixme: use cyclic buffer? max size, etc?
		memmove(p->data, p->data+p->readpos, p->writepos-p->readpos);

		p->writepos -= p->readpos;
		p->readpos = 0;
	}
	return len;
}
int VFSPIPE_WriteBytes(vfsfile_t *f, const void *buffer, int len)
{
	vfspipe_t *p = (vfspipe_t*)f;
	if (p->writepos + len > p->maxlen)
	{
		p->maxlen = p->writepos + len;
		p->data = realloc(p->data, p->maxlen);
	}
	memcpy(p->data+p->writepos, buffer, len);
	p->writepos += len;
	return len;
}

vfsfile_t *VFSPIPE_Open(void)
{
	vfspipe_t *newf;
	newf = malloc(sizeof(*newf));
	newf->data = NULL;
	newf->maxlen = 0;
	newf->readpos = 0;
	newf->writepos = 0;
	newf->funcs.Close = VFSPIPE_Close;
	newf->funcs.Flush = NULL;
	newf->funcs.GetLen = VFSPIPE_GetLen;
	newf->funcs.ReadBytes = VFSPIPE_ReadBytes;
	newf->funcs.Seek = VFSPIPE_Seek;
	newf->funcs.Tell = VFSPIPE_Tell;
	newf->funcs.WriteBytes = VFSPIPE_WriteBytes;
	newf->funcs.seekingisabadplan = true;

	return &newf->funcs;
}
#endif







#if defined(OFFICIAL_RELEASE)
	#define BUILDTYPE "rel"
#else
	#define BUILDTYPE "test"
	#define UPDATE_URL "http://triptohell.info/moodles/"
	#define UPDATE_URL_VERSION UPDATE_URL "version.txt"

	//we should perhaps use the uname stuff instead.
	#ifdef _WIN32
		#ifdef _WIN64
			#define UPDATE_URL_BUILD UPDATE_URL "win64/fte" EXETYPE ".exe"
		#else
			#define UPDATE_URL_BUILD UPDATE_URL "win32/fte" EXETYPE ".exe"
		#endif
	#else
		#if defined(__amd64__)
			#define UPDATE_URL_BUILD UPDATE_URL "linux_amd64/fteqw." EXETYPE "64"
		#else
			#define UPDATE_URL_BUILD UPDATE_URL "linux_x86/fteqw." EXETYPE "32"
		#endif
	#endif
#endif

#ifdef _WIN32
//#if defined(GLQUAKE) && defined(D3DQUAKE)
//	#define EXETYPE "qw"
//#elif defined(GLQUAKE)
//	#ifdef MINIMAL
//		#define EXETYPE "minglqw"
//	#else
		#define EXETYPE "glqw"
//	#endif
//#elif defined(D3DQUAKE)
//	#define EXETYPE "d3dqw"
//#elif defined(SWQUAKE)
//	#define EXETYPE "swqw"
//#else 	//erm...
//	#define EXETYPE "qw"
//#endif
#else
	#define EXETYPE "gl"
#endif

#ifdef _WIN32
qboolean MyRegGetStringValue(void *base, const char *keyname, const char *valuename, void *data, size_t datalen)
{
	qboolean result = false;
	HKEY subkey;
	DWORD type = REG_NONE;
	if (RegOpenKeyEx(base, keyname, 0, KEY_READ, &subkey) == ERROR_SUCCESS)
	{
		DWORD dl = datalen;
		result = ERROR_SUCCESS == RegQueryValueEx(subkey, valuename, NULL, &type, data, &dl);
		datalen = dl;
		RegCloseKey (subkey);
	}

	if (type == REG_SZ || type == REG_EXPAND_SZ)
		((char*)data)[datalen] = 0;
	else
		((char*)data)[0] = 0;
	return result;
}

qboolean MyRegSetValue(void *base, const char *keyname, const char *valuename, int type, const void *data, int datalen)
{
	HKEY subkey;
	if (RegCreateKeyEx(base, keyname, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subkey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(subkey, valuename, 0, type, data, datalen);
		RegCloseKey (subkey);
	}
	return true;
}
void MyRegDeleteKeyValue(void *base, const char *keyname, const char *valuename)
{
	HKEY subkey;
	if (RegOpenKeyEx(base, keyname, 0, KEY_WRITE, &subkey) == ERROR_SUCCESS)
	{
		RegDeleteValue(subkey, valuename);
		RegCloseKey (subkey);
	}
}

qboolean Update_GetHomeDirectory(char *homedir, int homedirsize)
{
	HMODULE shfolder = LoadLibrary("shfolder.dll");

	if (shfolder)
	{
		HRESULT (WINAPI *dSHGetFolderPath) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);
		dSHGetFolderPath = (void *)GetProcAddress(shfolder, "SHGetFolderPathA");
		if (dSHGetFolderPath)
		{
			char folder[MAX_PATH];
			// 0x5 == CSIDL_PERSONAL
			if (dSHGetFolderPath(NULL, 0x5, NULL, 0, folder) == S_OK)
			{
				Q_snprintfz(homedir, homedirsize, "%s/My Games/%s/", folder, FULLENGINENAME);
				return true;
			}
		}
//		FreeLibrary(shfolder);
	}
	return false;
}

void Sys_mkdir (const char *path)
{
	CreateDirectory (path, NULL);
}
#else
void Sys_mkdir (const char *path)
{
	mkdir (path, 0777);
}
qboolean Update_GetHomeDirectory(char *homedir, int homedirsize)
{
	qboolean result = false;
	char *val = getenv("HOME");
	if (!val) val = ".";
	else	result = true;

	Q_strncpyz(homedir, val, homedirsize);
	Q_strncatz(homedir, "/.fte/", homedirsize);

	return result;
}
#endif
static void	Update_CreatePath (char *path)
{
	char	*ofs;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}

#include "fs.h"
struct dl_download *enginedownloadactive;
void Update_Version_Updated(struct dl_download *dl)
{
	//happens in a thread, avoid va
	if (dl->file)
	{
		if (dl->status == DL_FINISHED)
		{
			char buf[8192];
			unsigned int size = 0, chunk, fullsize;
			char pendingname[MAX_OSPATH];
			FILE *pending;
			Update_GetHomeDirectory(pendingname, sizeof(pendingname));
			Q_strncatz(pendingname, "pending" BUILDTYPE EXETYPE ".tmp", sizeof(pendingname));
			fullsize = VFS_GETLEN(dl->file);
			Update_CreatePath(pendingname);
#ifndef _WIN32
			unlink(pendingname);
#endif
			pending = fopen(pendingname, "wb");
			if (pending)
			{
				while(1)
				{
					chunk = VFS_READ(dl->file, buf, sizeof(buf));
					if (!chunk)
						break;
					size += fwrite(buf, 1, chunk, pending);
				}
				fclose(pending);
				if (fullsize == size)
				{
#ifdef _WIN32
					MyRegSetValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, "pending" BUILDTYPE EXETYPE, REG_SZ, pendingname, strlen(pendingname)+1);
#else
					chmod(pendingname, S_IRUSR|S_IXUSR);
#endif
				}
				else
				{
					dl->status = DL_FAILED;
				}
			}
			else
				dl->status = DL_FAILED;
		}
		VFS_CLOSE(dl->file);
		dl->file = NULL;
	}
}
qboolean Plug_GetDownloadedName(char *updatedpath, int updatedpathlen)
{
#ifdef _WIN32
	char pendingpath[MAX_OSPATH];
	char temppath[MAX_OSPATH];

	Sys_LockMutex(globalmutex);

	if (!enginedownloadactive || (enginedownloadactive->status == DL_FINISHED || enginedownloadactive->status == DL_FAILED))
	{
		if (enginedownloadactive)
			DL_Close(enginedownloadactive);
		enginedownloadactive = NULL;

		/*if there's some downloaded binary which isn't running yet, kill the old active one and update it*/
		MyRegGetStringValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, "pending" BUILDTYPE EXETYPE, pendingpath, sizeof(pendingpath));
		if (*pendingpath)
		{
			MyRegDeleteKeyValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, "pending" BUILDTYPE EXETYPE);
			Update_GetHomeDirectory(temppath, sizeof(temppath));
			Update_CreatePath(temppath);
			Q_strncatz(temppath, "cur" BUILDTYPE EXETYPE".exe", sizeof(temppath));
			DeleteFile(temppath);
			if (MoveFile(pendingpath, temppath))
				MyRegSetValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, BUILDTYPE EXETYPE, REG_SZ, temppath, strlen(temppath)+1);
		}

		/*grab the binary to run from the registry*/
		MyRegGetStringValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, BUILDTYPE EXETYPE, updatedpath, updatedpathlen);
		if (*updatedpath && INVALID_FILE_ATTRIBUTES==GetFileAttributes(updatedpath))	//make sure its actually still there.
			*updatedpath = 0;
		if (!*updatedpath)
		{
			/*ooer, its not set, try and download one. updates are handled by the client itself.*/
			enginedownloadactive = DL_Create(UPDATE_URL_BUILD);
			if (enginedownloadactive)
			{
				enginedownloadactive->user_ctx = NULL;
				DL_CreateThread(enginedownloadactive, VFSPIPE_Open(1, false), Update_Version_Updated);
			}
		}
	}

	Sys_UnlockMutex(globalmutex);
#else

	char pendingpath[MAX_OSPATH];
	char currentpath[MAX_OSPATH];
	struct stat st;

	Update_GetHomeDirectory(currentpath, sizeof(currentpath));
	Q_strncatz(currentpath, "cur" BUILDTYPE EXETYPE, sizeof(currentpath));
	Update_GetHomeDirectory(pendingpath, sizeof(pendingpath));
	Q_strncatz(pendingpath, "pending" BUILDTYPE EXETYPE ".tmp", sizeof(pendingpath));

	//truncated to be the same string? :s
	if (!strcmp(currentpath, pendingpath))
		return false;

	Sys_LockMutex(globalmutex);

	if (!enginedownloadactive || (enginedownloadactive->status == DL_FINISHED || enginedownloadactive->status == DL_FAILED))
	{
		//make sure the download is cleaned up properly.
		if (enginedownloadactive)
			DL_Close(enginedownloadactive);
		enginedownloadactive = NULL;

		//if the engine wrote a new version to use, copy it over to the proper path now (two paths means we're less likely to be trying to write a file that is already open).
		if (!stat(pendingpath, &st))
		{
			//rename supposedly overwrites automagically. I'm paranoid.
			unlink(currentpath);
			rename(pendingpath, currentpath);
		}

		if (!stat(currentpath, &st))
		{
			//there's a current file, yay.
			Q_strncpyz(updatedpath, currentpath, updatedpathlen);
		}
		else
		{	//no engine available to run. we'll have to download one (we only need to do it in this case because the engine itself autoupdates,
			//and we can't because we don't actually know what version we have, only the binary that's running knows its version.
			//that's the theory anyway.
			Con_Printf("downloading %s\n", UPDATE_URL_BUILD);
			enginedownloadactive = DL_Create(UPDATE_URL_BUILD);
                        if (enginedownloadactive)
                        {
                                enginedownloadactive->user_ctx = NULL;
                                DL_CreateThread(enginedownloadactive, VFSPIPE_Open(), Update_Version_Updated);
                        }

		}
	}
	Sys_UnlockMutex(globalmutex);
	
#endif
	return !!*updatedpath;
}










struct context
{
	struct contextpublic pub;

	void *windowhnd;
	int windowleft;
	int windowtop;
	int windowwidth;
	int windowheight;

	int waitingfordatafiles;

	char *datadownload;
	char *gamename;
	char *password;
	char *onstart;
	char *onend;
	char *ondemoend;
	char *curserver;	//updated by engine

	void *hostinstance;

	int read;
	int written;

	qtvfile_t qtvf;

	unsigned char *splashdata;
	int splashwidth;
	int splashheight;
	struct dl_download *splashdownload;
	struct dl_download *packagelist;

	void *mutex;
	void *thread;
	char resetvideo;
	qboolean shutdown;
	qboolean multiplecontexts;

	struct context *next;

	struct browserfuncs bfuncs;

#ifdef _WIN32
	HANDLE pipetoengine;
	HANDLE pipefromengine;
	HANDLE engineprocess;
#else
	int pipetoengine;
	int pipefromengine;
	pid_t engineprocess;
#endif
};

#ifdef _WIN32

extern HWND sys_parentwindow;
extern unsigned int sys_parentwidth;
extern unsigned int sys_parentheight;
HINSTANCE	global_hInstance;
static char binarypath[MAX_PATH];

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	char *bp;
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		global_hInstance = hinstDLL;
		GetModuleFileName(global_hInstance, binarypath, sizeof(binarypath));
		bp = COM_SkipPath(binarypath);
		if (bp)
			*bp = 0;
		break;
	default:
		break;
	}
	return TRUE;
}
#endif

struct context *activecontext;
struct context *contextlist;

#define ADDRARG(x) do {if (argc < maxargs) argv[argc++] = strdup(x);} while(0)				//for strings that are safe
#define ADDCARG(x) do {if (argc < maxargs) argv[argc++] = cleanarg(x);} while(0)	//for arguments that we don't trust.
char *cleanarg(char *arg)
{
	unsigned char *c;
	//skip over any leading spaces.
	while (*arg && *arg <= ' ')
		arg++;

	//reject anything with a leading + or -
	if (*arg == '-' || *arg == '+')
		return strdup("badarg");

	//clean up the argument
	for (c = (unsigned char *)arg; *c; c++)
	{
		//remove special chars... we automagically add quotes so any that exist are someone trying to get past us.
		if (*c == ';' || *c == '\n' || *c == '\"')
			*c = '?';
		//remove other control chars.
		//we allow spaces
		if (*c < ' ')
			*c = '?';
	}

	return strdup(arg);
}

qboolean Plug_GetBinaryName(char *exe, int exelen, char *basedir, int basedirlen)
{
//	char buffer[1024];
//	char cmd[64];
//	char value[1024];
//	FILE *f;

	*exe = 0;
	*basedir = 0;

#ifdef _DEBUG
#ifdef _WIN32
	Q_strncpyz(exe, "C:/Games/Quake/fte_trunk/fteglqw_dbg.exe", exelen);
#else
	Q_strncpyz(exe, "/home/david/quake/fte_trunk/engine/debug/fteqw.gl", exelen);
#endif
#endif
/*
	Q_snprintfz(buffer, sizeof(buffer), "%s%s", binarypath, "npfte.txt");

	f = fopen(buffer, "rt");
	if (f)
	{
		while(fgets(buffer, sizeof(buffer), f))
		{
			*cmd = 0;
			*value = 0;
			COM_ParseOut(COM_ParseOut(buffer, cmd, sizeof(cmd)), value, sizeof(value));
			if (!strcmp(cmd, "relexe"))
				Q_snprintfz(exe, exelen, "%s%s", binarypath, value);
			else if (!strcmp(cmd, "absexe"))
				Q_strncpyz(exe, value, exelen);
			else if (!strcmp(cmd, "basedir"))
				Q_strncpyz(basedir, value, basedirlen);
		}
		fclose(f);
	}
*/
	if (!*exe)
		return Plug_GetDownloadedName(exe, exelen);
	return false;
}

int Plug_GenCommandline(struct context *ctx, char **argv, int maxargs)
{
	char *s;
	int argc;
	char tok[256];
	char exe[1024];
	char basedir[1024];
	qboolean autoupdate;

	*exe = 0;
	*basedir = 0;

	autoupdate = Plug_GetBinaryName(exe, sizeof(exe), basedir, sizeof(basedir));

	if (!*exe)
	{
		Con_Printf("Unable to determine what to run\n");
		return 0;	//error
	}

	argc = 0;

	ADDRARG(exe);

	if (autoupdate)
	{
		ADDRARG("-autoupdate");
	}

	ADDRARG("-plugin");

	if (*basedir)
	{
		ADDRARG("-basedir");
		ADDCARG(basedir);
	}

	switch(ctx->qtvf.connectiontype)
	{
	default:
		break;
	case QTVCT_STREAM:
		ADDRARG("+qtvplay");
		ADDCARG(ctx->qtvf.server);
		break;
	case QTVCT_CONNECT:
		ADDRARG("+connect");
		ADDCARG(ctx->qtvf.server);
		break;
	case QTVCT_JOIN:
		ADDRARG("+join");
		ADDCARG(ctx->qtvf.server);
		break;
	case QTVCT_OBSERVE:
		ADDRARG("+observe");
		ADDCARG(ctx->qtvf.server);
		break;
	case QTVCT_MAP:
		ADDRARG("+map");
		ADDCARG(ctx->qtvf.server);
		break;
	}

	if (ctx->password)
	{
		ADDRARG("+password");
		ADDCARG(ctx->password);
	}

	//figure out the game dirs (first token is the base game)
	s = ctx->gamename;
	s = COM_ParseOut(s, tok, sizeof(tok));
	if (!*tok || !strcmp(tok, "q1") || !strcmp(tok, "qw") || !strcmp(tok, "quake"))
		ADDRARG("-quake");
	else if (!strcmp(tok, "q2") || !strcmp(tok, "quake2"))
		ADDRARG("-q2");
	else if (!strcmp(tok, "q3") || !strcmp(tok, "quake3"))
		ADDRARG("-q3");
	else if (!strcmp(tok, "hl") || !strcmp(tok, "halflife"))
		ADDRARG("-halflife");
	else if (!strcmp(tok, "h2") || !strcmp(tok, "hexen2"))
		ADDRARG("-hexen2");
	else if (!strcmp(tok, "nex") || !strcmp(tok, "nexuiz"))
		ADDRARG("-nexuiz");
	else
	{
		ADDRARG("-basegame");
		ADDCARG(tok);
	}
	//later options are additions to that
	while ((s = COM_ParseOut(s, tok, sizeof(tok))))
	{
		if (argc == sizeof(argv)/sizeof(argv[0]))
			break;
		ADDRARG("-addbasegame");
		ADDCARG(tok);
	}
	//make sure we always use the home directory. because its better than using some random browser-defined working directory.
	ADDRARG("-usehome");

	if (ctx->datadownload)
	{
		ADDRARG("-manifest");
		ADDCARG(ctx->datadownload);
	}

	return argc;
}
qboolean Plug_GenCommandlineString(struct context *ctx, char *cmdline, int cmdlinelen)
{
	char *argv[64];
	int argc, i;
	argc = Plug_GenCommandline(ctx, argv, 64);
	if (!argc)
		return false;
	for (i = 0; i < argc; i++)
	{
		//add quotes for any arguments with spaces
		if (!*argv[i] || strchr(argv[i], ' ') || strchr(argv[i], '\t'))
		{
			Q_strncatz(cmdline, "\"", cmdlinelen);
			Q_strncatz(cmdline, argv[i], cmdlinelen);
			Q_strncatz(cmdline, "\"", cmdlinelen);
		}
		else
			Q_strncatz(cmdline, argv[i], cmdlinelen);
		Q_strncatz(cmdline, " ", cmdlinelen);
	}
	return true;
}

void Plug_ExecuteCommand(struct context *ctx, char *message, ...)
{
	va_list		va;

	char		finalmessage[1024];

	va_start (va, message);
	vsnprintf (finalmessage, sizeof(finalmessage)-1, message, va);
	va_end (va);

	if (ctx->pipetoengine)
	{
#ifdef _WIN32
		DWORD written = 0;
		WriteFile(ctx->pipetoengine, finalmessage, strlen(finalmessage), &written, NULL);
#else
		write(ctx->pipetoengine, finalmessage, strlen(finalmessage));
#endif
	}
}

qboolean Plug_CreatePluginProcess(struct context *ctx)
{
#ifdef _WIN32
	char cmdline[8192];
	PROCESS_INFORMATION childinfo;
	STARTUPINFO startinfo;
	SECURITY_ATTRIBUTES pipesec = {sizeof(pipesec), NULL, TRUE};
	if (!Plug_GenCommandlineString(ctx, cmdline, sizeof(cmdline)))
		return false;

	memset(&startinfo, 0, sizeof(startinfo));
	startinfo.cb = sizeof(startinfo);
	startinfo.hStdInput = NULL;
	startinfo.hStdError = NULL;
	startinfo.hStdOutput = NULL;
	startinfo.dwFlags |= STARTF_USESTDHANDLES;

	//create pipes for the stdin/stdout.
	CreatePipe(&ctx->pipefromengine, &startinfo.hStdOutput, &pipesec, 0);
	CreatePipe(&startinfo.hStdInput, &ctx->pipetoengine, &pipesec, 0);

	SetHandleInformation(ctx->pipefromengine, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(ctx->pipetoengine, HANDLE_FLAG_INHERIT, 0);

	Plug_ExecuteCommand(ctx, "vid_recenter %i %i %i %i %#p\n", ctx->windowleft, ctx->windowtop, ctx->windowwidth, ctx->windowheight, (void*)ctx->windowhnd);

	CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, binarypath, &startinfo, &childinfo);

	//these ends of the pipes were inherited by now, so we can discard them in the caller.
	CloseHandle(startinfo.hStdOutput);
	CloseHandle(startinfo.hStdInput);

	return true;
#else
	int in[2];
	int out[2];
        char *argv[64] = {NULL};
	int i;

	i = Plug_GenCommandline(ctx, argv, sizeof(argv) / sizeof(argv[0]));
	if (!i)
		return false;

	argv[i] = NULL;

/*	char buf[512];
	snprintf(buf, sizeof(buf), "%i args\n", i);
	write(2, buf, strlen(buf));
	for (i = 0; argv[i]; i++)
	{
		snprintf(buf, sizeof(buf), "argv[%i] = %s\n", i, argv[i]);
		write(2, buf, strlen(buf));
	}
*/
	pipe(in);
	pipe(out);

	ctx->pipefromengine = in[0];
	ctx->pipetoengine = out[1];

	Plug_ExecuteCommand(ctx, "vid_recenter %i %i %i %i %#p\n", ctx->windowleft, ctx->windowtop, ctx->windowwidth, ctx->windowheight, (void*)ctx->windowhnd);

	ctx->engineprocess = fork();
	if (ctx->engineprocess == 0)
	{
		//close the ends we don't care about
		close(in[0]);
		close(out[1]);

		//dupe them to the stdin/stdout, for ease of use, and clean up the extra handles
		dup2(out[0], 0);
		dup2(in[1], 1);
		//dup2(in[1], 2);
		close(in[1]);
		close(out[0]);

		//invoke the child and kill ourselves if that failed.
		write(2, "invoking engine\n", 16);
		execv(argv[0], argv);
		fprintf(stderr, "execv failed: %s\n", argv[0]);
		exit(1);
	}
	close(in[1]);
	close(out[0]);

	if (ctx->engineprocess == -1)
		return false;

	return true;
#endif
}

void Plug_LockPlugin(struct context *ctx, qboolean lockstate)
{
	if (!ctx || !ctx->mutex)
		return;

	if (lockstate)
		Sys_LockMutex(ctx->mutex);
	else
		Sys_UnlockMutex(ctx->mutex);
}
//#define Plug_LockPlugin(c,s) do{Plug_LockPlugin(c,s);VS_DebugLocation(__FILE__, __LINE__, s?"Lock":"Unlock"); }while(0)

int Plug_PluginThread(void *ctxptr)
{
	char buffer[1024];
	char *nl;
	int bufoffs = 0;
	struct context *ctx = ctxptr;

#if 0
	//I really don't know what to do about multiple active clients downloading at once.
	//maybe just depricate this feature. android ports have the same issue with missing files.
	//we should probably just have the engine take charge of the downloads.
	if (ctx->datadownload)
	{
		char token[1024];
		struct dl_download *dl;
		char *s = ctx->datadownload;
		char *c;
		vfsfile_t *f;
		while ((s = COM_ParseOut(s, token, sizeof(token))))
		{
			//FIXME: do we want to add some sort of file size indicator?
			c = strchr(token, ':');
			if (!c)
				continue;
			*c++ = 0;
			f = VFSSTDIO_Open(va("%s/%s", basedir, token), "rb", NULL);
			if (f)
			{
				Plug_ExecuteCommand(ctx, "echo " "Already have %s\n", token);
				VFS_CLOSE(f);
				continue;
			}

			Plug_ExecuteCommand(ctx, "echo " "Attempting to download %s\n", c);

			dl = DL_Create(c);
			dl->user_ctx = ctx;
			dl->next = ctx->packagelist;
			if (DL_CreateThread(dl, FS_OpenTemp(), UnpackAndExtractPakFiles_Complete))
				ctx->packagelist = dl;
		}

		ctx->pub.downloading = true;
		while(!ctx->shutdown && ctx->packagelist)
		{
			int total=0, done=0;
			ctx->resetvideo = false;
			Sys_LockMutex(ctx->mutex);
			for (dl = ctx->packagelist; dl; dl = dl->next)
			{
				total += dl->totalsize;
				done += dl->completed;
			}
			dl = ctx->packagelist;
			if (total != ctx->pub.dlsize || done != ctx->pub.dldone)
			{
				ctx->pub.dlsize = total;
				ctx->pub.dldone = done;
				if (ctx->bfuncs.StatusChanged)
					ctx->bfuncs.StatusChanged(ctx->hostinstance);
			}
			if (!dl->file)
				ctx->packagelist = dl->next;
			else
				dl = NULL;
			Sys_UnlockMutex(ctx->mutex);

			/*file downloads are not canceled while the plugin is locked, to avoid a race condition*/
			if (dl)
				DL_Close(dl);
			Sleep(10);
		}
		ctx->pub.downloading = false;
	}
#endif

	if (!Plug_CreatePluginProcess(ctx))
	{
		if (!enginedownloadactive)
		{
			strcpy(ctx->pub.statusmessage, "Unable to determine engine to run. Plugin is not correctly installed.");
			if (ctx->bfuncs.StatusChanged)
				ctx->bfuncs.StatusChanged(ctx->hostinstance);
			ctx->pub.running = false;
		}
		else
		{
			int os = -1;
			while(1)
			{
				Sys_LockMutex(globalmutex);
				if (!enginedownloadactive)
				{
					Sys_UnlockMutex(globalmutex);
					if (!Plug_CreatePluginProcess(ctx))
						ctx->pub.running = false;
					break;
				}

				if (os != enginedownloadactive->status)
				{
					os = enginedownloadactive->status;
					switch(os)
					{
					case DL_PENDING:
						strcpy(ctx->pub.statusmessage, "Download pending.");
						break;

					case DL_FAILED:
						strcpy(ctx->pub.statusmessage, "Engine download failed. Try later, or fix internet connection.");
						os = -2;	//stop trying.
						ctx->pub.running = false;
						break;

					case DL_RESOLVING:
						strcpy(ctx->pub.statusmessage, "Download pending. Resolving hostname.");
						break;

					case DL_QUERY:
						strcpy(ctx->pub.statusmessage, "Download pending. Reqeusting file.");
						break;

					case DL_ACTIVE:
						Q_snprintfz(ctx->pub.statusmessage, sizeof(ctx->pub.statusmessage), "Download pending. At %u of %u.", enginedownloadactive->completed, enginedownloadactive->totalsize);
						os = -1;	//keep refreshing
						break;

					case DL_FINISHED:
						strcpy(ctx->pub.statusmessage, "Download completed!");
						DL_Close(enginedownloadactive);
						enginedownloadactive = NULL;
						break;
					}
					Sys_UnlockMutex(globalmutex);
					if (ctx->bfuncs.StatusChanged)
						ctx->bfuncs.StatusChanged(ctx->hostinstance);
				}
				else
					Sys_UnlockMutex(globalmutex);
				if (os == -2)
					break;
#ifdef _WIN32
				Sleep(1000);
#else
				sleep(1);
#endif
			}
		}
	}

	if (ctx->bfuncs.StatusChanged)
		ctx->bfuncs.StatusChanged(ctx->hostinstance);

	if (ctx->pub.running)
	while(1)
	{
#ifdef _WIN32
		DWORD avail;
		//use Peek so we can read exactly how much there is without blocking, so we don't have to read byte-by-byte.
		PeekNamedPipe(ctx->pipefromengine, NULL, 0, NULL, &avail, NULL);
		if (!avail)
			avail = 1;	//so we do actually sleep.
		if (avail > sizeof(buffer)-1 - bufoffs)
			avail = sizeof(buffer)-1 - bufoffs;
		if (!ReadFile(ctx->pipefromengine, buffer + bufoffs, avail, &avail, NULL) || !avail)
		{
			//broken pipe, client died.
			*ctx->pub.statusmessage = 0;
			break;
		}
#else
		int avail;
//		Con_Printf("Attempting to read from pipe...\n");
		if (bufoffs == sizeof(buffer))
		{
			Con_Printf("ERROR: Pipe full!\n");
			bufoffs = 0;	//not much we can really do
		}
		avail = read(ctx->pipefromengine, buffer + bufoffs, 1);
		if (!avail)
		{
			Con_Printf("eof on pipe\n");
			//broken pipe, client died?
			*ctx->pub.statusmessage = 0;
			break;
		}
		if (avail < 0)
		{
			int err = errno;
			if (err == EAGAIN)
				continue;
			perror("hello moo\n");
			//broken pipe, client died?
			Q_strncpyz(ctx->pub.statusmessage, "pipe error", sizeof(ctx->pub.statusmessage));
			break;
		}
#endif
		bufoffs += avail;
		while(1)
		{
			buffer[bufoffs] = 0;
			nl = strchr(buffer, '\n');
			if (nl)
			{
				*nl = 0;
				if (!strncmp(buffer, "status ", 7))
				{
					//don't just strcpy it, copy by byte, saves locking.
					int i = strlen(buffer+7)+1;
					if (i > sizeof(ctx->pub.statusmessage))
						i = sizeof(ctx->pub.statusmessage);
					ctx->pub.statusmessage[i] = 0;
					while (i-->0)
					{
						ctx->pub.statusmessage[i] = buffer[7+i];
					}

					if (ctx->bfuncs.StatusChanged)
						ctx->bfuncs.StatusChanged(ctx->hostinstance);
				}
				else if (!strcmp(buffer, "status"))
				{
					*ctx->pub.statusmessage = 0;
					if (ctx->bfuncs.StatusChanged)
						ctx->bfuncs.StatusChanged(ctx->hostinstance);
					Con_Printf("Status changed to \"%s\"\n", buffer+6);
				}
				else if (!strcmp(buffer, "curserver"))
				{
					Plug_LockPlugin(ctx, true);
					free(ctx->curserver);
					ctx->curserver = strdup(buffer + 10);
					Plug_LockPlugin(ctx, false);
				}
				else
				{
					//handle anything else we need to handle here
					Con_Printf("Unknown command from engine \"%s\"\n", buffer);
				}
			}
			else
				break;
		}
	}
	if (ctx->bfuncs.StatusChanged)
		ctx->bfuncs.StatusChanged(ctx->hostinstance);

	if (ctx == activecontext)
		activecontext = NULL;
	ctx->pub.running = false;
	return 0;
}

//begins the context, fails if one is already active
qboolean Plug_StartContext(struct context *ctx)
{
	if (activecontext && !ctx->multiplecontexts)
		return false;

	Sys_LockMutex(globalmutex);
	if (!ctx->pub.running)
	{
		if (ctx->thread)
		{
			//make sure the thread is killed properly, so we don't get two threads trying to drive the same context
			Sys_UnlockMutex(globalmutex);
			Plug_StopContext(ctx, true);
			Sys_LockMutex(globalmutex);
		}

	}
	if (!ctx->pub.running && !ctx->thread)
	{
		ctx->pub.running = true;
		if (!ctx->multiplecontexts)
			activecontext = ctx;
		if (!ctx->mutex)
			ctx->mutex = Sys_CreateMutex();
		ctx->thread = Sys_CreateThread("pluginctx", Plug_PluginThread, ctx, THREADP_NORMAL, 0);
	}
	Sys_UnlockMutex(globalmutex);

	return true;
}

//asks a context to stop, is not instant.
void Plug_StopContext(struct context *ctx, qboolean wait)
{
	void *thread;
	if (ctx == NULL)
		ctx = activecontext;
	if (!ctx)
		return;

	Plug_ExecuteCommand(ctx, "quit force\n");

	thread = ctx->thread;
	if (thread)
	{
		if (wait)
		{
#ifdef _WIN32
			while (ctx->pub.running && ctx->windowhnd)
			{
				MSG msg;
				while (PeekMessage(&msg, ctx->windowhnd,  0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				Sleep(10);
			}
#endif
			ctx->thread = NULL;
			Sys_WaitOnThread(thread);
		}
	}
}

//creates a plugin context
struct context *Plug_CreateContext(void *sysctx, const struct browserfuncs *funcs)
{
	struct context *ctx;

	if (!sysctx || !funcs)
		return NULL;

	ctx = malloc(sizeof(struct context));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct context));
	memcpy(&ctx->bfuncs, funcs, sizeof(ctx->bfuncs));

	//link the instance to the context and the context to the instance
	ctx->hostinstance = sysctx;

	ctx->gamename = strdup("q1");

	//add it to the linked list
	ctx->next = contextlist;
	contextlist = ctx;

	ctx->qtvf.connectiontype = QTVCT_NONE;

	return ctx;
}

//change the plugin's parent window, width, and height, returns true if the window handle actually changed, false otherwise
qboolean Plug_ChangeWindow(struct context *ctx, void *whnd, int left, int top, int width, int height)
{
	qboolean result = false;

	Plug_LockPlugin(ctx, true);

	//if the window changed
	if (ctx->windowhnd != whnd)
	{
		result = true;
		ctx->windowhnd = whnd;
		ctx->resetvideo = 2;
	}

	ctx->windowleft = left;
	ctx->windowtop = top;
	ctx->windowwidth = width;
	ctx->windowheight = height;

	if (ctx->pub.running && !ctx->resetvideo)
		ctx->resetvideo = true;

	if (ctx->pub.running)
		Plug_ExecuteCommand(ctx, "vid_recenter %i %i %i %i %#p\n", ctx->windowleft, ctx->windowtop, ctx->windowwidth, ctx->windowheight, (void*)ctx->windowhnd);

	Plug_LockPlugin(ctx, false);

	return result;
}

void Plug_DestroyContext(struct context *ctx)
{
	struct context *prev;
	if (ctx == contextlist)
		contextlist = ctx->next;
	else
	{
		for (prev = contextlist; prev->next; prev = prev->next)
		{
			if (prev->next == ctx)
			{
				prev->next = ctx->next;
				break;
			}
		}
	}

	if (ctx->splashdownload)
	{
		DL_Close(ctx->splashdownload);
		ctx->splashdownload = NULL;
	}

	Plug_StopContext(ctx, true);

	if (ctx->mutex)
		Sys_DestroyMutex(ctx->mutex);

	//actually these ifs are not required, just the frees
	if (ctx->gamename)
		free(ctx->gamename);
	if (ctx->password)
		free(ctx->password);
	if (ctx->datadownload)
		free(ctx->datadownload);
	if (ctx->splashdata)
		free(ctx->splashdata);

	free(ctx);
}


////////////////////////////////////////

#if 0
#include "fs.h"
extern searchpathfuncs_t zipfilefuncs;

static int ExtractDataFile(const char *fname, int fsize, void *ptr)
{
	char buffer[8192];
	int read;
	void *zip = ptr;
	flocation_t loc;
	int slashes;
	const char *s;
	vfsfile_t *compressedpak;
	vfsfile_t *decompressedpak;

	if (zipfilefuncs.FindFile(zip, &loc, fname, NULL))
	{
		compressedpak = zipfilefuncs.OpenVFS(zip, &loc, "rb");
		if (compressedpak)
		{
			//this extra logic is so we can handle things like nexuiz/data/blah.pk3
			//as well as just data/blah.pk3
			slashes = 0;
			for (s = strchr(fname, '/'); s; s = strchr(s+1, '/'))
				slashes++;
			for (; slashes > 1; slashes--)
				fname = strchr(fname, '/')+1;

			if (!slashes)
			{
				FS_CreatePath(fname, FS_GAMEONLY);
				decompressedpak = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
			}
			else
			{
				FS_CreatePath(fname, FS_ROOT);
				decompressedpak = FS_OpenVFS(fname, "wb", FS_ROOT);
			}
			if (decompressedpak)
			{
				for(;;)
				{
					read = VFS_READ(compressedpak, buffer, sizeof(buffer));
					if (read <= 0)
						break;
					VFS_WRITE(decompressedpak, buffer, read);
				}
				VFS_CLOSE(decompressedpak);
			}
			VFS_CLOSE(compressedpak);
		}
	}
	return true;
}

static void UnpackAndExtractPakFiles_Complete(struct dl_download *dl)
{
	extern searchpathfuncs_t zipfilefuncs;
	void *zip;

	Plug_LockPlugin(dl->user_ctx, true);

	if (dl->status == DL_FINISHED)
		zip = zipfilefuncs.OpenNew(dl->file, dl->url);
	else
		zip = NULL;
	/*the zip code will have eaten the file handle*/
	dl->file = NULL;
	if (zip)
	{
		/*scan it to extract its contents*/
		zipfilefuncs.EnumerateFiles(zip, "*.pk3", ExtractDataFile, zip);
		zipfilefuncs.EnumerateFiles(zip, "*.pak", ExtractDataFile, zip);

		/*close it, delete the temp file from disk, etc*/
		zipfilefuncs.ClosePath(zip);

		/*restart the filesystem so those new files can be found*/
		Plug_ExecuteCommand(dl->user_ctx, "fs_restart\n");
	}

	Plug_LockPlugin(dl->user_ctx, false);
}
#endif

void LoadSplashImage(struct dl_download *dl)
{
	struct context *ctx = dl->user_ctx;
	vfsfile_t *f = dl->file;
	int x, y;
	int width = 0;
	int height = 0;
	int len;
	char *buffer;
	unsigned char *image;

	Plug_LockPlugin(ctx, true);
	ctx->splashwidth = 0;
	ctx->splashheight = 0;
	image = ctx->splashdata;
	ctx->splashdata = NULL;
	free(image);
	Plug_LockPlugin(ctx, false);

	if (!f)
	{
		if (ctx->bfuncs.StatusChanged)
			ctx->bfuncs.StatusChanged(ctx->hostinstance);
		return;
	}

	len = VFS_GETLEN(f);
	buffer = malloc(len);
	VFS_READ(f, buffer, len);
	VFS_CLOSE(f);
	dl->file = NULL;

	image = NULL;
	if (!image)
		image = ReadJPEGFile(buffer, len, &width, &height);
	if (!image)
		image = ReadPNGFile(buffer, len, &width, &height, dl->url);

	free(buffer);
	if (image)
	{
		Plug_LockPlugin(ctx, true);
		if (ctx->splashdata)
			free(ctx->splashdata);
		ctx->splashdata = malloc(width*height*4);
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				ctx->splashdata[(y*width + x)*4+0] = image[((height-y-1)*width + x)*4+2];
				ctx->splashdata[(y*width + x)*4+1] = image[((height-y-1)*width + x)*4+1];
				ctx->splashdata[(y*width + x)*4+2] = image[((height-y-1)*width + x)*4+0];
			}
		}
		ctx->splashwidth = width;
		ctx->splashheight = height;
		BZ_Free(image);
		Plug_LockPlugin(ctx, false);
		if (ctx->bfuncs.StatusChanged)
			ctx->bfuncs.StatusChanged(ctx->hostinstance);
	}
}

#if 0
static void ReadQTVFileDescriptor(struct context *ctx, vfsfile_t *f, const char *name)
{
	CL_ParseQTVFile(f, name, &ctx->qtvf);

	pscript_property_splash_sets(ctx, ctx->qtvf.splashscreen);
}

void CL_QTVPlay (vfsfile_t *newf, qboolean iseztv);
static void BeginDemo(struct context *ctx, vfsfile_t *f, const char *name)
{
	if (!activecontext)
		activecontext = ctx;

	CL_QTVPlay(f, false);
}
static void EndDemo(struct context *ctx, vfsfile_t *f, const char *name)
{
	Cmd_ExecuteString("disconnect", RESTRICT_LOCAL);
}
#endif
/////////////////////////////////////






struct pscript_property
{
	char *name;

	char *cvarname;

	char *(*getstring)(struct context *ctx);
	void (*setstring)(struct context *ctx, const char *val);

	int (*getint)(struct context *ctx);
	void (*setint)(struct context *ctx, int val);

	float (*getfloat)(struct context *ctx);
	void (*setfloat)(struct context *ctx, float val);
};

int pscript_property_running_getb(struct context *ctx)
{
	if (ctx->pub.running)
		return true;
	else
		return false;
}

void pscript_property_running_setb(struct context *ctx, int i)
{
	i = !!i;
	if (ctx->pub.running == i)
		return;
	if (i)
		Plug_StartContext(ctx);
	else
		Plug_StopContext(ctx, false);
}

char *pscript_property_startserver_gets(struct context *ctx)
{
	return strdup(ctx->qtvf.server);
}
void pscript_property_startserver_sets(struct context *ctx, const char *val)
{
	if (strchr(val, '$') || strchr(val, ';') || strchr(val, '\n'))
		return;

	ctx->qtvf.connectiontype = QTVCT_JOIN;
	Q_strncpyz(ctx->qtvf.server, val, sizeof(ctx->qtvf.server));
}
char *pscript_property_curserver_gets(struct context *ctx)
{
	if (!pscript_property_running_getb(ctx))
		return pscript_property_startserver_gets(ctx);

	if (ctx->curserver)
		return strdup(ctx->curserver);
	else
		return strdup("");
}
void pscript_property_curserver_sets(struct context *ctx, const char *val)
{
	if (strchr(val, '$') || strchr(val, ';') || strchr(val, '\n'))
		return;

	if (!pscript_property_running_getb(ctx))
	{
		pscript_property_startserver_sets(ctx, val);
		return;
	}

	Plug_ExecuteCommand(ctx, "connect \"%s\"\n", val);
}

void pscript_property_stream_sets(struct context *ctx, const char *val)
{
	if (strchr(val, '$') || strchr(val, ';') || strchr(val, '\n'))
		return;

	ctx->qtvf.connectiontype = QTVCT_STREAM;
	Q_strncpyz(ctx->qtvf.server, val, sizeof(ctx->qtvf.server));

	Plug_ExecuteCommand(ctx, "qtvplay \"%s\"\n", val);
}
void pscript_property_map_sets(struct context *ctx, const char *val)
{
	if (strchr(val, '$') || strchr(val, ';') || strchr(val, '\n'))
		return;
	ctx->qtvf.connectiontype = QTVCT_MAP;
	Q_strncpyz(ctx->qtvf.server, val, sizeof(ctx->qtvf.server));

	Plug_ExecuteCommand(ctx, "map \"%s\"\n", val);
}

float pscript_property_curver_getf(struct context *ctx)
{
	int base = FTE_VER_MAJOR * 10000 + FTE_VER_MINOR * 100;
	return base;
//	return version_number();
}

void pscript_property_availver_setf(struct context *ctx, float val)
{
	ctx->pub.availver = val;
	if (ctx->pub.availver <= pscript_property_curver_getf(ctx))
		ctx->pub.availver = 0;
}

void pscript_property_datadownload_sets(struct context *ctx, const char *val)
{
	free(ctx->datadownload);
	ctx->datadownload = strdup(val);
}

void pscript_property_game_sets(struct context *ctx, const char *val)
{
	if (strchr(val, '$') || strchr(val, ';') || strchr(val, '\n'))
		return;

	if (!strstr(val, "."))
		if (!strstr(val, "/"))
			if (!strstr(val, "\\"))
				if (!strstr(val, ":"))
				{
					free(ctx->gamename);
					ctx->gamename = strdup(val);
				}
}

void pscript_property_splash_sets(struct context *ctx, const char *val)
{
	if (ctx->splashdownload)
		DL_Close(ctx->splashdownload);
	ctx->splashdownload = NULL;

	if (val != ctx->qtvf.splashscreen)
		Q_strncpyz(ctx->qtvf.splashscreen, val, sizeof(ctx->qtvf.splashscreen));

/*
	ctx->splashdownload = DL_Create(ctx->qtvf.splashscreen);
	ctx->splashdownload->user_ctx = ctx;
	if (!DL_CreateThread(ctx->splashdownload, VFSPIPE_Open(), LoadSplashImage))
	{
		DL_Close(ctx->splashdownload);
		ctx->splashdownload = NULL;
	}
*/
}

char *pscript_property_build_gets(struct context *ctx)
{
	return strdup(DISTRIBUTION " " __DATE__ " " __TIME__
#if defined(DEBUG) || defined(_DEBUG)
		" (debug)"
#endif
		);
}

float pscript_property_multi_getf(struct context *ctx)
{
	return ctx->multiplecontexts;
}
void pscript_property_multi_setf(struct context *ctx, float f)
{
	ctx->multiplecontexts = !!f;
}

static struct pscript_property pscript_properties[] =
{
	{"",			NULL,	pscript_property_curserver_gets, pscript_property_curserver_sets},
	{"server",		NULL,	pscript_property_curserver_gets, pscript_property_curserver_sets},
	{"running",		NULL,	NULL, NULL, pscript_property_running_getb, pscript_property_running_setb},
	{"startserver",	NULL,	pscript_property_startserver_gets, pscript_property_startserver_sets},
	{"join",		NULL,	NULL, pscript_property_curserver_sets},
	{"playername",	"name"},
	{NULL,			"skin"},
	{NULL,			"team"},
	{NULL,			"topcolor"},
	{NULL,			"bottomcolor"},
	{NULL,			"password"},	//cvars are write only, just so you know.
//	{NULL,			"spectator"},
	{"mapsrc",		"cl_download_mapsrc"},
	{"fullscreen",	"vid_fullscreen"},

	{"datadownload",NULL,	NULL, pscript_property_datadownload_sets},
	
	{"game",		NULL,	NULL, pscript_property_game_sets},
	{"availver",	NULL,	NULL, NULL,	NULL, NULL,	NULL, pscript_property_availver_setf},
	{"plugver",		NULL,	NULL, NULL,	NULL, NULL,	pscript_property_curver_getf},
	{"multiple",	NULL,	NULL, NULL,	NULL, NULL,	pscript_property_multi_getf, pscript_property_multi_setf},
	
	{"splash",		NULL,	NULL, pscript_property_splash_sets},

	{"stream",		NULL,	NULL, pscript_property_stream_sets},
	{"map",			NULL,	NULL, pscript_property_map_sets},

	{"build",		NULL,	pscript_property_build_gets},

	{NULL}
};

int Plug_FindProp(struct context *ctx, const char *field)
{
	struct pscript_property *prop;
	for (prop = pscript_properties; prop->name||prop->cvarname; prop++)
	{
		if (!stricmp(prop->name?prop->name:prop->cvarname, field))
		{
//			if (prop->onlyifactive)
//			{
//				if (!ctx->pub.running)
//					return -1;
//			}
			return prop - pscript_properties;
		}
	}
	return -1;
}

qboolean Plug_SetString(struct context *ctx, int fieldidx, const char *value)
{
	struct pscript_property *field = pscript_properties + fieldidx;
	if (!ctx || fieldidx < 0 || fieldidx >= sizeof(pscript_properties)/sizeof(pscript_properties[0]) || !value)
		return false;
	if (field->setstring)
	{
		Plug_LockPlugin(ctx, true);
		field->setstring(ctx, value);
		Plug_LockPlugin(ctx, false);
	}
	else if (field->setint)
	{
		Plug_LockPlugin(ctx, true);
		field->setint(ctx, atoi(value));
		Plug_LockPlugin(ctx, false);
	}
	else if (field->setfloat)
	{
		Plug_LockPlugin(ctx, true);
		field->setfloat(ctx, atof(value));
		Plug_LockPlugin(ctx, false);
	}
	else if (field->cvarname && ctx->pub.running)
	{
		Plug_LockPlugin(ctx, true);
		Plug_ExecuteCommand(ctx, "%s \"%s\"\n", field->cvarname, value);
		Plug_LockPlugin(ctx, false);
	}
	else
		return false;
	return true;
}
qboolean Plug_SetWString(struct context *ctx, int fieldidx, const wchar_t *value)
{
	char tmp[1024];
	wcstombs(tmp, value, sizeof(tmp));
	return Plug_SetString(ctx, fieldidx, tmp);
}
qboolean Plug_SetInteger(struct context *ctx, int fieldidx, int value)
{
	struct pscript_property *field = pscript_properties + fieldidx;
	if (!ctx || fieldidx < 0 || fieldidx >= sizeof(pscript_properties)/sizeof(pscript_properties[0]))
		return false;
	if (field->setint)
	{
		Plug_LockPlugin(ctx, true);
		field->setint(ctx, value);
		Plug_LockPlugin(ctx, false);
	}
	else if (field->setfloat)
	{
		Plug_LockPlugin(ctx, true);
		field->setfloat(ctx, value);
		Plug_LockPlugin(ctx, false);
	}
	else if (field->cvarname && ctx->pub.running)
	{
		Plug_LockPlugin(ctx, true);
		Plug_ExecuteCommand(ctx, "%s \"%i\"\n", field->cvarname, value);
		Plug_LockPlugin(ctx, false);
	}
	else
		return false;
	return true;
}
qboolean Plug_SetFloat(struct context *ctx, int fieldidx, float value)
{
	struct pscript_property *field = pscript_properties + fieldidx;
	if (!ctx || fieldidx < 0 || fieldidx >= sizeof(pscript_properties)/sizeof(pscript_properties[0]))
		return false;
	if (field->setfloat)
	{
		Plug_LockPlugin(ctx, true);
		field->setfloat(ctx, value);
		Plug_LockPlugin(ctx, false);
	}
	else if (field->setint)
	{
		Plug_LockPlugin(ctx, true);
		field->setint(ctx, value);
		Plug_LockPlugin(ctx, false);
	}
	else if (field->cvarname && ctx->pub.running)
	{
		Plug_LockPlugin(ctx, true);
		Plug_ExecuteCommand(ctx, "%s \"%f\"\n", field->cvarname, value);
		Plug_LockPlugin(ctx, false);
	}
	else
		return false;
	return true;
}

qboolean Plug_GetString(struct context *ctx, int fieldidx, const char **value)
{
	struct pscript_property *field = pscript_properties + fieldidx;
	if (!ctx || fieldidx < 0 || fieldidx >= sizeof(pscript_properties)/sizeof(pscript_properties[0]))
		return false;

	if (field->getstring)
	{
		*value = field->getstring(ctx);
		return true;
	}
	return false;
}
void Plug_GotString(const char *value)
{
	free((char*)value);
}
qboolean Plug_GetInteger(struct context *ctx, int fieldidx, int *value)
{
	struct pscript_property *field = pscript_properties + fieldidx;
	if (!ctx || fieldidx < 0 || fieldidx >= sizeof(pscript_properties)/sizeof(pscript_properties[0]))
		return false;

	if (field->getint)
	{
		*value = field->getint(ctx);
		return true;
	}
	return false;
}
qboolean Plug_GetFloat(struct context *ctx, int fieldidx, float *value)
{
	struct pscript_property *field = pscript_properties + fieldidx;
	if (!ctx || fieldidx < 0 || fieldidx >= sizeof(pscript_properties)/sizeof(pscript_properties[0]))
		return false;

	if (field->getfloat)
	{
		*value = field->getfloat(ctx);
		return true;
	}
	return false;
}

#ifdef _WIN32
void *Plug_GetSplashBack(struct context *ctx, void *hdc, int *width, int *height)
{
	BITMAPINFOHEADER bmh;

	if (!ctx->splashdata)
		return NULL;

	bmh.biSize = sizeof(bmh);
    bmh.biWidth = *width = ctx->splashwidth;
    bmh.biHeight = *height = ctx->splashheight;
    bmh.biPlanes = 1;
    bmh.biBitCount = 32;
    bmh.biCompression = BI_RGB;
    bmh.biSizeImage = 0;
    bmh.biXPelsPerMeter = 0;
    bmh.biYPelsPerMeter = 0;
    bmh.biClrUsed = 0;
    bmh.biClrImportant = 0;

	return CreateDIBitmap(hdc, 
            &bmh, 
            CBM_INIT, 
            (LPSTR)ctx->splashdata, 
            (LPBITMAPINFO)&bmh, 
            DIB_RGB_COLORS ); 
}
void Plug_ReleaseSplashBack(struct context *ctx, void *bmp)
{
	DeleteObject(bmp);
}
#endif

static const struct plugfuncs exportedplugfuncs_1 =
{
	Plug_CreateContext,
	Plug_DestroyContext,
	Plug_LockPlugin,
	Plug_StartContext,
	Plug_StopContext,
	Plug_ChangeWindow,

	Plug_FindProp,
	Plug_SetString,
	Plug_GetString,
	Plug_GotString,
	Plug_SetInteger,
	Plug_GetInteger,
	Plug_SetFloat,
	Plug_GetFloat,

#ifdef _WIN32
	Plug_GetSplashBack,
	Plug_ReleaseSplashBack,
#endif

	Plug_SetWString
};

const struct plugfuncs *Plug_GetFuncs(int ver)
{
	if (!globalmutex)
		globalmutex = Sys_CreateMutex();
	if (ver == 1)
		return &exportedplugfuncs_1;
	else
		return NULL;
}
