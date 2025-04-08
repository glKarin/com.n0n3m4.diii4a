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

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <fstream>

class CHttpConnection;

#include <pugixml.hpp>
typedef void CURL;

// Shared_ptr typedef
typedef std::shared_ptr<pugi::xml_document> XmlDocumentPtr;

typedef struct SHA256_CTX SHA256_CTX;

/**
 * greebo: An object representing a single HttpRequest, holding 
 * the result (string) and status information.
 *
 * Use the Perform() method to execute the request.
 */
class CHttpRequest
{
public:

	enum RequestStatus
	{
		NOT_PERFORMED_YET,
		OK,	// successful
		IN_PROGRESS,
		FAILED,
		ABORTED,
	};

private:
	// The connection we're working with
	CHttpConnection* _conn;

	// The URL we're supposed to query
	std::string _url;

	std::vector<char> _buffer;

	// The curl handle
	CURL* _handle;

	// The current state
	RequestStatus _status;

	std::string _destFilename;

	std::ofstream _destStream;

	// True if we should cancel the download
	bool _cancelFlag;

	double _progress;

	bool _computeSha256 = false;
	std::unique_ptr<SHA256_CTX> _sha256state;

public:
	CHttpRequest(CHttpConnection& conn, const std::string& url);

	CHttpRequest(CHttpConnection& conn, const std::string& url, const std::string& destFilename);

	~CHttpRequest();

	void EnableSha256();

	// Callbacks for CURL
	static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, CHttpRequest* self);
	static size_t WriteFileCallback(void* ptr, size_t size, size_t nmemb, CHttpRequest* self);

	static int TDMHttpProgressFunc(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

	RequestStatus GetStatus();

	// Perform the request
	void Perform();

	void Cancel();

	// Between 0.0 and 1.0
	double GetProgressFraction();

	// Returns the result string
	std::string GetResultString();

	// Returns the result as XML document
	XmlDocumentPtr GetResultXml();

	idStr GetSha256() const;
private:
	void InitRequest();

	void UpdateProgress();
};
typedef std::shared_ptr<CHttpRequest> CHttpRequestPtr;

#endif /* _HTTP_REQUEST_H_ */
