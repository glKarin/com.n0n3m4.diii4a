//maxclients is a QW thing. NQ engines use maxplayers (which requires a disconnect to apply)
#include "../menusys/mitem_grid.qc"

static string newgameinfo;

nonstatic void(mitem_desktop desktop) M_Dir =
{	//implementing this primarily so its available in QSS. :P
	string path = argv(1);
	string pack = argv(2);
	searchhandle h;
	if (path == "")
	{
		print("m_dir <FILEPATH> [PACKAGE]\n");
		return;
	}
	if (pack)
		h = search_begin(path, SB_FULLPACKAGEPATH|SB_FORCESEARCH|16, 0, argv(2));
	else
		h = search_begin(path, SB_FULLPACKAGEPATH|16|32, 0);
	print(sprintf("Directory listing of %S (%g files)\n", path, search_getsize(h)));
	for (float i = 0; i < search_getsize(h); i++)
	{
		float sz = search_getfilesize(h,i);
		string szfmt = "b";
		if (sz > 512*1024*1024)
			sz /= 1024*1024*1024, szfmt = "gb";
		else if (sz > 512*1024)
			sz /= 1024*1024, szfmt = "mb";
		else if (sz > 512)
			sz /= 1024, szfmt = "kb";
		
		print(sprintf("  %S (%.4g %s, %s, %S)\n",
			search_getfilename(h,i),
			sz,szfmt,
			search_getfilemtime(h,i),
			search_getpackagename(h,i)
			));

#if 1
		float f = search_fopen(h,i);
		if (f>=0)
		{
			print(sprintf("^`u8:" "\t%S\n", substring(fgets(f), 0, 64)));
			fclose(f);
		}
		else
			print("\tsearch_fopen failure\n");
#endif
	}
	search_end(h);
};
nonstatic void(mitem_desktop desktop) M_FOpen =
{
	float f = fopen(argv(1), FILE_READ);
	string s;
	if (f>=0)
	{
		while ((s=fgets(f)))
			print(sprintf("^`u8:" "%s\n", s));
		fclose(f);
		print("<EOF>\n");
	}
	else
		print("fopen failure\n");
};

static string(string name) basemapname =
{
	if (!strncmp(name, "maps/", 5))
		name = substring(name, 5, -1);
	if (substring(name, -4, -1) == ".bsp")
		name = substring(name, 0, -5);
	return name;
};
static string(string name) enginemapname =
{
	float ofs = strstrofs(name, ":");
	if (ofs<0)
	{
		if (!strncmp(name, "maps/", 5))
			name = substring(name, 5, -1);
		if (substring(name, -8, -1) == ".bsp.gz")
			name = strcat(substring(name, 0, -9),"^8.gz");	//fiddle a little with its display
		if (substring(name, -4, -1) == ".bsp")
			name = substring(name, 0, -5);
	}
	else
	{	//faff with the cullers to try to make it more readable.
		name = sprintf("^8%s:^7%s", substring(name, 0, ofs), substring(name, ofs, -1));
	}
	return name;
};

#define FOURCC(a,b,c,d) ((int)(a)<<0i)|((int)(b)<<8i)|((int)(c)<<16i)|((int)(d)<<24i)
static string(string name) getmapdesc =
{
	if (strstrofs(name, ":")>=0)
	{	//FIXME: look up the package's title?
		return strcat("^8", basemapname(name));
	}
	if (checkbuiltin(fread) && checkbuiltin(fseek) && checkbuiltin(memalloc) && checkbuiltin(memfree) && checkbuiltin(memgetval) && checkbuiltin(itof) && checkbuiltin(ftoi))
	{	//we can do it, so do it.
		filestream f = fopen(name, FILE_READ);
		if (f < 0)
			print(sprintf("Unable to read %s\n", name));
		
		name = basemapname(name);
		if (f < 0)
			return name;

		int *header = memalloc(sizeof(int)*4);
		int bspver = 0, entofs, entlen;
		if (fread(f, (void*)header, sizeof(int)*4) == sizeof(int)*4 && ((bspver=header[0]),(
			bspver == FOURCC('I','B','S','P') ||	//IBSP (q2/q3)
			bspver == FOURCC('R','B','S','P') ||	//RBSP (jk2o etc)
			bspver == FOURCC('F','B','S','P') ||	//IBSP (qfusion/warsow)
			bspver == 29i ||	//q1
			bspver == 30i ||	//hl
			bspver == FOURCC('B','S','P','2'))))	//bsp2
		{
			if (bspver == FOURCC('I','B','S','P'))
			{	//has an actual version number! ooo...
				entofs = header[2];
				entlen = header[3];
			}
			else
			{
				entofs = header[1];
				entlen = header[2];
			}
			fseek(f, entofs);
			string s = (string)memalloc(entlen+1);
			fread(f, (void*)s, entlen);
			float argc = tokenize(s);
			if (argv(0) == "{")
			{
				for (float p = 1; p < argc; p+=2)
				{
					string t = argv(p);
					if (t == "message")
					{	//finally found the human-readable name of the map. woo.
						name = argv(p+1);
						break;
					}
					if (t == "}")	//don't read the message from some kind of trigger
						break;
					if (t == "{")	//some sort of corruption
						break;
				}
			}
			else
				name = "ERROR";
			memfree((void*)s);
		}
		else
			name = sprintf("UNSUPPORTED %i", bspver);
		memfree(header);
		fclose(f);
		return name;
	}
	return "";	//we can't do that in this engine... don't show anything, because its pointless showing the same thing twice.
};

static string(string name) packagetogamedir =
{
	float so = strstrofs(name, "/");
	if (so>=0)
		name = substring(name, 0, so);

	//don't hide id1/dm4 etc just because the user previously auto-downloaded eg qw/aerowalk
	name = strtolower(name);
	if (name == "qw" || name == "fte")
		name = "id1";

	return name;
};
class mitem_maplist : mitem_grid
{
	strbuf	names;
	strbuf	descs;
	virtual void() item_remove =
	{
		buf_del(names);
		buf_del(descs);
		super::item_remove();
	};
	void() mitem_maplist =
	{
		searchhandle searchtable;
		searchtable = search_begin("maps/*.bsp:maps/*.bsp.gz", SB_FULLPACKAGEPATH|SB_MULTISEARCH, 0);
		float files = search_getsize(searchtable);
		if (files && checkbuiltin(search_getpackagename))
		{	//find which gamedir the first entry is in (highest-priority is first)
			string gd = packagetogamedir(search_getpackagename(searchtable,0));

			for (float count = 1; count < files; count++)
			{
				string mgd = packagetogamedir(search_getpackagename(searchtable,count));
				if (mgd != gd)
				{	//the gamedir changed... truncate the list here.
					files = count;
					break;
				}
			}
		}
		grid_numchildren = 0;
		names = buf_create();
		for (float count = 0; count < files; count++)
		{
			string n = search_getfilename(searchtable, count);
			if (!strncmp(n, "maps/_", 6) || !strncmp(n, "maps/b_", 7))
				continue;	//don't add b_* names nor _*
			bufstr_add(names, n, TRUE);
		}
		search_end(searchtable);

		for (float pkg = 0; ; pkg++)
		{
			string n = getpackagemanagerinfo(pkg, GPMI_NAME);
			if not(n)
				break;	//end of list.
			string maps = getpackagemanagerinfo(pkg, GPMI_MAPS);
			if not(maps)
				continue;	//none? not a mappack? don't waste time on the tokenizing.
			float eq = strstrofs(n, "=");
			if (eq >= 0)
				n = substring(n, 0, eq);
			float c = tokenize_console(maps);
			for (float i = 0; i < c; i++)
				bufstr_add(names, sprintf("%s:%s", n, argv(i)), TRUE);
		}

		buf_sort(names, 0, FALSE);	//make sure they're sorted now.
		grid_numchildren = buf_getsize(names);
		descs = buf_create();

		item_resized();	//FIXME: is this needed?
		grid_kactive = grid_numchildren?0:-1;
		grid_selectionchanged(-1,grid_kactive);
	};
	virtual void(vector pos, float idx) grid_draw =
	{
		string map = bufstr_get(names, idx);
		string desc = bufstr_get(descs, idx);
		if not(desc)
		{
			desc = map;
			static float lasttime;
			if (time != lasttime)
			{	//fix up the name and cache it.
				lasttime = time;
				desc = getmapdesc(map);
				bufstr_set(descs, idx, desc);
			}
		}
		map = basemapname(map);

		vector col = (map == get("map"))?[1.16, 0.54, 0.41]:'1 1 1';
		if (item_flags & IF_MFOCUSED && idx == grid_mactive)
			col_z = 0; 
		if (item_flags & IF_KFOCUSED && idx == grid_kactive)
			col_x = 0;

		vector valuepos = [pos_x+item_size_x/2, pos_y]; 
		pos_x = valuepos_x - stringwidth(map, TRUE, '1 1 0'*this.item_scale) - 8;
		valuepos_x += 1;
		ui.drawstring(pos, map, '1 1 0' * item_scale, col, item_alpha, 0);
		ui.drawstring(valuepos, desc, '1 1 0' * item_scale, col, item_alpha, 0);
	};
	virtual float(vector pos, float scan, float chr, float down, float idx) grid_keypress =
	{
		if (down)
			if (ISCONFIRMKEY(scan) || ((scan == K_MOUSE1 || scan == K_TOUCHTAP) && mouseinbox(pos, this.item_size)))
			{
				string map = bufstr_get(names, idx);
				map = enginemapname(map);
				set("map", map);
				return TRUE;
			}
		return FALSE;
	};
//	virtual void(float olditem, float newitem) grid_selectionchanged;
};

class mitem_newgame : mitem_exmenu
{
	virtual float(string key) isvalid =
	{
		return TRUE;
	};
	virtual string(string key) get =
	{
		return infoget(newgameinfo, key);
	};
	virtual void(string key, string newval) set =
	{
		string old = newgameinfo;
		newgameinfo = strzone(infoadd(newgameinfo, key, newval));
		if (old)
			strunzone(old);
	};
};

nonstatic void(mitem_desktop desktop) M_NewGame =
{
	mitem_pic banner;
	string gametype = argv(1);
	local float pos;
	mitem_exmenu m;
	if (gametype == "sp")
	{
		//single player has no options. the start map itself gives skill+episode options.
		//use 0 for maxplayers. this allows splitscreen to allocate more.
		localcmd("\ndisconnect; deathmatch 0; coop 0; maxplayers 0; timelimit 0; fraglimit 0; teamplay 0; samelevel 0; startmap_sp\n");
		return;
	}
	if (gametype == "begin")
	{
		cvar_set("hostname", 		infoget(newgameinfo, "hostname"));
		cvar_set("deathmatch", 	infoget(newgameinfo, "deathmatch"));
		cvar_set("coop", 		infoget(newgameinfo, "coop"));
		cvar_set("teamplay", 		infoget(newgameinfo, "teamplay"));
		cvar_set("sv_public", 	infoget(newgameinfo, "sv_public"));
		cvar_set("timelimit", 	infoget(newgameinfo, "timelimit"));
		cvar_set("fraglimit", 	infoget(newgameinfo, "fraglimit"));
		string map = 			infoget(newgameinfo, "map");
		if (map == "")
		{
			if (infoget(newgameinfo, "deathmatch"))
				map = sprintf("dm%g", floor(random(1, 6)));
			else
				map = "start";
		}

		if (engine == E_QSS)
		{
			if (stof(infoget(newgameinfo, "maxclients")) != cvar("maxplayers"))
				localcmd(sprintf("\ndisconnect; maxplayers %S\n", infoget(newgameinfo, "maxclients")?:"0"));
		}
		else
			cvar_set("maxclients", 	infoget(newgameinfo, "maxclients"));

		//'map' acts slightly differently in different engines.
		//qss: kicks everyone, resets all parms
		//fte: just resets parms.
		//whereas 'changelevel' works more predictably, but does mean you might get more weapons than were really intended.
		if (engine == E_QSS && gametype == "warp")
			localcmd(sprintf("\nchangelevel %S\n", map));
		else
			localcmd(sprintf("\nmap %S\n", map));
		return;
	}

	if (newgameinfo)
		strunzone(newgameinfo);
	newgameinfo = "";
	newgameinfo = infoadd(newgameinfo, "hostname", cvar_string("hostname"));
	newgameinfo = infoadd(newgameinfo, "deathmatch", cvar_string("deathmatch"));
	newgameinfo = infoadd(newgameinfo, "teamplay", cvar_string("teamplay"));
	newgameinfo = infoadd(newgameinfo, "sv_public", cvar_string("sv_public"));
	newgameinfo = infoadd(newgameinfo, "maxclients", cvar_string((engine == E_QSS)?"maxplayers":"maxclients"));
	newgameinfo = infoadd(newgameinfo, "timelimit", cvar_string("timelimit"));
	newgameinfo = infoadd(newgameinfo, "fraglimit", cvar_string("fraglimit"));
	newgameinfo = strzone(newgameinfo);

	//create the menu
	m = spawn(mitem_newgame, item_text:_("New Game"), item_flags:IF_SELECTABLE, item_command:"m_main");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

#if 1
	mitem_frame fr = m;
#else
	//create a frame for its scrollbar.
	float h = 100;
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [-160, -h], [160, h*2]);
#endif

	switch(gametype)
	{
	case "tdm":
	case "dm":
	case "coop":
	case "sp":
	case "warp":
		break;
	default:
		//show game type selection
		pos = (16/-2)*(4);
		banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
		m.addm(banner, [(160-banner.item_size_x)*0.5, pos-32], [(160+banner.item_size_x)*0.5, pos-8]);
		m.addm(spawn(mitem_text, item_text:"Single Player", item_command:"m_pop;m_newgame sp", item_scale:16, item_flags:IF_CENTERALIGN), [0, pos], [160, pos+16]);	pos += 16;
		m.addm(spawn(mitem_text, item_text:"Cooperative", item_command:"m_pop;m_newgame coop", item_scale:16, item_flags:IF_CENTERALIGN), [0, pos], [160, pos+16]);	pos += 16;
		m.addm(spawn(mitem_text, item_text:"Deathmatch", item_command:"m_pop;m_newgame dm", item_scale:16, item_flags:IF_CENTERALIGN), [0, pos], [160, pos+16]);	pos += 16;
		m.addm(spawn(mitem_text, item_text:"Team Deathmatch", item_command:"m_pop;m_newgame tdm", item_scale:16, item_flags:IF_CENTERALIGN), [0, pos], [160, pos+16]);	pos += 16;

		m.addm(spawn(mitem_text, item_text:"Map Selection", item_command:"m_pop;m_newgame warp", item_scale:16, item_flags:IF_CENTERALIGN), [0, pos], [160, pos+16]);	pos += 16;

		m.add(spawn (mitem_spinnymodel, item_text: "progs/soldier.mdl",firstframe:73, framecount:8, shootframe:81, shootframes:9), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, 12*-16/2], [0, 12*16/2]);
		return;
	}
	
	pos = -100;

	banner = spawn(mitem_pic, item_text:"gfx/p_multi.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.addm(banner, [(0-banner.item_size_x)*0.5, pos-32], [(0+banner.item_size_x)*0.5, pos-8]);

	if (gametype != "warp")
	{
		fr.addm(menuitemeditt_spawn(_("Hostname"), 		"hostname", '280 8'), 					[-160, pos], [160, pos+8]); pos += 8;
		fr.addm(menuitemcombo_spawn(_("Public"),			"sv_public",				'280 8', 	(engine == E_QSS)?_(	"0	\"LAN\" "
																			"1	\"Internet\" "
																		):_(	"-1	\"Reject All (Splitscreen)\" "
																			"0	\"Private (Manual IP Sharing)\" "
																			"1	\"Public (Manual Config)\" "
																			"2	\"Public (Holepunch)\" "
																			)),	[-160, pos], [160, pos+8]); pos += 8;
		fr.addm(menuitemcombo_spawn(_("Max Clients"),	"maxclients",				'280 8', _(
																			"2	\"Two\" "
																			"3	\"Three\" "
																			"4	\"Four\" "
																			"8	\"Eight\" "
																			"16	\"Sixteen\" "
																			"32	\"Thirty Two\" "
																			)),	[-160, pos], [160, pos+8]); pos += 8;
	}

	if (gametype == "dm" || gametype == "tdm")
	{
		if (m.get("deathmatch") == "0")
		{
			m.set("deathmatch", "1");
			m.set("coop", "0");
		}
		fr.addm(menuitemcombo_spawn(_("Deathmatch Mode"),	"deathmatch",					'280 8', _(
																			"1	\"Weapons Respawn\" "
																			"2	\"Weapons Stay\" "
																			"3	\"Powerups Respawn\" "
																			"4	\"Start With Weapons\" "
																			)),	[-160, pos], [160, pos+8]); pos += 8;
	}
	else
	{
		if (m.get("coop") == "0")
		{
			m.set("deathmatch", "0");
			m.set("coop", "1");
		}
	}
	if (gametype == "tdm")
	{
		if (m.get("teamplay") == "0")
			m.set("teamplay", "1");
	}
	if (gametype == "dm")
	{
		if (m.get("teamplay") != "0")
			m.set("teamplay", "0");
	}
	if (gametype == "coop")
		fr.addm(menuitemcheck_spawn(_("No Friendly Fire"),		"teamplay", 			'280 8'),	[-160, pos], [160, pos+8]), pos += 8;
//	if (gametype == "dm" || gametype == "tdm")

	if (gametype == "coop")
		m.set("map", "start");
	else
	{
		m.set("map", "");

		if (gametype != "warp")
		{
			fr.addm(menuitemcombo_spawn(_("Time Limit"),	"timelimit",					'280 8', _(
																			"0	\"No Limit\" "
																			"5	\"5 minutes\" "
																			"10	\"10 minutes\" "
																			"15	\"15 minutes\" "
																			"20	\"20 minutes\" "
																			"25	\"25 minutes\" "
																			"30	\"30 minutes\" "
																			"35	\"35 minutes\" "
																			"40	\"40 minutes\" "
																			"45	\"45 minutes\" "
																			"50	\"50 minutes\" "
																			"55	\"55 minutes\" "
																			"60	\"1 hour\" "
																			)),	[-160, pos], [160, pos+8]); pos += 8;
			fr.addm(menuitemcombo_spawn(_("Frag Limit"),	"fraglimit",					'280 8', _(
																			"0	\"No Limit\" "
																			"10	\"10 frags\" "
																			"20	\"20 frags\" "
																			"30	\"30 frags\" "
																			"40	\"40 frags\" "
																			"50	\"50 frags\" "
																			"60	\"60 frags\" "
																			"70	\"70 frags\" "
																			"80	\"80 frags\" "
																			"90	\"90 frags\" "
																			"100	\"100 frags\" "
																			)),	[-160, pos], [160, pos+8]); pos += 8;
		}

		pos += 8;
		fr.addm(spawn(mitem_maplist, item_scale:8, item_flags:IF_CENTERALIGN|IF_SELECTABLE), [-160, pos], [150, 100-16-8]), pos = 100-16;
	}

	fr.addm(spawn(mitem_text, item_text:"BEGIN!", item_command:"m_pop;m_newgame begin", item_scale:16, item_flags:IF_CENTERALIGN), [-160, pos], [160, pos+16]);pos += 16;


	//random art for style
	if (gametype == "coop")
	{
		m.add(spawn (mitem_spinnymodel, item_text: "progs/soldier.mdl", firstframe:73, framecount:8, shootframe:81, shootframes:9), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, -240/2], [0, 240/2]);
	}
	else
	{
		m.add(spawn (mitem_spinnymodel, item_text: "progs/player.mdl", firstframe:0, framecount:6, shootframe:119, shootframes:6), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, 12*-16/2], [0, 12*16/2]);
	}

	addmenuback(m);
};
