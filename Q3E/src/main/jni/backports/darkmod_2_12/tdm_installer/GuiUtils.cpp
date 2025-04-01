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
#include "GuiUtils.h"
#include <string>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Output.H>

//we need Public Morozov to access individual widgets
#define private public
#include <FL/Fl_File_Chooser.H>
#undef private

#include <memory>
#include "LogUtils.h"


bool UnattendedMode = false;

//taken from fl_file_dir.cxx (popup)
static void PopupDialogModal(Fl_File_Chooser *fc) {
	fc->show();

	//deactivate Fl::grab(), because it is incompatible with modal windows
	Fl_Window* g = Fl::grab();
	if (g)
		Fl::grab(0);

	while (fc->shown())
		Fl::wait();

	//regrab the previous popup menu, if there was one
	if (g)
		Fl::grab(g);
}

const char *GuiChooseDirectory(const char *message) {
	static std::unique_ptr<Fl_File_Chooser> fc;
	fc.reset(new Fl_File_Chooser(".", "*", Fl_File_Chooser::CREATE | Fl_File_Chooser::DIRECTORY, message));

	//customize look of file chooser
	GuiSetStyles(fc->window);
	fc->preview(0);
	fc->favoritesButton->hide();
	fc->newButton->show();
	fc->previewButton->hide();
	fc->showChoice->hide();
	fc->previewBox->hide();
	fc->showHiddenButton->hide();
	Fl_Widget *ch = fc->fileList;
	Fl_Group *par = fc->fileList->parent();
	par->resize(par->x(), par->y() - 30, par->w() - 30, par->h() + 30);
	ch->resize(ch->x(), ch->y(), ch->w() - 5, ch->h() + 20);

	PopupDialogModal(fc.get());
	return fc->value();
}

int GuiMessageBox(GuiMessageBoxFlags flags, const char *message, const char *buttonOk, const char *buttonCancel) {
	int buttons = flags & mbfButtonsMask;
	int type = flags & mbfTypeMask;
	switch (buttons) {
		case mbfButtonsOk: {
			if (type == mbfTypeError) {
				g_logger->warningf("Showing error message: %s", message);
				if (UnattendedMode)
					exit(3);
				fl_alert("ERROR: %s", message);
			}
			else if (type == mbfTypeMessage)
				fl_message("%s", message);
			else
				ZipSyncAssert(false);
			return 0;
		}
		case mbfButtonsYesNo: {
			int idx = -1;
			if (type == mbfTypeWarning) {
				g_logger->warningf("Showing warning message: %s", message);
				if (UnattendedMode && (flags & mbfDefaultCancel))
					exit(2);
				if (flags & mbfDefaultCancel)
					idx = fl_choice("%s", buttonOk, buttonCancel, nullptr, message);
				else
					idx = !fl_choice("%s", buttonCancel, buttonOk, nullptr, message);
			}
			else
				ZipSyncAssert(false);
			return idx;
		}
		default: {
			ZipSyncAssert(false);
		}
	}
}

static void SetStyleRecursive(Fl_Widget *widget) {
	static const Fl_Color ProgressFillColor = fl_rgb_color(0, 255, 0);
	static const Fl_Color ButtonNormalColor = fl_rgb_color(225);
	static const Fl_Color ButtonDownColor = fl_rgb_color(204, 228, 247);

	if (Fl_Group *w = dynamic_cast<Fl_Group*>(widget)) {
		int k = w->children();
		for (int i = 0; i < k; i++)
			SetStyleRecursive(w->child(i));
	}
	else if (Fl_Progress *w = dynamic_cast<Fl_Progress*>(widget)) {
		w->selection_color(ProgressFillColor);
	}
	else if (Fl_Check_Button *w = dynamic_cast<Fl_Check_Button*>(widget)) {
	}
	else if (Fl_Button *w = dynamic_cast<Fl_Button*>(widget)) {
		w->box(FL_BORDER_BOX);
		w->down_box(FL_BORDER_BOX);
		w->color(ButtonNormalColor);
		w->down_color(ButtonDownColor);
	}
	else if (Fl_Output *w = dynamic_cast<Fl_Output*>(widget)) {
		w->box(FL_BORDER_BOX);
		w->color(FL_BACKGROUND_COLOR);
	}
	else if (Fl_File_Input *w = dynamic_cast<Fl_File_Input*>(widget)) {
		w->box(FL_BORDER_BOX);
		w->down_box(FL_BORDER_BOX);
	}
}
void GuiSetStyles(Fl_Widget *rootWindow) {
	Fl::set_color(FL_BACKGROUND_COLOR, fl_rgb_color(240));
	SetStyleRecursive(rootWindow);
}
