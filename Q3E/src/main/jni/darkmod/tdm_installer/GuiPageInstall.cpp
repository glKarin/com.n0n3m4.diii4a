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
#include "GuiPageInstall.h"
#include "GuiFluidAutoGen.h"
#include "LogUtils.h"
#include "Actions.h"
#include "OsUtils.h"
#include "ProgressIndicatorGui.h"
#include "GuiUtils.h"


static void Install_UpdateAdditional() {
	if (Actions::CanRestoreOldConfig())
		g_Install_ButtonRestoreCfg->activate();
	else
		g_Install_ButtonRestoreCfg->deactivate();
	if (Actions::IfShortcutExists())
		g_Install_ButtonCreateShortcut->label("Recreate shortcut");
	else
		g_Install_ButtonCreateShortcut->label("Create shortcut");
}

void Install_MetaPerformInstall() {
	g_Install_ProgressDownload->hide();
	g_Install_ProgressVerify->hide();
	g_Install_OutputRemainDownload->hide();
	g_Install_ProgressRepack->hide();
	g_Install_ProgressFinalize->hide();

	g_Install_TextFinishedInstall->hide();
	g_Install_TextAdditional->hide();
	g_Install_ButtonRestoreCfg->hide();
	g_Install_ButtonCreateShortcut->hide();

	g_Wizard->next();
	g_Install_ButtonClose->deactivate();
	g_Install_ButtonCancel->activate();

	try {
		GuiDeactivateGuard deactivator(g_PageInstall, {g_Install_ButtonCancel});
		g_Install_ProgressDownload->show();
		Fl::flush();
		ProgressIndicatorGui progress1(g_Install_ProgressDownload);
		ProgressIndicatorGui progress2(g_Install_ProgressVerify);
		progress1.AttachRemainsLabel(g_Install_OutputRemainDownload);
		progress1.AttachMainWindow(g_Window);
		progress2.AttachMainWindow(g_Window);
		Actions::PerformInstallDownload(&progress1, &progress2, g_Settings_CheckNoMultipartByteranges->value());
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		ZipSync::DoClean(OsUtils::GetCwd());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	try {
		GuiDeactivateGuard deactivator(g_PageInstall, {});
		ProgressIndicatorGui progress(g_Install_ProgressRepack);
		progress.AttachMainWindow(g_Window);
		Actions::PerformInstallRepack(&progress);
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		ZipSync::DoClean(OsUtils::GetCwd());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	try {
		GuiDeactivateGuard deactivator(g_PageInstall, {});
		ProgressIndicatorGui progress(g_Install_ProgressFinalize);
		Actions::PerformInstallFinalize(&progress);
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		ZipSync::DoClean(OsUtils::GetCwd());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	g_Install_TextInstalling->hide();
	g_Install_TextFinishedInstall->show();
	g_Install_TextAdditional->show();
	g_Install_ButtonRestoreCfg->show();
	g_Install_ButtonCreateShortcut->show();

	g_Install_ButtonClose->activate();
	g_Install_ButtonCancel->deactivate();

	Install_UpdateAdditional();
}

void cb_Install_ButtonClose(Fl_Widget *self) {
	g_logger->infof("Closing installer after successful install");
	exit(0);
}

void cb_Install_ButtonRestoreCfg(Fl_Widget *self) {
	try {
		Actions::DoRestoreOldConfig();
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
	}
	Install_UpdateAdditional();
}

void cb_Install_ButtonCreateShortcut(Fl_Widget *self) {
	try {
		Actions::CreateShortcut();
		g_Install_ButtonCreateShortcut->deactivate();
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
	}
	Install_UpdateAdditional();
}
