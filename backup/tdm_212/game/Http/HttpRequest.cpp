/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "HttpRequest.h"
#include "HttpConnection.h"

#ifdef WIN32
#pragma warning(disable: 4800) // stgatilov: suppress "forcing value to bool" warning in WinSDK

#include <winsock2.h> // greebo: need to include winsock2 before curl/curl.h
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#endif

#include "curl/curl.h"
#define ExtLibs
#include "hashing/sha256.h"


CHttpRequest::CHttpRequest(CHttpConnection& conn, const std::string& url) :
	_conn(&conn),
	_url(url),
	_handle(NULL),
	_status(NOT_PERFORMED_YET),
	_cancelFlag(false),
	_progress(0)
{}

CHttpRequest::CHttpRequest(CHttpConnection& conn, const std::string& url, const std::string& destFilename) :
	_conn(&conn),
	_url(url),
	_handle(NULL),
	_status(NOT_PERFORMED_YET),
	_destFilename(destFilename),
	_cancelFlag(false),
	_progress(0)
{}

CHttpRequest::~CHttpRequest() {}

void CHttpRequest::EnableSha256()
{
	_computeSha256 = true;
}

idStr CHttpRequest::GetSha256() const 
{
	if (!_sha256state)
		return "";
	uint8_t digest[32];
	sha256_final(_sha256state.get(), digest);
	idStr res;
	for (int i = 0; i < 32; i++) {
		char tmp[8];
		idStr::snPrintf(tmp, sizeof(tmp), "%02x", digest[i]);
		res += tmp;
	}
	return res;
}

// Agent Jones #3766
int CHttpRequest::TDMHttpProgressFunc(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    bool IsTDMRunning = common->WindowAvailable();
    
	if (IsTDMRunning == false || static_cast<CHttpRequest*>(clientp)->_cancelFlag)
	{
        return 1; // Cancel the request
    }
    return 0;
}

void CHttpRequest::InitRequest()
{
	// Init the curl session
	_handle = ExtLibs::curl_easy_init();

	// specify URL to get
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_URL, _url.c_str() );

	// Connect the callback
	if (!_destFilename.empty())
	{
		ExtLibs::curl_easy_setopt( _handle, CURLOPT_WRITEFUNCTION, CHttpRequest::WriteFileCallback );
	}
	else
	{
		ExtLibs::curl_easy_setopt( _handle, CURLOPT_WRITEFUNCTION, CHttpRequest::WriteMemoryCallback );
	}

	// We pass ourselves as user data pointer to the callback function
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_WRITEDATA, this );

	// stgatilov: return CURLE_HTTP_RETURNED_ERROR if file is missing on server (any HTTP code >= 400)
	curl_easy_setopt(_handle, CURLOPT_FAILONERROR, 1);

	// Set agent
	idStr agent = "The Dark Mod Agent/";
	agent += va("%d.%02d", TDM_VERSION_MAJOR, TDM_VERSION_MINOR);

#ifdef WIN32
	agent += " Win32";
#elif defined(__linux__)
	agent += " Linux";
#elif defined(MACOS_X)
	agent += " MacOSX";
#endif

	ExtLibs::curl_easy_setopt( _handle, CURLOPT_USERAGENT, agent.c_str() );

	// Tels: #3261: only allow FTP, FTPS, HTTP and HTTPS (HTTPS and FTPS need SSL support compiled in)
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_PROTOCOLS, CURLPROTO_FTP + CURLPROTO_FTPS + CURLPROTO_HTTP + CURLPROTO_HTTPS );

	// Tels: #3261: allow redirects on the server, with a limit of 10 redirects, and limit
	// 	 the protocols to FTP, FTPS, HTTP, HTTPS to avoid rogue servers giving us random
	//	 things like Telnet or POP3 on random targets.
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_FOLLOWLOCATION, true );
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_MAXREDIRS, 10 );
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_FTP + CURLPROTO_FTPS + CURLPROTO_HTTP + CURLPROTO_HTTPS );

    // #3418: we need to provide the ca bundle
    idStr capath = g_Global.GetDarkmodPath().c_str();
    capath.AppendPath("ca-bundle.crt");
    idFile* catest = fileSystem->OpenExplicitFileRead(capath.c_str());
    if (catest == NULL)
    {
        gameLocal.Warning("HTTPS downloads: 'ca-bundle.crt' missing from Darkmod folder. HTTPS downloads will fail.");
    }
    else
    {
        delete catest;
    }
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_CAINFO, capath.c_str() );

	// Agent Jones #3766

	// The default progress meter function is not used here. Disable it.
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_NOPROGRESS, false );

	// Progress callback
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_XFERINFOFUNCTION, CHttpRequest::TDMHttpProgressFunc );
	ExtLibs::curl_easy_setopt( _handle, CURLOPT_XFERINFODATA, this );//this will become the clientp arg in TDMHttpProgressFunc
																				
	// end #3766

	// Get the proxy from the HttpConnection class
	if (_conn->HasProxy())
	{
		ExtLibs::curl_easy_setopt( _handle, CURLOPT_PROXY, _conn->GetProxyHost().c_str() );
		ExtLibs::curl_easy_setopt( _handle, CURLOPT_PROXYUSERPWD, (_conn->GetProxyUsername() + ":" + _conn->GetProxyPassword()).c_str() );
	}
}

void CHttpRequest::Perform()
{
	InitRequest();

	_progress = 0;
	_status = IN_PROGRESS;

	// Check target file
	if (!_destFilename.empty())
	{
		_destStream.open(_destFilename.c_str(), std::ofstream::out|std::ofstream::binary);
	}

	if (_computeSha256)
	{
		_sha256state.reset(new SHA256_CTX());
		sha256_init(_sha256state.get());
	}

	CURLcode result = ExtLibs::curl_easy_perform( _handle );

	if (!_destFilename.empty())
	{
		_destStream.flush();
		_destStream.close();
	}

	if (_cancelFlag)
	{
		_status = ABORTED;
	}
	else
	{
		switch (result)
		{
		case CURLE_OK:
			_status = OK;
			_progress = 1.0;
			break;
		default:
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Download from '%s' failed with curl status %i.", _url.c_str(), result);
			_status = FAILED;
		};
	}

	ExtLibs::curl_easy_cleanup( _handle );

	_handle = NULL;
}

void CHttpRequest::Cancel()
{
	// The memory callback will catch this flag
	_cancelFlag = true;
}

CHttpRequest::RequestStatus CHttpRequest::GetStatus()
{
	return _status;
}

double CHttpRequest::GetProgressFraction()
{
	return _progress;
}

std::string CHttpRequest::GetResultString()
{
	return _buffer.empty() ? "" : std::string(&_buffer.front());
}

XmlDocumentPtr CHttpRequest::GetResultXml()
{
	XmlDocumentPtr doc(new pugi::xml_document);
	
	doc->load_string(GetResultString().c_str());

	return doc;
}

void CHttpRequest::UpdateProgress()
{
	double size;
	double downloaded;
	CURLcode result = ExtLibs::curl_easy_getinfo( _handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size );

	if (result != CURLE_OK) 
	{
		_progress = 0;
		return;
	}

	result = ExtLibs::curl_easy_getinfo( _handle, CURLINFO_SIZE_DOWNLOAD, &downloaded );

	if (result != CURLE_OK) 
	{
		_progress = 0;
		return;
	}

	_progress = downloaded / size;

	if (_progress > 1.0)
	{
		_progress = 1.0;
	}
}

size_t CHttpRequest::WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, CHttpRequest* self)
{
	if (self->_cancelFlag)
	{
		return 0; // cancel the process
	}

	// Needed size
	std::size_t bytesToCopy = size * nmemb;

	std::vector<char>& buf = self->_buffer; // shortcut 

	std::size_t appendPosition = buf.size() > 0 ? buf.size() - 2 : 0;

	// The first allocation should request one extra byte for the trailing \0
	self->_buffer.resize(buf.size() > 0 ? buf.size() + bytesToCopy : bytesToCopy + 1);

	// Push the bytes
	memcpy(&(buf[appendPosition]), ptr, bytesToCopy);

	// Append trailing \0 if possible
	if (buf.size() > 0)
	{
		buf[buf.size() - 1] = 0;
	}

	self->UpdateProgress();

	if (self->_computeSha256)
	{
		sha256_update(self->_sha256state.get(), (uint8_t*)ptr, bytesToCopy);
	}

	return static_cast<size_t>(bytesToCopy);
}

size_t CHttpRequest::WriteFileCallback(void* ptr, size_t size, size_t nmemb, CHttpRequest* self)
{
	if (self->_cancelFlag)
	{
		return 0; // cancel the process
	}

	// Needed size
	std::size_t bytesToCopy = size * nmemb;

	self->_destStream.write(static_cast<const char*>(ptr), bytesToCopy);

	self->UpdateProgress();

	if (self->_computeSha256)
	{
		sha256_update(self->_sha256state.get(), (uint8_t*)ptr, bytesToCopy);
	}

	return static_cast<size_t>(bytesToCopy);
}


