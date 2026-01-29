#ifdef WORKINDP
	#define show_scoreboard sb_showscores
#endif

#define RSES_NOLERP 1
#define RSES_NOROTATE 2
#define RSES_NOTRAILS 4
#define RSES_NOLIGHTS 8

float show_scoreboard;
string dbgstr;
vector mousepos;

void()	CSQC_Shutdown = {};

//check engine extensions, and give suitable warning
void() checkengineversion =
{
	if (checkextension("FTE_STRINGS"))
	{
		isdp = substring(" ", 0, -1) == "";
		if (isdp)
			print("broken FTE_STRINGS support\n");
	}
	else
		print("no FTE_STRINGS support, this mod might crash... sorry\n");
};


float(float isnew) RefreshOther =
{
	self.drawmask = MASK_ENGINE;
	setmodelindex(self, self.modelindex);

	if (self.entnum <= maxclients)
		RefreshPlayer(isnew);
	else if (self.colormap)
		self.modelindex = 0;	//don't show dead bodies, we do dying ourselves.

	return PREDRAW_AUTOADD;
};



float(string centertext) centerprinted =
{
//warning!
//centertext is a temporary string!
//warning!

	return false;	//we don't want to handle it, let the engine do it.
};

.float classnum;

void(float isnew) CSQC_Ent_Update =
{
	local float classtype;
	local var void(float isnew) fnc;

	classtype = readbyte();

//remove if the class changed.
	if (self.classnum != classtype)
	{
		if (self.removefunc)
			self.removefunc();
		isnew = true;
		self.classnum = classtype;
	}

	fnc = ParseEntityClass[classtype];
	if (!fnc)
		error("Unrecognised entity class: Your csqc needs updating");
	fnc(isnew);

//	if (isnew)
//	print("csqc ", self.model, " updated at ", vtos(self.origin), "\n");
};

void() regcommands =
{
	registercommand("tetris");
	registercommand("skinchooser");
	registercommand("randomskin");
	registercommand("test");
	registercommand("+showscores");
	registercommand("-showscores");
	registercommand("+showteamscores");
	registercommand("-showteamscores");

	registercommand("osgk");
	registercommand("osgk_command");
	registercommand("osgk_resize");
	registercommand("osgk_mousemove");
	registercommand("osgk_keypress");
	
	registercommand("test_viewinfo");
};

float(string str) CSQC_ConsoleCommand =
{
	local float args;
	args = tokenize(str);
	switch(argv(0))
	{
	case "test":
		sendevent("testevent", "fsev", 64, "hello world", player_local, '73 72 71');
		break;
	case "tetris":
		Menu_Tetris();
		break;
	case "skinchooser":
		Menu_SkinChooser();
		break;
	case "randomskin":
		SelectRandomSkin();
		break;
	case "+showscores":
		show_scoreboard = 1;
		return false;
	case "+showteamscores":
		show_scoreboard = 2;
		return false;
	case "-showscores":
	case "-showteamscores":
		show_scoreboard = 0;
		return false;
	case "osgk":
		Menu_OSGK(argv(1));
		break;
	case "osgk_command":
		gecko_navigate(argv(1), argv(2));
		break;
	case "osgk_keypress":
		gecko_keyevent(argv(1), stof(argv(2)), 2);
		break;
	case "osgk_resize":
		gecko_resize(argv(1), stof(argv(2)), stof(argv(3)));
		break;
	case "osgk_mousemove":
		gecko_mousemove(argv(1), stof(argv(2)), stof(argv(3)));
		break;
	case "test_viewinfo":
		if (player_local)
		{
			print(sprintf("View origin is at %v\n", vieworg));
			print(sprintf("local player entity %i is num %g on server, at %v\n", player_local, player_local.entnum, player_local.origin));
		}
		else
			print("Local player entity is not known. Engine is providing the default view origin.\n");
		break;
	default:
		return false;
	}
	return true;
};

void(string line) CSQC_Parse_StuffCmd =
{
	print(sprintf("stufftext: %S\n", substring(line, 0, -2)));
	localcmd(line);
};
float(string msg) CSQC_Parse_CenterPrint =
{
	print(sprintf("centerprint: %S\n", msg));
	cprint(msg);
	return false;
};

void(string msg, float type) CSQC_Parse_Print =
{
//	print("print: ");
	print(msg);

/*	if (dbgstr)
		strunzone(dbgstr);
	dbgstr = strzone(msg);
*/
};

void() CSQC_Ent_Remove =	//the ent disappeared on the server.
{
	if (self.removefunc)
		self.removefunc();
	remove(self);
};

entity viewentity;
//note that a better way of animating weapon firing would be as a response to an event, or as a clientside predicted thing.
void(entity ve) DoThatViewModelThing =
{
	float newframe, newmodel;

	newframe = getstatf(STAT_WEAPONFRAME);
	newmodel = getstatf(STAT_WEAPONMODEL);

	if (newmodel != ve.modelindex)
	{	//changed entirly.
		ve.modelindex = newmodel;
		ve.frame2 = ve.frame = newframe;
		ve.lerptime = time;
	}
	else if (newframe != ve.frame)
	{
		ve.frame2 = ve.frame;
		ve.frame = newframe;
		ve.lerptime = time;
	}
	ve.lerpfrac = 1-(time-ve.lerptime)*10;

	ve.origin = '0 0 0';
	ve.angles = '0 0 0';
};

void(float apilevel, string enginename, float engineversion) CSQC_Init =
{
	__using renderflags;

	checkengineversion();

	precache_model("progs/eyes.mdl");
	precache_model("progs/player.mdl");
	precache_model("progs/missile.mdl");
	precache_sound("zombie/z_gib.wav");
	precache_sound("player/udeath.wav");
	precache_sound("weapons/r_exp3.wav");

	viewentity = spawn();
	viewentity.renderflags = RF_VIEWMODEL|RF_DEPTHHACK;

	regcommands();

	if (checkextension("EXT_CSQC_1"))
	{
//		deltalisten("*", RefreshOther, 0);	//catch-all
		deltalisten("progs/player.mdl", RefreshPlayer, RSES_NOLERP|RSES_NOROTATE);	//animate players/player-corpses
	}
};

void(entity ent) CSQC_DrawViewModel =
{
	__using shaderforname, checkbuiltin, forceshader;

	if (player_local.sveffects & SVE_INVIS)
	{
		if (!checkbuiltin(shaderforname))
			return;
		ent.forceshader = shaderforname("powerups/invisibility");
	}
	else
		ent.forceshader = 0;

	ent.fatness = 0;
	DoThatViewModelThing(ent);

#ifdef WORKINDP
	//DP doesn't support RF_VIEWMODEL
	//so hack around that
	ent.renderflags = RF_DEPTHHACK;	//nope, not a view model, that just breaks things
	ent.origin = vieworg;
	ent.angles = input_angles;
	ent.angles_x *= autocvar(r_meshpitch, -1);
#endif

	addentity(ent);
	if ((player_local.sveffects & SVE_QUAD) && checkbuiltin(shaderforname))
	{
		ent.fatness = -2;
		ent.forceshader = shaderforname("powerups/quad");
		addentity(ent);
	}
	if ((player_local.sveffects & SVE_GOD) && checkbuiltin(shaderforname))
	{
		ent.fatness = -2.8;
		ent.forceshader = shaderforname("powerups/regen");
		addentity(ent);
	}
}

//a bit of fun
void() CSQC_Input_Frame =
{
/*	//swap jump/attack
	if (input_buttons == 1)
		input_buttons = 2;
	else if (input_buttons == 2)
		input_buttons = 1;*/

	if (input_impulse)
		input_buttons |= 32|64|128;

	if (autocvar(test_pmove,0) & 1)
		input_buttons |= 32;	//print server pmoves
	if (autocvar(test_pmove,0) & 2)
		input_buttons |= 64;	//print client pmoves
	if (autocvar(test_pmove,0) & 4)
		input_buttons |= 128;	//print client partials
};

void() CSQC_Delta_Remove =
{
	if (self == player_local)
		player_local = world;

//	print("delt ", ftos(self.modelindex), "(", self.model, ") removed\n");
	if (self.removefunc)
		self.removefunc();
	remove(self);
};


float (float event, float parama, float paramb, float paramc) CSQC_InputEvent =
{
	if (event == IE_KEYDOWN)
	{
		switch(CVARF(cg_editor))
		{
		case 1:
			if (editor_lights_key(parama, paramb))
				return true;
			break;
		case 2:
			if (editor_terrain_key(parama, paramb))
				return true;
			break;
		}
	}
	else if (event == IE_MOUSEABS)
		mousepos = [parama, paramb];
	else if (event == IE_MOUSEDELTA)
	{	//move it...
		mousepos += [parama, paramb];

		//bound it.
		vector ssize = getviewprop(VF_SCREENVSIZE);
		mousepos_x = bound(0, mousepos_x, ssize_x);
		mousepos_y = bound(0, mousepos_y, ssize_y);
	}

	return Menu_InputEvent(event, parama, paramb);
};

void(float width, float height, float do2d) CSQC_UpdateView =
{
	float hudtype = CVARF(cg_hudtype);

	chasecam = CVARF(cg_thirdPerson);
	if (getstatf(STAT_HEALTH) <= 0)
		chasecam = 1;

	clearscene();

	//force fullscreen views (ignore viewsize).
	setviewprop(VF_MIN, '0 0 0');
	setviewprop(VF_SIZE_X, width);
	setviewprop(VF_SIZE_Y, height);
#if 0
	setviewprop(VF_MIN_X, width/2);
	setviewprop(VF_MIN_Y, height/2);
	setviewprop(VF_SIZE_X, width/2);
	setviewprop(VF_SIZE_Y, height/2);
#endif

	if (hudtype != 1)
	{
		setviewprop(VF_DRAWENGINESBAR, 0);
	}
	else
	{
		setviewprop(VF_DRAWENGINESBAR, 1);
	}
	setviewprop(VF_DRAWCROSSHAIR, !player_local || !checkbuiltin(project));

	addentities(MASK_NORMAL|MASK_ENGINE);

	if (player_local)
	{
		setviewprop(VF_ORIGIN, vieworg);
		setviewprop(VF_ANGLES, view_angles);

		makevectors(view_angles);
		SetListener(vieworg, v_forward, v_right, v_up);

		if (!chasecam)
		{
			CSQC_DrawViewModel(viewentity);
		}
	}
	else
	{
		//engine didn't tell us about our player entity. that's not right...
		addentities(MASK_ENGINEVIEWMODEL);
	}

	switch(CVARF(cg_editor))
	{
	case 1:
		/*add light ents*/
		editor_lights_add();
		hudtype = 0;
		break;
	case 2:
		break;
	}

	renderscene();

	if (do2d)
	{
		switch(CVARF(cg_editor))
		{
		case 1:
			/*add light ents*/
			editor_lights_overlay();
			hudtype = 0;
			break;
		case 2:
			break;
		default:
			if (autocvar(cg_imageview, "") != "")	//eg: set cg_imageview "https://www.quaddicted.com/reviews/screenshots/dmd.jpg"
				drawpic([0,0,0], autocvar(cg_imageview, ""), [width,height], [1,1,1],1, 0);
			else if (hudtype)
				Hud_Draw(hudtype, show_scoreboard, width, height);
			//quicky `project` test
			if (player_local)
			{
				if (checkbuiltin(project))
				{
					setviewprop(VF_DRAWCROSSHAIR, 0);
					vector shotorg = player_local.origin+'0 0 16';	//quake is weird.
					traceline(shotorg, shotorg + v_forward*8192, FALSE, player_local);
					drawcharacter(project(trace_endpos)-'4 4', '+', '8 8', '1 1 1',1);
				}
		
				if (autocvar(cg_unprojecttest, FALSE))
				{
					static float nexttime;
					nexttime += clframetime;
					drawcharacter(mousepos-'4 4', '+', '8 8', '1 1 1',1);
					if (nexttime > 1)
					{
						vector near = unproject([mousepos_x,mousepos_y,0]);
						vector far = unproject([mousepos_x,mousepos_y,1]);
						traceline(near, far, FALSE, player_local);
						te_explosion(trace_endpos);
						nexttime-=1;
					}
				}
			}
			
			break;
		}

		Menu_Think();
	}

	if (dbgstr)
	{
//		drawrawstring('16 64 0', dbgstr, '16 16 16', '1 1 1', 0.5);
//		drawstring('16 80 0', dbgstr, '16 16 16', 0.5);
	}
};

void(float vwidth, float vheight, float notmenu) CSQC_UpdateViewLoading =
{
	drawfill([0,0],[vwidth,vheight], [0,0,0],1, 0);

	const vector tsize = '24 24';
	string text = "CSQCTest is Loading!\nPlease wait...";
	vector pos = ([vwidth,vheight]-[8,stringwidth(text,TRUE,tsize)])*0.5;
	drawstring(pos, text, tsize, '1 1 1',1,0);
};
