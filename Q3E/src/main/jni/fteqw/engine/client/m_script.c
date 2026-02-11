//read menu.h

#include "quakedef.h"
#include "shader.h"

#ifndef NOBUILTINMENUS

int selectitem;
emenu_t *menu_script;

void M_Script_Option (emenu_t *menu, char *optionvalue, qboolean isexplicit)
{
	menuoption_t *mo;
	char *scriptname = menu->data;

	char buf[8192];
	//FIXME: not sure about these, as the user typically can't see what they'll do.
	int expandlevel = RESTRICT_SERVER;
	int execlevel = RESTRICT_LOCAL;

	Cbuf_AddText("wait\n", execlevel);

	if (!scriptname || !*scriptname)
	{
		if (isexplicit)
			Cbuf_AddText(va("%s\n", optionvalue), execlevel);
		return;
	}

	//update the option
	Cbuf_AddText(va("set option %s\n", COM_QuotedString(optionvalue, buf, sizeof(buf), false)), execlevel);

	//expand private arguments
	for (mo = menu->options, *buf = 0; mo; mo = mo->common.next)
	{
		if (mo->common.type == mt_edit)
		{
			if (strlen(buf) + strlen(mo->edit.text) + 2 >= sizeof(buf))
				break;
			memmove(buf+strlen(mo->edit.text)+1, buf, strlen(buf)+1);
			memcpy(buf, mo->edit.text, strlen(mo->edit.text));
			buf[strlen(mo->edit.text)] = ' ';
		}
	}
	Cmd_TokenizeString(buf, false, false);
	Cmd_ExpandString(scriptname, buf, sizeof(buf), &expandlevel, true, false, false);

	//and execute it as-is
	Cbuf_AddText(buf, execlevel);
	Cbuf_AddText("\n", execlevel);
}

void M_Script_Remove (emenu_t *menu)
{
	if (menu == menu_script)
		menu_script = NULL;

	M_Script_Option(menu, "cancel", false);
}
qboolean M_Script_Key (struct emenu_s *menu, int key, unsigned int unicode)
{
	if (menu->selecteditem && menu->selecteditem->common.type == mt_edit)
		return false;
	if (key >= '0' && key <= '9' && menu->data)
	{
		if (key == '0')	//specal case so that "hello" < "0"... (plus matches common impulses)
			M_Script_Option(menu, "10", false);
		else
			M_Script_Option(menu, va("%i", key-'0'), false);
		return true;
	}
	return false;
}

void M_MenuS_Callback_f (void)
{
	if (menu_script)
	{
		M_Script_Option(menu_script, Cmd_Argv(1), true);
	}
}
void M_MenuS_Clear_f (void)
{
	if (menu_script)
	{
		M_RemoveMenu(menu_script);
	}
}

void M_MenuS_Script_f (void)	//create a menu.
{
	int items;
	char *alias = Cmd_Argv(1);

	selectitem = 0;
	items=0;

	if (menu_script)
	{
		menuoption_t *option;
		for (option = menu_script->options; option; option = option->common.next)
		{
			if (option->common.type == mt_button)
			{
				if (menu_script->selecteditem == option)
					selectitem = items;
				items++;
			}
		}
		selectitem = items - selectitem-1;
		M_MenuS_Clear_f();
	}

	menu_script = M_CreateMenu(0);
	menu_script->remove = M_Script_Remove;
	menu_script->key = M_Script_Key;
	
	Key_Dest_Remove(kdm_console);

	if (Cmd_Argc() == 1)
		menu_script->data = Cmd_ParseMultiline(true);
	else
		menu_script->data = Z_StrDup(alias);
}

void M_MenuS_Box_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	int width = atoi(Cmd_Argv(3));
	int height = atoi(Cmd_Argv(4));

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	MC_AddBox(menu_script, x, y, width, height);
}

void M_MenuS_CheckBox_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *cvarname = Cmd_Argv(4);
	int bitmask = atoi(Cmd_Argv(5));
	cvar_t *cvar;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	cvar = Cvar_Get(cvarname, text, 0, "User variables");
	if (!cvar)
		return;
	MC_AddCheckBox(menu_script, x, x+160, y, text, cvar, bitmask);
}

void M_MenuS_Slider_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *cvarname = Cmd_Argv(4);
	float min = atof(Cmd_Argv(5));
	float max = atof(Cmd_Argv(6));
	cvar_t *cvar;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	cvar = Cvar_Get(cvarname, text, 0, "User variables");
	if (!cvar)
		return;
	MC_AddSlider(menu_script, x, x+160, y, text, cvar, min, max, 0);
}

void M_MenuS_Picture_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *picname = Cmd_Argv(3);
	mpic_t *p;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	p = R2D_SafeCachePic(picname);
	if (!p)
		return;

	if (!strcmp(Cmd_Argv(1), "-"))
		MC_AddCenterPicture(menu_script, y, p->height, picname);
	else
		MC_AddPicture(menu_script, x, y, p->width, p->height, picname);
}

void M_MenuS_Edit_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *def = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	MC_AddEditCvar(menu_script, x, x+160, y, text, def, false);
}
void M_MenuS_EditPriv_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *def = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	MC_AddEdit(menu_script, x, x+160, y, text, def);
}

void M_MenuS_Text_f (void)
{
	menuoption_t *option;
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *command = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	if (Cmd_Argc() == 4)
		MC_AddBufferedText(menu_script, x, 0, y, text, false, false);
	else
	{
		option = (menuoption_t *)MC_AddConsoleCommand(menu_script, x, 0, y, text, va("menucallback \"%s\"\n", command));
		if (selectitem-- == 0)
			menu_script->selecteditem = option;
	}
}

void M_MenuS_TextBig_f (void)
{
	menuoption_t *option;
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *command = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	if (!*command)
		MC_AddConsoleCommandQBigFont(menu_script, x, y, text, command);
	else
	{
		option = (menuoption_t *)MC_AddConsoleCommandQBigFont(menu_script, x, y, text, va("menucallback %s\n", command));
		if (selectitem-- == 0)
			menu_script->selecteditem = option;
	}
}

void M_MenuS_Bind_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *caption = Cmd_Argv(3);
	char *command = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	if (!*caption)
		caption = command;

	MC_AddBind(menu_script, x, x+160, y, command, caption, NULL);
}

void M_MenuS_Comboi_f (void)
{
	int opt;
	char *opts[64];
	char *values[64];
	char valuesb[64][8];
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *caption = Cmd_Argv(3);
	char *command = Cmd_Argv(4);
	char *line;

	cvar_t *var;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	var = Cvar_Get(command, "0", 0, "custom cvars");
	if (!var)
		return;

	if (!*caption)
		caption = command;

	for (opt = 0; opt < sizeof(opts)/sizeof(opts[0])-2 && *(line=Cmd_Argv(5+opt)); opt++)
	{
		opts[opt] = line;
		Q_snprintfz(valuesb[opt], sizeof(valuesb[opt]), "%i", opt);
		values[opt] = valuesb[opt];
	}
	opts[opt] = NULL;

	MC_AddCvarCombo(menu_script, x, x+160, y, caption, var, (const char **)opts, (const char **)values);
}

char *Hunk_TempString(char *s)
{
	char *h;
	h = Hunk_TempAllocMore(strlen(s)+1);
	strcpy(h, s);
	return h;
}

void M_MenuS_Combos_f (void)
{
	int opt;
	char *opts[64];
	char *values[64];
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *caption = Cmd_Argv(3);
	char *command = Cmd_Argv(4);
	char *line;

	cvar_t *var;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	var = Cvar_Get(command, "0", 0, "custom cvars");
	if (!var)
		return;

	if (!*caption)
		caption = command;

	line = Cmd_Argv(5);
	if (!*line)
	{
		line = Cbuf_GetNext(Cmd_ExecLevel, true);
		if (*line != '{')
			Cbuf_InsertText(line, Cmd_ExecLevel, true);	//whoops. Stick the trimmed string back in to the cbuf.
		else
			line = "{";
	}
	if (!strcmp(line, "{"))
	{
		char *line;
		Hunk_TempAlloc(4);
		for (opt = 0; opt < sizeof(opts)/sizeof(opts[0])-2; opt++)
		{
			line = Cbuf_GetNext(Cmd_ExecLevel, true);
			line = COM_Parse(line);
			if (!strcmp(com_token, "}"))
				break;
			opts[opt] = Hunk_TempString(com_token);
			line = COM_Parse(line);
			values[opt] = Hunk_TempString(com_token);
		}
	}
	else
	{
		for (opt = 0; opt < sizeof(opts)/sizeof(opts[0])-2; opt++)
		{
			line = Cmd_Argv(5+opt*2);
			if (!*line)
				break;
			opts[opt] = line;
			values[opt] = Cmd_Argv(5+opt*2 + 1);
		}
	}
	opts[opt] = NULL;

	MC_AddCvarCombo(menu_script, x, x+160, y, caption, var, (const char **)opts, (const char **)values);
}

/*
menuclear
conmenu menucallback

menubox 0 0 320 8
menutext 0 0 "GO GO GO!!!" 		"radio21"
menutext 0 8 "Fall back" 		"radio22"
menutext 0 8 "Stick together" 		"radio23"
menutext 0 16 "Get in position"		"radio24"
menutext 0 24 "Storm the front"	 	"radio25"
menutext 0 24 "Report in"	 	"radio26"
menutext 0 24 "Cancel"	
*/
void M_Script_Init(void)
{
	Cmd_AddCommandD("menuclear",	M_MenuS_Clear_f,	"Pop the currently scripted menu.");
	Cmd_AddCommandD("menucallback",	M_MenuS_Callback_f,	"Explicitly invoke the active script menu's callback function with the given option set.");
	Cmd_AddCommandD("conmenu",		M_MenuS_Script_f,	"conmenu <callback>\nCreates a new (built-in) scripted menu. any following commands that define scipted menu items will add their items to this new menu. The callback will be called with argument 'cancel' when the menu is closed.");
	Cmd_AddCommandD("menubox",		M_MenuS_Box_f,		"x y width height");
	Cmd_AddCommandD("menuedit",		M_MenuS_Edit_f,		"x y caption cvarname");
	Cmd_AddCommandD("menueditpriv",	M_MenuS_EditPriv_f, "x y caption def");
	Cmd_AddCommandD("menutext",		M_MenuS_Text_f,		"x y caption cbcommand");
	Cmd_AddCommandD("menutextbig",	M_MenuS_TextBig_f,	"x y caption cbcommand");
	Cmd_AddCommandD("menupic",		M_MenuS_Picture_f,	"x y picname");
	Cmd_AddCommandD("menucheck",	M_MenuS_CheckBox_f,	"x y caption cvarname bitmask");
	Cmd_AddCommandD("menuslider",	M_MenuS_Slider_f,	"x y caption cvarname min max");
	Cmd_AddCommandD("menubind",		M_MenuS_Bind_f,		"x y caption bindcommand");
	Cmd_AddCommandD("menucomboi",	M_MenuS_Comboi_f,	"x y caption cvarname [caption0] [caption1] ...");
	Cmd_AddCommandD("menucombos",	M_MenuS_Combos_f,	"x y caption cvarname [caption0] [value0] [caption1] [value1] ...\nif 'caption0' is { then the options will be parsed from trailing lines\n");
}
#endif
