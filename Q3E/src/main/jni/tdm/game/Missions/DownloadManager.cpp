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



#include "DownloadManager.h"

CDownloadManager::CDownloadManager() :
	_nextAvailableId(1),
	_allDownloadsDone(true)
{}

int CDownloadManager::AddDownload(const CDownloadPtr& download)
{
	int id = _nextAvailableId++;

	_downloads[id] = download;

	_allDownloadsDone = false;

	return id;
}

CDownloadPtr CDownloadManager::GetDownload(int id)
{
	Downloads::iterator found = _downloads.find(id);

	return (found != _downloads.end()) ? found->second : CDownloadPtr();
}

void CDownloadManager::ClearDownloads()
{
	_downloads.clear();
}

bool CDownloadManager::DownloadInProgress()
{
	for (Downloads::const_iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		if (i->second->GetStatus() == CDownload::IN_PROGRESS)
		{
			return true;
		}
	}

	return false;
}

void CDownloadManager::RemoveDownload(int id)
{
	Downloads::iterator found = _downloads.find(id);

	if (found != _downloads.end()) 
	{
		_downloads.erase(found);
	}
}

void CDownloadManager::ProcessDownloads()
{
	if (_allDownloadsDone || _downloads.empty()) 
	{
		return; // nothing to do
	}

	if (DownloadInProgress())
	{
		return; // download still in progress
	}

	// No download in progress, pick a new from the queue
	for (Downloads::const_iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		if (i->second->GetStatus() == CDownload::NOT_STARTED_YET)
		{
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Starting download: %i", i->first);

			i->second->Start();

			// Check if this download has a related one, if yes, launch both at once
			int relatedId = i->second->GetRelatedDownloadId();

			if (relatedId != -1)
			{
				CDownloadPtr related = GetDownload(relatedId);

				if (related)
				{
					DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Starting related download: %i", relatedId);

					related->Start();
				}
			}

			return;
		}
	}

	// No download left to handle
	_allDownloadsDone = true;
}
