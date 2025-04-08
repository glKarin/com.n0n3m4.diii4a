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

#ifndef _HTTP_CONNECTION_H_
#define _HTTP_CONNECTION_H_


class CHttpRequest;
typedef std::shared_ptr<CHttpRequest> CHttpRequestPtr;

/**
 * greebo: An object representing a single HttpConnection, holding 
 * proxy settings and providing error handling.
 *
 * Use the CreateRequest() method to generate a new request object.
 *
 * TDM provides a single http connection object via the gameLocal class:
 *
 * gameLocal.m_HttpConnection->CreateRequest("http://www.thedarkmod.com");
 *
 * Note: the m_HttpConnection object can be NULL if HTTP requests have been
 * disabled by the user.
 */
class CHttpConnection
{
	friend class idGameLocal;

private:
	CHttpConnection();

public:
	~CHttpConnection();

	bool HasProxy();

	idStr GetProxyHost();
	idStr GetProxyUsername();
	idStr GetProxyPassword();

	/**
	 * Constructs a new HTTP request using the given URL (optional: filename)
	 */ 
	CHttpRequestPtr CreateRequest(const std::string& url);
	CHttpRequestPtr CreateRequest(const std::string& url, const std::string& destFilename);
};
typedef std::shared_ptr<CHttpConnection> CHttpConnectionPtr;

#endif /* _HTTP_CONNECTION_H_ */
