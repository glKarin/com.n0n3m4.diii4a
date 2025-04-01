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
#include "GuiPageSettings.h"
#include "GuiFluidAutoGen.h"
#include <FL/Fl_File_Chooser.H>
#include "StdFilesystem.h"
#include "StdString.h"
#include "OsUtils.h"
#include "Actions.h"
#include "State.h"
#include "ProgressIndicatorGui.h"
#include "GuiUtils.h"


void cb_Settings_InputInstallDirectory(Fl_Widget *self) {
	std::string installDir = g_Settings_InputInstallDirectory->value();
	bool invalid = false;

	//normalize current path
	std::string normalizedDir = installDir;
	if (normalizedDir.find("..") != -1) {
		try {
			normalizedDir = stdext::canonical(normalizedDir).string();	//collapse parents
		} catch(stdext::filesystem_error &) {
			invalid = true;	//happens on Linux sometimes
		}
	}
	normalizedDir = stdext::path(normalizedDir).string();	//normalize slashes
	if (normalizedDir != installDir) {
		g_Settings_InputInstallDirectory->value(normalizedDir.c_str());
		installDir = normalizedDir;
	}

	//color red if path is bad
	static int DefaultColor = g_Settings_InputInstallDirectory->color();
	if (installDir.find('.') != -1)
		invalid = true;
	if (!stdext::path(installDir).is_absolute())
		invalid = true;
	bool pathExists = stdext::exists(installDir);
	bool pathIsDir = stdext::is_directory(installDir);
	if (pathExists && !pathIsDir)
		invalid = true;
	if (invalid) {
		g_Settings_InputInstallDirectory->color(FL_RED);
		g_Settings_ButtonRestartNewDir->deactivate();
	}
	else {
		g_Settings_InputInstallDirectory->color(DefaultColor);
		g_Settings_ButtonRestartNewDir->activate();
	}
	g_Settings_InputInstallDirectory->redraw();

	//update Next and Restart buttons
	std::string defaultDir = OsUtils::GetCwd();
	if (defaultDir != installDir) {
		if (pathIsDir)
			g_Settings_ButtonRestartNewDir->label("Restart");
		else
			g_Settings_ButtonRestartNewDir->label("Create and Restart");
		g_Settings_ButtonRestartNewDir->show();
		g_Settings_ButtonNext->deactivate();
	}
	else {
		g_Settings_ButtonRestartNewDir->hide();
		g_Settings_ButtonNext->activate();
	}
}

void cb_Settings_ButtonBrowseInstallDirectory(Fl_Widget *self) {
	//note: modal
	const char *chosenPath = GuiChooseDirectory("Choose where to install TheDarkMod");
	if (chosenPath) {
		g_Settings_InputInstallDirectory->value(chosenPath);
		g_Settings_InputInstallDirectory->do_callback();
	}
}

void cb_Settings_ButtonRestartNewDir(Fl_Widget *self) {
	std::string installDir = g_Settings_InputInstallDirectory->value();

	try {
		auto warnings = Actions::CheckSpaceAndPermissions(installDir);
		for (const std::string &message : warnings) {
			int idx = GuiMessageBox(mbfWarningMajor, message.c_str(), "I know what I'm doing", "Stop");
			if (idx == 1)
				return;
		}
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		return;
	}

	try {
		Actions::RestartWithInstallDir(installDir);
		//note: this line is never executed
	} catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
	}
}

void cb_Settings_CheckAdvancedSettings(Fl_Widget *self) {
	if (g_Settings_CheckAdvancedSettings->value()) {
		g_Settings_CheckSkipSelfUpdate->activate();
		g_Settings_CheckSkipConfigDownload->activate();
		g_Settings_CheckForceScan->activate();
		g_Settings_CheckBitwiseExact->activate();
		g_Settings_CheckNoMultipartByteranges->activate();
	}
	else {
		g_Settings_CheckSkipSelfUpdate->deactivate();
		g_Settings_CheckSkipConfigDownload->deactivate();
		g_Settings_CheckForceScan->deactivate();
		g_Settings_CheckBitwiseExact->deactivate();
		g_Settings_CheckNoMultipartByteranges->deactivate();
		g_Settings_CheckSkipSelfUpdate->value(false);
		g_Settings_CheckSkipConfigDownload->value(false);
		g_Settings_CheckForceScan->value(false);
		g_Settings_CheckBitwiseExact->value(false);
		g_Settings_CheckNoMultipartByteranges->value(false);
	}
}

void cb_Settings_ButtonNext(Fl_Widget *self) {
	try {
		auto warnings = Actions::CheckSpaceAndPermissions(OsUtils::GetCwd());
		for (const std::string &message : warnings) {
			int idx = GuiMessageBox(mbfWarningMajor, message.c_str(), "I know what I'm doing", "Stop");
			if (idx == 1)
				return;
		}
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		return;
	}

	try {
		Actions::StartLogFile();
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		return;
	}

	try {
		bool skipUpdate = g_Settings_CheckSkipSelfUpdate->value();
		GuiDeactivateGuard deactivator(g_PageSettings, {});
		ProgressIndicatorGui progress(g_Settings_ProgressScanning);
		if (!skipUpdate) {
			if (Actions::NeedsSelfUpdate(&progress)) {
				GuiMessageBox(mbfMessage, "New version of installer has been downloaded. Installer will be restarted to finish update.");
				Actions::DoSelfUpdate();
			}
		}
		g_Settings_ProgressScanning->hide();
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		g_Settings_ProgressScanning->hide();
		return;
	}

	try {
		bool skipUpdate = g_Settings_CheckSkipConfigDownload->value();
		GuiDeactivateGuard deactivator(g_PageSettings, {});
		ProgressIndicatorGui progress(g_Settings_ProgressScanning);
		Actions::ReadConfigFile(!skipUpdate, &progress);
		g_Settings_ProgressScanning->hide();
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		g_Settings_ProgressScanning->hide();
		return;
	}

	try {
		GuiDeactivateGuard deactivator(g_PageSettings, {});
		ProgressIndicatorGui progress(g_Settings_ProgressScanning);
		progress.AttachMainWindow(g_Window);
		Actions::ScanInstallDirectoryIfNecessary(g_Settings_CheckForceScan->value(), &progress);
		g_Settings_ProgressScanning->hide();
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		g_Settings_ProgressScanning->hide();
		return;
	}

	//update versions tree on page "Version"
	g_Version_TreeVersions->clear();
	std::vector<std::string> allVersions = g_state->_config.GetAllVersions();
	std::string defaultVersion = g_state->_config.GetDefaultVersion();
	for (const std::string &version : allVersions) {
		std::vector<std::string> guiPath = g_state->_config.GetFolderPath(version);
		guiPath.push_back(version);
		std::string wholePath = stdext::join(guiPath, "/");
		Fl_Tree_Item *item = g_Version_TreeVersions->add(wholePath.c_str());
		if (defaultVersion == version) {
			g_Version_TreeVersions->select(item);
		}
	}
	g_Version_OutputLastInstalledVersion->value(g_state->_lastInstall.GetVersion().c_str());

	g_Version_ChoiceMirror->clear();
	g_Version_ChoiceMirror->add("[auto]");
	for (const std::string &mirror : g_state->_config.GetAllMirrors())
		g_Version_ChoiceMirror->add(mirror.c_str());
	g_Version_ChoiceMirror->value(0);

	bool customVersion = g_Settings_CheckCustomVersion->value();
	g_state->_versionRefreshed.clear();
	g_Version_TreeVersions->do_callback();
	g_Wizard->next();

	if (!customVersion) {
		//user wants default version, so auto-click "Next" button for him
		g_Version_ButtonNext->do_callback();
	}
}
