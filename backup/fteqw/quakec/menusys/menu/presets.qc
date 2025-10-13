/***************************************************************************
Uses the engine command to apply the preset, and tries an exec instead if a config file exists.
Doesn't track the current one or anything.
Really simple and stupid menu.
no background tint, so the game is still visible so you can preview it.

you should probably remove the fps_preset part. I left it in so that I don't have to bother with providing cfg files. note that this isn't a valid solution for dp, but whatever.
the great thing about providing source is that you can change it.
you can instead just shove your preset settings directly into the quotes instead of exec.
*/

struct
{
	string name;
	string preset;
	string config;
} presets[] =
{
	{"Simple",		"286",		"configs/preset_simple.cfg"},
	{"Fast",		"fast",		"configs/preset_fast.cfg"},
	{"Regression",	"vanilla",		"configs/preset_vanilla.cfg"},
	{"Faithful",	"normal",		"configs/preset_faithful.cfg"},
	{"Nice",		"nice",		"configs/preset_nice.cfg"},
	{"Realtime",	"realtime",		"configs/preset_realtime.cfg"}
};
nonstatic float(string fname) checkfileexists =
{
	if (dp_workarounds)
	{
		float sh = search_begin(fname, FALSE, TRUE);
		float result = FALSE;
		if (sh >= 0)
		{
			result = !!search_getsize(sh);
			search_end(sh);
		}
		return result;
	}
	else
	{
		//FTE's whichpack returns empty if its not in a pack, and null if its not found anywhere.
		if (whichpack(fname))
			return TRUE;
		return FALSE;
	}
};
nonstatic void(mitem_desktop desktop) M_Preset =
{
	local float i;
	mitem_exmenu m;
	m = spawn(mitem_exmenu, item_text:_("Graphical Presets"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();


	float h = 200 * 0.5;
	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_PARENT_MID, [(160-60-banner.item_size_x)*0.5, -h-32], [banner.item_size_x, -h-8]);
	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, -h], [0, h*2]);
	float fl = RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN;

	float fs, y=0;
	fs = search_begin("configs/preset_*.cfg", TRUE, TRUE);	
	float c = search_getsize(fs);
	for (i = 0; i < c; i++)
	{
		string fname = search_getfilename(fs, i);
		string iname = substring(fname, 15, -5);
		string dname = GetFirstLineComment(fname, iname);
		iname = sprintf("exec \"configs/preset_%s.cfg\";vid_reload", iname);
		if (dname && !fr.findchildcmd(iname))
		{
			fr.add(spawn(mitem_text, item_text:dname, item_command:iname, item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, y], '100 16');
			y+=16;
		}
	}
	search_end(fs);	
	if (c <= 0)
	{
		//add the default options
		for (i = 0, float pos = 0; i < presets.length; i++)
		{
			fr.add(spawn(mitem_text, item_text:presets[i].name, item_command:sprintf("fps_preset %s", presets[i].preset), item_scale:16, item_flags:IF_CENTERALIGN), fl, [0, pos], [100, 16]);
			pos += 16;
		}
	}

	//random art for style
	m.addm(spawn (mitem_spinnymodel, item_text: "progs/suit.mdl"), [-160, 12*-16/2], [0, 12*16/2]);

	addmenuback(m);
};
