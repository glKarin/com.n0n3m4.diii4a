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



#include "HttpConnection.h"
#include "HttpRequest.h"

#ifdef WIN32
#pragma warning(disable: 4800) // stgatilov: suppress "forcing value to bool" warning in WinSDK
#include <winsock2.h> // greebo: need to include winsock2 before curl/curl.h
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#endif

#include "curl/curl.h"
#define ExtLibs

CHttpConnection::CHttpConnection()
{
	ExtLibs::curl_global_init(CURL_GLOBAL_ALL);
}

CHttpConnection::~CHttpConnection()
{
	// Clean up cURL
	ExtLibs::curl_global_cleanup();
}

bool CHttpConnection::HasProxy()
{
	return idStr::Length(cv_tdm_proxy.GetString()) > 0;
}

idStr CHttpConnection::GetProxyHost()
{
	return cv_tdm_proxy.GetString();
}

idStr CHttpConnection::GetProxyUsername()
{
	return cv_tdm_proxy_user.GetString();
}

idStr CHttpConnection::GetProxyPassword()
{
	return cv_tdm_proxy_pass.GetString();
}

CHttpRequestPtr CHttpConnection::CreateRequest(const std::string& url)
{
	return CHttpRequestPtr(new CHttpRequest(*this, url));
}

CHttpRequestPtr CHttpConnection::CreateRequest(const std::string& url, const std::string& destFilename)
{
	return CHttpRequestPtr(new CHttpRequest(*this, url, destFilename));
}
