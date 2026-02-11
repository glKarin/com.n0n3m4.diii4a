class options_effects : mitem_exmenu
{
//	virtual float(string key) isvalid = {};
	virtual string(string key) get =
	{
		if (key == "r_replacemodels")
			return super::get(key)!=""?"1":"0";	//make it a boolean
		return super::get(key);
	};
	virtual void(string key, string newval) set =
	{
		if (key == "r_replacemodels")	//convert from boolean to some arbitrary list.
			newval = stof(newval)?"iqm md5mesh md3":"";
		super::set(key, newval);
	};
};

nonstatic void(mitem_desktop desktop) M_Options_Effects =
{
	mitem_exmenu m;
	m = spawn(options_effects, item_text:_("Effects Options"), item_flags:IF_SELECTABLE, item_command:"m_options");
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

	fr.add(menuitemcheck_spawn(_("Show Framerate"),			cv3("showfps", "scr_showfps", "show_fps"), 		'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcheck_spawn(_("High Res Textures"),		"gl_load24bit", 			'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcombo_spawn(_("Replacement Models"),		"r_replacemodels",			'280 8', _(
																			"\"\" \"Off\""
																			"\"md3 md2 md5mesh\" \"On\""
																)),	fl, [0, pos], [0, 8]); pos += 8;

	fr.add(menuitemcombo_spawn(_("Texture Mode"),		"gl_texturemode",			'280 8', _(
																			"GL_NEAREST	\"Nearest\" "
																			"GL_NEAREST_MIPMAP_NEAREST	\"Nearest (Nearest mips)\" "
																			"GL_NEAREST_MIPMAP_LINEAR	\"Nearest (Linear mips)\" "
																			"GL_LINEAR	\"Linear\" "
																			"GL_LINEAR_MIPMAP_NEAREST	\"Linear (Nearest mips)\" "
																			"GL_LINEAR_MIPMAP_LINEAR	\"Linear (Linear mips)\" "

																			"n.l	\"Nearest (Smooth)\" "
																			//"l.n	\"Linear (Unsmooth)\" "

																			//"nnl	\"Nearest (Nearest mips, Smooth)\" "
																			//"lnn	\"Linear (Nearest mips, Unsmooth)\" "
																			"nll	\"Nearest (Smooth mips)\" "
																			//"lln	\"Linear (Linear mips, Unsmooth)\" "
																)),	fl, [0, pos], [0, 8]); pos += 8;

	fr.add(menuitemcombo_spawn(_("Anistrophy"),		cv2("gl_texture_anisotropy"/*qs*/, "gl_texture_anisotropic_filtering"/*old fte*/),				'280 8', _(
																			"0	\"Off\" "
																			"2	\"2\" "
																			"4	\"4\" "
																			"8	\"8\" "
																			"16	\"16\" "
																)),	fl, [0, pos], [0, 8]); pos += 8;

	fr.add(menuitemcheck_spawn(_("Replacement Models"),		"r_replacemodels",			'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcheck_spawn(_("Double-bright Models"),	"gl_overbright_models", 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemcheck_spawn(_("Lightmaps Only"),			"r_lightmap", 				'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	if (engine == E_QSS)
	{
		fr.add(menuitemcheck_spawn(_("Wireframe"),			"r_showtris", 				'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Simple DLights"),		"gl_flashblend", 			'280 8'),	fl, [0, pos], [0, 8]); pos += 8;

		fr.add(menuitemslider_spawn(_("Stereo"),			"r_stereo",	'0 8 0.25',		'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
	}
	else
	{
		fr.add(menuitemcheck_spawn(_("Wireframe"),			"r_wireframe", 				'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Bloom"),				"r_bloom",					'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Simple Textures"),	"r_drawflat", 				'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Paletted Rendering"),	"r_softwarebanding", 		'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("HDR"),				"r_hdr_irisadaptation", 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Coronas"),			"r_coronas", 				'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Relief Mapping"),		"r_glsl_offsetmapping", 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("RT Dynamic Lights"),	"r_shadow_realtime_dlight", '280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("RT World Lighting"),	"r_shadow_realtime_world", 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Raytraced Shadows"),	"r_shadow_raytrace",		'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcheck_spawn(_("Half-Rate Shading"),	"r_halfrate",				'280 8'),	fl, [0, pos], [0, 8]); pos += 8;

		fr.add(menuitemcombo_spawn(_("Water Effects"),			"r_waterstyle",			'280 8', _(
																			"1	\"Classic\" "
																			"2	\"Ripples\" "
																			"3	\"Reflections\" "
																)),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcombo_spawn(_("View Projection"),		"r_projection",			'280 8', _(
																			"0	\"Standard\" "
																			"1	\"Stereographic\" "
																			"2	\"Fish-Eye\" "
																			"3	\"Panoramic\" "
																			"4	\"Lambert Azimuthal Equal-Area\" "
																			"5	\"Equirectangular\" "
																			"6	\"Panini\" "
																)),	fl, [0, pos], [0, 8]); pos += 8;
		fr.add(menuitemcombo_spawn(_("View Projection Fov"),		"ffov",				'280 8', _(
																			"90	\"Normal\" "
																			"180	\"180 degrees\" "
																			"270	\"270 degrees\" "
																			"360	\"360 degrees\" "
																)),	fl, [0, pos], [0, 8]); pos += 8;
	}
	fr.add(menuitemslider_spawn(_("Particle Density"),		"r_part_density",	'0.25 4 0.25',	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;

	fr.add(spawn(mitem_text, item_text:_("Apply"), 	item_command:"vid_restart", item_scale:8, item_flags:IF_RIGHTALIGN),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [0, pos], [-8, 8]); pos += 8;

	addmenuback(m);
};

