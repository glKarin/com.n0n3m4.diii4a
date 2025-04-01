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

#ifndef _MISSION_MANAGER_H_
#define _MISSION_MANAGER_H_

#include "ModInfo.h"

#include "StdFilesystem.h"
namespace fs = stdext;

// Shared_ptr typedef
#include <pugixml.hpp>
typedef std::shared_ptr<pugi::xml_document> XmlDocumentPtr;

#define MISSION_LIST_SORT_BY_TITLE    0
#define MISSION_LIST_SORT_BY_DATE     1

#define MISSION_LIST_SORT_DIRECTION_ASC     0
#define MISSION_LIST_SORT_DIRECTION_DESC    1

class CMissionDB;
typedef std::shared_ptr<CMissionDB> CMissionDBPtr;

const char* const TMP_MISSION_SCREENSHOT_PREFIX = "previewshot_";
const char* const TMP_MISSION_SCREENSHOT_FOLDER = "_missionshots"; // relative to fms/

struct MissionScreenshot
{
	// Image filename relative to the fms/<mission>/ folder
	// This will empty if the screenie hasn't been downloaded yet
	idStr	filename;

	// The server-relative URL for downloading that screenshot
	idStr	serverRelativeUrl;

	// Get the local image file name without path, e.g. "monastery01.jpg"
	idStr GetLocalFilename() const;

	// Returns the image filename including extension
	idStr GetRemoteFilename() const;

	// Returns the file extension of the server file (lowercase, without dot, e.g. "png")
	idStr GetRemoteFileExtension() const;
};
typedef std::shared_ptr<MissionScreenshot> MissionScreenshotPtr;

struct DownloadableMod
{
	enum Type
	{
		Single,				// single mission
		Multi,				// multi-mission or campaign
	};

	int		id;				// ID of the mission/mod in the online database
	idStr	modName;		// usually the name of the pk4 (e.g. "heart")
	idStr	title;			// the title or display name ("Heart of Lone Salvation")
	idStr	author;			// author/s
	float	sizeMB;			// size in MB
	Type	type;
	idStr	releaseDate;	// date of release in ISO format (2010-12-30)
	idStr	language;		// the language ("english")
	int		version;		// version number, initial release carries version 1
	bool	isUpdate;		// whether this mission is an update of one already installed

    // gnartsch:			// whether the localization pack is availlable and needs to be downloaded
	bool	needsL10NpackDownload;

	// The list of mission download URLs
	idStringList missionUrls;
	idStr missionSha256;

	// The list of Localisation pack download URLs
	idStringList l10nPackUrls;
	idStr l10nPackSha256;

	// Begin Initially empty variables, need to be filled per request by the mission manager

	// This is false by default, is set to true once the mission manager has filled in the details
	bool	detailsLoaded;

	// A single- or multi-line text, as defined in the online FM database
	idStr	description;

	// A list of screenshots as downloaded from the mission database
	idList<MissionScreenshotPtr> screenshots;

	// End Initially empty variables

	// Default constructor
	DownloadableMod();
	~DownloadableMod();

	// Static sort compare functor, sorting by mod title
	typedef DownloadableMod* DownloadableModPtr;

	static int SortCompareTitle(const DownloadableModPtr* a, const DownloadableModPtr* b);
	static int SortCompareDate(const DownloadableModPtr* a, const DownloadableModPtr* b);

	// Gets the local path to the screenshot image (relative to darkmod path, e.g. fms/_missionshots/preview_monst02.jpg)
	idStr GetLocalScreenshotPath(int screenshotNum) const;
};
// Use raw pointers in the DownloadableModList
// to allow the use of the qsort algorithm as used in idStr::Sort()
typedef idList<DownloadableMod*> DownloadableModList;

/**
 * greebo: A campaign defines a certain map sequence. In the simplest
 * setup the mapsequence has only one map file name in it, in the 
 * most complicated case each sequence step has a finite set of 
 * map files (the second mission in a campaign might consist of 
 * two actual map files the player is switching in between).
 */
struct MapSequenceElement
{
	// The list of names applicable for that sequence element
	idList<idStr> mapNames;
};
typedef idList<MapSequenceElement> MapSequence;


/**
 * greebo: The MissionManager class handles all the mod selection, 
 * mission progress, PK4 download, unpacking, installation, etc.
 *
 * It's important to distinguish between the terms "mod" and "mission".
 * A single mod (PK4) can contain more than one mission (e.g. in a campaign
 * scenario).
 *
 * Each mission of a chosen mod can itself comprise one or more maps. 
 * If a (future) mission incorporates loading zones there might be some
 * map switching going on within the same mission.
 *
 * The simplest case is a single-mission mod with the mission consisting 
 * of just a single map, like the early TDM missions "The Outpost" et al.
 *
 * Previously I've been using the term mission as synonym for the PK4
 * that got downloaded from our mission db on the web, but this is no longer
 * accurate. It's possible that some local variables or members still have
 * "mission" in their name in spite of referring to a mod. Consider that WIP.
 */
class CMissionManager
{
private:
	CMissionDBPtr _missionDB;

	// A plain list of available fs_currentfm names
	idStringList _availableMods;

	// List of available mods along with info
	idList<CModInfoPtr> _modListPrimary;

	// A list of path => path associations for moving files around
	typedef std::list< std::pair<fs::path, fs::path> > MoveList;

	// The list of new mods
	idStringList _newFoundMods;

	// The map file which should be loaded next (e.g. "patently_dangerous")
	idStr _curStartingMap;

	// The map sequence as defined in the map sequence file of campaigns
	// The first mission has index 0
	MapSequence _mapSequence;

	// The index into the _mapSequence list, defines the currently active mission
	// This value is saved and restored
	int _curMissionIndex;

	DownloadableModList _downloadableModsPrimary;

	// The ID of the "Downloading mission list from server" message
	int _refreshModListDownloadId;

	// The ID of the "get mission details" request
	int _modDetailsDownloadId;

	// The ID of the screenshot download
	int _modScreenshotDownloadId;

public:
	enum InstallResult
	{
		INSTALLED_OK,
		INDEX_OUT_OF_BOUNDS,
		COPY_FAILURE,
	};

	// Status of various requests managed by this class (download mod list, 
	enum RequestStatus
	{
		NOT_IN_PROGRESS,
		IN_PROGRESS,
		FAILED,
		SUCCESSFUL,
		MALFORMED,	//stgatilov: invalid data or checksum
	};

public:
	CMissionManager();

	~CMissionManager();

	// This initialises the list of available mods
	void Init();

	// Save/Restore data
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	// Save missionDB to hard drive right now!
	// Note: it is done automatically in destructor.
	void SaveDatabase() const;

	// Creates mod list
	void CreatePrimaryModList();

	// Get the primary mod list
	const idList<CModInfoPtr>& GetPrimaryModList() const;

	// Returns the number of available mods
	int GetNumMods();

	// Returns the mod info by index (or NULL if out of bounds)
	CModInfoPtr GetModInfo(int index);

	// returns the mod info by name (always non-NULL)
	CModInfoPtr GetModInfo(const idStr& name);

	// Returns the info structure for the currently ongoing mod (or NULL if none)
	CModInfoPtr GetCurrentModInfo();

	// Returns the name of the currently installed mod
	idStr GetCurrentModName();

	// greebo: Returns the (file)name of the current mission (there might be multiple missions 
	// in a campaign, this method returns the one that should be loaded next).
	// Example: "patently_dangerous", no file extension, no maps/ prefix.
	const idStr& GetCurrentStartingMap() const;

	// Returns the current mission index (0-based: first mission == 0)
	// Will always return 0 for non-campaigns
	int GetCurrentMissionIndex() const;

	// Set the current mission index
	void SetCurrentMissionIndex(int index);

	// Returns TRUE if the currently installed mod is a campaign
	bool CurrentModIsCampaign() const;

	// Returns TRUE if there is another mission available to play
	bool NextMissionAvailable() const;

	// greebo: Proceeds the current mission index to the next available one. 
	// Returns TRUE if that was successful, FALSE if the current mod is  
	// not a campaign or if there is no next mission available
	bool ProceedToNextMission();

	// Removes everything except for the mod PK4 and the metadata files from the mod save path
	//stgatilov: this feature is dangerous for mappers and useless for players
	//see https://forums.thedarkmod.com/index.php?/topic/21085-dark-mod-just-deleted-my-entire-fmsvenice-folder-and-eveything-in-it/
	//void CleanupModFolder(const idStr& name);

	// Called by MissionData when the player completed a mission
	void OnMissionComplete();

	// Called by gameLocal when the player start/loads a mission
	void OnMissionStart();

	// Clears the mod list and searches for PK4s to install, then calls GenerateModList()
	void ReloadModList();

	// The number of newly available mods
	int GetNumNewMods();

	idStr GetNewFoundModsText();

	void ClearNewModList();

	void AddToNewModList(const idStrList& newModsList);

	// Reload darkmod.txt for newly downloaded/found PK4s, to update any outdated mission db entries
	void RefreshMetaDataForNewFoundMods();

	// Installs mod (by index)
	InstallResult InstallMod(int index);

	// Installs mod (by fs_currentfm name)
	InstallResult InstallMod(const idStr& name);

	// Writes the given mod name to the currentfm.txt file
	bool WriteCurrentFmFile(const idStr& modName);

	// Uninstalls the currently installed mod, basically clearing our currentfm.txt
	void UninstallMod();

	// --------- Downloadable Mod List Request --------

	// Checks online for available mods, returns the download ID for progress checking
	int StartReloadDownloadableMods();

	// Returns true if the mod list download is currently in progress,
	// call ProcessReloadDownloadableMissionsRequest() to process it
	bool IsDownloadableModsRequestInProgress();

	// Processes the pending mod list download request. Returns the download status
	// for reference (FAILED, SUCCESS, etc.)
	RequestStatus ProcessReloadDownloadableModsRequest();

	// -------- Mod Details Request ----------

	// Starts a new request to download details of the given mod
	int StartDownloadingModDetails(DownloadableMod* mod);

	// Returns true if the mod details download is currently in progress,
	// call ProcessReloadModDetailsRequest() to process it
	bool IsModDetailsRequestInProgress();

	// Processes the pending mod details download request. Returns the status
	// for reference (FAILED, SUCCESS, etc.)
	RequestStatus ProcessReloadModDetailsRequest();

	// -------- Screenshot Requests -----------
	bool IsMissionScreenshotRequestInProgress();
	int StartDownloadingMissionScreenshot(DownloadableMod* mod, int screenshotNum);
	CMissionManager::RequestStatus ProcessMissionScreenshotRequest();

	// Get the primary downloadable mission list
	const DownloadableModList& GetPrimaryDownloadableMods() const;

	// Convenience method which copies a file from <source> to <dest>
	// If <overwrite> is set to TRUE, any existing destination file will be removed beforehand
	// Note: CopyFile is already #define'd in a stupid WinBase.h header file, hence DoCopyFile.
	static bool DoCopyFile(const fs::path& source, const fs::path& dest, bool overwrite = false);

	// Removes the given file, returns TRUE if this succeeded or if file wasn't there in the first place, FALSE on error
	static bool DoRemoveFile(const fs::path& fileToRemove);

	// Moves the given file, from <fromPath> to <toPath>
	static bool DoMoveFile(const fs::path& fromPath, const fs::path& toPath);

	// Sub-routine of SearchForNewMods() investigating the FM folder
	// It moves incoming pk4/zip files into FM subdirectories
	static idStrList SearchForNewMods(const idStr& fmsDir);

private:
	// Finds out which map is the starting map (must be called after InitCurrentMod)
	void InitStartingMap();

	// Attempts to read the map sequence file for the current mod
	void InitMapSequence();

	// Sub-routine of SearchForNewMods() investigating the FM folder
	// using the given extension (including dot ".pk4", ".zip")
	static idStrList SearchForNewMods(const idStr& fmsDir, const idStr& extension, MoveList* appendMoveList);

	// Returns the path to the "darkmod" base
	fs::path GetDarkmodPath();

	// Finds all available mods
	void GenerateModList();

	// Loads the mod list from the given XML
	bool LoadModListFromXml(const XmlDocumentPtr& doc);

	// Loads mod details from the given XML, storing the data in the mod with the given index
	void LoadModDetailsFromXml(const XmlDocumentPtr& doc, int modIndex);

	// Request status according to the pending download
	RequestStatus GetRequestStatusForDownloadId(int downloadId);

	// Replaces stuff like &#13;
	static idStr ReplaceXmlEntities(const idStr& input);

	bool ProcessMissionScreenshot(const fs::path& tempFilename, DownloadableMod& mod, int screenshotNum);
};
typedef std::shared_ptr<CMissionManager> CMissionManagerPtr;

#endif /* _MISSION_MANAGER_H_ */
