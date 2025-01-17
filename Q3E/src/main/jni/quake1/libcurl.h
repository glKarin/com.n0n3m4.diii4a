#include <stddef.h>
#include "qtypes.h"
#include "qdefs.h"

enum
{
	CURLCBSTATUS_OK = 0,
	CURLCBSTATUS_FAILED = -1, // failed for generic reason (e.g. buffer too small)
	CURLCBSTATUS_ABORTED = -2, // aborted by curl --cancel
	CURLCBSTATUS_SERVERERROR = -3, // only used if no HTTP status code is available
	CURLCBSTATUS_UNKNOWN = -4 // should never happen
};
typedef void (*curl_callback_t) (int status, size_t length_received, unsigned char *buffer, void *cbdata);
// code is one of the CURLCBSTATUS constants, or the HTTP error code (when > 0).

void Curl_Frame(void);
bool Curl_Select(int timeout_ms);
qbool Curl_Running(void);
qbool Curl_Begin_ToFile(const char *URL, double maxspeed, const char *name, int loadtype, qbool forthismap);

qbool Curl_Begin_ToMemory(const char *URL, double maxspeed, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata);
qbool Curl_Begin_ToMemory_POST(const char *URL, const char *extraheaders, double maxspeed, const char *post_content_type, const unsigned char *postbuf, size_t postbufsize, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata);
void Curl_Cancel_ToMemory(curl_callback_t callback, void* cbdata);

void Curl_Init(void);
void Curl_Init_Commands(void);
void Curl_Shutdown(void);
qbool Curl_Available(void);
void Curl_CancelAll(void);
void Curl_Clear_forthismap(void);
qbool Curl_Have_forthismap(void);
void Curl_Register_predownload(void);

void Curl_ClearRequirements(void);
void Curl_RequireFile(const char *filename);
void Curl_SendRequirements(void);

typedef struct Curl_downloadinfo_s
{
	char filename[MAX_QPATH];
	double progress;
	double speed;
	qbool queued;
}
Curl_downloadinfo_t;
Curl_downloadinfo_t *Curl_GetDownloadInfo(int *nDownloads, const char **additional_info, char *addinfo, size_t addinfolength);
	// this may and should be Z_Free()ed
	// the result is actually an array
	// an additional info string may be returned in additional_info as a
	// pointer to a static string (but the argument may be NULL if the caller
	// does not care)
