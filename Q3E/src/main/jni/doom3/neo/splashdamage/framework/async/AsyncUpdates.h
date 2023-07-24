// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __ASYNC_UPDATES_H__
#define __ASYNC_UPDATES_H__

// update status reported by the update server
// NOTE: any change to this needs to be reflected in the update server code enums
typedef enum {
	UPDATE_AVAIL_UNKNOWN = 0,
	UPDATE_AVAIL_NOREPLY,		// update server couldn't be reached
	UPDATE_AVAIL_NONE,			// update server replied no update
	UPDATE_AVAIL_WEB,			// go to a web page
	UPDATE_AVAIL_WEB_REQUIRED,	// required web update
	UPDATE_AVAIL_SOFT,			// new release, upgrading softly
	UPDATE_AVAIL_REQUIRED		// this release is retired, you have to update
} updateAvailability_t;

// send some amount of info back to the update server
// we use this to keep a few stats and monitor the health of the update system
// NOTE: any change to this needs to be reflected in the update server code enums
typedef enum {
	UPDATE_TKB_UNKNOWN = 0,
	UPDATE_TKB_LATER,
	UPDATE_TKB_DOWNLOAD,
	UPDATE_TKB_DLDONE,
	UPDATE_TKB_FAILED_CREATE,	// file creation failed for download store
	UPDATE_TKB_FAILED_OPEN,		// can't open file after DL
	UPDATE_TKB_FAILED_CHECKSUM,	// wrong checksum (send checksum)
	UPDATE_TKB_FAILED_CURL,		// curl error (send error code and error string)
	UPDATE_TKB_WEBSITE,
	UPDATE_TKB_EXEC
} updateTalkback_t;

// update states
typedef enum {
	UPDATE_IDLE = 0,			// not working. triggers a polling cycle regularly on dedicated server
	UPDATE_WAITING,				// waiting on the master to give update information
	UPDATE_PROCESS_UPDATE,		// update message was received, transit to the right stuff
	UPDATE_INITIATE_DOWNLOAD,	// start the download
	UPDATE_REMINDING,			// regularly print update information to the console and to clients playing (dedicated server)
	UPDATE_PROMPTING_SETUP,		// setup GUI prompting the user for download (client only)
	UPDATE_PROMPTING,			// waiting on download prompt (client only)
	UPDATE_DOWNLOADING,			// progress report
	UPDATE_DOWNLOAD_FAILED,		// failed or cancelled
	UPDATE_PROMPT_DL_FAILED,	// ask website (client only)
	UPDATE_DOWNLOAD_DONE,		// download successful, see where we go from there
	UPDATE_EXECUTE,				// quit and execute the downloaded installer
	UPDATE_REMIND_READY,		// remind that the installer is ready (dedicated server)
	UPDATE_PROMPT_READY
} updateState_t;

typedef enum {
	UPDATE_GUI_MIN = -1,
	UPDATE_GUI_NONE = 0,
	UPDATE_GUI_DOWNLOAD,
	UPDATE_GUI_WEBSITE,
	UPDATE_GUI_LATER,
	UPDATE_GUI_YES,
	UPDATE_GUI_NO,
	UPDATE_GUI_CANCEL,
	UPDATE_GUI_MAX
} guiUpdateResponse_t;

#endif // !__ASYNC_UPDATES_H__
