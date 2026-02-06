nonstatic void(mitem_desktop desktop) M_Options_Hud =
{
	local float i;
	local float h;
	mitem_exmenu m;
	m = spawn(mitem_exmenu, item_text:_("Hud Configurations"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	h = 200;
	h *= 0.5;	//and halve it

	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [banner.item_size_x*-0.5, -h-32], [banner.item_size_x*0.5, -h-8]);
	
	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, -h+16], [0, h*2+32]);

	float fs = search_begin("huds/*.cfg", TRUE, TRUE);
	float fc = search_getsize(fs);

	float y = 0;
	string fname = "huds/hud_default.cfg";
	string iname = "Default";
	string dname = iname;
	if ((int)whichpack(fname))
	{
		dname = GetFirstLineComment(fname, iname);
		fr.add(spawn(mitem_text, item_text:dname, item_command:sprintf("set plug_sbar 3; cvarreset hud_*; exec \"%s\"; hud_recalculate", fname), item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, y], [0, 16]);
	}
	else
		fr.add(spawn(mitem_text, item_text:dname, item_command:"cvarreset hud_*; set plug_sbar 0", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, y], [0, 16]);
	y += 16;

	//add the options
	for (i = 0; i < fc; i++)
	{
		fname = search_getfilename(fs, i);
		iname = substring(fname, 5, -5);
		if (iname == "hud_default")
			continue;
		if (substring(fname, 0, 4) == "hud_")
			iname = substring(fname, 4, -1);
		dname = GetFirstLineComment(fname, iname);
//		print(sprintf("\"%s\" \"%s\" - \"%s\"\n", fname, iname, dname));
		if (dname)
		{
			fr.add(spawn(mitem_text, item_text:dname, item_command:sprintf("set plug_sbar 3; cvarreset hud_*; exec \"%s\"; hud_recalculate", fname), item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, y], [0, 16]);
			y += 16;
		}
	}
	search_end(fs);


	//random art for style
	m.addm(spawn (mitem_spinnymodel, item_text: "progs/g_nail2.mdl", zbias:-16), [-160-60, 12*-16/2], [-60, 12*16/2]);

	addmenuback(m);
};
