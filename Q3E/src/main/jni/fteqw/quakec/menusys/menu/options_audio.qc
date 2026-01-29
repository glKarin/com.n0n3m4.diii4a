static class audiomenu : mitem_exmenu
{
	virtual float(string key) isvalid =
	{
		if (key == "sndspeed")
			return super::isvalid(key) && stof(super::get("snd_mixspeed"))==44100;
		return super::isvalid(key);
	};
	virtual string(string key) get =
	{
		if (key == "s_device" || key == "cl_voip_capturedevice")
		{	//the cvar supports multiple options, but we only support one. :(
			//so just return the first to avoid getting too confused.
			tokenize(super::get(key));
			return argv(0);
		}
		return super::get(key);
	};
	virtual void(string key, string val) set =
	{
		if (key == "s_device" || key == "cl_voip_capturedevice")
		{	//add some quotes.
			val = strcat("\"", val, "\"");
		}
		super::set(key, val);
	};
};

nonstatic void(mitem_desktop desktop) M_Options_Audio =
{
	local float pos;
	mitem_exmenu m;
	m = spawn(audiomenu, item_text:_("Audio Options"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	float h = 200 * 0.5;
	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [banner.item_size_x*-0.5, -h-32], [banner.item_size_x*0.5, -h-8]);
	
	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [-160, -h], [160, h*2]);
	
	float fl = RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN;
	pos = 0;

	//add the options
	fr.add(spawn(mitem_text, item_text:_("Restart Sound"), 	item_command:"snd_restart", item_scale:8, item_flags:IF_RIGHTALIGN),		RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [0, pos], [-8, 8]); pos += 8;
	pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcombo_spawn(_("Sound Device"),		"s_device",							'280 8', cvar_string("_s_device_opts")),	fl, [0, pos], [0, 8]); pos += 8;
	fr.add(menuitemslider_spawn(_("Master Volume"),		"volume",			'0.0 1 0.1', 	'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Ambient Volume"),	"ambient_level",	'0 0.5 0.05', 	'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemslider_spawn(_("Self Volume"),		"s_localvolume",	'0 1 0.1', 		'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Music Volume"),		"bgmvolume",		'0 0.5 0.05', 	'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcombo_spawn(_("Channels"),			"s_numspeakers",					'280 8', _(
																"1 \"Mono\" "
																"2 \"Stereo\" "
																"4 \"Quad\" "
																"6 \"Surround\" "
																)),		fl, [0, pos], [0, 8]), pos += 8;
	if (engine==E_QSS)
	fr.add(menuitemcombo_spawn(_("Audio Filtering"),	"sndspeed",	'280 8', _(
																"\"\"	\"Off\" "
																"11025	\"On\" "
																)),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcombo_spawn(_("Audio Quality"),		cv2("snd_mixspeed"/*qs*/, "s_khz"),	'280 8', _(
																"8000	\"8000hz (telephone quality)\" "
																"11025	\"11025hz (vanilla quake)\" "
																"22050	\"22050hz\" "
																"44100	\"44100hz (cd quality)\" "
																"48000	\"48000hz (dvd quality)\" "
																//higher values are probably pointless when source data doesn't go that high, so not going to list them.
																//"96000	\"96000hz (blu-ray quality)\" "
																//"192000	\"192000hz (professional quality)\" "
																)),	fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcheck_spawn(_("Doppler"),			"s_doppler", 						'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcheck_spawn(_("8bit audio"),			cv2("s_loadas8bit"/*fte*/, "loadas8bit"), 					'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcheck_spawn(_("Swap Speakers"),		"s_swapstereo", 					'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Latency"),			cv2("s_mixahead"/*fte*/, "_snd_mixahead"),	'0.1 0.3 0.01',	'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcheck_spawn(_("Disable Sound"),		"nosound",							'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	//ambient fade
	if (engine!=E_QSS)
	fr.add(menuitemcheck_spawn(_("Static Sounds"),		"cl_staticsounds", 					'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcheck_spawn(_("Mix in Background"),	"s_inactive",						'280 8'),		fl, [0, pos], [0, 8]), pos += 8;

	pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcombo_spawn(_("Microphone Device"),	"cl_voip_capturedevice",			'280 8', cvar_string("_cl_voip_capturedevice_opts")),
																fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("VOIP Playback Vol"),	"cl_voip_play",		'0 2 0.1',		'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcheck_spawn(_("VOIP Test"),			"cl_voip_test",						'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("VOIP Record Vol"),	"cl_voip_micamp",	'0 4 0.1',		'280 8'),		fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcombo_spawn(_("VOIP Mode"),			"cl_voip_send",						'280 8', _(
															"0 \"Push-To-Talk\" 1 "
															"\"Voice Activation\" "
															"2 \"Continuous\""
															)),		fl, [0, pos], [0, 8]), pos += 8;
	//VAD threshhold
	//ducking
	//noise cancelation
	fr.add(menuitemcombo_spawn(_("VOIP Codec"),			"cl_voip_codec",			'280 8',_(
															"\"\"	\"Auto\" "
															"0	\"speex (narrow 11khz)\" "
															//"1	\"raw (wasteful)\" "
															"2	\"opus\" "
															"3	\"speex (narrow 8khz)\" "
															"4	\"speex (wide 16khz)\" "
															"5	\"speex (ultrawide 32khz)\" "
															)),		fl, [0, pos], [0, 8]), pos += 8;
														
	fr.add(menuitemslider_spawn(_("Opus bitrate"),		"cl_voip_bitrate",	'0.5 128 0.5','280 8'),		fl, [0, pos], [0, 8]), pos += 8;

	addmenuback(m);
};