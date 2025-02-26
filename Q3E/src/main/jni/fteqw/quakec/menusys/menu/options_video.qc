static string() PrepareVideoModes =
{
	string s = _("\"0x0\" \"Default\"");
	
	for (float i = 0; ; i++)
	{
		vector m = getresolution(i);
		if (!m_x || !m_y)
			break;
		s = sprintf("%s \"%dx%d\" \"%d x %d\"", s, m_x, m_y, m_x, m_y);
	}
	return s;
};

class options_video : mitem_exmenu
{
	string videomode;
	void() options_video =
	{
		videomode = strzone(strcat(super::get("vid_width"), "x", super::get("vid_height")));
	};

	virtual float(string key) isvalid =
	{
		if (key == "vid_mode")
			return super::isvalid("vid_width") && super::isvalid("vid_height");
		return super::isvalid(key);
	};
	virtual string(string key) get =
	{
		if (key == "vid_mode")
			return videomode;
		return super::get(key);
	};
	virtual void(string key, string newval) set =
	{
		if (key == "vid_mode")
		{
			string old = videomode;
			videomode = strzone(newval);
			tokenizebyseparator(newval, "x");
			super::set("vid_width", argv(0));
			super::set("vid_height", argv(1));
			if (old)
				strunzone(old);
		}
		else
			super::set(key, newval);
	};
};

nonstatic void(mitem_desktop desktop) M_Options_Video =
{
	mitem_exmenu m;
	m = spawn(options_video, item_text:_("Video Options"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	float h = 200 * 0.5;
	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_PARENT_MID, [(160-160-banner.item_size_x)*0.5, -h-32], [banner.item_size_x, -h-8]);
	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [-160, -h], [160, h*2]);
	float fl = RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN;
	float pos = 0;
	
	pos += 8;
	fr.add(spawn(mitem_text, item_text:_("Apply / Restart"), 	item_command:"vid_restart", item_scale:8, item_flags:IF_RIGHTALIGN),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [0, pos], [-8, 8]); pos += 8;
	pos += 8;
	
	if (cvar_type("vid_renderer")) fr.add(menuitemcombo_spawn(_("Renderer"),	"vid_renderer",		'280 8', strcat("\"\" \"Default\" ", cvar_string("_vid_renderer_opts"))),	fl, [0, pos], [0, 8]); pos += 8;

	//add the options
	if (!dp_workarounds)
	{
		fr.add(menuitemcombo_spawn(_("Display Mode"),	"vid_fullscreen",				'280 8', 
																		engine==E_FTE?
																			"0	\"Windowed\" "
																			"1	\"Fullscreen\" "
																			"2	\"Borderless Windowed\" "
																		:
																			"0	\"Windowed\" "
																			"1	\"Fullscreen\" "
																		),	fl, [0, pos], [0, 8]); pos += 8;
	}
	else
	{
		fr.add(menuitemcheck_spawn(_("Fullscreen"),		"vid_fullscreen", 			'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	}
	if (cvar_type("vid_resizable")) fr.add(menuitemcheck_spawn(_("Resizable"),	"vid_resizable", 	'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcombo_spawn(_("MSAA"),	cv3("vid_samples"/*dp*/, "vid_fsaa"/*qs*/, "vid_multisample"/*fte*/),			'280 8',
																		"0	\"Off\" "
																		"2	\"2x\" "
																		"4	\"4x\" "
																		),	fl, [0, pos], [0, 8]); pos += 8;

	fr.add(menuitemcombo_spawn(_("Video Mode"),		"vid_mode",		'280 8', PrepareVideoModes()), fl, [0, pos], [0, 8]); pos += 8;

static const string scaleoptions = _(
	"0  \"Default\" "
	"1.5    \"x1.5\" "
	"2  \"x2\" "
	"4  \"x4\" ");

	if (cvar_type("vid_conautoscale"))
		fr.add(menuitemcombo_spawn(_("Video Zoom"),	"vid_conautoscale",	'280 8', scaleoptions),	fl, [0, pos], [0, 8]), pos += 8;
	if (cvar_type("scr_conscale"))
		fr.add(menuitemcombo_spawn(_("Console Zoom"),	"scr_conscale",	'280 8', scaleoptions),	fl, [0, pos], [0, 8]), pos += 8;
	if (cvar_type("scr_sbarscale"))
		fr.add(menuitemcombo_spawn(_("Sbar Zoom"),	"scr_sbarscale",	'280 8', scaleoptions),	fl, [0, pos], [0, 8]), pos += 8;
	if (cvar_type("scr_menuscale"))
		fr.add(menuitemcombo_spawn(_("Menu Zoom"),	"scr_menuscale",	'280 8', scaleoptions),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcombo_spawn(_("Colour Depth"),	cv2("vid_bitsperpixel", "vid_bpp"),	'280 8', _(
																		"16	\"16bit\" " //r5g6b5, or so. unsupported on win10.
																		"24	\"24bit\" "	//rgba8 - we don't count the alpha.
																		"30	\"30bit\" "	//rgb10a2 - we don't count the alpha. unsupported on nvidia.
																		"48	\"48bit\" "	//half-floats - we don't count the alpha. unsupported on nvidia.
																		)),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcombo_spawn(_("Refresh Rate"),	cv2("vid_refreshrate"/*qs*/, "vid_displayfrequency"),			'280 8', _(
																		"0	\"Default\" "
																	//	"60	\"60\" "
																	//	"75	\"75\" "
																		)),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcheck_spawn(_("VSync"),		cv2("vid_vsync", "vid_wait"), 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcheck_spawn(_("Show Framerate"),	cv3("showfps"/*dp*/, "scr_showfps"/*qs*/, "show_fps"/*id/qw*/), 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemslider_spawn(_("View Size"),	"viewsize",				'50 120 10', '280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemslider_spawn(_("Field Of View"),	"fov",				'50 140 5', '280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemslider_spawn(_("Gamma"),	cv2("v_gamma"/*dp*/, "gamma"/*quake*/),				'1.3 0.5 -0.1', '280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemslider_spawn(_("Contrast"),	cv2("v_contrast"/*dp*/, "contrast"/*quake*/),				'0.7 2 0.1', '280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	if (engine!=E_QSS)fr.add(menuitemslider_spawn(_("Brightness"),	cv2("v_brightness"/*dp*/, "brightness"/*quake*/),			'0 0.4 0.05', '280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)fr.add(menuitemcombo_spawn(_("Hardware Gamma"),	"vid_hardwaregamma",			'280 8',
																		"0	\"Off\" "
																		"1	\"Auto\" "
																		"2	\"Soft\" "
																		"3	\"Hard\" "
																		"4	\"Scene Only\" "
																		),	fl, [0, pos], [0, 8]), pos += 8;

	addmenuback(m);
};

