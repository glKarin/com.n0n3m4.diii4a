static var float c_modellist = -1;
static var float c_skinlist = -1;

static float c_modelnum;
static float c_skinnum;

static string(float num) ModelForNum =
{
	string str;
	float slashpos;
	str = search_getfilename(c_modellist, num);
	str = substring(str, 15, -1);   // models/players/
	slashpos = strstrofs(str, "/");
	return substring(str, 0, slashpos);
}
static string(float num) SkinForNum =
{
	string str;
	float slashpos;
	str = search_getfilename(c_skinlist, num);
	str = substring(str, 15, -1);   // models/players/
	slashpos = strstrofs(str, "/");
	return substring(str, slashpos+7, -6);	// /upper_   .skin-1
}

static void() CheckSkinNumber =
{
	if (c_skinnum < 0)
		c_skinnum = search_getsize(c_skinlist)-1;
	else if (c_skinnum >= search_getsize(c_skinlist))
		c_skinnum = 0;
};

static void() FindPlayerSkins =
{
	string str;

	if (c_modelnum < 0)
		c_modelnum = search_getsize(c_modellist)-1;
	else if (c_modelnum >= search_getsize(c_modellist))
		c_modelnum = 0;

	str = ModelForNum(c_modelnum);

	if (c_skinlist >= 0)
		search_end(c_skinlist);
	c_skinlist = -1;
	if (str != "")
		c_skinlist = search_begin(strcat("models/players/", str, "/upper_*.skin"), true, true);

	if (c_skinlist < 0)
	{
		print("You have no skins available\n");
		return;
	}

	for (c_skinnum = search_getsize(c_skinlist)-1; c_skinnum>0; c_skinnum--)
	{
		if (SkinForNum(c_skinnum) == "default")
			break;	//we found the default, use it as default. :p
	}
};

static void() FindPlayerModels =
{
	if (c_modellist >= 0)
		search_end(c_modellist);
	c_modellist = search_begin("models/players/*/animation.cfg", true, true);
	c_modelnum = 0;
	c_skinnum = 0;

	FindPlayerSkins();
};

static void() DrawSkinChooser =
{
	string modname;
	string skinname;
	if (c_skinlist < 0)
		return;
	if (c_modellist < 0)
		return;

	modname = ModelForNum(c_modelnum);
	skinname = SkinForNum(c_skinnum);
	cprint(modname, "/", skinname);

	chasecam = true;
}

static void(float event, float button, float mousex, float mousey) SkinChooserMenuEvent =
{
	string str;
	if (event == ME_DRAW)
	{
		DrawSkinChooser();
		return;
	}

	if (event == ME_MOUSEMOVE)
	{
		return;
	}
	if (event == ME_KEYDOWN)
	{
		if (button == K_ESCAPE)
		{
			cprint("");
			Menu_Main();

			if (c_skinlist >= 0)
				search_end(c_skinlist);
			if (c_modellist >= 0)
				search_end(c_modellist);
			return;
		}
		if (button == K_ENTER)
		{
			str = SkinForNum(c_skinnum);
			str = strcat(ModelForNum(c_modelnum), "/", str);
			localcmd("skin ");
			localcmd(str);
			localcmd("\n");
		}
		if (button == K_LEFTARROW)
		{
			c_skinnum--;
			CheckSkinNumber();
		}
		if (button == K_RIGHTARROW)
		{
			c_skinnum++;
			CheckSkinNumber();
		}
		if (button == K_UPARROW)
		{
			c_modelnum--;
			FindPlayerSkins();
		}
		if (button == K_DOWNARROW)
		{
			c_modelnum++;
			FindPlayerSkins();
		}
	}
};


nonstatic void() Menu_SkinChooser
{
	c_modellist = -1;
	c_skinlist = -1;
	FindPlayerModels();

	Menu_Activate(SkinChooserMenuEvent);
};

nonstatic void() SelectRandomSkin =
{
	local string str;

	FindPlayerModels();
	c_modelnum = floor(random()*search_getsize(c_modellist));
	FindPlayerSkins();
	c_skinnum = floor(random()*search_getsize(c_skinlist));

	str = SkinForNum(c_skinnum);
	str = strcat(ModelForNum(c_modelnum), "/", str);
	localcmd("skin ");
	localcmd(str);
	localcmd("\n");
};
