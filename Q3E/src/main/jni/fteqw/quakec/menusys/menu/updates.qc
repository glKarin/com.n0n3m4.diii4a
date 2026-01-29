#include "../menusys/mitem_grid.qc"

enum : int
{
	GPMI_NAME,		//name of the package, for use with the pkg command.
	GPMI_CATEGORY,		//category text
	GPMI_TITLE,		//name of the package, for showing the user.
	GPMI_VERSION,		//version info (may have multiple with the same name but different versions)
	GPMI_DESCRIPTION,	//some blurb
	GPMI_LICENSE,		//what license its distributed under
	GPMI_AUTHOR,		//name of the person(s) who created it
	GPMI_WEBSITE,		//where to contribute/find out more info/etc
	GPMI_INSTALLED,		//current state
	GPMI_ACTION,		//desired state
	GPMI_AVAILABLE,		//whether it may be downloaded or not.
	GPMI_FILESIZE,		//whether it may be downloaded or not.
	GPMI_GAMEDIR,		//whether it may be downloaded or not.
	GPMI_MAPS,			//whether it may be downloaded or not.
};

class mitem_updategrid : mitem_grid
{
	virtual void(vector pos) item_draw =
	{	//make sure we see any updates as they're detected...
		__using getpackagemanagerinfo, ftoi;
		if (getpackagemanagerinfo(grid_numchildren, GPMI_NAME))
		{
			grid_numchildren++;
			while(getpackagemanagerinfo(grid_numchildren, GPMI_NAME))
				grid_numchildren++;
			item_resized();
		}
		super::item_draw(pos);
	};
	virtual void(vector pos, float idx) grid_draw;
	virtual float(vector pos, float scan, float char, float down, float idx) grid_keypress;
//	virtual void(float olditem, float newitem) grid_selectionchanged;
};
void(vector pos, float idx) mitem_updategrid::grid_draw =
{
	__using getpackagemanagerinfo, ftoi;
	string text = getpackagemanagerinfo(idx, GPMI_TITLE);

	vector col = item_rgb;
	if (item_flags & IF_MFOCUSED && idx == grid_mactive)
		col_z = 0;
	if (item_flags & IF_KFOCUSED && idx == grid_kactive)
		col_x = 0;

	string status = getpackagemanagerinfo(idx, GPMI_INSTALLED);
	string action = getpackagemanagerinfo(idx, GPMI_ACTION);

	if (action == "purge")		//delete the thing
		action = "^&F4 ";
	else if (action=="reinstall")	//purge then reinstall(the purge status was explicit at least.)
		action = "^&F2P";
	else if (action=="user")	//install(explcitly by user)
		action = "^&F2 ";
	else if (action=="auto")	//install(to satisfy dependancies)
		action = "^&FA ";
	else if (action=="disable")
		action = "^&FE ";
	else if (action=="retain")
		action = "^&FE ";
	else
		action = "^&F4 ";

	if (status == "0%" || status == "pending")
	{
		ui.drawfill(pos, [item_size_x, item_scale], '0 0 0.5', 0.1, 0);
		action = status = " ";
	}
	else if (strstrofs(status, "%")>=0)
	{
		float frac = stof(status)/100;
		ui.drawfill(pos, [item_size_x*frac, item_scale], '0 0 0.5', 0.9, 0);
		action = status = " ";
	}
	else if (status == "corrupt")
		status = "^&FE?";	//yellow
	else if (status == "enabled")
		status = "^&F2 ";	//green
	else if (status == "present")
		status = "^&FE ";	//yellow
	else
		status = "^&F4 ";	//red

	ui.drawstring(pos, strcat(status, action, "^&F-", text), '1 1 0' * item_scale, col, item_alpha, 0);
};
/*void(float olditem, float newitem) mitem_updategrid::grid_selectionchanged =
{
};
*/float(vector pos, float scan, float char, float down, float idx) mitem_updategrid::grid_keypress =
{
	__using getpackagemanagerinfo, ftoi;
	string text;
	if (!down)
		return FALSE;
	else if (scan == K_DEL||scan == K_BACKSPACE||scan == K_LEFTARROW)
	{
		text = getpackagemanagerinfo(idx, GPMI_NAME);
		if (!text)
			return FALSE;

		string action = getpackagemanagerinfo(idx, GPMI_ACTION);
		if (action == "purge")		//delete the thing
			action = "rem";
		else if (action=="reinstall")	//purge then reinstall(the purge status was explicit at least.)
			action = "rem";
		else if (action=="user")	//install(explcitly by user)
			action = "rem";
		else if (action=="auto")	//install(to satisfy dependancies)
			action = "rem";
		else 
			action = "del";

		text = strcat("pkg quiet_", action, " \"", text, "\"\n");
		localcmd(text);
	}
	else if (scan == K_RIGHTARROW || scan==K_ENTER || scan == K_MOUSE1 || scan == K_TOUCHTAP)
	{
		text = getpackagemanagerinfo(idx, GPMI_NAME);
		if (!text)
			return FALSE;
		text = strcat("pkg quiet_add \"", text, "\"\n");
		localcmd(text);
	}
	else
		return FALSE;
	return TRUE;
};




class menu_updates : mitem_exmenu
{
	string filter;

	mitem_updategrid grid;

	virtual string(string key) get =
	{
		__using getpackagemanagerinfo, ftoi;
		if (key == "info")
		{
			string text=__NULL__, tmp;
			tmp = getpackagemanagerinfo(grid.grid_kactive, GPMI_VERSION);		text = strcat(text, "^aVersion:^a ", tmp?:"Unspecified");
			tmp = getpackagemanagerinfo(grid.grid_kactive, GPMI_AUTHOR);		text = strcat(text, "\n^aAuthor:^a ", tmp?:"Unknown");
			tmp = getpackagemanagerinfo(grid.grid_kactive, GPMI_LICENSE);		text = strcat(text, "\n^aLicense:^a ", tmp?:"Unknown");
			tmp = getpackagemanagerinfo(grid.grid_kactive, GPMI_WEBSITE);		text = strcat(text, "\n^aWebsite:^a ", tmp?:"Unknown");
			tmp = getpackagemanagerinfo(grid.grid_kactive, GPMI_DESCRIPTION);	text = strcat(text, "\n^aDescription:^a ", tmp?:"No description specified");
			return text;
		}
		return super::get(key);
	};
};



void(mitem_desktop desktop) M_Menu_Updates =
{
	mitem it;
	float h = (480+240)/2;

	if (!checkbuiltin(getpackagemanagerinfo))
		return;	//not supported in this engine...

	localcmd("pkg update\n");

	//create the menu, give it focus, and make sure its displayed over everything else.
	menu_updates m = spawn(menu_updates, item_text:_("Updates List"), item_flags:IF_SELECTABLE, item_command:"m_main", frame_stdheight:h);
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/ttl_cstm.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [banner.item_size_x*-0.5, h*-0.5], [banner.item_size_x*0.5, 24]);

	//spawn our grid for the actual options. this provides a scrollbar if we have too many items.
	m.grid = spawn(mitem_updategrid, item_flags: IF_SELECTABLE, item_scale: 8);
	m.add(m.grid, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, h*-0.5+32], [40, h*0.5]);

	it = spawn(mitem_label, item_text: "info", item_flags: 0, item_scale: 8);
	m.add(it, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MID, [40, h*-0.5+32], [0, h*0.5-16]);

	it = spawn(mitem_button, item_text: "Apply", item_command: "pkg apply\n", item_flags: IF_SELECTABLE, item_scale: 8);
	m.add(it, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MID, [40+1, h*0.5-16+1], [0-1, h*0.5-1]);

	//and give us a suitable menu tint too, just because.
	addmenuback(m);
};
