const static struct
{
	string name;
	string cmd;
} binds[] =
{
	{_("Forwards"), 		"+forward"},
	{_("Back"),	 		"+back"},
	{_("Move Left"),		"+moveleft"},
	{_("Move Right"),		"+moveright"},
	{_("Turn Left"),		"+left"},
	{_("Turn Right"),		"+right"},
	{_("Look Up"),			"+lookup"},
	{_("Look Down"),		"+lookdown"},
	{_("Swim Up"),			"+moveup"},
	{_("Swim Down"),		"+movedown"},
	{_("Center view"),		"centerview"},
	{_("Jump"),	 		"+jump"},
	{_("Attack"),	 		"+attack"},
	{_("Next Weapon"),		"impulse 10"},
	{_("Prev Weapon"),		"impulse 12"},
	{_("Scores"),			"+showscores"},
	{_("Server Chat"),		"messagemode"},
	{_("Team Chat"),		"messagemode2"},
	{_("Voice Chat"),		"+voip"},
//	{_("Mouse Look"),		"+mlook"},
	{_("Keyboard Look"),		"+klook"},
	{_("Strafe"),			"+strafe"},
	{_("Run"),			"+speed"},
	{0,				0},
	{_("Axe"),			"impulse 1"},
	{_("Shotgun"),			"impulse 2"},
	{_("Super Shotgun"),		"impulse 3"},
	{_("Nailgun"),			"impulse 4"},
	{_("Super Nailgun"),		"impulse 5"},
	{_("Grenade Launcher"),		"impulse 6"},
	{_("Rocket Launcher"),		"impulse 7"},
	{_("Lightning Gun"),		"impulse 8"},
//	{_("Railgun"),			"impulse 9"},

	{0,				0},
	{_("Ready Up"),			"ready"},
	{_("Break Match"),		"break"},
};
void(mitem_desktop desktop) M_Options_Keys =
{
	float i;
	float h;

	//create the menu, give it focus, and make sure its displayed over everything else.
	mitem_exmenu m = spawn(mitem_exmenu, item_text:_("Key Options"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	//figure out the size of the stuff
//	h = sizeof(binds) / sizeof(binds[0]);
//	h *= 8;
	h = 200;
	h *= 0.5;	//and halve it

	//draw title art above the options
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/ttl_cstm.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [banner.item_size_x*-0.5, -h-32], [banner.item_size_x*0.5, -h-8]);

	//spawn a container frame for the actual options. this provides a scrollbar if we have too many items.
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_OWN_MIN, [-140, -h], [280, h*2]);

	float f = fopen("bindlist.lst", FILE_READ);
	if (f >= 0)
	{
		//throw a load of bind options onto it by reading from the array.
		for (i = 0; ; )
		{
			string line = fgets(f);
			if not (line)
				break;	//eof
			float args = tokenize(line);
			if (!args)
				continue;	//blank line
			string c = argv(0);
			string n = argv(1);
			string t = argv(2);
			if (c == "-")	//command only
			{
				if (n != "")
				{
					mitem it = menuitemtext_spawn(n, "", 8);
					it.item_flags &= ~IF_SELECTABLE;
					fr.add(it, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MIN | RS_X_MAX_OWN_MIN|RS_Y_MAX_OWN_MIN, [-it.item_size_x/2, i], it.item_size);
				}
			}
			else
				fr.add(menuitembind_spawn(n, 		c, '280 8'), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, i], '0 8');
			i += 8;
		}
		fclose(f);
	}
	else
	{
		//throw a load of bind options onto it by reading from the array.
		for (i = 0; i < sizeof(binds) / sizeof(binds[0]); i++)
		{
			if (binds[i].name == "")	//no name is a spacer
				continue;
			fr.add(menuitembind_spawn(binds[i].name, 		binds[i].cmd, '280 8'), RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_OWN_MIN, [0, (i*8)], '0 8');
		}
	}
	
	//and give us a suitable menu tint too, just because.
	addmenuback(m);
};
