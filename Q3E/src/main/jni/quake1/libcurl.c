#include "quakedef.h"
#include "fs.h"
#include "libcurl.h"
#include "thread.h"
#include "com_list.h"
#include "image.h"
#include "jpeg.h"
#include "image_png.h"

static cvar_t curl_enabled = {CF_SHARED | CF_ARCHIVE, "curl_enabled","1", "whether libcurl may be used to GET files or POST data"};
static cvar_t curl_maxdownloads = {CF_SHARED | CF_ARCHIVE, "curl_maxdownloads","3", "maximum number of concurrent HTTP/FTP downloads"};
static cvar_t curl_maxspeed = {CF_SHARED | CF_ARCHIVE, "curl_maxspeed","0", "maximum download speed (KiB/s)"};
static cvar_t curl_useragent = {CF_SHARED, "curl_useragent","1", "send the User-Agent string (note: turning this off may break stuff)"};
static cvar_t curl_useragent_append = {CF_SHARED, "curl_useragent_append","", "a string to append to the User-Agent string (useful for name and version number of your mod)"};

static cvar_t sv_curl_defaulturl = {CF_SERVER, "sv_curl_defaulturl","", "default autodownload source URL"};
static cvar_t sv_curl_serverpackages = {CF_SERVER, "sv_curl_serverpackages","", "list of required files for the clients, separated by spaces"};
static cvar_t sv_curl_maxspeed = {CF_SERVER, "sv_curl_maxspeed","0", "maximum download speed for clients downloading from sv_curl_defaulturl (KiB/s)"};

static cvar_t developer_curl = {CF_SHARED, "developer_curl","0", "whether verbose libcurl output should be printed to stderr"};

/*
=================================================================

  Minimal set of definitions from libcurl

  WARNING: for a matter of simplicity, several pointer types are
  casted to "void*", and most enumerated values are not included

=================================================================
*/

typedef struct CURL_s CURL;
typedef struct CURLM_s CURLM;
typedef struct curl_slist curl_slist;
typedef enum
{
	CURLE_OK = 0
}
CURLcode;
typedef enum
{
	CURLM_CALL_MULTI_PERFORM=-1, /* please call curl_multi_perform() soon */
	CURLM_OK = 0
}
CURLMcode;
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_SSL 1
#define CURL_GLOBAL_WIN32 2
#define CURLOPTTYPE_LONG          0
#define CURLOPTTYPE_OBJECTPOINT   10000
#define CURLOPTTYPE_FUNCTIONPOINT 20000
#define CURLOPTTYPE_OFF_T         30000
#define CINIT(name,type,number) CURLOPT_ ## name = CURLOPTTYPE_ ## type + number
typedef enum
{
	CINIT(WRITEDATA, OBJECTPOINT, 1),
	CINIT(URL,  OBJECTPOINT, 2),
	CINIT(ERRORBUFFER, OBJECTPOINT, 10),
	CINIT(WRITEFUNCTION, FUNCTIONPOINT, 11),
	CINIT(POSTFIELDS, OBJECTPOINT, 15),
	CINIT(REFERER, OBJECTPOINT, 16),
	CINIT(USERAGENT, OBJECTPOINT, 18),
	CINIT(LOW_SPEED_LIMIT, LONG , 19),
	CINIT(LOW_SPEED_TIME, LONG, 20),
	CINIT(RESUME_FROM, LONG, 21),
	CINIT(HTTPHEADER, OBJECTPOINT, 23),
	CINIT(VERBOSE, LONG, 41),
	CINIT(POST, LONG, 47),         /* HTTP POST method */
	CINIT(FOLLOWLOCATION, LONG, 52),  /* use Location: Luke! */
	CINIT(POSTFIELDSIZE, LONG, 60),
	CINIT(PRIVATE, OBJECTPOINT, 103),
	CINIT(PROTOCOLS, LONG, 181),
	CINIT(REDIR_PROTOCOLS, LONG, 182)
}
CURLoption;
#define CURLPROTO_HTTP   (1<<0)
#define CURLPROTO_HTTPS  (1<<1)
#define CURLPROTO_FTP    (1<<2)
typedef enum
{
	CURLINFO_TEXT = 0,
	CURLINFO_HEADER_IN,    /* 1 */
	CURLINFO_HEADER_OUT,   /* 2 */
	CURLINFO_DATA_IN,      /* 3 */
	CURLINFO_DATA_OUT,     /* 4 */
	CURLINFO_SSL_DATA_IN,  /* 5 */
	CURLINFO_SSL_DATA_OUT, /* 6 */
	CURLINFO_END
}
curl_infotype;
#define CURLINFO_STRING   0x100000
#define CURLINFO_LONG     0x200000
#define CURLINFO_DOUBLE   0x300000
#define CURLINFO_SLIST    0x400000
#define CURLINFO_MASK     0x0fffff
#define CURLINFO_TYPEMASK 0xf00000
typedef enum
{
	CURLINFO_NONE, /* first, never use this */
	CURLINFO_EFFECTIVE_URL    = CURLINFO_STRING + 1,
	CURLINFO_RESPONSE_CODE    = CURLINFO_LONG   + 2,
	CURLINFO_TOTAL_TIME       = CURLINFO_DOUBLE + 3,
	CURLINFO_NAMELOOKUP_TIME  = CURLINFO_DOUBLE + 4,
	CURLINFO_CONNECT_TIME     = CURLINFO_DOUBLE + 5,
	CURLINFO_PRETRANSFER_TIME = CURLINFO_DOUBLE + 6,
	CURLINFO_SIZE_UPLOAD      = CURLINFO_DOUBLE + 7,
	CURLINFO_SIZE_DOWNLOAD    = CURLINFO_DOUBLE + 8,
	CURLINFO_SPEED_DOWNLOAD   = CURLINFO_DOUBLE + 9,
	CURLINFO_SPEED_UPLOAD     = CURLINFO_DOUBLE + 10,
	CURLINFO_HEADER_SIZE      = CURLINFO_LONG   + 11,
	CURLINFO_REQUEST_SIZE     = CURLINFO_LONG   + 12,
	CURLINFO_SSL_VERIFYRESULT = CURLINFO_LONG   + 13,
	CURLINFO_FILETIME         = CURLINFO_LONG   + 14,
	CURLINFO_CONTENT_LENGTH_DOWNLOAD   = CURLINFO_DOUBLE + 15,
	CURLINFO_CONTENT_LENGTH_UPLOAD     = CURLINFO_DOUBLE + 16,
	CURLINFO_STARTTRANSFER_TIME = CURLINFO_DOUBLE + 17,
	CURLINFO_CONTENT_TYPE     = CURLINFO_STRING + 18,
	CURLINFO_REDIRECT_TIME    = CURLINFO_DOUBLE + 19,
	CURLINFO_REDIRECT_COUNT   = CURLINFO_LONG   + 20,
	CURLINFO_PRIVATE          = CURLINFO_STRING + 21,
	CURLINFO_HTTP_CONNECTCODE = CURLINFO_LONG   + 22,
	CURLINFO_HTTPAUTH_AVAIL   = CURLINFO_LONG   + 23,
	CURLINFO_PROXYAUTH_AVAIL  = CURLINFO_LONG   + 24,
	CURLINFO_OS_ERRNO         = CURLINFO_LONG   + 25,
	CURLINFO_NUM_CONNECTS     = CURLINFO_LONG   + 26,
	CURLINFO_SSL_ENGINES      = CURLINFO_SLIST  + 27
}
CURLINFO;

typedef enum
{
	CURLMSG_NONE, /* first, not used */
	CURLMSG_DONE, /* This easy handle has completed. 'result' contains
					 the CURLcode of the transfer */
	CURLMSG_LAST
}
CURLMSG;
typedef struct
{
	CURLMSG msg;       /* what this message means */
	CURL *easy_handle; /* the handle it concerns */
	union
	{
		void *whatever;    /* message-specific data */
		CURLcode result;   /* return code for transfer */
	}
	data;
}
CURLMsg;

static void (*qcurl_global_init) (long flags);
static void (*qcurl_global_cleanup) (void);

static CURL * (*qcurl_easy_init) (void);
static void (*qcurl_easy_cleanup) (CURL *handle);
static CURLcode (*qcurl_easy_setopt) (CURL *handle, CURLoption option, ...);
static CURLcode (*qcurl_easy_getinfo) (CURL *handle, CURLINFO info, ...);
static const char * (*qcurl_easy_strerror) (CURLcode);

static CURLM * (*qcurl_multi_init) (void);
static CURLMcode (*qcurl_multi_perform) (CURLM *multi_handle, int *running_handles);
static CURLMcode (*qcurl_multi_wait) (CURLM *multi_handle, void*, unsigned int extra_nfds, int timeout_ms, int *ret);
static CURLMcode (*qcurl_multi_add_handle) (CURLM *multi_handle, CURL *easy_handle);
static CURLMcode (*qcurl_multi_remove_handle) (CURLM *multi_handle, CURL *easy_handle);
static CURLMsg * (*qcurl_multi_info_read) (CURLM *multi_handle, int *msgs_in_queue);
static void (*qcurl_multi_cleanup) (CURLM *);
static const char * (*qcurl_multi_strerror) (CURLcode);
static curl_slist * (*qcurl_slist_append) (curl_slist *list, const char *string);
static void (*qcurl_slist_free_all) (curl_slist *list);

static dllfunction_t curlfuncs[] =
{
	{"curl_global_init",		(void **) &qcurl_global_init},
	{"curl_global_cleanup",		(void **) &qcurl_global_cleanup},
	{"curl_easy_init",			(void **) &qcurl_easy_init},
	{"curl_easy_cleanup",		(void **) &qcurl_easy_cleanup},
	{"curl_easy_setopt",		(void **) &qcurl_easy_setopt},
	{"curl_easy_strerror",		(void **) &qcurl_easy_strerror},
	{"curl_easy_getinfo",		(void **) &qcurl_easy_getinfo},
	{"curl_multi_init",			(void **) &qcurl_multi_init},
	{"curl_multi_perform",		(void **) &qcurl_multi_perform},
	{"curl_multi_wait",		(void **) &qcurl_multi_wait},
	{"curl_multi_add_handle",	(void **) &qcurl_multi_add_handle},
	{"curl_multi_remove_handle",(void **) &qcurl_multi_remove_handle},
	{"curl_multi_info_read",	(void **) &qcurl_multi_info_read},
	{"curl_multi_cleanup",		(void **) &qcurl_multi_cleanup},
	{"curl_multi_strerror",		(void **) &qcurl_multi_strerror},
	{"curl_slist_append",		(void **) &qcurl_slist_append},
	{"curl_slist_free_all",		(void **) &qcurl_slist_free_all},
	{NULL, NULL}
};

// Handle for CURL DLL
static dllhandle_t curl_dll = NULL;
// will be checked at many places to find out if qcurl calls are allowed

#define LOADTYPE_NONE 0
#define LOADTYPE_PAK 1
#define LOADTYPE_CACHEPIC 2
#define LOADTYPE_SKINFRAME 3

void *curl_mutex = NULL;

typedef struct downloadinfo_s
{
	char filename[MAX_OSPATH];
	char url[1024];
	char referer[256];
	qfile_t *stream;
	fs_offset_t startpos;
	CURL *curle;
	qbool started;
	int loadtype;
	size_t bytes_received; // for buffer
	double bytes_received_curl; // for throttling
	double bytes_sent_curl; // for throttling
	llist_t list;
	qbool forthismap;
	double maxspeed;
	curl_slist *slist; // http headers

	unsigned char *buffer;
	size_t buffersize;
	curl_callback_t callback;
	void *callback_data;

	const unsigned char *postbuf;
	size_t postbufsize;
	const char *post_content_type;
	const char *extraheaders;
}
downloadinfo;
LIST_HEAD(downloads);
static int numdownloads = 0;

static qbool noclear = false;

static int numdownloads_fail = 0;
static int numdownloads_success = 0;
static int numdownloads_added = 0;
static char command_when_done[256] = "";
static char command_when_error[256] = "";

/*
====================
Curl_CommandWhenDone

Sets the command which is to be executed when the last download completes AND
all downloads since last server connect ended with a successful status.
Setting the command to NULL clears it.
====================
*/
static void Curl_CommandWhenDone(const char *cmd)
{
	if(!curl_dll)
		return;
	if(cmd)
		dp_strlcpy(command_when_done, cmd, sizeof(command_when_done));
	else
		*command_when_done = 0;
}

/*
FIXME
Do not use yet. Not complete.
Problem: what counts as an error?
*/

static void Curl_CommandWhenError(const char *cmd)
{
	if(!curl_dll)
		return;
	if(cmd)
		dp_strlcpy(command_when_error, cmd, sizeof(command_when_error));
	else
		*command_when_error = 0;
}

/*
====================
Curl_Clear_forthismap

Clears the "will disconnect on failure" flags.
====================
*/
void Curl_Clear_forthismap(void)
{
	downloadinfo *di;
	if(noclear)
		return;
	if (curl_mutex) Thread_LockMutex(curl_mutex);
	List_For_Each_Entry(di, &downloads, downloadinfo, list)
		di->forthismap = false;
	Curl_CommandWhenError(NULL);
	Curl_CommandWhenDone(NULL);
	numdownloads_fail = 0;
	numdownloads_success = 0;
	numdownloads_added = 0;
	if (curl_mutex) Thread_UnlockMutex(curl_mutex);
}

/*
====================
Curl_Have_forthismap

Returns true if a download needed for the current game is running.
====================
*/
qbool Curl_Have_forthismap(void)
{
	return numdownloads_added != 0;
}

void Curl_Register_predownload(void)
{
	if (curl_mutex) Thread_LockMutex(curl_mutex);
	Curl_CommandWhenDone("cl_begindownloads");
	Curl_CommandWhenError("cl_begindownloads");
	if (curl_mutex) Thread_UnlockMutex(curl_mutex);
}

/*
====================
Curl_CheckCommandWhenDone

Checks if a "done command" is to be executed.
All downloads finished, at least one success since connect, no single failure
-> execute the command.
*/
static void Curl_CheckCommandWhenDone(void)
{
	if(!curl_dll)
		return;
	if(numdownloads_added && ((numdownloads_success + numdownloads_fail) == numdownloads_added))
	{
		if(numdownloads_fail == 0)
		{
			Con_DPrintf("cURL downloads occurred, executing %s\n", command_when_done);
			Cbuf_AddText(cmd_local, "\n");
			Cbuf_AddText(cmd_local, command_when_done);
			Cbuf_AddText(cmd_local, "\n");
		}
		else
		{
			Con_DPrintf("cURL downloads FAILED, executing %s\n", command_when_error);
			Cbuf_AddText(cmd_local, "\n");
			Cbuf_AddText(cmd_local, command_when_error);
			Cbuf_AddText(cmd_local, "\n");
		}
		Curl_Clear_forthismap();
	}
}

/*
====================
CURL_CloseLibrary

Load the cURL DLL
====================
*/
static qbool CURL_OpenLibrary (void)
{
	const char* dllnames [] =
	{
#if defined(WIN32)
		"libcurl-4.dll",
		"libcurl-3.dll",
#elif defined(MACOSX)
		"libcurl.4.dylib", // Mac OS X Notyetreleased
		"libcurl.3.dylib", // Mac OS X Tiger
		"libcurl.2.dylib", // Mac OS X Panther
#else
		"libcurl.so.4",
		"libcurl.so.3",
		"libcurl.so", // FreeBSD
#endif
		NULL
	};

	// Already loaded?
	if (curl_dll)
		return true;

	// Load the DLL
	return Sys_LoadDependency (dllnames, &curl_dll, curlfuncs);
}


/*
====================
CURL_CloseLibrary

Unload the cURL DLL
====================
*/
static void CURL_CloseLibrary (void)
{
	Sys_FreeLibrary (&curl_dll);
}


static CURLM *curlm = NULL;
static double bytes_received = 0; // used for bandwidth throttling
static double bytes_sent = 0; // used for bandwidth throttling
static double curltime = 0;

/*
====================
CURL_fwrite

fwrite-compatible function that writes the data to a file. libcurl can call
this.
====================
*/
static size_t CURL_fwrite(void *data, size_t size, size_t nmemb, void *vdi)
{
	fs_offset_t ret = -1;
	size_t bytes = size * nmemb;
	downloadinfo *di = (downloadinfo *) vdi;

	if(di->buffer)
	{
		if(di->bytes_received + bytes <= di->buffersize)
		{
			memcpy(di->buffer + di->bytes_received, data, bytes);
			ret = bytes;
		}
		// otherwise: buffer overrun, ret stays -1
	}

	if(di->stream)
	{
		ret = FS_Write(di->stream, data, bytes);
	}

	di->bytes_received += bytes;

	//Con_Printf("CURL_fwrite callback timestamp: %f bytes: %ld\n", host.realtime, ret);

	return ret;
	// Why not ret / nmemb?
	// Because CURLOPT_WRITEFUNCTION docs say to return the number of bytes.
	// Yes, this is incompatible to fwrite(2).
}

typedef enum
{
	CURL_DOWNLOAD_SUCCESS = 0,
	CURL_DOWNLOAD_FAILED,
	CURL_DOWNLOAD_ABORTED,
	CURL_DOWNLOAD_SERVERERROR
}
CurlStatus;

static void curl_default_callback(int status, size_t length_received, unsigned char *buffer, void *cbdata)
{
	downloadinfo *di = (downloadinfo *) cbdata;
	switch(status)
	{
		case CURLCBSTATUS_OK:
			Con_DPrintf("Download of %s: OK\n", di->filename);
			break;
		case CURLCBSTATUS_FAILED:
			Con_DPrintf("Download of %s: FAILED\n", di->filename);
			break;
		case CURLCBSTATUS_ABORTED:
			Con_DPrintf("Download of %s: ABORTED\n", di->filename);
			break;
		case CURLCBSTATUS_SERVERERROR:
			Con_DPrintf("Download of %s: (unknown server error)\n", di->filename);
			break;
		case CURLCBSTATUS_UNKNOWN:
			Con_DPrintf("Download of %s: (unknown client error)\n", di->filename);
			break;
		default:
			Con_DPrintf("Download of %s: %d\n", di->filename, status);
			break;
	}
}

static void curl_quiet_callback(int status, size_t length_received, unsigned char *buffer, void *cbdata)
{
	curl_default_callback(status, length_received, buffer, cbdata);
}

static unsigned char *decode_image(downloadinfo *di, const char *content_type)
{
	unsigned char *pixels = NULL;
	fs_offset_t filesize = 0;
	unsigned char *data = FS_LoadFile(di->filename, tempmempool, true, &filesize);
	if(data)
	{
		int mip = 0;
		if(!strcmp(content_type, "image/jpeg"))
			pixels = JPEG_LoadImage_BGRA(data, filesize, &mip);
		else if(!strcmp(content_type, "image/png"))
			pixels = PNG_LoadImage_BGRA(data, filesize, &mip);
		else if(filesize >= 7 && !strncmp((char *) data, "\xFF\xD8", 7))
			pixels = JPEG_LoadImage_BGRA(data, filesize, &mip);
		else if(filesize >= 7 && !strncmp((char *) data, "\x89PNG\x0D\x0A\x1A\x0A", 7))
			pixels = PNG_LoadImage_BGRA(data, filesize, &mip);
		else
			Con_Printf("Did not detect content type: %s\n", content_type);
		Mem_Free(data);
	}
	// do we call Image_MakeLinearColorsFromsRGB or not?
	return pixels;
}

/*
====================
Curl_EndDownload

stops a download. It receives a status (CURL_DOWNLOAD_SUCCESS,
CURL_DOWNLOAD_FAILED or CURL_DOWNLOAD_ABORTED) and in the second case the error
code from libcurl, or 0, if another error has occurred.
====================
*/
static qbool Curl_Begin(const char *URL, const char *extraheaders, double maxspeed, const char *name, int loadtype, qbool forthismap, const char *post_content_type, const unsigned char *postbuf, size_t postbufsize, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata);
static void Curl_EndDownload(downloadinfo *di, CurlStatus status, CURLcode error, const char *content_type_)
{
	char content_type[64];
	qbool ok = false;
	if(!curl_dll)
		return;
	switch(status)
	{
		case CURL_DOWNLOAD_SUCCESS:
			ok = true;
			di->callback(CURLCBSTATUS_OK, di->bytes_received, di->buffer, di->callback_data);
			break;
		case CURL_DOWNLOAD_FAILED:
			di->callback(CURLCBSTATUS_FAILED, di->bytes_received, di->buffer, di->callback_data);
			break;
		case CURL_DOWNLOAD_ABORTED:
			di->callback(CURLCBSTATUS_ABORTED, di->bytes_received, di->buffer, di->callback_data);
			break;
		case CURL_DOWNLOAD_SERVERERROR:
			// reopen to enforce it to have zero bytes again
			if(di->stream)
			{
				FS_Close(di->stream);
				di->stream = FS_OpenRealFile(di->filename, "wb", false);
			}

			if(di->callback)
				di->callback(error ? (int) error : CURLCBSTATUS_SERVERERROR, di->bytes_received, di->buffer, di->callback_data);
			break;
		default:
			if(di->callback)
				di->callback(CURLCBSTATUS_UNKNOWN, di->bytes_received, di->buffer, di->callback_data);
			break;
	}
	if(content_type_)
		dp_strlcpy(content_type, content_type_, sizeof(content_type));
	else
		*content_type = 0;

	if(di->curle)
	{
		qcurl_multi_remove_handle(curlm, di->curle);
		qcurl_easy_cleanup(di->curle);
		if(di->slist)
			qcurl_slist_free_all(di->slist);
	}

	if(!di->callback && ok && !di->bytes_received)
	{
		Con_Printf("ERROR: empty file\n");
		ok = false;
	}

	if(di->stream)
		FS_Close(di->stream);

#define CLEAR_AND_RETRY() \
	do \
	{ \
		di->stream = FS_OpenRealFile(di->filename, "wb", false); \
		FS_Close(di->stream); \
		if(di->startpos && !di->callback) \
		{ \
			Curl_Begin(di->url, di->extraheaders, di->maxspeed, di->filename, di->loadtype, di->forthismap, di->post_content_type, di->postbuf, di->postbufsize, NULL, 0, NULL, NULL); \
			di->forthismap = false; \
		} \
	} \
	while(0)

	if(ok && di->loadtype == LOADTYPE_PAK)
	{
		ok = FS_AddPack(di->filename, NULL, true, true);
		if(!ok)
			CLEAR_AND_RETRY();
	}
	else if(ok && di->loadtype == LOADTYPE_CACHEPIC)
	{
		const char *p;
		unsigned char *pixels = NULL;

		p = di->filename;
#ifdef WE_ARE_EVIL
		if(!strncmp(p, "dlcache/", 8))
			p += 8;
#endif

		pixels = decode_image(di, content_type);
		if(pixels)
			Draw_NewPic(p, image_width, image_height, pixels, TEXTYPE_BGRA, TEXF_ALPHA | TEXF_CLAMP);
		else
			CLEAR_AND_RETRY();
	}
	else if(ok && di->loadtype == LOADTYPE_SKINFRAME)
	{
		const char *p;
		unsigned char *pixels = NULL;

		p = di->filename;
#ifdef WE_ARE_EVIL
		if(!strncmp(p, "dlcache/", 8))
			p += 8;
#endif

		pixels = decode_image(di, content_type);
		if(pixels)
			R_SkinFrame_LoadInternalBGRA(p, TEXF_FORCE_RELOAD | TEXF_MIPMAP | TEXF_ALPHA, pixels, image_width, image_height, 0, 0, 0, false); // TODO what sRGB argument to put here?
		else
			CLEAR_AND_RETRY();
	}

	List_Delete(&di->list);

	--numdownloads;
	if(di->forthismap)
	{
		if(ok)
			++numdownloads_success;
		else
			++numdownloads_fail;
	}
	Z_Free(di);
}

/*
====================
CleanURL

Returns a "cleaned up" URL for display (to strip login data)
====================
*/
static const char *CleanURL(const char *url, char *urlbuf, size_t urlbuflength)
{
	const char *p, *q, *r;

	// if URL is of form anything://foo-without-slash@rest, replace by anything://rest
	p = strstr(url, "://");
	if(p)
	{
		q = strchr(p + 3, '@');
		if(q)
		{
			r = strchr(p + 3, '/');
			if(!r || q < r)
			{
				dpsnprintf(urlbuf, urlbuflength, "%.*s%s", (int)(p - url + 3), url, q + 1);
				return urlbuf;
			}
		}
	}

	return url;
}

/*
====================
CheckPendingDownloads

checks if there are free download slots to start new downloads in.
To not start too many downloads at once, only one download is added at a time,
up to a maximum number of cl_curl_maxdownloads are running.
====================
*/
static void CheckPendingDownloads(void)
{
	const char *h;
	char urlbuf[1024];
	char vabuf[1024];
	if(!curl_dll)
		return;
	if(numdownloads < curl_maxdownloads.integer)
	{
		downloadinfo *di;
		List_For_Each_Entry(di, &downloads, downloadinfo, list)
		{
			if(!di->started)
			{
				if(!di->buffer)
				{
					Con_Printf("Downloading %s -> %s", CleanURL(di->url, urlbuf, sizeof(urlbuf)), di->filename);

					di->stream = FS_OpenRealFile(di->filename, "ab", false);
					if(!di->stream)
					{
						Con_Printf("\nFAILED: Could not open output file %s\n", di->filename);
						Curl_EndDownload(di, CURL_DOWNLOAD_FAILED, CURLE_OK, NULL);
						return;
					}
					FS_Seek(di->stream, 0, SEEK_END);
					di->startpos = FS_Tell(di->stream);

					if(di->startpos > 0)
						Con_Printf(", resuming from position %ld", (long) di->startpos);
					Con_Print("...\n");
				}
				else
				{
					Con_DPrintf("Downloading %s -> memory\n", CleanURL(di->url, urlbuf, sizeof(urlbuf)));
					di->startpos = 0;
				}

				di->curle = qcurl_easy_init();
				di->slist = NULL;
				qcurl_easy_setopt(di->curle, CURLOPT_URL, di->url);
				if(curl_useragent.integer)
				{
					const char *ua
#ifdef HTTP_USER_AGENT
						= HTTP_USER_AGENT;
#else
						= engineversion;
#endif
					if(!ua)
						ua = "";
					if(*curl_useragent_append.string)
						ua = va(vabuf, sizeof(vabuf), "%s%s%s",
							ua,
							(ua[0] && ua[strlen(ua)-1] != ' ')
								? " "
								: "",
							curl_useragent_append.string);
					qcurl_easy_setopt(di->curle, CURLOPT_USERAGENT, ua);
				}
				else
					qcurl_easy_setopt(di->curle, CURLOPT_USERAGENT, "");
				if(developer_curl.integer) 
					qcurl_easy_setopt(di->curle, CURLOPT_VERBOSE, (long) 1);
				qcurl_easy_setopt(di->curle, CURLOPT_REFERER, di->referer);
				qcurl_easy_setopt(di->curle, CURLOPT_RESUME_FROM, (long) di->startpos);
				qcurl_easy_setopt(di->curle, CURLOPT_FOLLOWLOCATION, 1);
				qcurl_easy_setopt(di->curle, CURLOPT_WRITEFUNCTION, CURL_fwrite);
				qcurl_easy_setopt(di->curle, CURLOPT_LOW_SPEED_LIMIT, (long) 256);
				qcurl_easy_setopt(di->curle, CURLOPT_LOW_SPEED_TIME, (long) 45);
				qcurl_easy_setopt(di->curle, CURLOPT_WRITEDATA, (void *) di);
				qcurl_easy_setopt(di->curle, CURLOPT_PRIVATE, (void *) di);
				qcurl_easy_setopt(di->curle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS | CURLPROTO_FTP);
				if(qcurl_easy_setopt(di->curle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS | CURLPROTO_FTP) != CURLE_OK)
				{
					Con_Printf("^1WARNING:^7 for security reasons, please upgrade to libcurl 7.19.4 or above. In a later version of DarkPlaces, HTTP redirect support will be disabled for this libcurl version.\n");
					//qcurl_easy_setopt(di->curle, CURLOPT_FOLLOWLOCATION, 0);
				}
				if(di->post_content_type)
				{
					qcurl_easy_setopt(di->curle, CURLOPT_POST, 1);
					qcurl_easy_setopt(di->curle, CURLOPT_POSTFIELDS, di->postbuf);
					qcurl_easy_setopt(di->curle, CURLOPT_POSTFIELDSIZE, di->postbufsize);
					di->slist = qcurl_slist_append(di->slist, va(vabuf, sizeof(vabuf), "Content-Type: %s", di->post_content_type));
				}

				// parse extra headers into slist
				// \n separated list!
				h = di->extraheaders;
				while(h)
				{
					const char *hh = strchr(h, '\n');
					if(hh)
					{
						char *buf = (char *) Mem_Alloc(tempmempool, hh - h + 1);
						memcpy(buf, h, hh - h);
						buf[hh - h] = 0;
						di->slist = qcurl_slist_append(di->slist, buf);
						h = hh + 1;
					}
					else
					{
						di->slist = qcurl_slist_append(di->slist, h);
						h = NULL;
					}
				}

				qcurl_easy_setopt(di->curle, CURLOPT_HTTPHEADER, di->slist);
				
				qcurl_multi_add_handle(curlm, di->curle);
				di->started = true;
				++numdownloads;
				if(numdownloads >= curl_maxdownloads.integer)
					break;
			}
		}
	}
}

/*
====================
Curl_Init

this function MUST be called before using anything else in this file.
On Win32, this must be called AFTER WSAStartup has been done!
====================
*/
void Curl_Init(void)
{
	CURL_OpenLibrary();
	if(!curl_dll)
		return;
	if (Thread_HasThreads()) curl_mutex = Thread_CreateMutex();
	qcurl_global_init(CURL_GLOBAL_SSL);
	curlm = qcurl_multi_init();
}

/*
====================
Curl_Shutdown

Surprise... closes all the stuff. Please do this BEFORE shutting down LHNET.
====================
*/
void Curl_ClearRequirements(void);
void Curl_Shutdown(void)
{
	if(!curl_dll)
		return;
	Curl_ClearRequirements();
	Curl_CancelAll();
	if (curl_mutex) Thread_DestroyMutex(curl_mutex);
	CURL_CloseLibrary();
	curl_dll = NULL;
}

// for VM_checkextension()
qbool Curl_Available(void)
{
	return curl_dll ? true : false;
}

/*
====================
Curl_Find

Finds the internal information block for a download given by file name.
====================
*/
static downloadinfo *Curl_Find(const char *filename)
{
	downloadinfo *di;
	if(!curl_dll)
		return NULL;
	List_For_Each_Entry(di, &downloads, downloadinfo, list)
		if(!strcasecmp(di->filename, filename))
			return di;
	return NULL;
}

void Curl_Cancel_ToMemory(curl_callback_t callback, void *cbdata)
{
	downloadinfo *di, *ndi;
	if(!curl_dll)
		return;
	List_For_Each_Entry_Safe(di, ndi, &downloads, downloadinfo, list)
	{
		if(di->callback == callback && di->callback_data == cbdata)
		{
			di->callback = curl_quiet_callback; // do NOT call the callback
			Curl_EndDownload(di, CURL_DOWNLOAD_ABORTED, CURLE_OK, NULL);
		}
	}
}

/*
====================
Curl_Begin

Starts a download of a given URL to the file name portion of this URL (or name
if given) in the "dlcache/" folder.
====================
*/
static qbool Curl_Begin(const char *URL, const char *extraheaders, double maxspeed, const char *name, int loadtype, qbool forthismap, const char *post_content_type, const unsigned char *postbuf, size_t postbufsize, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata)
{
	if(buf)
		if(loadtype != LOADTYPE_NONE)
			Host_Error("Curl_Begin: loadtype and buffer are both set");

	if(!curl_dll || !curl_enabled.integer)
	{
		return false;
	}
	else
	{
		char fn[MAX_OSPATH];
		char urlbuf[1024];
		const char *p, *q;
		size_t length;
		downloadinfo *di;

		// if URL is protocol:///* or protocol://:port/*, insert the IP of the current server
		p = strchr(URL, ':');
		if(p)
		{
			if(!strncmp(p, ":///", 4) || !strncmp(p, "://:", 4))
			{
				char addressstring[128];
				*addressstring = 0;
				InfoString_GetValue(cls.userinfo, "*ip", addressstring, sizeof(addressstring));
				q = strchr(addressstring, ':');
				if(!q)
					q = addressstring + strlen(addressstring);
				if(*addressstring)
				{
					dpsnprintf(urlbuf, sizeof(urlbuf), "%.*s://%.*s%s", (int) (p - URL), URL, (int) (q - addressstring), addressstring, URL + (p - URL) + 3);
					URL = urlbuf;
				}
			}
		}

		// Note: This extraction of the file name portion is NOT entirely correct.
		//
		// It does the following:
		//
		//   http://host/some/script.cgi/SomeFile.pk3?uid=ABCDE -> SomeFile.pk3
		//   http://host/some/script.php?uid=ABCDE&file=/SomeFile.pk3 -> SomeFile.pk3
		//   http://host/some/script.php?uid=ABCDE&file=SomeFile.pk3 -> script.php
		//
		// However, I'd like to keep this "buggy" behavior so that PHP script
		// authors can write download scripts without having to enable
		// AcceptPathInfo on Apache. They just have to ensure that their script
		// can be called with such a "fake" path name like
		// http://host/some/script.php?uid=ABCDE&file=/SomeFile.pk3
		//
		// By the way, such PHP scripts should either send the file or a
		// "Location:" redirect; PHP code example:
		//
		//   header("Location: http://www.example.com/");
		//
		// By the way, this will set User-Agent to something like
		// "Nexuiz build 22:27:55 Mar 17 2006" (engineversion) and Referer to
		// dp://serverhost:serverport/ so you can filter on this; an example
		// httpd log file line might be:
		//
		//   141.2.16.3 - - [17/Mar/2006:22:32:43 +0100] "GET /maps/tznex07.pk3 HTTP/1.1" 200 1077455 "dp://141.2.16.7:26000/" "Nexuiz Linux 22:07:43 Mar 17 2006"

		if (curl_mutex) Thread_LockMutex(curl_mutex);

		if(buf)
		{
			if(!name)
				name = CleanURL(URL, urlbuf, sizeof(urlbuf));
		}
		else
		{
			if(!name)
			{
				name = CleanURL(URL, urlbuf, sizeof(urlbuf));
				p = strrchr(name, '/');
				p = p ? (p+1) : name;
				q = strchr(p, '?');
				length = q ? (size_t)(q - p) : strlen(p);
				dpsnprintf(fn, sizeof(fn), "dlcache/%.*s", (int)length, p);
			}
			else
			{
				dpsnprintf(fn, sizeof(fn), "dlcache/%s", name);
			}

			name = fn; // make it point back

			// already downloading the file?
			{
				downloadinfo *existingdownloadinfo = Curl_Find(fn);
				if(existingdownloadinfo)
				{
					Con_Printf("Can't download %s, already getting it from %s!\n", fn, CleanURL(existingdownloadinfo->url, urlbuf, sizeof(urlbuf)));

					// however, if it was not for this map yet...
					if(forthismap && !existingdownloadinfo->forthismap)
					{
						existingdownloadinfo->forthismap = true;
						// this "fakes" a download attempt so the client will wait for
						// the download to finish and then reconnect
						++numdownloads_added;
					}

					if (curl_mutex) Thread_UnlockMutex(curl_mutex);
					return false;
				}
			}

			if(FS_FileExists(fn))
			{
				if(loadtype == LOADTYPE_PAK)
				{
					qbool already_loaded;
					if(FS_AddPack(fn, &already_loaded, true, true))
					{
						Con_DPrintf("%s already exists, not downloading!\n", fn);
						if(already_loaded)
							Con_DPrintf("(pak was already loaded)\n");
						else
						{
							if(forthismap)
							{
								++numdownloads_added;
								++numdownloads_success;
							}
						}

						if (curl_mutex) Thread_UnlockMutex(curl_mutex);
						return false;
					}
					else
					{
						qfile_t *f = FS_OpenRealFile(fn, "rb", false);
						if(f)
						{
							char b[4] = {0};
							FS_Read(f, b, sizeof(b)); // no "-1", I will use memcmp

							if(memcmp(b, "PK\x03\x04", 4) && memcmp(b, "PACK", 4))
							{
								Con_DPrintf("Detected non-PAK %s, clearing and NOT resuming.\n", fn);
								FS_Close(f);
								f = FS_OpenRealFile(fn, "wb", false);
								if(f)
									FS_Close(f);
							}
							else
							{
								// OK
								FS_Close(f);
							}
						}
					}
				}
				else
				{
					// never resume these
					qfile_t *f = FS_OpenRealFile(fn, "wb", false);
					if(f)
						FS_Close(f);
				}
			}
		}

		// if we get here, we actually want to download... so first verify the
		// URL scheme (so one can't read local files using file://)
		if(strncmp(URL, "http://", 7) && strncmp(URL, "ftp://", 6) && strncmp(URL, "https://", 8))
		{
			Con_Printf("Curl_Begin(\"%s\"): nasty URL scheme rejected\n", URL);
			if (curl_mutex) Thread_UnlockMutex(curl_mutex);
			return false;
		}

		if(forthismap)
			++numdownloads_added;
		di = (downloadinfo *) Z_Malloc(sizeof(*di));
		dp_strlcpy(di->filename, name, sizeof(di->filename));
		dp_strlcpy(di->url, URL, sizeof(di->url));
		dpsnprintf(di->referer, sizeof(di->referer), "dp://%s/", cls.netcon ? cls.netcon->address : "notconnected.invalid");
		di->forthismap = forthismap;
		di->stream = NULL;
		di->startpos = 0;
		di->curle = NULL;
		di->started = false;
		di->loadtype = loadtype;
		di->maxspeed = maxspeed;
		di->bytes_received = 0;
		di->bytes_received_curl = 0;
		di->bytes_sent_curl = 0;
		di->extraheaders = extraheaders;
		di->buffer = buf;
		di->buffersize = bufsize;
		if(callback == NULL)
		{
			di->callback = curl_default_callback;
			di->callback_data = di;
		}
		else
		{
			di->callback = callback;
			di->callback_data = cbdata;
		}

		if(post_content_type)
		{
			di->post_content_type = post_content_type;
			di->postbuf = postbuf;
			di->postbufsize = postbufsize;
		}
		else
		{
			di->post_content_type = NULL;
			di->postbuf = NULL;
			di->postbufsize = 0;
		}

		List_Add(&di->list, &downloads);

		if (curl_mutex)
			Thread_UnlockMutex(curl_mutex);

		return true;
	}
}

qbool Curl_Begin_ToFile(const char *URL, double maxspeed, const char *name, int loadtype, qbool forthismap)
{
	return Curl_Begin(URL, NULL, maxspeed, name, loadtype, forthismap, NULL, NULL, 0, NULL, 0, NULL, NULL);
}
qbool Curl_Begin_ToMemory(const char *URL, double maxspeed, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata)
{
	return Curl_Begin(URL, NULL, maxspeed, NULL, false, false, NULL, NULL, 0, buf, bufsize, callback, cbdata);
}
qbool Curl_Begin_ToMemory_POST(const char *URL, const char *extraheaders, double maxspeed, const char *post_content_type, const unsigned char *postbuf, size_t postbufsize, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata)
{
	return Curl_Begin(URL, extraheaders, maxspeed, NULL, false, false, post_content_type, postbuf, postbufsize, buf, bufsize, callback, cbdata);
}

/*
====================
Curl_Frame

call this regularily as this will always download as much as possible without
blocking.
====================
*/
void Curl_Frame(void)
{
	double maxspeed;
	downloadinfo *di;

	noclear = false;

	if(!curl_enabled.integer && cls.state != ca_dedicated)
		return;

	if(!curl_dll)
		return;

	if (curl_mutex) Thread_LockMutex(curl_mutex);

	Curl_CheckCommandWhenDone();

	if(List_Is_Empty(&downloads))
	{
		if (curl_mutex) Thread_UnlockMutex(curl_mutex);
		return;
	}

	if(host.realtime < curltime) // throttle
	{
		if (curl_mutex) Thread_UnlockMutex(curl_mutex);
		return;
	}

	{
		int remaining;
		CURLMcode mc;

		do
		{
			mc = qcurl_multi_perform(curlm, &remaining);
		}
		while(mc == CURLM_CALL_MULTI_PERFORM);

		List_For_Each_Entry(di, &downloads, downloadinfo, list)
		{
			double b = 0;
			if(di->curle)
			{
				qcurl_easy_getinfo(di->curle, CURLINFO_SIZE_UPLOAD, &b);
				bytes_sent += (b - di->bytes_sent_curl);
				di->bytes_sent_curl = b;
				qcurl_easy_getinfo(di->curle, CURLINFO_SIZE_DOWNLOAD, &b);
				bytes_sent += (b - di->bytes_received_curl);
				di->bytes_received_curl = b;
			}
		}

		for(;;)
		{
			CURLMsg *msg = qcurl_multi_info_read(curlm, &remaining);
			if(!msg)
				break;
			if(msg->msg == CURLMSG_DONE)
			{
				const char *ct = NULL;
				CurlStatus failed = CURL_DOWNLOAD_SUCCESS;
				CURLcode result;
				qcurl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &di);
				result = msg->data.result;
				if(result)
				{
					failed = CURL_DOWNLOAD_FAILED;
				}
				else
				{
					long code;
					qcurl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);
					switch(code / 100)
					{
						case 4: // e.g. 404?
						case 5: // e.g. 500?
							failed = CURL_DOWNLOAD_SERVERERROR;
							result = (CURLcode) code;
							break;
					}
					qcurl_easy_getinfo(msg->easy_handle, CURLINFO_CONTENT_TYPE, &ct);
				}

				Curl_EndDownload(di, failed, result, ct);
			}
		}
	}

	CheckPendingDownloads();

	// when will we curl the next time?
	// we will wait a bit to ensure our download rate is kept.
	// we now know that realtime >= curltime... so set up a new curltime

	// use the slowest allowing download to derive the maxspeed... this CAN
	// be done better, but maybe later
	maxspeed = curl_maxspeed.value;
	List_For_Each_Entry(di, &downloads, downloadinfo, list)
		if(di->maxspeed > 0)
			if(di->maxspeed < maxspeed || maxspeed <= 0)
				maxspeed = di->maxspeed;

	if(maxspeed > 0)
	{
		double bytes = bytes_sent + bytes_received; // maybe smoothen a bit?
		curltime = host.realtime + bytes / (maxspeed * 1024.0);
		bytes_sent = 0;
		bytes_received = 0;
	}
	else
		curltime = host.realtime;

	if (curl_mutex) Thread_UnlockMutex(curl_mutex);
}

/*
====================
Curl_Select

Sleeps until there's some transfer progress or a timeout is reached,
unfortunately the timeout is only in milliseconds.
This allows good throughput even at very low FPS.
Less important on newer libcurl versions but still helps.

Returns 0 immediately if there's no transfers to wait for,
or > 0 if a transfer is ready or the timeout was reached.
====================
*/
bool Curl_Select(int timeout_ms)
{
	CURLMcode err;

	if (List_Is_Empty(&downloads))
		return false;

	err = qcurl_multi_wait(curlm, NULL, 0, timeout_ms, NULL);
	if (err == CURLM_OK)
		return true;
	Con_Printf(CON_ERROR "curl_multi_wait() failed with code %d\n", err);
	return false;
}

/*
====================
Curl_CancelAll

Stops ALL downloads.
====================
*/
void Curl_CancelAll(void)
{
	if(!curl_dll)
		return;

	if (curl_mutex) Thread_LockMutex(curl_mutex);

	while(!List_Is_Empty(&downloads))
	{
		Curl_EndDownload(List_First_Entry(&downloads, downloadinfo, list), CURL_DOWNLOAD_ABORTED, CURLE_OK, NULL);
		// INVARIANT: downloads will point to the next download after that!
	}

	if (curl_mutex) Thread_UnlockMutex(curl_mutex);
}

/*
====================
Curl_Running

returns true if there is a download running.
====================
*/
qbool Curl_Running(void)
{
	if(!curl_dll)
		return false;

	return !List_Is_Empty(&downloads);
}

/*
====================
Curl_GetDownloadAmount

returns a value from 0.0 to 1.0 which represents the downloaded amount of data
for the given download.
====================
*/
static double Curl_GetDownloadAmount(downloadinfo *di)
{
	if(!curl_dll)
		return -2;
	if(di->curle)
	{
		double length;
		qcurl_easy_getinfo(di->curle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length);
		if(length > 0)
			return (di->startpos + di->bytes_received) / (di->startpos + length);
		else
			return 0;
	}
	else
		return -1;
}

/*
====================
Curl_GetDownloadSpeed

returns the speed of the given download in bytes per second
====================
*/
static double Curl_GetDownloadSpeed(downloadinfo *di)
{
	if(!curl_dll)
		return -2;
	if(di->curle)
	{
		double speed;
		qcurl_easy_getinfo(di->curle, CURLINFO_SPEED_DOWNLOAD, &speed);
		return speed;
	}
	else
		return -1;
}

/*
====================
Curl_Info_f

prints the download list
====================
*/
// TODO rewrite using Curl_GetDownloadInfo?
static void Curl_Info_f(cmd_state_t *cmd)
{
	downloadinfo *di;
	char urlbuf[1024];
	if(!curl_dll)
		return;
	if(Curl_Running())
	{
		if (curl_mutex) Thread_LockMutex(curl_mutex);
		Con_Print("Currently running downloads:\n");
		List_For_Each_Entry(di, &downloads, downloadinfo, list)
		{
			double speed, percent;
			Con_Printf("  %s -> %s ",  CleanURL(di->url, urlbuf, sizeof(urlbuf)), di->filename);
			percent = 100.0 * Curl_GetDownloadAmount(di);
			speed = Curl_GetDownloadSpeed(di);
			if(percent >= 0)
				Con_Printf("(%.1f%% @ %.1f KiB/s)\n", percent, speed / 1024.0);
			else
				Con_Print("(queued)\n");
		}
		if (curl_mutex) Thread_UnlockMutex(curl_mutex);
	}
	else
	{
		Con_Print("No downloads running.\n");
	}
}

/*
====================
Curl_Curl_f

implements the "curl" console command

curl --info
curl --cancel
curl --cancel filename
curl url

For internal use:

curl [--pak] [--forthismap] [--for filename filename...] url
	--pak: after downloading, load the package into the virtual file system
	--for filename...: only download of at least one of the named files is missing
	--forthismap: don't reconnect on failure

curl --clear_autodownload
	clears the download success/failure counters

curl --finish_autodownload
	if at least one download has been started, disconnect and drop to the menu
	once the last download completes successfully, reconnect to the current server
====================
*/
static void Curl_Curl_f(cmd_state_t *cmd)
{
	double maxspeed = 0;
	int i;
	int end;
	int loadtype = LOADTYPE_NONE;
	qbool forthismap = false;
	const char *url;
	const char *name = 0;

	if(!curl_dll)
	{
		Con_Print("libcurl DLL not found, this command is inactive.\n");
		return;
	}

	if(!curl_enabled.integer)
	{
		Con_Print("curl support not enabled. Set cl_curl_enabled to 1 to enable.\n");
		return;
	}

	if(Cmd_Argc(cmd) < 2)
	{
		Con_Print("usage:\ncurl --info, curl --cancel [filename], curl url\n");
		return;
	}

	url = Cmd_Argv(cmd, Cmd_Argc(cmd) - 1);
	end = Cmd_Argc(cmd);

	for(i = 1; i != end; ++i)
	{
		const char *a = Cmd_Argv(cmd, i);
		if(!strcmp(a, "--info"))
		{
			Curl_Info_f(cmd);
			return;
		}
		else if(!strcmp(a, "--cancel"))
		{
			if(i == end - 1) // last argument
				Curl_CancelAll();
			else
			{
				downloadinfo *di = Curl_Find(url);
				if(di)
					Curl_EndDownload(di, CURL_DOWNLOAD_ABORTED, CURLE_OK, NULL);
				else
					Con_Print("download not found\n");
			}
			return;
		}
		else if(!strcmp(a, "--pak"))
		{
			loadtype = LOADTYPE_PAK;
		}
		else if(!strcmp(a, "--cachepic"))
		{
			loadtype = LOADTYPE_CACHEPIC;
		}
		else if(!strcmp(a, "--skinframe"))
		{
			loadtype = LOADTYPE_SKINFRAME;
		}
		else if(!strcmp(a, "--for")) // must be last option
		{
			for(i = i + 1; i != end - 1; ++i)
			{
				if(!FS_FileExists(Cmd_Argv(cmd, i)))
					goto needthefile; // why can't I have a "double break"?
			}
			// if we get here, we have all the files...
			return;
		}
		else if(!strcmp(a, "--forthismap"))
		{
			forthismap = true;
		}
		else if(!strcmp(a, "--as"))
		{
			if(i < end - 1)
			{
				++i;
				name = Cmd_Argv(cmd, i);
			}
		}
		else if(!strcmp(a, "--clear_autodownload"))
		{
			// mark all running downloads as "not for this map", so if they
			// fail, it does not matter
			Curl_Clear_forthismap();
			return;
		}
		else if(!strcmp(a, "--finish_autodownload"))
		{
			if(numdownloads_added)
			{
				char donecommand[256];
				if(cls.netcon)
				{
					if(cl.loadbegun) // curling won't inhibit loading the map any more when at this stage, so bail out and force a reconnect
					{
						dpsnprintf(donecommand, sizeof(donecommand), "connect %s", cls.netcon->address);
						Curl_CommandWhenDone(donecommand);
						noclear = true;
						CL_Disconnect();
						noclear = false;
						Curl_CheckCommandWhenDone();
					}
					else
						Curl_Register_predownload();
				}
			}
			return;
		}
		else if(!strncmp(a, "--maxspeed=", 11))
		{
			maxspeed = atof(a + 11);
		}
		else if(*a == '-')
		{
			Con_Printf("curl: invalid option %s\n", a);
			// but we ignore the option
		}
	}

needthefile:
	Curl_Begin_ToFile(url, maxspeed, name, loadtype, forthismap);
}

/*
static void curl_curlcat_callback(int code, size_t length_received, unsigned char *buffer, void *cbdata)
{
	Con_Printf("Received %d bytes (status %d):\n%.*s\n", (int) length_received, code, (int) length_received, buffer);
	Z_Free(buffer);
}

void Curl_CurlCat_f(cmd_state_t *cmd)
{
	unsigned char *buf;
	const char *url = Cmd_Argv(cmd, 1);
	buf = Z_Malloc(16384);
	Curl_Begin_ToMemory(url, buf, 16384, curl_curlcat_callback, NULL);
}
*/

/*
====================
Curl_Init_Commands

loads the commands and cvars this library uses
====================
*/
void Curl_Init_Commands(void)
{
	Cvar_RegisterVariable (&curl_enabled);
	Cvar_RegisterVariable (&curl_maxdownloads);
	Cvar_RegisterVariable (&curl_maxspeed);
	Cvar_RegisterVariable (&curl_useragent);
	Cvar_RegisterVariable (&curl_useragent_append);
	Cvar_RegisterVirtual  (&curl_enabled,          "cl_curl_enabled");
	Cvar_RegisterVirtual  (&curl_maxdownloads,     "cl_curl_maxdownloads");
	Cvar_RegisterVirtual  (&curl_maxspeed,         "cl_curl_maxspeed");
	Cvar_RegisterVirtual  (&curl_useragent,        "cl_curl_useragent");
	Cvar_RegisterVirtual  (&curl_useragent_append, "cl_curl_useragent_append");

	Cvar_RegisterVariable (&sv_curl_defaulturl);
	Cvar_RegisterVariable (&sv_curl_serverpackages);
	Cvar_RegisterVariable (&sv_curl_maxspeed);

	Cvar_RegisterVariable (&developer_curl);

	Cmd_AddCommand(CF_CLIENT | CF_CLIENT_FROM_SERVER, "curl", Curl_Curl_f, "download data from an URL and add to search path");
	//Cmd_AddCommand(cmd_local, "curlcat", Curl_CurlCat_f, "display data from an URL (debugging command)");
}

/*
====================
Curl_GetDownloadInfo

returns an array of Curl_downloadinfo_t structs for usage by GUIs.
The number of elements in the array is returned in int *nDownloads.
const char **additional_info may be set to a string of additional user
information, or to NULL if no such display shall occur. The returned
array must be freed later using Z_Free.
====================
*/
Curl_downloadinfo_t *Curl_GetDownloadInfo(int *nDownloads, const char **additional_info, char *addinfo, size_t addinfolength)
{
	int i;
	downloadinfo *di;
	Curl_downloadinfo_t *downinfo;

	if(!curl_dll)
	{
		*nDownloads = 0;
		if(additional_info)
			*additional_info = NULL;
		return NULL;
	}

	if (curl_mutex) Thread_LockMutex(curl_mutex);

	i = 0;
	List_For_Each_Entry(di, &downloads, downloadinfo, list)
		++i;

	downinfo = (Curl_downloadinfo_t *) Z_Malloc(sizeof(*downinfo) * i);
	i = 0;
	List_For_Each_Entry(di, &downloads, downloadinfo, list)
	{
		// do not show infobars for background downloads
		if(developer.integer <= 0)
			if(di->buffer)
				continue;
		dp_strlcpy(downinfo[i].filename, di->filename, sizeof(downinfo[i].filename));
		if(di->curle)
		{
			downinfo[i].progress = Curl_GetDownloadAmount(di);
			downinfo[i].speed = Curl_GetDownloadSpeed(di);
			downinfo[i].queued = false;
		}
		else
		{
			downinfo[i].queued = true;
		}
		++i;
	}

	if(additional_info)
	{
		// TODO: can I clear command_when_done as soon as the first download fails?
		if(*command_when_done && !numdownloads_fail && numdownloads_added)
		{
			if(!strncmp(command_when_done, "connect ", 8))
				dpsnprintf(addinfo, addinfolength, "(will join %s when done)", command_when_done + 8);
			else if(!strcmp(command_when_done, "cl_begindownloads"))
				dpsnprintf(addinfo, addinfolength, "(will enter the game when done)");
			else
				dpsnprintf(addinfo, addinfolength, "(will do '%s' when done)", command_when_done);
			*additional_info = addinfo;
		}
		else
			*additional_info = NULL;
	}

	*nDownloads = i;
	if (curl_mutex) Thread_UnlockMutex(curl_mutex);
	return downinfo;
}


/*
====================
Curl_FindPackURL

finds the URL where to find a given package.

For this, it reads a file "curl_urls.txt" of the following format:

	data*.pk3	-
	revdm*.pk3	http://revdm/downloads/are/here/
	*			http://any/other/stuff/is/here/

The URLs should end in /. If not, downloads will still work, but the cached files
can't be just put into the data directory with the same download configuration
(you might want to do this if you want to tag downloaded files from your
server, but you should not). "-" means "don't download".

If no single pattern matched, the cvar sv_curl_defaulturl is used as download
location instead.

Note: pak1.pak and data*.pk3 are excluded from autodownload at another point in
this file for obvious reasons.
====================
*/
static const char *Curl_FindPackURL(const char *filename)
{
	static char foundurl[1024]; // invoked only by server
	fs_offset_t filesize;
	char *buf = (char *) FS_LoadFile("curl_urls.txt", tempmempool, true, &filesize);
	if(buf && filesize)
	{
		// read lines of format "pattern url"
		char *p = buf;
		char *pattern = NULL, *patternend = NULL, *url = NULL, *urlend = NULL;
		qbool eof = false;

		pattern = p;
		while(!eof)
		{
			switch(*p)
			{
				case 0:
					eof = true;
					// fallthrough
				case '\n':
				case '\r':
					if(pattern && url && patternend)
					{
						if(!urlend)
							urlend = p;
						*patternend = 0;
						*urlend = 0;
						if(matchpattern(filename, pattern, true))
						{
							dp_strlcpy(foundurl, url, sizeof(foundurl));
							Z_Free(buf);
							return foundurl;
						}
					}
					pattern = NULL;
					patternend = NULL;
					url = NULL;
					urlend = NULL;
					break;
				case ' ':
				case '\t':
					if(pattern && !patternend)
						patternend = p;
					else if(url && !urlend)
						urlend = p;
					break;
				default:
					if(!pattern)
						pattern = p;
					else if(pattern && patternend && !url)
						url = p;
					break;
			}
			++p;
		}
	}
	if(buf)
		Z_Free(buf);
	return sv_curl_defaulturl.string;
}

typedef struct requirement_s
{
	struct requirement_s *next;
	char filename[MAX_OSPATH];
}
requirement;
static requirement *requirements = NULL;


/*
====================
Curl_RequireFile

Adds the given file to the list of requirements.
====================
*/
void Curl_RequireFile(const char *filename)
{
	requirement *req = (requirement *) Z_Malloc(sizeof(*requirements));
	req->next = requirements;
	dp_strlcpy(req->filename, filename, sizeof(req->filename));
	requirements = req;
}

/*
====================
Curl_ClearRequirements

Clears the list of required files for playing on the current map.
This should be called at every map change.
====================
*/
void Curl_ClearRequirements(void)
{
	while(requirements)
	{
		requirement *req = requirements;
		requirements = requirements->next;
		Z_Free(req);
	}
}

/*
====================
Curl_SendRequirements

Makes the current host_clients download all files he needs.
This is done by sending him the following console commands:

	curl --clear_autodownload
	curl --pak --for maps/pushmoddm1.bsp --forthismap http://where/this/darn/map/is/pushmoddm1.pk3
	curl --finish_autodownload
====================
*/
static qbool Curl_SendRequirement(const char *filename, qbool foundone, char *sendbuffer, size_t sendbuffer_len)
{
	const char *p;
	const char *thispack = FS_WhichPack(filename);
	const char *packurl;

	if(!thispack || !*thispack)
		return false;

	p = strrchr(thispack, '/');
	if(p)
		thispack = p + 1;

	packurl = Curl_FindPackURL(thispack);

	if(packurl && *packurl && strcmp(packurl, "-"))
	{
		if(!foundone)
			dp_strlcat(sendbuffer, "curl --clear_autodownload\n", sendbuffer_len);

		dp_strlcat(sendbuffer, "curl --pak --forthismap --as ", sendbuffer_len);
		dp_strlcat(sendbuffer, thispack, sendbuffer_len);
		if(sv_curl_maxspeed.value > 0)
			dpsnprintf(sendbuffer + strlen(sendbuffer), sendbuffer_len - strlen(sendbuffer), " --maxspeed=%.1f", sv_curl_maxspeed.value);
		dp_strlcat(sendbuffer, " --for ", sendbuffer_len);
		dp_strlcat(sendbuffer, filename, sendbuffer_len);
		dp_strlcat(sendbuffer, " ", sendbuffer_len);
		dp_strlcat(sendbuffer, packurl, sendbuffer_len);
		dp_strlcat(sendbuffer, thispack, sendbuffer_len);
		dp_strlcat(sendbuffer, "\n", sendbuffer_len);

		return true;
	}

	return false;
}
void Curl_SendRequirements(void)
{
	// for each requirement, find the pack name
	char sendbuffer[4096] = "";
	requirement *req;
	qbool foundone = false;
	const char *p;

	for(req = requirements; req; req = req->next)
		foundone = Curl_SendRequirement(req->filename, foundone, sendbuffer, sizeof(sendbuffer)) || foundone;

	p = sv_curl_serverpackages.string;
	while(COM_ParseToken_Simple(&p, false, false, true))
		foundone = Curl_SendRequirement(com_token, foundone, sendbuffer, sizeof(sendbuffer)) || foundone;

	if(foundone)
		dp_strlcat(sendbuffer, "curl --finish_autodownload\n", sizeof(sendbuffer));

	if(strlen(sendbuffer) + 1 < sizeof(sendbuffer))
		SV_ClientCommands("%s", sendbuffer);
	else
		Con_Printf("Could not initiate autodownload due to URL buffer overflow\n");
}
