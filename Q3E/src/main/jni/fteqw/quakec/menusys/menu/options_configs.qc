/***************************************************************************
Uses the engine command to apply the preset, and tries an exec instead if a config file exists.
Doesn't track the current one or anything.
Really simple and stupid menu.
no background tint, so the game is still visible so you can preview it.
*/

nonstatic void(mitem_desktop desktop) M_Configs =
{
	float i;
	mitem_exmenu m;
	m = spawn(mitem_exmenu, item_text:_("Game Presets / Configs"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();


	float h = 200*0.5;
	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_PARENT_MID, [(160-60-banner.item_size_x)*0.5, -h-32], [banner.item_size_x, -h-8]);
	
	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, -h], [0, h*2]);
	



	float fs, y=0;
	float c = 0;
	fs = search_begin("configs/game_*.cfg", TRUE, TRUE);	
	if (fs >= 0)
	{
		c = search_getsize(fs);
		for (i = 0; i < c; i++)
		{
			string fname = search_getfilename(fs, i);
			string iname = strzone(substring(fname, 13, -5));
			string dname = GetFirstLineComment(fname, iname);
			iname = sprintf("exec \"%s\"", fname);
			if (dname && !fr.findchildcmd(iname))
				fr.add(spawn(mitem_text, item_text:dname, item_command:iname, item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, y+=16], '100 16');
		}
		search_end(fs);	
	}
	if (c <= 0)
		fr.add(spawn(mitem_text, item_text:"No configs found", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, y], '100 16');

	//random art for style
	m.addm(spawn (mitem_spinnymodel, item_text: "progs/g_rock2.mdl", zbias:-16), [-160-60, 12*-16/2], [-60, 12*16/2]);

	addmenuback(m);
};

