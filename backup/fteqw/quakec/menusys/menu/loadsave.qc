#ifndef LOADSAVE_QC
#define LOADSAVE_QC

//I'm feeling lazy, so I'm going to only provide X slots, like quake's menu.
static string savenames[] =
{
	"a0",
	"a1",
	"a2",
	"quick",
	"s0",
	"s1",
	"s2",
	"s3",
	"s4",
	"s5",
	"s6",
	"s7",
	"s8",
	"s9",
};
#define NUMSAVESLOTS savenames.length

/*
class mitem_savescreeny : mitem
{
	virtual void(vector pos) item_draw =
	{
		string s = sprintf("saves/s%g/screeny.png", selectedsaveslot);
		if not(whichpack(s))
			if (drawgetimagesize(s) != '0 0 0')
				ui.drawpic(pos, s, item_size, item_rgb, item_alpha, 0);
	};
};
*/
class mitem_saveoption : mitem_text
{
	string slot;
	float mode;
	
	virtual void() mitem_saveoption =
	{
		if (mode)
			item_flags |= IF_SELECTABLE;
	};

	virtual void(vector pos) item_draw =
	{
		//some sort of pulsing if its active.
		if (item_flags & IF_KFOCUSED)
			ui.drawfill(pos, item_size, '1 0 0', sin(cltime)*0.125+0.150, 0);
		float w = stringwidth(item_text, TRUE, '1 1 0'*item_scale);
		ui.drawstring(pos + [(item_size_x-w)/2, 0], item_text, '1 1 0' * item_scale, menuitem_textcolour(this), item_alpha, 0);
	};

	virtual float(vector pos, float scan, float char, float down) item_keypress =
	{
		if (!down)
			return FALSE;
		if (ISCONFIRMKEY(scan) || ((scan == K_MOUSE1 || scan == K_TOUCHTAP) && mouseinbox(pos, this.item_size)))
		{
			if (item_flags & IF_KFOCUSED)
			{
				switch(mode)
				{
				case 0:
					break;	//can't load a slot which is empty.
				case 1:
					localcmd(sprintf("m_pop;load %s\n", slot));
					break;
				case 2:
					//FTE has a savegame_legacy command if you want compatibility with other engines.
					localcmd(sprintf("m_pop;wait;echo \"%s\";save %s\n", _("Saving Game"), slot));
					//localcmd(sprintf("m_pop;wait;screenshot saves/s%g/screeny.png;echo \"%s\";save s%g\n", slot, _("Saving Game"), slot));
					break;
				}
			}
			else
			{
				item_parent.item_focuschange(this, IF_KFOCUSED);
			}
			return TRUE;
		}
		return FALSE;
	};
};

class mitem_savepreview : mitem
{
	//assumption: the only selectable children in the parent are save options.
	virtual void(vector pos) item_draw =
	{
		mitem_saveoption sel;
		sel = (mitem_saveoption)item_parent.item_kactivechild;
		if (sel)
		{
			string s = sprintf("saves/%s/screeny.tga", sel.slot);
			if (drawgetimagesize(s) != '0 0 0')
				ui.drawpic(pos, s, item_size, item_rgb, item_alpha, 0);
		}
	};
};

static string(string savename) scansave =
{
	string l;
	float f = fopen(sprintf("saves/%s/info.fsv", savename), FILE_READ);
	if (f < 0)
		f = fopen(sprintf("%s.sav", savename), FILE_READ);
	if (f < 0)
		return __NULL__;	//weird

	fgets(f);	//should be the version
	l = fgets(f);	//description
	if (l)
		l = strreplace("_", " ", l);
	fclose(f);
	return l;
};

void(mitem_desktop desktop, float mode) M_LoadSave =
{
	mitem_exmenu m = spawn(mitem_exmenu, item_text:"Load/Save", item_flags:IF_SELECTABLE, item_command:"m_main");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();

	string l;
	float i;
	float smode;
	float pos = NUMSAVESLOTS*16/-2;

	mitem_pic banner = spawn(mitem_pic, item_text:((mode==2)?"gfx/p_save.lmp":"gfx/p_load.lmp"), item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, [(banner.item_size_x)*-0.5, pos-32], [(banner.item_size_x)*0.5, pos-8]);

	for (i = 0; i < NUMSAVESLOTS; i++)
	{
		l = scansave(savenames[i]);
		smode = mode;
		if (l=="")
		{
			l = "Empty Slot";
			if (mode==1)
				smode = 0;
		}
		m.addm(spawn (mitem_saveoption, item_scale:16, slot:savenames[i], mode:smode, item_text:l), [-320, pos+i*16], [320, pos+(i+1)*16]);
	}

	m.addm(spawn(mitem_savepreview), [-320, -240], [320, 240]);
	addmenuback(m);
};

void(mitem_desktop desktop) M_Load =
{
	M_LoadSave(desktop, 1);
};
void(mitem_desktop desktop) M_Save =
{
	if (!(isserver() || dp_workarounds))
		M_Main(desktop);	//can't save when not connected. this should be rare, but can if you use the console or the main menu options are stale.
	else
		M_LoadSave(desktop, 2);
};
#endif
