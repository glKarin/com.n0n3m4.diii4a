/***************************************************************************
Quit menu. Quite lame for now.
*/
float() cvars_haveunsaved = #0;
nonstatic void(mitem_desktop desktop) M_Quit =
{
	local float pos;
	mitem_exmenu m;
	float unsaved = FALSE;
	m = spawn(mitem_exmenu, item_text:_("Options"), item_flags:IF_SELECTABLE, item_command:"m_main");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	//center the actual items
	pos = (16/-2)*(2);

	//draw title art above the options
//	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
//	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [(-160-banner.item_size_x)*0.5, pos-32], [(-160+banner.item_size_x)*0.5, pos-8]);

	if (checkbuiltin(cvars_haveunsaved))
		unsaved = cvars_haveunsaved();	//engines that don't have this are assumed to always save regardless. Which makes prompting for it irrelevant. The all-saved text ignores saving entirely so it still makes sense.
	if (unsaved)
	{
		m.add(spawn(mitem_text, item_text:"Save configuration?", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos-(8+16)], [0, pos-8]);	pos += 16;

		m.add(spawn(mitem_text, item_text:"Save and quit", item_command:"m_pop;cfg_save;quit", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;
		m.add(spawn(mitem_text, item_text:"Quit and discard settings", item_command:"m_pop;quit", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;
		m.add(spawn(mitem_text, item_text:"Keep playing.", item_command:"m_pop;m_main", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;
	}
	else
	{
		m.add(spawn(mitem_text, item_text:"Really Quit?", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos-(8+16)], [0, pos-8]);	pos += 16;

		m.add(spawn(mitem_text, item_text:"Yes, I'm a quitter.", item_command:"m_pop;quit", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;
		m.add(spawn(mitem_text, item_text:"No, keep playing!", item_command:"m_pop;m_main", item_scale:16, item_flags:IF_CENTERALIGN), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, pos], [0, pos+16]);	pos += 16;
	}

	//random art for style
	m.add(spawn (mitem_spinnymodel, item_text: "progs/suit.mdl"), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [0, 12*-16/2], [160, 12*16/2]);
	addmenuback(m);
};
