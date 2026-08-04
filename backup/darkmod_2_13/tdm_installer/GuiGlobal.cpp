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
#include "GuiGlobal.h"
#include "GuiFluidAutoGen.h"
#include "OsUtils.h"
#include "State.h"
#include "Constants.h"
#include "GuiPageSettings.h"
#include "GuiPageVersion.h"
#include "GuiPageConfirm.h"
#include "GuiPageInstall.h"
#include "ProgressIndicatorGui.h"
#include "GuiUtils.h"
#include "LogUtils.h"


void cb_Settings_ButtonReset(Fl_Widget *self) {
	g_state->Reset();
	g_Settings_InputInstallDirectory->value(OsUtils::GetCwd().c_str());
	g_Settings_InputInstallDirectory->do_callback();
	g_Settings_CheckCustomVersion->value(false);
	g_Settings_CheckAdvancedSettings->value(false);
	g_Settings_CheckAdvancedSettings->do_callback();

	g_Version_InputCustomManifestUrl->value("");
	g_Version_TreeVersions->do_callback();
	g_Version_OutputCurrentSize->value("");
	g_Version_OutputFinalSize->value("");
	g_Version_OutputAddedSize->value("");
	g_Version_OutputRemovedSize->value("");
	g_Version_OutputDownloadSize->value("");
}

void cb_RaiseInterruptFlag(Fl_Widget *self) {
	ProgressIndicatorGui::Interrupt();
}

void cb_TryToExit(Fl_Widget *self) {
	if (!GuiDeactivateGuard::IsAnyActive()) {
		//no action in progress: close window and stop program
		g_Window->hide();
	}
}

//============================================================

static void GuiToInitialState() {
	{
		static char buff[256];
		sprintf(buff, "TheDarkMod installer v%s (built on %s)", TDM_INSTALLER_VERSION, __DATE__);
		g_Window->label(buff);
	}
	g_Window->position( ( Fl::w() - g_Window->w() ) / 2, ( Fl::h() - g_Window->h() ) / 2 );

	//----- "settings" page -----
	static Fl_Text_Buffer g_Settings_StringGreetings;
	g_Settings_StringGreetings.text(
		"This application will install TheDarkMod, or update existing installation \nto the most recent version available. "
		"An active internet connection will be required. \n"
		"Review the settings below, then click Next to start. "
	);
	g_Settings_TextGreetings->buffer(g_Settings_StringGreetings);
	g_Settings_ButtonRestartNewDir->hide();
	g_Settings_ProgressScanning->hide();

	//----- "version" page -----
	g_Version_TextCustomManifestMessage->hide();
	g_Version_ProgressDownloadManifests->hide();

	//----- "confirm" page -----
	static Fl_Text_Buffer g_Confirm_StringReadyToInstall;
	g_Confirm_StringReadyToInstall.text(
		"TheDarkMod is ready to install or update. \n"
		"Check the information below and click START. \n"
	);
	g_Confirm_TextReadyToInstall->buffer(g_Confirm_StringReadyToInstall);

	//----- "install" page -----
	static Fl_Text_Buffer g_Install_StringInstalling;
	g_Install_StringInstalling.text(
		"TheDarkMod is being installed right now. \n"
		"Please wait... \n"
	);
	g_Install_TextInstalling->buffer(g_Install_StringInstalling);
	static Fl_Text_Buffer g_Install_StringFinishedInstall;
	g_Install_StringFinishedInstall.text(
		"Installation finished successfully! \n"
		"Click Close to exit. \n"
	);
	static Fl_Text_Buffer g_Install_StringAdditional;
	g_Install_StringAdditional.text(
		"Additional actions:\n"
	);
	g_Install_TextFinishedInstall->buffer(g_Install_StringFinishedInstall);
	g_Install_TextFinishedInstall->hide();
	g_Install_ProgressDownload->hide();
	g_Install_OutputRemainDownload->hide();
	g_Install_ProgressRepack->hide();
	g_Install_ProgressFinalize->hide();
	g_Install_TextAdditional->buffer(g_Install_StringAdditional);
	g_Install_TextAdditional->hide();

	g_Wizard->value(g_PageSettings);
}

static void GuiInstallCallbacks() {
	g_Window->callback(cb_TryToExit);

	g_Settings_InputInstallDirectory->when(FL_WHEN_CHANGED);
	g_Settings_InputInstallDirectory->callback(cb_Settings_InputInstallDirectory);
	g_Settings_ButtonBrowseInstallDirectory->callback(cb_Settings_ButtonBrowseInstallDirectory);
	g_Settings_ButtonRestartNewDir->callback(cb_Settings_ButtonRestartNewDir);
	g_Settings_ButtonReset->callback(cb_Settings_ButtonReset);
	g_Settings_CheckAdvancedSettings->callback(cb_Settings_CheckAdvancedSettings);
	g_Settings_ButtonNext->callback(cb_Settings_ButtonNext);

	g_Settings_ButtonReset->do_callback();

	g_Version_TreeVersions->callback(cb_Version_TreeVersions);
	g_Version_InputCustomManifestUrl->when(FL_WHEN_CHANGED);
	g_Version_InputCustomManifestUrl->callback(cb_Version_InputCustomManifestUrl);
	g_Version_ButtonRefreshInfo->callback(cb_Version_ButtonRefreshInfo);
	g_Version_ButtonNext->callback(cb_Version_ButtonNext);
	g_Version_ChoiceMirror->callback(cb_Version_ChoiceMirror);

	g_Confirm_ButtonBack->callback(cb_Confirm_ButtonBack);
	g_Confirm_ButtonStart->callback(cb_Confirm_ButtonStart);

	g_Install_ButtonRestoreCfg->callback(cb_Install_ButtonRestoreCfg);
	g_Install_ButtonCreateShortcut->callback(cb_Install_ButtonCreateShortcut);
	g_Install_ButtonCancel->callback(cb_RaiseInterruptFlag);
	g_Install_ButtonClose->callback(cb_Install_ButtonClose);
}


static void GuiSetIcon(Fl_Window *window) {
#ifdef _WIN32
	window->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(101)));
#endif
}

void GuiInitAll() {
	GuiSetIcon(g_Window);
	GuiToInitialState();
	GuiInstallCallbacks();
	GuiSetStyles(g_Window);
}

void GuiLoaded(void*) {
	if (OsUtils::HasElevatedPrivilegesWindows()) {
		int idx = GuiMessageBox(mbfWarningMajor,
			"The installer was run \"as admin\". This is strongly discouraged!\n"
			"If you continue, admin rights will most likely be necessary to play the game.",
			"I know what I'm doing", "Exit"
		);
		if (idx == 1)
			exit(0);
	}
}

void GuiInitHelp() {
	GuiSetIcon(g_HelpWindow);
	GuiSetStyles(g_HelpWindow);

	static Fl_Text_Buffer g_Help_StringParameters;
	g_Help_StringParameters.text(
		"List of command line parameters:\n"
		"\n"
		"--help\n"
		"    Display this list of parameters.\n"
		"\n"
		"--unattended\n"
		"    Install game without any human intervention.\n"
		"    The most recent stable version is installed by default\n"
		"    into the directory where this installer is located.\n"
		"    BEWARE: it won't ask for any confirmation!\n"
		"    Make sure to look into tdm_installer.log after it finishes.\n"
		"    Return code should be nonzero if an error happens.\n"
		"\n"
		"--version {ver}\n"
		"    Install the version {ver} instead of the default one.\n"
		"    Only has effect when combined with --unattended.\n"
	);
	g_Help_TextParameters->buffer(g_Help_StringParameters);
}

void GuiUnattended(int argc, char **argv) {
	UnattendedMode = true;

	bool skipConfigDownload = false;
	std::string version;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--skip-config-download") == 0)
			skipConfigDownload = true;
		if (strcmp(argv[i], "--version") == 0 && i+1 < argc)
			version = argv[i+1];
	}

	Fl::flush();
	GuiLoaded(nullptr);

	Fl::flush();
	g_Settings_CheckCustomVersion->value(true);
	g_Settings_CheckAdvancedSettings->value(true);
	g_Settings_CheckSkipSelfUpdate->value(true);
	if (skipConfigDownload)
		g_Settings_CheckSkipConfigDownload->value(true);
	Fl::flush();
	g_Settings_ButtonNext->do_callback();

	Fl::flush();
	g_logger->infof("Running in unattended mode");
	if (!version.empty()) {
		int cnt = 0;
		g_Version_TreeVersions->deselect_all();
		for (Fl_Tree_Item *item = g_Version_TreeVersions->first(); item; item = g_Version_TreeVersions->next(item)) {
			const char *label = item->label();
			if (version == label) {
				g_Version_TreeVersions->select(item);
				cnt++;
			}
		}
		if (cnt != 1) {
			std::string message = "Specified version " + version + " not found";
			GuiMessageBox(mbfError, message.c_str());
		}
	}
	g_Version_ButtonNext->do_callback();

	Fl::flush();
	g_Confirm_ButtonStart->do_callback();

	Fl::flush();
	g_Install_ButtonClose->do_callback();

	exit(0);	//never reached
}
