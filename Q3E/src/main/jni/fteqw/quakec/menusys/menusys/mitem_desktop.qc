/***************************************************************************
desktop and top-level functions.
*/

#ifdef USEPOINTERS
typedef mitem mitem_desktop;
#else
class mitem_desktop : mitem_frame
{
	virtual float(vector pos, float scan, float char, float down)	item_keypress;
	virtual void(mitem newfocus, float flag) item_focuschange;
	virtual void(vector pos) item_draw;		//draws the game, then calls mitem_frame::item_draw for children.
#ifndef MENU
	virtual void(float seat, vector minpos, vector size) drawgame = __NULL__;	//if overridden, should call renderscene and then draw whatever hud it needs to. this will do splitscreen efficiently and automatically.
#endif
	void() mitem_desktop;			//the constructor. uninteresting. just ensures the size defaults to fullscreen.
};
#endif

#if !defined(MENU) && !defined(CSQC_SIMPLE)
float sb_showscores;
#endif

float drawfont;
float(string fontname, string fontmaps, string sizes, float slot, optional float fix_scale, optional float fix_voffset) loadfont = #357;

void() mitem_desktop::mitem_desktop =
{
#define menu_font_win 		autocvar(menu_font_win, "cour")
#define menu_font		autocvar(menu_font, "Courier New")
#define menu_font_fallback	autocvar(gl_font, "")
	queryscreensize();

	__using(loadfont, drawfont)
	if (checkextension("DP_GFX_FONTS"))
	{
		//make sure we have a font that can cope with slightly up-scaled stuff.
		//windows is special because we can load from the system fonts
		string fontname = menu_font_fallback;
		if (menu_font_win != "" && !strncasecmp(cvar_string("sys_platform"), "Win", 3))
			fontname = menu_font_win;
		else if (menu_font != "")
			fontname = menu_font;
		drawfont = loadfont("", fontname, "8 12 16 outline=1", -1);
	}

	item_text = "desktop";
	if (!item_flags)
	{
		item_size = ui.screensize;
		item_flags = IF_SELECTABLE | IF_MFOCUSED | IF_KFOCUSED | IF_RESIZABLE | IF_NOCURSOR;
	}

	if (!ui.kgrabs && !ui.mgrabs && (item_flags & IF_NOCURSOR))
	{
		ui.kgrabs = this;
		ui.mgrabs = this;
	}
};

void(mitem newfocus, float flag) mitem_desktop::item_focuschange =
{
	super::item_focuschange(newfocus, flag);

	if (flag & IF_KFOCUSED)
	{
		//if we're deselecting the current one, reenable grabs
		if (newfocus == __NULL__)
		{
			if(item_flags & IF_NOCURSOR)
			{
				ui.kgrabs = this;
				ui.mgrabs = this;
			}
		}
		else
		{
			if (ui.kgrabs == this)
			{
				ui.kgrabs = __NULL__;
				ui.mgrabs = __NULL__;
			}
		}
	}
};

//the interact flag says that the mouse is held down on the desktop
float(vector pos, float scan, float char, float down) mitem_desktop::item_keypress =
{
	float oldfl = item_flags;

	if (down & 2)
	{
		down &= 1;
		//if we're grabbing, then cancel if they press escape, otherwise block other items from taking the keys.
		if (scan == K_TOUCHTAP || (scan >= K_MOUSE1 && scan <= K_MOUSE5))
			return 2;	//block other wigits, don't cancel the event so the engine still does its thing
		else
		{
			if (scan == K_ESCAPE && down)
			{
				local mitem sc;
				//block the keyevent if we already have menus loaded but not focused (select one if we do).
				for (sc = item_children; sc; sc = sc.item_next)
				{
					if (sc.item_flags & IF_SELECTABLE)
					{
						if (!item_kactivechild)
							item_focuschange(sc, IF_KFOCUSED);
						return 3;
					}
				}
				//make sure our code takes it, instead of showing the engine menu...
#ifdef MENU
				m_toggle(0);
				return 3;
#else
				return 2|CSQC_ConsoleCommand("togglemenu");
#endif
			}
			return 2;
		}
	}

	if (mitem_frame::item_keypress(pos, scan, char, down))
		return TRUE;

	if ((scan == K_TOUCHTAP || scan == K_MOUSE1) && down)
	{
#if defined(CSQC) && defined(FTE_SPLITSCREEN)
		__using(numclientseats)
		{
			if (numclientseats)
				cvar_set("in_forceseat", "0");	//disable it.
		}
#endif
		if (item_flags & IF_NOCURSOR)
		{
			ui.kgrabs = this;
			ui.mgrabs = this;
		}
		return TRUE;
	}
	if (scan == K_MOUSE2 && down)
	{
#if defined(CSQC) && defined(FTE_SPLITSCREEN)
		__using(numclientseats)
		{
			if (numclientseats > 3)
				cvar_set("in_forceseat", ftos(1 + ((ui.mousepos[0]>ui.screensize[0]/2)?1:0) + ((ui.mousepos[1]>ui.screensize[1]/2)?2:0)));
			else if (numclientseats > 1)
				cvar_set("in_forceseat", ftos(1 + floor(ui.mousepos[1]*numclientseats/ui.screensize[1])));
			else if (numclientseats)
				cvar_set("in_forceseat", "0");	//disable it.
		}
#endif
		if (item_flags & IF_NOCURSOR)
		{
			ui.kgrabs = this;
			ui.mgrabs = this;
		}
		return TRUE;
	}

#ifdef CSQC
	//catch otherwise unhangled escape presses, just to be sure we can use escape to toggle the menu
	if (scan == K_ESCAPE && down)
	{
		return CSQC_ConsoleCommand("togglemenu");
	}
#endif
	return FALSE;
};

#if !defined(MENU) && !defined(CSQC_SIMPLE)
static vector(vector v) vtodpp =
{
	//so fucking disgustingly ugly.
	if (dp_workarounds)
	{
		v_x *= cvar("vid_width") / cvar("vid_conwidth");
		v_y *= cvar("vid_height") / cvar("vid_conheight");
	}
	return v;
};

//vector pmove_org;
void(float seat, vector minpos, vector size) mitem_desktop::drawgame_helper =
{
	if not(seat)
	{
		clearscene();
		addentities(MASK_ENGINE|MASK_VIEWMODEL);
	}
	else __using(VF_VIEWENTITY, VF_LPLAYER)
	{
		setviewprop(VF_LPLAYER, seat);
		setproperty(VF_VIEWENTITY, player_localentnum);	
		addentities(MASK_VIEWMODEL);	//don't do mask_engine because that's already done
	}
//	if (dp_workarounds)
//		setproperty(VF_ORIGIN, pmove_org);
	setproperty(VF_MIN, minpos);
	setproperty(VF_SIZE, vtodpp(size));

	setproperty(VF_DRAWCROSSHAIR, (ui.mgrabs == this));

	drawgame(seat, minpos, size);
	if (mouseinbox(minpos, size))
	{
		ui.havemouseworld = TRUE;
//		ui.mouseworldnear = unproject([ui.mousepos[0], ui.mousepos[1], 0]);
//		ui.mouseworldfar = unproject([ui.mousepos[0], ui.mousepos[1], 100000]);
	}
};
#endif

void(vector pos) mitem_desktop::item_draw =
{
#if !defined(MENU) && !defined(CSQC_SIMPLE)
//menuqc picks up the game/console as a background
	string constate = serverkey("constate");
	if (constate != "" && constate != "active")	//allow empty, so things still kinda work with dp too.
	{
		drawfill(pos, ui.screensize, '0 0 0', 1, 0);
	}
	else if (this.drawgame != __NULL__)
	{
#ifdef FTE_SPLITSCREEN
		__using numclientseats, VF_LPLAYER;
		if (numclientseats > 3)
		{
			drawgame_helper(0, [0, 0], 0.5*ui.screensize);
			drawgame_helper(1, [ui.screensize[0]*0.5, 0], 0.5*ui.screensize);
			drawgame_helper(2, [0, ui.screensize[1]*0.5], 0.5*ui.screensize);
			drawgame_helper(3, [ui.screensize[0]*0.5, ui.screensize[1]*0.5], 0.5*ui.screensize);
			setviewprop(VF_LPLAYER, 0);
		}
		else if (numclientseats > 2)
		{
			drawgame_helper(0, [0, 0], [ui.screensize[0], ui.screensize[1]*0.333]);
			drawgame_helper(1, [0, ui.screensize[1]*0.333], [ui.screensize[0], ui.screensize[1]*0.333]);
			drawgame_helper(2, [0, ui.screensize[1]*0.666], [ui.screensize[0], ui.screensize[1]*0.333]);
			setviewprop(VF_LPLAYER, 0);
		}
		else if (numclientseats > 1)
		{
			drawgame(0, [0, 0], [ui.screensize[0], ui.screensize[1]*0.5]);
			drawgame(1, [0, ui.screensize[1]*0.5], [ui.screensize[0], ui.screensize[1]*0.5]);
			setviewprop(VF_LPLAYER, 0);
		}
		else
#endif
		{
			drawgame_helper(0, '0 0', ui.screensize);
		}
	}
#endif
	super::item_draw(pos);

	if (ui.kgrabs == this)
	{
#if defined(CSQC) && !defined(CSQC_SIMPLE)
		if (sb_showscores && ui.mgrabs == this)
			ui.mgrabs = __NULL__;
		else
#endif
		if (!ui.mgrabs)
			ui.mgrabs = this;
	}
};


static var float oldgrabstate;	//to work around a DP bug (as well as unnecessary spam)

void(float force) items_updategrabs =
{
	if (!ui.mgrabs || !(ui.mgrabs.item_flags & IF_NOCURSOR))
	{
		if (!oldgrabstate || force)
		{
			oldgrabstate = TRUE;
#ifdef MENU
			setkeydest(2);
			if (!checkbuiltin(setcursormode))
				setmousetarget(2);	//legacy junk
			else
#endif
			if (checkextension("FTE_QC_HARDWARECURSORS"))
				setcursormode(TRUE, autocvar(cl_cursor, "gfx/cursor.lmp"), autocvar(cl_cursorbias, 4.0)*'1 1', autocvar(cl_cursorscale, 1.0));	//because we can
			else
				setcursormode(TRUE);	//because DP sucks.

			//evilness. make sure we fit the screen by forcing the screen size upwards. this may also affect reported video mode choices as well as being just extra scaling.
			if (autocvar(vid_minsize,   '640 480').x < 640 ||
				autocvar(vid_minsize,   '640 480').y < 480)
				cvar_set("vid_minsize", "640 480");
		}
	}
	else if (oldgrabstate || force)
	{
		oldgrabstate = FALSE;
#ifdef MENU
		setkeydest(0);
		if (!checkbuiltin(setcursormode))
			setmousetarget(1);
		else
#endif
			setcursormode(FALSE);
	}
};

void(mitem_desktop desktop, vector screensize) items_draw =
{
	ui.screensize = screensize;
	queryscreensize();

#ifdef MENU
	ui.mousepos = getmousepos();
#else
	if (ui.havemouseworld)
		ui.havemouseworld = 2;	//stale, but not too stale
#endif

	if (desktop.item_size != ui.screensize)
	{
		desktop.item_size = ui.screensize;
		desktop.item_resized();
	}
	ui.drawrectmax = ui.screensize;

	desktop.item_draw(desktop.item_position);
	drawresetcliparea();

	items_updategrabs(FALSE);
	if (dp_workarounds && oldgrabstate)
	{
//		//hopefully dp isn't broken and reports zero sizes for files that failed...
//		if (drawgetimagesize(autocvar_cl_cursor) == '0 0')
//			ui.drawpic(ui.mousepos - autocvar_cl_cursorbias*'1 1', autocvar_cl_cursor, autocvar_cl_cursorsize*'1 1', '1 1 1', 1, 0);
//		else
			ui.drawcharacter(ui.mousepos - [stringwidth("+", TRUE, '4 4')*0.5, 4], '+', '8 8', '1 1 1', 1, 0);
	}

#ifndef MENU
	if (ui.havemouseworld == 2)	//if its still stale then its totally invalid.
		ui.havemouseworld = FALSE;
#endif
	ui.oldmousepos = ui.mousepos;
};

float(mitem_desktop desktop, float evtype, float scanx, float chary, float devid) items_keypress =
{
	local float result = FALSE;
	vector pos;
	mitem p;

	switch(evtype)
	{
#define IE_PASTE 8
	case IE_PASTE: //down, with JUST char info.
	case IE_KEYDOWN:
	case IE_KEYUP:
		if (scanx == K_LCTRL)
			ui.ctrlheld = ((evtype==IE_KEYDOWN)?ui.ctrlheld|1:ui.ctrlheld&~1);
		if (scanx == K_RCTRL)
			ui.ctrlheld = ((evtype==IE_KEYDOWN)?ui.ctrlheld|2:ui.ctrlheld&~2);
		if (scanx == K_LSHIFT)
			ui.shiftheld = ((evtype==IE_KEYDOWN)?ui.shiftheld|1:ui.shiftheld&~1);
		if (scanx == K_RSHIFT)
			ui.shiftheld = ((evtype==IE_KEYDOWN)?ui.shiftheld|2:ui.shiftheld&~2);
#ifdef HEIRACHYDEBUG
		if (scanx == K_F1 && evtype == IE_KEYDOWN)
		{
			mitem_printtree(desktop, "items_keypress", __LINE__);
			return TRUE;
		}
#endif
		if (scanx == K_TOUCHTAP || (scanx >= K_MOUSE1 && scanx <= K_MOUSE5))
		{
			if (ui.mgrabs)
			{
				pos = '0 0 0';
				for (p = ui.mgrabs; p; p = p.item_parent)
					pos += p.item_position;
				result = ui.mgrabs.item_keypress(pos, scanx, chary, (evtype == IE_KEYDOWN)|2);
				if (result & 2)
				{
					ui.mousedown = 0;
					return result & 1;
				}
			}
		}
		else
		{
			if (ui.kgrabs)
			{
				pos = '0 0 0';
				for (p = ui.kgrabs; p; p = p.item_parent)
					pos += p.item_position;
				result = ui.kgrabs.item_keypress(pos, scanx, chary, (evtype == IE_KEYDOWN||evtype == IE_PASTE)|2);
				if (result & 2)
					return result & 1;
			}
		}
		if (desktop && desktop.item_keypress)
			result = desktop.item_keypress(desktop.item_position, scanx, chary, evtype == IE_KEYDOWN||evtype == IE_PASTE);
		if (scanx == K_TOUCHTAP)
		{
			if (evtype == IE_KEYDOWN)
				ui.mousedown |= 1<<23;
			else
				ui.mousedown &~= 1<<23;
		}
		else if (scanx >= K_MOUSE1 && scanx <= K_MOUSE5)
		{
			if (evtype == IE_KEYDOWN)
				ui.mousedown |= pow(1, scanx-K_MOUSE1);
			else
				ui.mousedown &~= pow(1, scanx-K_MOUSE1);
		}
		result = result & 1;
		break;
	case IE_MOUSEDELTA:
		result = !ui.mgrabs || !(ui.mgrabs.item_flags & IF_NOCURSOR);
		if (result)
		{
			queryscreensize();
			ui.mousepos[0] = bound(0, ui.mousepos[0]+scanx, ui.screensize[0]);
			ui.mousepos[1] = bound(0, ui.mousepos[1]+chary, ui.screensize[1]);
		}
		break;
	case IE_MOUSEABS:
		ui.mousepos[0] = scanx;
		ui.mousepos[1] = chary;
		result = !ui.mgrabs || !(ui.mgrabs.item_flags & IF_NOCURSOR);
		break;
	}
	return result;
};


