/*
The main / root menu.
Just a load of text with console commands attached.
Choice of buttons available is somewhat dynamic.

There's also some generic kludge crap in here, like menu background tints
*/

enum
{
	E_FTE,
	E_QSS,
} engine; //For use ONLY with known possible cvar values.
void() FingerprintEngine =
{	//this is evil.
	//different engines have different values/meanings for different cvars.
	//basically we end up having no choice but to guess which engine we're running in, otherwise we can't tailor all the options properly.

	string e = cvar_string("pr_engine");
	if (strstrofs(e, "QSS")>=0)
		engine = E_QSS;
	else if (strstrofs(e, "FTE")>=0)
		engine = E_FTE;
	else
		engine = E_FTE;	//don't really know.
};

/*
Adds a background tint to a (typically) exmenu parent.
In FTE, we use built-in stuff to give a sepia effect.
In DP, we just tint it black.
*/
nonstatic void(mitem_frame m) addmenuback =
{
	if (iscachedpic("menutint"))	//fte internal hacks! meh, admit it. its cool.
		m.add(spawn (mitem_pic, item_text:"menutint", item_alpha:0.5),
												RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, [0, 0], [0, 0]);
	else
		m.add(spawn (mitem_fill, item_rgb:'0 0 0.01', item_alpha:0.5),
												RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, [0, 0], [0, 0]);
};

/*
helper functions to avoid blowing up in older clients.

*/
#define cv2(dpc,qwc) (cvar_type(dpc)?dpc:qwc)
#define cv3(dpc,qs,qwc) (cvar_type(dpc)?dpc:(cvar_type(qs)?qs:qwc))
#ifdef CSQC
	#define ISMENUQC	FALSE
#else
	#define ISMENUQC	TRUE
#endif
float(string cmd, float assumption) checkcommand2 =
{
	__using checkcommand;
	if (!checkextension("FTE_QC_CHECKCOMMAND"))
		return assumption;
	return checkcommand(cmd);
};
float(__variant cmd, float assumption) checkbuiltin2 =
{
	if (!checkbuiltin(checkbuiltin))	//teehee. if the #0 stuff isn't working (DP) then it ends up calling an OP_DONE(0) instruction giving a return false.
		return assumption;
	return checkbuiltin(cmd);
};
#define checkbuiltin2(c,a) ((a)?checkbuiltin2(c,a):checkbuiltin(c))

#ifdef CSQC
float() clientstate =
{
	if (serverkey("constate") != "disconnected")
		return 2;
	return 1;
};
#endif

nonstatic void(mitem_desktop desktop) M_Main =
{
	local float y;
	local mitem_exmenu m;

	//no dupes please.
	m = (mitem_exmenu)desktop.findchildtext(_("Main Menu"));
	if (m)
	{
		m.totop();
		return;
	}

	//create a fullscreen frame
	m = spawn(mitem_exmenu, item_text:_("Main Menu"), item_flags:IF_SELECTABLE);
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

//	m.item_flags |= IF_NOKILL;
//	m.adda(menuitempic_spawn ("gfx/qplaque.lmp", 	'32 144'), 	'16 4');

	y = 7*-16/2;

	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/ttl_main.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [(160-banner.item_size_x)*0.5, y-32], [(160+banner.item_size_x)*0.5, y-8]);


//a macro, in a desperate attempt at readability
#define menuitemtext_cladd16(m,t,c,y) m.addm(spawn(mitem_text, item_text:t, item_command:c, item_scale:16, item_flags:IF_CENTERALIGN), [0, y], [160, y+16])

	if (clientstate() == 2)								{menuitemtext_cladd16(m, isdemo()?_("Return To Demo"):_("Return To Game"),
																								 			"m_pop",				y); y += 16;}
	if (clientstate() == 2)								{menuitemtext_cladd16(m, isdemo()?_("End Demo"):(isserver?_("End Game"):_("Disconnect")),
																											"m_pop;disconnect",		y); y += 16;}
	if (isserver() != 1 && checkbuiltin2(gethostcacheindexforkey,ISMENUQC))	{menuitemtext_cladd16(m, _("Join Server"),
																											"m_pop;m_servers",		y); y += 16;}
	if (checkcommand2("map", TRUE))						{menuitemtext_cladd16(m, _("New Game"),				"m_pop;m_newgame",		y); y += 16;}
	if (checkcommand2("menu_demo", FALSE))				{menuitemtext_cladd16(m, _("Demos"),				"m_pop;menu_demo",		y); y += 16;}
	if (checkcommand2("save", TRUE) && (isserver()||dp_workarounds))	{menuitemtext_cladd16(m, _("Save"),	"m_pop;m_save",			y); y += 16;}
	if (checkcommand2("load", TRUE))					{menuitemtext_cladd16(m, _("Load"),					"m_pop;m_load",			y); y += 16;}
	if (checkcommand2("cef", FALSE))					{menuitemtext_cladd16(m, _("Browser"),				"m_pop;cef google.com",	y); y += 16;}
	if (checkcommand2("xmpp", FALSE))					{menuitemtext_cladd16(m, _("Social"),				"m_pop;xmpp",			y); y += 16;}
	if (checkcommand2("irc", FALSE))					{menuitemtext_cladd16(m, _("IRC"),					"m_pop;irc /info",		y); y += 16;}
	/*if (checkbuiltin2(getpackagemanagerinfo,FALSE))	{menuitemtext_cladd16(m, _("Updates+Packages"),		"m_pop;m_updates",		y); y += 16;}
	else*/ if (checkcommand2("menu_download", FALSE))	{menuitemtext_cladd16(m, _("Updates+Packages"),		"m_pop;menu_download",	y); y += 16;}
	if (checkcommand2("qi", FALSE))						{menuitemtext_cladd16(m, _("Quake Injector"),		"m_pop;qi",				y); y += 16;}
	if (checkbuiltin2(getgamedirinfo,TRUE))				{menuitemtext_cladd16(m, _("Mods"),					"m_pop;m_mods",			y); y += 16;}
	if (checkcommand2("sys_openfile", FALSE))			{menuitemtext_cladd16(m, _("Open/Add External File"),"m_pop;sys_openfile",	y); y += 16;}	//opens a file chooser dialog in eg chromium.
	if (checkcommand2("xr_toggle", FALSE))				{menuitemtext_cladd16(m, _("Toggle VR"),			"m_pop;xr_toggle",		y); y += 16;}	//for webxr support
														{menuitemtext_cladd16(m, _("Options"),				"m_pop;m_options",		y); y += 16;}
														{menuitemtext_cladd16(m, _("Quit"), 				"m_pop;m_quit",			y); y += 16;}

	//spinny quad/pent, for the luls
	local string it = (random()<0.9)?"progs/quaddama.mdl":"progs/invulner.mdl";
	m.add(spawn (mitem_spinnymodel, item_text: it), RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [-160, 12*-16/2], [0, 12*16/2]);

#ifdef REVISION
	#define MENUSYS_REVISION REVISION
#else
	#define MENUSYS_REVISION "<UNKNOWN>"
#endif
#ifdef CSQC
	#define MODLEPOSTFIX "-CSQC"
#else
	#define MODLEPOSTFIX "-MQC"
#endif
	m.add(spawn(mitem_text, item_text:strcat("MenuSys "MENUSYS_REVISION MODLEPOSTFIX"; ", cvar_string("pr_engine")), item_scale:8), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MAX|RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX,[0,-8],[0,0]);
	addmenuback(m);
};
