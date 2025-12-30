// From ZDoom!

#include <algorithm>

#include "zdoomsupport.h"

#ifdef USE_TEXTSCREEN
#include "textscreen.h"

int I_WADSEL = -1;
txt_window_t *window;

void TXT_PickWad(TXT_UNCAST_ARG(widget), int wad)
{
	I_WADSEL = wad;

	TXT_CloseWindow(window);
}
#endif

#ifndef NO_GTK
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#endif

#if defined(__APPLE__)
int I_PickIWad_Cocoa (WadStuff *wads, int numwads, bool showwin, int defaultiwad);
#elif defined(__ANDROID__)
int I_PickIWad_Android (WadStuff *wads, int numwads, bool showwin, int defaultiwad);
#endif

#ifndef NO_GTK
extern bool GtkAvailable;

// GtkTreeViews eats return keys. I want this to be like a Windows listbox
// where pressing Return can still activate the default button.
gint AllowDefault(GtkWidget *widget, GdkEventKey *event, gpointer func_data)
{
	if (event->type == GDK_KEY_PRESS && event->keyval == GDK_KEY_Return)
	{
		gtk_window_activate_default (GTK_WINDOW(func_data));
	}
	return FALSE;
}

// Double-clicking an entry in the list is the same as pressing OK.
gint DoubleClickChecker(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
	if (event->type == GDK_2BUTTON_PRESS)
	{
		*(int *)func_data = 1;
		gtk_main_quit();
	}
	return FALSE;
}

// When the user presses escape, that should be the same as canceling the dialog.
gint CheckEscape (GtkWidget *widget, GdkEventKey *event, gpointer func_data)
{
	if (event->type == GDK_KEY_PRESS && event->keyval == GDK_KEY_Escape)
	{
		gtk_main_quit();
	}
	return FALSE;
}

void ClickedOK(GtkButton *button, gpointer func_data)
{
	*(int *)func_data = 1;
	gtk_main_quit();
}

int I_PickIWad_Gtk (WadStuff *wads, int numwads, bool showwin, int defaultiwad)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *bbox;
	GtkWidget *widget;
	GtkWidget *tree;
	GtkWidget *check;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkTreeIter iter, defiter;
	int close_style = 0;
	int i;

	FString caption;
	caption.Format("%s: Select an IWAD to use", GetGameCaption());

	// Create the dialog window.
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(window), caption);
	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width (GTK_CONTAINER(window), 10);
	g_signal_connect (window, "delete_event", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect (window, "key_press_event", G_CALLBACK(CheckEscape), NULL);

	// Create the vbox container.
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_add (GTK_CONTAINER(window), vbox);

	// Create the top label.
	widget = gtk_label_new (GAMENAME " found more than one IWAD\nSelect from the list below to determine which one to use:");
	gtk_box_pack_start (GTK_BOX(vbox), widget, false, false, 0);
	gtk_widget_set_halign (widget, GTK_ALIGN_START);

	// Create a list store with all the found IWADs.
	store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	for (i = 0; i < numwads; ++i)
	{
		FString filepart;
		if(wads[i].Path.Size() == 1)
		{
			filepart = strrchr(wads[i].Path[0], '/');
			if(filepart.IsEmpty())
				filepart = wads[i].Path[0];
			else
				filepart = filepart.Mid(1);
		}
		else
			filepart.Format("*.%s", wads[i].Extension.GetChars());
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
			0, filepart.GetChars(),
			1, wads[i].Name.GetChars(),
			2, i,
			-1);
		if (i == defaultiwad)
		{
			defiter = iter;
		}
	}

	// Create the tree view control to show the list.
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("IWAD", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(tree), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Game", renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(tree), column);
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(tree), true, true, 0);
	g_signal_connect(G_OBJECT(tree), "button_press_event", G_CALLBACK(DoubleClickChecker), &close_style);
	g_signal_connect(G_OBJECT(tree), "key_press_event", G_CALLBACK(AllowDefault), window);

	// Select the default IWAD.
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
	gtk_tree_selection_select_iter (selection, &defiter);

	// Create the hbox for the bottom row.
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX(vbox), hbox, false, false, 0);

	// Create the "Don't ask" checkbox.
	check = gtk_check_button_new_with_label ("Don't ask me this again");
	gtk_box_pack_start (GTK_BOX(hbox), check, false, false, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), !showwin);

	// Create the OK/Cancel button box.
	bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX(bbox), 10);
	gtk_box_pack_end (GTK_BOX(hbox), bbox, false, false, 0);

	// Create the OK button.
	widget = gtk_button_new_with_label ("OK");
	gtk_box_pack_start (GTK_BOX(bbox), widget, false, false, 0);
	gtk_widget_set_can_default (widget, true);
	gtk_widget_grab_default (widget);
	g_signal_connect (widget, "clicked", G_CALLBACK(ClickedOK), &close_style);
	g_signal_connect (widget, "activate", G_CALLBACK(ClickedOK), &close_style);

	// Create the cancel button.
	widget = gtk_button_new_with_label ("Cancel");
	gtk_box_pack_start (GTK_BOX(bbox), widget, false, false, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK(gtk_main_quit), &window);

	// Finally we can show everything.
	gtk_widget_show_all (window);

	gtk_main ();

	if (close_style == 1)
	{
		GtkTreeModel *model;
		GValue value = { 0, { {0} } };
		
		// Find out which IWAD was selected.
		gtk_tree_selection_get_selected (selection, &model, &iter);
		gtk_tree_model_get_value (GTK_TREE_MODEL(model), &iter, 2, &value);
		i = g_value_get_int (&value);
		g_value_unset (&value);
		
		// Set state of queryiwad based on the checkbox.
		queryiwad = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check));
	}
	else
	{
		i = -1;
	}
	
	if (GTK_IS_WINDOW(window))
	{
		gtk_widget_destroy (window);
		// If we don't do this, then the X window might not actually disappear.
		while (g_main_context_iteration (NULL, FALSE)) {}
	}

	return i;
}
#endif

bool sortwad(WadStuff const & wada, WadStuff const & wadb) {
	return !(wada.Name.Compare(wadb.Name) < 0);
}

int I_PickIWad (WadStuff *wads, int numwads, bool showwin, int defaultiwad)
{
	int i;

	if (!showwin)
	{
		return defaultiwad;
	}

#if !defined(__APPLE__) && !defined(_WIN32) && !defined(__ANDROID__)
	const char *str;
	if((str=getenv("KDE_FULL_SESSION")) && strcmp(str, "true") == 0)
	{
		FString cmd;
		cmd.Format("kdialog --title \"%s: Select an IWAD to use\""
		           " --menu \"" GAMENAME " found more than one IWAD\n"
		           "Select from the list below to determine which one to use:\"", GetGameCaption());

		for(i = 0; i < numwads; ++i)
		{
			FString filepart;
			if(wads[i].Path.Size() == 1)
			{
				filepart = strrchr(wads[i].Path[0], '/');
				if(filepart.IsEmpty())
					filepart = wads[i].Path[0];
				else
					filepart = filepart.Mid(1);
			}
			else
				filepart.Format("*.%s", wads[i].Extension.GetChars());
			// Menu entries are specified in "tag" "item" pairs, where when a
			// particular item is selected (and the Okay button clicked), its
			// corresponding tag is printed to stdout for identification.
			cmd.AppendFormat(" \"%d\" \"%s (%s)\"", i, wads[i].Name.GetChars(), filepart.GetChars());
		}

		if(defaultiwad >= 0 && defaultiwad < numwads)
		{
			FString filepart;
			if(wads[defaultiwad].Path.Size() == 1)
			{
				filepart = strrchr(wads[defaultiwad].Path[0], '/');
				if(filepart.IsEmpty())
					filepart = wads[defaultiwad].Path[0];
				else
					filepart = filepart.Mid(1);
			}
			else
				filepart.Format("*.%s", wads[defaultiwad].Extension.GetChars());
			cmd.AppendFormat(" --default \"%s (%s)\"", wads[defaultiwad].Name.GetChars(), filepart.GetChars());
		}

		FILE *f = popen(cmd, "r");
		if(f != NULL)
		{
			char gotstr[16];

			if(fgets(gotstr, sizeof(gotstr), f) == NULL ||
			   sscanf(gotstr, "%d", &i) != 1)
				i = -1;

			// Exit status = 1 means the selection was canceled (either by
			// Cancel/Esc or the X button), not that there was an error running
			// the program. In that case, nothing was printed so fgets will
			// have failed. Other values can indicate an error running the app,
			// so fall back to whatever else can be used.
			int status = pclose(f);
			if(WIFEXITED(status) && (WEXITSTATUS(status) == 0 || WEXITSTATUS(status) == 1))
				return i;
		}
	}
#endif
#ifndef NO_GTK
	if (GtkAvailable)
	{
		return I_PickIWad_Gtk (wads, numwads, showwin, defaultiwad);
	}
#elif defined(__APPLE__)
	return I_PickIWad_Cocoa (wads, numwads, showwin, defaultiwad);
#elif defined(__ANDROID__) && !defined(_DIII4A)
	return I_PickIWad_Android (wads, numwads, showwin, defaultiwad);
#endif
#ifndef USE_TEXTSCREEN
	printf ("Please select a game wad (or 0 to exit):\n");
	for (i = 0; i < numwads; ++i)
	{
		FString filepart;
		if(wads[i].Path.Size() == 1)
		{
			filepart = strrchr(wads[i].Path[0], '/');
			if(filepart.IsEmpty())
				filepart = wads[i].Path[0];
			else
				filepart = filepart.Mid(1);
		}
		else
			filepart.Format("*.%s", wads[i].Extension.GetChars());
		printf ("%d. %s (%s)\n", i+1, wads[i].Name.GetChars(), filepart.GetChars());
	}
	printf ("Which one? ");
	if (scanf ("%d", &i) != 1 || i > numwads)
		return numwads-1;
	return i-1;
#else
	if (!TXT_Init())
	{
		I_FatalError("Failed to initialise GUI");
	}

	FString caption;
	caption.Format("%s: Select an IWAD to use", GetGameCaption());
    TXT_SetDesktopTitle(const_cast<char*>(caption.GetChars()));
	window = TXT_NewWindow(const_cast<char*>("Pick a game wad"));
	txt_table_t *table = TXT_NewTable(1);

	std::sort(wads, wads+numwads, sortwad);

	for (i = 0; i < numwads; ++i)
	{
		FString filepart;
		if(wads[i].Path.Size() == 1)
		{
			filepart = strrchr(wads[i].Path[0], '/');
			if(filepart.IsEmpty())
				filepart = wads[i].Path[0];
			else
				filepart = filepart.Mid(1);
		}
		else
			filepart.Format("*.%s", wads[i].Extension.GetChars());

		TXT_AddWidget(table, TXT_NewButton2((char*)wads[i].Name.GetChars(), (void (*)(void*, void*))TXT_PickWad, (void*)(intptr_t)i));

	}

	TXT_AddWidget(window, TXT_NewScrollPane(0, (numwads < 10) ? numwads : 10, table));
	TXT_GUIMainLoop();

	TXT_Shutdown();

	return I_WADSEL;
#endif
}
