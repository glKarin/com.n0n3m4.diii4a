/***************************************************************************
Options menu.
just a simple list.
*/
nonstatic void(mitem_desktop desktop) M_Options =
{
	local float pos;
	mitem_exmenu m;
	m = spawn(mitem_exmenu, item_text:_("Options"), item_flags:IF_SELECTABLE, item_command:"m_main");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

/*	//center the actual items
	pos = (16/-2)*(9);

	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.addm(banner, [(-160-banner.item_size_x)*0.5, pos-32], [(-160+banner.item_size_x)*0.5, pos-8]);
*/

	float h = 200 * 0.5;
	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-80-banner.item_size_x*0.5, -h-32], [-80+banner.item_size_x*0.5, -h-8]);

	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [-160-160, -h], [0+160, h*2]);

	float fl = RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN;
	pos = 0;



	//and show the options.
	if (checkcommand2("fps_preset", FALSE))
		{fr.add(spawn(mitem_text, item_text:"Graphical Presets",	item_command:"m_pop;m_preset", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;}
	fr.add(spawn(mitem_text, item_text:"Game Configs", 			item_command:"m_pop;m_configs", 	item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;
	fr.add(spawn(mitem_text, item_text:"Basic Setup", 			item_command:"m_pop;m_basicopts",	item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;
	fr.add(spawn(mitem_text, item_text:"Keys", 				item_command:"m_pop;m_keys", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;
	fr.add(spawn(mitem_text, item_text:"Audio", 				item_command:"m_pop;m_audio", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;
	fr.add(spawn(mitem_text, item_text:"Video", 				item_command:"m_pop;m_video", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;
	fr.add(spawn(mitem_text, item_text:"Effects", 				item_command:"m_pop;m_effects", 	item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;
	if (checkcommand2("r_particledesc", FALSE))
		{fr.add(spawn(mitem_text, item_text:"Particles",		item_command:"m_pop;m_particles", 	item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;}
	if (checkcommand2("ezhud_nquake", FALSE))
		{fr.add(spawn(mitem_text, item_text:"Hud",			item_command:"m_pop;m_hud", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;}
	if (checkbuiltin2(buf_cvarlist, TRUE))
		{fr.add(spawn(mitem_text, item_text:"Advanced Guru Settings",	item_command:"m_pop;m_cvars", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;}
	if (checkcommand2("cvarreset", FALSE) || checkcommand2("resetall", FALSE))
		{fr.add(spawn(mitem_text, item_text:"Reset to Defaults", 	item_command:"m_reset", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;}
	if (checkcommand2("cfg_save", FALSE))
		{fr.add(spawn(mitem_text, item_text:"Save Settings", 		item_command:"cfg_save", 		item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [0, 16]);	pos += 16;}
		
	//random art for style
	m.addm(spawn (mitem_spinnymodel, item_text: "progs/suit.mdl"), [0, 12*-16/2], [160, 12*16/2]);
	addmenuback(m);
};



static void(mitem_desktop desktop, string question, string affirmitive, string affirmitiveaction, string negative, string negativeaction) M_SimplePrompt =
{
	local float pos;
	mitem_exmenu m;
	m = spawn(mitem_exmenu, item_text:_("Options"), item_flags:IF_SELECTABLE, item_command:"");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_exclusive = m;
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	//center the actual items
	pos = (16/-2)*(2);

	//draw title art above the options
//	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
//	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [(-160-banner.item_size_x)*0.5, pos-32], [(-160+banner.item_size_x)*0.5, pos-8]);

	m.add(spawn(mitem_text, item_text:question, item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos-(8+16)], [0, pos-8]);	pos += 16;

	m.add(spawn(mitem_text, item_text:affirmitive, item_command:affirmitiveaction, item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;
	m.add(spawn(mitem_text, item_text:negative, item_command:negativeaction, item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;

	//random art for style
	m.add(spawn (mitem_spinnymodel, item_text: "progs/suit.mdl"), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [0, 12*-16/2], [160, 12*16/2]);
	addmenuback(m);
};

nonstatic void(mitem_desktop desktop) M_Reset =
{
	string cmd = "m_pop;cvarreset *;exec default.cfg";
	if (!checkcommand2("cvarreset", FALSE))
		cmd = "m_pop;resetall;exec default.cfg";
	M_SimplePrompt(desktop, "Really Reset All Settings?", "Reset All", cmd, "Cancel", "m_pop");
};
