class mitem_playerpreview : mitem_spinnymodel
{
	virtual void(vector pos) item_draw =
	{
		if (checkbuiltin(setcustomskin))
		{
			//if you wanted to get more advanced, you could use q3 skins here.
			if (cvar("noskins")==1)
				setcustomskin(self, "", sprintf("q1upper \"%s\"\nq1lower \"%s\"\n\n", cvar_string("topcolor"), cvar_string("bottomcolor")));
			else if (cvar_string("cl_teamskin") != "")
				setcustomskin(self, "", sprintf("q1upper \"%s\"\nq1lower \"%s\"\nqwskin \"%s\"\n", cvar_string("topcolor"), cvar_string("bottomcolor"), cvar_string("cl_teamskin")));
			else
				setcustomskin(self, "", sprintf("q1upper \"%s\"\nq1lower \"%s\"\nqwskin \"%s\"\n", cvar_string("topcolor"), cvar_string("bottomcolor"), cvar_string("skin")));
		}
	
		super::item_draw(pos);
	};
};

static string() skinopts =
{
	string opts = "";
	float s = search_begin("skins/*.*", TRUE, TRUE);
	if (s < 0)
		return opts;
	float n = search_getsize(s);
	for (float i = 0; i < n; i++)
	{
		string f = substring(search_getfilename(s, i), 6, -5);

		//urgh!
		if (strstrofs(f, "_pants") >= 0)
			continue;
		if (strstrofs(f, "_shirt") >= 0)
			continue;
		if (strstrofs(f, "_bump") >= 0)
			continue;
		if (strstrofs(f, "_norm") >= 0)
			continue;
		if (strstrofs(f, "_luma") >= 0)
			continue;
		if (strstrofs(f, "_glow") >= 0)
			continue;
		if (strstrofs(f, "_gloss") >= 0)
			continue;

		opts = strcat(opts, "\"", f, "\" \"", f, "\" ");
	}
	return opts;
};

var float autocvar_m_pitch = 0.022;
class options_basic : mitem_exmenu
{
	virtual float(string key) isvalid =
	{
		if (key == "m_pitchsign")
			return TRUE;
		if (key == "cl_run")
			return TRUE;
		if (engine == E_QSS)
		{
			if (key == "topcolor")
				return super::isvalid("_cl_color");
			if (key == "bottomcolor")
				return super::isvalid("_cl_color");
		}
		return super::isvalid(key);
	};
	virtual string(string key) get =
	{
		if (key == "m_pitchsign")
			return (autocvar_m_pitch<0)?"1":"0";
		if (key == "cl_run")
			return (stof(super::get("cl_forwardspeed")) > 200)?"1":"0";
		if (engine == E_QSS)
		{
			if (key == "topcolor")
				return ftos(floor(stof(super::get("_cl_color"))/16));
			if (key == "bottomcolor")
				return ftos(stof(super::get("_cl_color"))&15);
		}
		return super::get(key);
	};
	virtual void(string key, string newval) set =
	{
		if (key == "m_pitchsign")
		{
			float invert;
			if (stof(newval))
				invert = autocvar_m_pitch > 0;
			else
				invert = autocvar_m_pitch < 0;
			if (invert)
				cvar_set("m_pitch", ftos(-autocvar_m_pitch));
		}
		else if (key == "cl_run")
		{
			float setbackspeed = (super::get("cl_backspeed") != "");
			if (stof(newval))
			{
				super::set("cl_forwardspeed", "400");
				super::set("cl_sidespeed", "400");
				if (setbackspeed)
					super::set("cl_backspeed", "400");
				super::set("cl_movespeedkey", "0.5");	//makes +speed act like +walk
			}
			else
			{
				//these are the defaults from winquake.
				super::set("cl_forwardspeed", "200");
				super::set("cl_sidespeed", "350");
				if (setbackspeed)
					super::set("cl_backspeed", "200");
				super::set("cl_movespeedkey", "2.0");
			}
		}
		else if (engine == E_QSS && (key == "topcolor" || key == "bottomcolor"))
		{
			float c = stof(super::get("_cl_color"));
			if (key == "topcolor")
				c = (c&~0xf0)|(stof(newval)*16);
			else
				c = (c&~0x0f)|stof(newval);
			super::set("_cl_color", ftos(c));
		}
		else
			super::set(key, newval);
	};
};
nonstatic void(mitem_desktop desktop) M_Options_Basic =
{
	mitem_exmenu m;
	m = spawn(options_basic, item_text:_("Basic Options"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	float h = 200 * 0.5;
	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/ttl_cstm.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_PARENT_MID, [(160-160-banner.item_size_x)*0.5, -h-32], [banner.item_size_x, -h-8]);
	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [-160, -h], [160, h*2]);
	float fl = RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN;
	float pos = 0;
	

	fr.add(menuitemeditt_spawn(_("Player Name"), 		cv2("_cl_name"/*nq*/, "name"),					'280 8'), 	fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemeditt_spawn(_("Player Team"), 		"team",											'280 8'), 	fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemcombo_spawn(_("Player Skin"), 		"skin",											'280 8', 	skinopts()), fl, [0, pos], [0, 8]), pos += 8;


	fr.add(menuitemcolour_spawn(_("Upper Colour"),		"topcolor",						engine==E_QSS,	'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcolour_spawn(_("Team Colour"),		"bottomcolor",					engine==E_QSS,	'280 8'),	fl, [0, pos], [0, 8]), pos += 8;	/*aka: arse colour, used for .team field in nq*/
	pos += 8;
	fr.add(menuitemcheck_spawn (_("Always Run"),		"cl_run",										'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemcheck_spawn (_("Invert Mouse"),		"m_pitchsign",									'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Sensitivity"),		"sensitivity",					'3 20 1',		'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Fov"),				"fov",							'80 130 5',		'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemslider_spawn(_("Viewmodel Fov"),		"r_viewmodel_fov",				'80 130 5',		'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Gamma"),				cv2("v_gamma", "gamma"),		'0.4 1.3 0.1',	'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	fr.add(menuitemslider_spawn(_("Contrast"),			cv2("v_contrast", "contrast"),	'0.8 1.8 0.1',	'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	if (engine!=E_QSS)
	fr.add(menuitemslider_spawn(_("Brightness"),		cv2("v_brightness", "brightness"),'0.0 0.5 0.1','280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	if (engine==E_FTE)
		fr.add(menuitemslider_spawn(_("Crosshair"),		"crosshair",					'0.0 19 1',		'280 8'),	fl, [0, pos], [0, 8]), pos += 8;
	else
		fr.add(menuitemcheck_spawn(_("Crosshair"),		"crosshair",									'280 8'),	fl, [0, pos], [0, 8]), pos += 8;

	m.add(spawn (mitem_playerpreview, item_text: "progs/player.mdl", firstframe:0, framecount:6, shootframe:119, shootframes:6), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-200, 12*-16/2], [-40, 12*16/2]);

	addmenuback(m);
};
