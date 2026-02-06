class mitem_pictext : mitem_text
{
	string pic;
	virtual void(vector pos) item_draw =
	{
		if (pic)
			drawpic(pos, pic, [item_size_y,item_size_y], '1 1 1', 1, 0); 
		pos_x += item_size_y;
		super::item_draw(pos);
	};
};

void(mitem_desktop desktop) M_Menu_Mods =
{
	float h = (480+240)/2;

	//create the menu, give it focus, and make sure its displayed over everything else.
	mitem_exmenu m = spawn(mitem_exmenu, item_text:_("Mods List"), item_flags:IF_SELECTABLE, item_command:"m_main", frame_stdheight:h);
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/ttl_cstm.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [banner.item_size_x*-0.5, h*-0.5], [banner.item_size_x*0.5, 24]);

	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_PARENT_MID, [-140, h*-0.5+32], [280, h*0.5]);

	float mod;
	for (mod = 0; ; mod++)
	{
		__using GGDI_DESCRIPTION, GGDI_LOADCOMMAND, GGDI_ICON;

		string gamedir = getgamedirinfo(mod, GGDI_GAMEDIR);
		if not(gamedir)
			break;
		string desc = getgamedirinfo(mod, GGDI_DESCRIPTION);
		if (desc=="")
		{	//clean up some stuff a little, if we know it.
			string lwr = strtolower(gamedir);	//windows sucks.
			if (lwr == "hipnotic")
				desc = "Scourge of Armagon";
			else if (lwr == "rogue")
				desc = "Dissolution of Eternity";
			else if (lwr == "dopa")
				desc = "Dimension of the Past";
			else if (lwr == "mg1")
				desc = "Dimensions of the Machine";
			else if (lwr == "ad")
				desc = "Arcane Dimensions";
			else if (lwr == "quoth")
				desc = "Quoth";
			else if (lwr == "rally")
				desc = "Quake Rally";
			else if (lwr == "fortress")
				desc = "Team Fortress";
		}
		if (desc=="")
			desc = sprintf("%s/", gamedir);	//no description given, so make it clear that its an actual subdir name
		else
			desc = sprintf("%s ^h(%s)", desc, gamedir);	//include the gamedir, faded somewhat.
		string cmd = getgamedirinfo(mod, GGDI_LOADCOMMAND);
		if not(cmd)	//for dp users, if they somehow run this
			cmd = sprintf("gamedir %s", gamedir);

		string icon = getgamedirinfo(mod, GGDI_ICON);

		//add some extra stuff to reset the menu
		cmd = strcat("m_pop; ", cmd, "; togglemenu");

		fr.add(spawn(mitem_pictext, pic:icon, item_text:desc, item_command:cmd, item_scale:16, item_flags:0), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, mod*16], [0, 16]);
	}

	if (!mod)
		fr.add(spawn(mitem_text, item_text:"No mods detected", item_scale:16, item_flags:0), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, mod*16], [0, 16]);
	
	//and give us a suitable menu tint too, just because.
	addmenuback(m);
};
