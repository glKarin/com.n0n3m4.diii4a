//very very basic server browser.
//this was written as an example for the api, rather than a truely usable server browser.

//WARNING: make sure you only create one server browser widget otherwise they'll fight each other

#ifndef MOD_GAMEDIR
//look up what its meant to be.
//#define MOD_GAMEDIR cvar_string("game")
#endif

#include "../menusys/mitem_menu.qc"		//subwindow

var float autocvar_sb_shownumplayers = 1;
var float autocvar_sb_showmaxplayers = 0;
var float autocvar_sb_showfraglimit = 0;
var float autocvar_sb_showtimelimit = 0;
var float autocvar_sb_showping = 1;
var float autocvar_sb_showgamedir = 0;
var float autocvar_sb_showaddress = 0;
var float autocvar_sb_showmap = 1;
var float autocvar_sb_showname = 1;

//#define COLUMN(width, sortname, title, draw)
#define COLUMN_NUMPLAYERS		COLUMN(2*8,	numplayers,	"Pl", 			ui.drawstring(pos, sprintf("%-2g", gethostcachenumber(field_numplayers, sv)), '8 8', col, 1, 0);)
#define COLUMN_MAXPLAYERS		COLUMN(2*8,	maxplayers,	"MP", 		ui.drawstring(pos, sprintf("%-2g", gethostcachenumber(field_maxplayers, sv)), '8 8', col, 1, 0);)
#define COLUMN_PING			COLUMN(4*8,	ping,			"Ping", 		ui.drawstring(pos, sprintf("%-4g", gethostcachenumber(field_ping, sv)), '8 8', col, 1, 0);)
#define COLUMN_FRAGLIMIT		COLUMN(4*8,	fraglimit,		"FL",			ui.drawstring(pos, sprintf("%-3g", gethostcachenumber(field_fraglimit, sv)), '8 8', col, 1, 0);)
#define COLUMN_TIMELIMIT		COLUMN(4*8,	timelimit,		"TL",			ui.drawstring(pos, sprintf("%-3g", gethostcachenumber(field_timelimit, sv)), '8 8', col, 1, 0);)
#define COLUMN_GAMEDIR		COLUMN(8*8,	gamedir,		"Gamedir",	ui.drawstring(pos, sprintf("%-.8s", gethostcachestring(field_gamedir, sv)), '8 8', col, 1, 0);)
#define COLUMN_ADDRESS		COLUMN(21*8,	address,		"Address",		ui.drawstring(pos, sprintf("%-.21s", gethostcachestring(field_address, sv)), '8 8', col, 1, 0);)
#define COLUMN_MAP			COLUMN(8*8,	map,			"Map",		ui.drawstring(pos, sprintf("%-.8s", gethostcachestring(field_map, sv)), '8 8', col, 1, 0);)
#define COLUMN_HOSTNAME		COLUMN(64*8,	name,		"Name",		ui.drawstring(pos, sprintf("%s", gethostcachestring(field_name, sv)), '8 8', col, 1, 0);)
//FIXME: add a little * icon before the hostname for favourites or something

#define COLUMNS 				COLUMN_NUMPLAYERS	COLUMN_MAXPLAYERS	COLUMN_PING	COLUMN_FRAGLIMIT	COLUMN_TIMELIMIT	COLUMN_GAMEDIR	COLUMN_ADDRESS	COLUMN_MAP	COLUMN_HOSTNAME

class mitem_servers : mitem
{
	float server_selected;
	mitem_vslider slider;
	float dbltime;
	float dobound;

	void() mitem_servers =
	{
		dbltime = cltime - 10;
		this.item_flags |= IF_SELECTABLE;
		server_selected = -1;

		//clear the filter
		resethostcachemasks();
#ifdef MOD_GAMEDIR
		if (MOD_GAMEDIR != "")
		{
			//constrain the list to only servers with the right gamedir.
			sethostcachemaskstring(0, gethostcacheindexforkey("gamedir"), MOD_GAMEDIR, SLIST_TEST_EQUAL);
		}
#endif
		//sort by ping by default
		sethostcachesort(gethostcacheindexforkey("ping"), FALSE);
		//(re)query the servers.
		refreshhostcache();

		resorthostcache();
	};

	virtual void(vector pos) item_draw =
	{
		local float sv, maxsv = gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT);
		local float maxy = self.item_size_y;
		float left = pos_x;
	
		local vector omin = ui.drawrectmin, omax = ui.drawrectmax;
		local vector cmin = pos + '0 8', cmax = pos + item_size;
	
		cmin_x = max(omin_x, cmin_x);
		cmin_y = max(omin_y, cmin_y);
		cmax_x = min(omax_x, cmax_x);
		cmax_y = min(omax_y, cmax_y);

		slider.maxv = maxsv - (floor(maxy/8) - 1);

		//constrain the view the lazy way.
		if (dobound)
		{
			dobound = FALSE;
			if (slider.val < 0)
				slider.val = 0;
			while (slider.val + floor(maxy/8) - 1 <= server_selected)
				slider.val+=1;
			while (server_selected < slider.val && slider.val > 0)
				slider.val-=1;
		}

		float field_name = gethostcacheindexforkey("name");
		float field_ping = gethostcacheindexforkey("ping");
		float field_numplayers = gethostcacheindexforkey("numhumans");
		if (field_numplayers < 0)
			field_numplayers = gethostcacheindexforkey("numplayers");
		float field_maxplayers = gethostcacheindexforkey("maxplayers");
		float field_gamedir = gethostcacheindexforkey("gamedir");
		if (field_gamedir < 0)
			field_gamedir = gethostcacheindexforkey("mod");
		float field_address = gethostcacheindexforkey("cname");
		float field_map = gethostcacheindexforkey("map");
		float field_timelimit = gethostcacheindexforkey("timelimit");
		float field_fraglimit = gethostcacheindexforkey("fraglimit");

		maxy = ceil(maxy/8) - 1;
		if (maxsv  > slider.val + maxy)
			maxsv = slider.val + maxy;
	
		float sort = gethostcachevalue(SLIST_SORTFIELD);
		string colkey = __NULL__;
#define COLUMN(width, sortname, title, draw) if (field_##sortname<0) autocvar_sb_show##sortname = FALSE; if (autocvar_sb_show##sortname) {if (ui.mousepos[0] > pos_x && ui.mousepos[1] < pos_y+8) colkey = #sortname; pos_x += width+8;}
		COLUMNS
#undef COLUMN

		vector col;
		pos_x = left;
#define COLUMN(width, sortname, title, draw) if (autocvar_sb_show##sortname) {col = '1 1 1'; if (sort == field_##sortname) col_z = 0; if (colkey == #sortname) col_x = 0; ui.drawstring(pos, title, '8 8', col, 1, 0); pos_x += width+8;}
		COLUMNS
#undef COLUMN

		//make sure things get cut off if we have too many rows (or fractional start)
		ui.setcliparea(cmin[0], cmin[1], cmax[0] - cmin[0], cmax[1] - cmin[1]);

		pos_y += 8 * (1-(slider.val-floor(slider.val)));
		for (float y=pos_y,	sv = max(0, floor(slider.val)); sv < maxsv; sv+=1)
		{
			col = (sv&1)?'0.1 0.1 0.1':'0.15 0.1 0.05';
			drawfill([left, y], [item_size_x,8], col, 0.8, 0); 
			y += 8;
		}
		for (	sv = max(0, floor(slider.val)); sv < maxsv; sv+=1)
		{
			col = ((server_selected==sv)?'1 1 0':'1 1 1');
	
			//if isproxy
			//if islocal
			//if isfavorite
	
			pos_x = left;
#define COLUMN(width, sortname, title, draw) if (autocvar_sb_show##sortname) {draw   pos_x += width+8; }
			COLUMNS
#undef COLUMN
			pos_y += 8;
		}
	
		ui.setcliparea(omin_x, omin_y, omax_x - omin_x, omax_y - omin_y);
	};
	static string() getgamedircmd =
	{	//sadly the vanilla NQ network protocol gave clients no idea which gamedir the server is actually using.
		//many extended protocols still lack that info, so be sure to try to switch according to the server browser's gamedir info, because its more robust than not even trying.
		float field_gamedir = gethostcacheindexforkey("gamedir");
		if (field_gamedir < 0)
			field_gamedir = gethostcacheindexforkey("mod");
		if (field_gamedir < 0)
			return "";	//erk. dodgy engine?

		string gd = gethostcachestring(field_gamedir, server_selected);
		if (strstrofs(gd, "\"")>=0 || strstrofs(gd, "\n")>=0 || strstrofs(gd, "\r")>=0)
			return "";	//no, just no. trust the engine to reject bad paths, we're just preventing cbuf corruption here (DP doesn't support "%S").
		return sprintf("gamedir \"%s\";", gd);
	};
	virtual float(vector pos, float scan, float char, float down) item_keypress =
	{
		float displaysize;
		string addr;
		/*just sink all inputs*/
		if (!down)
			return FALSE;
		if (scan != K_MOUSE1 && scan != K_TOUCHTAP)
			dbltime = cltime - 10;
		if (scan == K_MOUSE1 || scan == K_TOUCHTAP)
		{
			float news = ui.mousepos[1] - (pos_y+8);
			news = floor(news / 8);
			if (news == -1)
			{
				string colkey = "";
#define COLUMN(width, sortname, title, draw) if (autocvar_sb_show##sortname) {if (ui.mousepos[0] > pos_x) colkey = #sortname; pos_x += width+8;}
				COLUMNS
#undef COLUMN
				if (colkey != "")
				{
					float fld = gethostcacheindexforkey(colkey);
					if (colkey == "numplayers")
					{
						//favour descending order
						sethostcachesort(fld, (gethostcachevalue(SLIST_SORTFIELD) != fld) || !gethostcachevalue(SLIST_SORTDESCENDING));
					}
					else
					{
						//favour ascending order
						sethostcachesort(fld, (gethostcachevalue(SLIST_SORTFIELD) == fld) && !gethostcachevalue(SLIST_SORTDESCENDING));
					}
					resorthostcache();	//tell the engine that its okay to resort everything.
					dobound = TRUE;
				}
			}
			if (news < 0 || news >= gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT))
				return FALSE;
	
			news += floor(slider.val);

			if (server_selected == news && dbltime > cltime)
			{
				//connect on double clicks. because we can.
				addr = gethostcachestring(gethostcacheindexforkey("cname"), server_selected);
				if (addr)
					localcmd(sprintf("m_pop;%sconnect \"%s\"\n", getgamedircmd(), addr));
			}
			else
				server_selected = news;
			dobound = TRUE;
	
			dbltime = cltime + 0.5;
		}
		else if (ISCONFIRMKEY(scan))
		{	//connect normally
			addr = gethostcachestring(gethostcacheindexforkey("cname"), server_selected);
			if (addr)
				localcmd(sprintf("m_pop;%sconnect \"%s\"\n", getgamedircmd(), addr));
		}
		else if (scan == 's' || scan == K_GP_X)
		{	//s = join as a spectator
			addr = gethostcachestring(gethostcacheindexforkey("cname"), server_selected);
			if (addr)
				localcmd(sprintf("m_pop;%sobserve \"%s\"\n", getgamedircmd(), addr));
		}
		else if (scan == 'j')
		{	//j = join as a player
			addr = gethostcachestring(gethostcacheindexforkey("cname"), server_selected);
			if (addr)
				localcmd(sprintf("m_pop;%sjoin \"%s\"\n", getgamedircmd(), addr));
		}
		else if (ISUPARROW(scan) || scan == K_MWHEELUP)
		{
			this.server_selected -= 1;
			if (this.server_selected < 0)
			{
				this.server_selected = gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT);
				if (this.server_selected)
					this.server_selected -= 1;
			}
			dobound = TRUE;
		}
		else if (ISDOWNARROW(scan) || scan == K_MWHEELDOWN)
		{
			this.server_selected += 1;
			if (this.server_selected >= gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT))
				this.server_selected = 0;
			dobound = TRUE;
		}
		else if (scan == K_PGUP)
		{
			displaysize = (item_size[1]-8)/(8);	//this many rows
			displaysize = floor(displaysize*0.5);
			if (displaysize < 1)
				displaysize = 1;
			this.server_selected -= displaysize ;
			if (this.server_selected < 0)
				this.server_selected = 0;
			dobound = TRUE;
		}
		else if (scan == K_PGDN)
		{
			displaysize = (item_size[1]-8)/(8);	//this many rows
			displaysize = floor(displaysize*0.5);
			if (displaysize < 1)
				displaysize = 1;
			this.server_selected += displaysize;
			if (this.server_selected >= gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT))
				this.server_selected = gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT)-1;
			dobound = TRUE;
		}
		else if (scan == K_HOME)
		{
			this.server_selected = 0;
			dobound = TRUE;
		}
		else if (scan == K_END)
		{
			this.server_selected = gethostcachevalue(SLIST_HOSTCACHEVIEWCOUNT)-1;
			if (this.server_selected < 0)
				this.server_selected = 0;
			dobound = TRUE;
		}
		else if (char == 'r' || char == 'R' || scan == K_F5 || scan == K_GP_Y)
		{
			refreshhostcache();
			resorthostcache();
		}
		else
			return FALSE;
		return TRUE;
	};
};

class mitem_servers_players : mitem
{
	mitem_servers listing;

	void() mitem_servers_players =
	{
//		this.item_flags |= IF_SELECTABLE;
	};
	
	virtual void(vector pos) item_draw =
	{
		float player;
		float y;
		float m;
		vector opos = pos;
		if (listing.server_selected < 0)
			return;
		for (player = 0, y = 0; player < 256; player++)
		{
			float key = gethostcacheindexforkey(sprintf("player%g", player));
			if (key < 0)
				break;	//probably DP. this isn't going to work for us.
			string playerinfo = gethostcachestring(key, listing.server_selected);
			if (!playerinfo)
				break;
			tokenize(playerinfo);
			float userid	= stof(argv(0));
			float frags		= stof(argv(1));
			float ontime	= stof(argv(2));
			float ping		= stof(argv(3));
			string name	= argv(4);
			string skin		= argv(5);
			vector top		= stov(argv(6));
			vector bot		= stov(argv(7));

			drawfill(pos,		'16 4',	top, 1, 0);
			drawfill(pos+'0 4 0',	'16 4',	bot, 1, 0);
			drawstring(pos,	sprintf("%g", frags), '8 8', '1 1 1', 1, 0);
			drawstring(pos+'20 0',	name, '8 8', '1 1 1', 1, 0);
			pos_y += 8;
			if (++y == 8)
			{
				y-= 8;
				pos_y = opos_y;
				pos_x += 16*8;
			}
		}
	
		if (y)
		{
			pos_y = opos_y;
			pos_x += 16*6;
			y = 0;
		}

		if (checkbuiltin(drawtextfield))
		{
//			drawtextfield(opos, item_size, 3, gethostcachestring(gethostcacheindexforkey("serverinfo"), listing.server_selected));
			m = tokenizebyseparator(gethostcachestring(gethostcacheindexforkey("serverinfo"), listing.server_selected), "\\");
			for(player = 1; player <= m; player += 2)
			{
				drawtextfield(pos,		'64 8', 6, argv(player));
				drawtextfield(pos+'68 0',	[32*8-40, 8], 3, argv(player+1));
		
				pos_y += 8;
				if (++y == 8)
				{
					y-= 8;
					pos_y -= 8*8;
					pos_x += 32*8;
				}
			}
		}
	};
};


nonstatic void(mitem_desktop desktop) M_Servers =
{
	mitem_menu o;

	if (checkcommand2("menu_servers", FALSE) && argv(1) != "force")
	{
		localcmd("menu_servers\n");
		return;
	}
	
#ifdef CSQC
	if not (checkextension("FTE_CSQC_SERVERBROWSER"))
	{
		print(_("Sorry, your client does not support FTE_CSQC_SERVERBROWSER\n"));
		return;
	}
#endif

	o = (mitem_menu)desktop.findchildtext(_("Servers List"));
	if (o)
		o.totop();
	else
	{
#if 1
		mitem_exmenu m;
		m = spawn(mitem_exmenu, item_text:_("Options"), item_flags:IF_SELECTABLE, item_command:"m_main");
		desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
		desktop.item_focuschange(m, IF_KFOCUSED);
		m.totop();
#else
		mitem_menu m;
		m = menu_spawn(desktop, _("Servers List"), '320 200');
		m.item_command = "m_main";
		m.item_flags |= IF_RESIZABLE;
		desktop.item_focuschange(m, IF_KFOCUSED);
		m.totop();
#endif

		mitem_vslider sl = spawn(mitem_vslider, stride:4, item_flags:IF_SELECTABLE);
		mitem_servers ls = spawn(mitem_servers, slider:sl);
	
		m.add(ls, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '-16 -68');
		m.add(spawn(mitem_servers_players, listing:ls),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, [8*20, -8*8], [0, -8*0]);
		m.add(sl,	RS_X_MIN_PARENT_MAX|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, [-16, 8], [0, -68]);

		//only add checkboxes for field names accepted by this engine.
		if (gethostcacheindexforkey("ping") >= 0)
			m.add(menuitemcheck_spawn(_("Ping"),			"sb_showping", 			[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*8], [8*20, -8*7]);
		if (gethostcacheindexforkey("cname") >= 0)
			m.add(menuitemcheck_spawn(_("Address"),		"sb_showaddress", 		[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*7], [8*20, -8*6]);
		if (gethostcacheindexforkey("map") >= 0)
			m.add(menuitemcheck_spawn(_("Map"),			"sb_showmap", 			[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*6], [8*20, -8*5]);
		if (gethostcacheindexforkey("gamedir") >= 0 || gethostcacheindexforkey("mod") >= 0)
			m.add(menuitemcheck_spawn(_("Gamedir"),		"sb_showgamedir", 		[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*5], [8*20, -8*4]);
		if (gethostcacheindexforkey("numplayers") >= 0)
			m.add(menuitemcheck_spawn(_("NumPlayers"),		"sb_shownumplayers",	[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*4], [8*20, -8*3]);
		if (gethostcacheindexforkey("maxplayers") >= 0)
			m.add(menuitemcheck_spawn(_("MaxPlayers"),		"sb_showmaxplayers",	[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*3], [8*20, -8*2]);
		if (gethostcacheindexforkey("fraglimit") >= 0)
			m.add(menuitemcheck_spawn(_("Fraglimit"),		"sb_showfraglimit", 		[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*2], [8*20, -8*1]);
		if (gethostcacheindexforkey("timelimit") >= 0)
			m.add(menuitemcheck_spawn(_("Timelimit"),		"sb_showtimelimit", 		[8*8, 8]),	RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX | RS_X_MAX_PARENT_MIN|RS_Y_MAX_PARENT_MAX, [0, -8*1], [8*20, -8*0]);
	}
};

