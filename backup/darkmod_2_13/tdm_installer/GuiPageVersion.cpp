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
#include "GuiPageVersion.h"
#include "GuiFluidAutoGen.h"
#include "Actions.h"
#include "State.h"
#include "LogUtils.h"
#include "ProgressIndicatorGui.h"
#include "GuiUtils.h"


static void Version_UpdateGui() {
	Fl_Tree_Item *firstSel = g_Version_TreeVersions->first_selected_item();
	Fl_Tree_Item *lastSel = g_Version_TreeVersions->last_selected_item();
	bool oneSelected = (firstSel && firstSel == lastSel);
	bool isVersionSelected = (oneSelected && firstSel->children() == 0);

	std::string customUrl = g_Version_InputCustomManifestUrl->value();
	bool hasCustomUrl = !customUrl.empty();
	bool invalidCustomUrl = hasCustomUrl && !ZipSync::PathAR::IsHttp(customUrl);

	bool correctStats = false;
	if (isVersionSelected) {
		std::string selVersion = firstSel->label();
		if (hasCustomUrl)
			selVersion = selVersion + " & " + customUrl;
		if (selVersion == g_state->_versionRefreshed)
			correctStats = true;
	}

	if (isVersionSelected && !invalidCustomUrl) {
		g_Version_ButtonNext->activate();
		g_Version_ButtonRefreshInfo->activate();
	}
	else {
		g_Version_ButtonNext->deactivate();
		g_Version_ButtonRefreshInfo->deactivate();
	}

	if (correctStats) {
		g_Version_OutputCurrentSize->activate();
		g_Version_OutputFinalSize->activate();
		g_Version_OutputAddedSize->activate();
		g_Version_OutputRemovedSize->activate();
		g_Version_OutputDownloadSize->activate();
		g_Version_ButtonRefreshInfo->hide();
	}
	else {
		g_Version_OutputCurrentSize->deactivate();
		g_Version_OutputFinalSize->deactivate();
		g_Version_OutputAddedSize->deactivate();
		g_Version_OutputRemovedSize->deactivate();
		g_Version_OutputDownloadSize->deactivate();
		g_Version_ButtonRefreshInfo->show();
	}

	if (hasCustomUrl)
		g_Version_TextCustomManifestMessage->show();
	else
		g_Version_TextCustomManifestMessage->hide();
	static int DefaultColor = g_Version_InputCustomManifestUrl->color();
	if (invalidCustomUrl)
		g_Version_InputCustomManifestUrl->color(FL_RED);
	else
		g_Version_InputCustomManifestUrl->color(DefaultColor);
	g_Version_InputCustomManifestUrl->redraw();
}

void cb_Version_TreeVersions(Fl_Widget *self) {
	Version_UpdateGui();
}

void cb_Version_InputCustomManifestUrl(Fl_Widget *self) {
	Version_UpdateGui();
}

void cb_Version_ButtonRefreshInfo(Fl_Widget *self) {
	Fl_Tree_Item *firstSel = g_Version_TreeVersions->first_selected_item();
	ZipSyncAssert(firstSel);	//never happens
	std::string version = firstSel->label();

	std::string customUrl = g_Version_InputCustomManifestUrl->value();
	if (!customUrl.empty() && !g_state->_config.IsUrlTrusted(customUrl)) {
		int idx = GuiMessageBox(mbfWarningMinor, 
			"The custom URL you entered does NOT belong to TheDarkMod main server.\n"
			"Make sure you got this URL from a trusted source!",
			"Continue", "Stop"
		);
		if (idx == 1)
			return;
	}

	//find information for the new version
	Actions::VersionInfo info;
	try {
		GuiDeactivateGuard deactivator(g_PageVersion, {});
		g_Version_ProgressDownloadManifests->show();
		Fl::flush();
		ProgressIndicatorGui progress(g_Version_ProgressDownloadManifests);
		progress.AttachMainWindow(g_Window);
		info = Actions::RefreshVersionInfo(version, customUrl, g_Settings_CheckBitwiseExact->value(), &progress);
		g_Version_ProgressDownloadManifests->hide();
	}
	catch(const std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		g_Version_ProgressDownloadManifests->hide();
		return;
	}

	//update GUI items
	if (info.missingSize == 0) {
		auto BytesToString = [](uint64_t bytes) -> std::string {
			return std::to_string((bytes + 999999) / 1000000) + " MB";
		};
		g_Version_OutputCurrentSize->value(BytesToString(info.currentSize).c_str());
		g_Version_OutputFinalSize->value(BytesToString(info.finalSize).c_str());
		g_Version_OutputAddedSize->value(BytesToString(info.addedSize).c_str());
		g_Version_OutputRemovedSize->value(BytesToString(info.removedSize).c_str());
		g_Version_OutputDownloadSize->value(BytesToString(info.downloadSize).c_str());
	}
	else {
		static const char *IMPOSSIBLE = "MISS";
		g_Version_OutputCurrentSize->value(IMPOSSIBLE);
		g_Version_OutputFinalSize->value(IMPOSSIBLE);
		g_Version_OutputAddedSize->value(IMPOSSIBLE);
		g_Version_OutputRemovedSize->value(IMPOSSIBLE);
		g_Version_OutputDownloadSize->value(IMPOSSIBLE);
	}

	//remember that we display info for this version
	g_state->_versionRefreshed = version;
	if (!customUrl.empty())
		g_state->_versionRefreshed += " & " + customUrl;
	//will activate outputs and hide refresh button
	g_Version_TreeVersions->do_callback();
}

void cb_Version_ButtonNext(Fl_Widget *self) {
	//make sure selected version has been evaluated/refreshed
	if (g_Version_ButtonRefreshInfo->visible() && g_Version_ButtonRefreshInfo->active()) {
		g_Version_ButtonRefreshInfo->do_callback();
		if (g_Version_ButtonRefreshInfo->visible()) {
			//could not load manifest -> stop
			return;
		}
	}

	if (!g_state->_updater) {
		std::string text = ZipSync::formatMessage(
			"Custom manifest URL cannot be used with base version %s.\n"
			"Make sure you select correct base version.",
			g_Version_TreeVersions->first_selected_item()->label()
		);
		GuiMessageBox(mbfError, text.c_str());
		return;
	}

	g_Confirm_OutputInstallDirectory->value(g_Settings_InputInstallDirectory->value());
	g_Confirm_OutputLastInstalledVersion->value(g_Version_OutputLastInstalledVersion->value());
	g_Confirm_OutputVersionToInstall->value(g_state->_versionRefreshed.c_str());

	g_Confirm_OutputCurrentSize->value(g_Version_OutputCurrentSize->value());
	g_Confirm_OutputFinalSize->value(g_Version_OutputFinalSize->value());
	g_Confirm_OutputAddedSize->value(g_Version_OutputAddedSize->value());
	g_Confirm_OutputRemovedSize->value(g_Version_OutputRemovedSize->value());
	g_Confirm_OutputDownloadSize->value(g_Version_OutputDownloadSize->value());

	g_Wizard->next();
}

void cb_Version_ButtonPrev(Fl_Widget *self) {
	bool customVersion = g_Settings_CheckCustomVersion->value();
	if (customVersion) {
		g_Wizard->value(g_PageVersion);
	}
	else {
		g_Wizard->value(g_PageSettings);
	}
}

void cb_Version_ChoiceMirror(Fl_Widget *self) {
	int idx = g_Version_ChoiceMirror->value();
	std::string caption = g_Version_ChoiceMirror->text();
	if (idx == 0)	//[auto]
		g_state->_preferredMirror.clear();
	else {
		g_state->_preferredMirror = caption;
	}
}
