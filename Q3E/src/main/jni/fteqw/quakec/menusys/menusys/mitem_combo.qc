/***************************************************************************
combo item.
No longer using pointers, now using a tokenized string. Less efficient, but saves creating lots of different arrays, just pass a string.
The string list is doubled, first is the actual value, second is the friendly text.
Will show actual value when focused, and will show readable value when not.
The possible values is a separate popup.
*/

//FIXME: should probably set up a grabs to intercept right-click / escape outside of the item

class mitem_combo;
class mitem_combo_popup;

class mitem_combo : mitem_edit
{
	virtual void(vector pos)								item_draw;
	virtual float(vector pos, float scan, float char, float down)	item_keypress;
	virtual void(mitem newfocus, float changedflag)				item_focuschange;

	mitem_combo_popup		cpopup;
	string	mstrlist;
	float		firstrow;
	float		visrows;

	virtual void() item_remove;
	
	virtual void() item_resized =
	{
		if (isvalid(item_command))
			item_flags |= IF_SELECTABLE;		
		else
			item_flags &= ~IF_SELECTABLE;
		super::item_resized();
	};
	void() mitem_combo =
	{
		spos = -1;
	};
};

class mitem_combo_popup : mitem
{
	virtual void(vector pos)								item_draw;
	virtual float(vector pos, float scan, float char, float down)	item_keypress;
	virtual void(mitem newfocus, float changedflag)				item_focuschange;

	mitem_combo 		pcombo;
	mitem_vslider		pslider;
	float			slidercache;
	
	virtual void() item_remove =
	{
		if (pslider)
			pslider.item_remove();
		if (pcombo)
			pcombo.cpopup = 0;
		super::item_remove();
	};
};

void() mitem_combo::item_remove =
{
	mitem_combo_popup p = cpopup;
	if (p)
		p.item_remove();
	strunzone(mstrlist);
	super::item_remove();
};

void(vector pos) mitem_combo::item_draw =
{
	local float i, v;
	local string curval = get(item_command);
	vector rgb = item_rgb;
	if (!(item_flags & IF_SELECTABLE))
		rgb *= 0.2;
	
	if (cpopup)
		cpopup.item_position = pos + [item_size_x / 2, item_size_y];

	if (spos >= 0)
	{
		super::item_draw(pos);
		return;
	}
	else
		mitem::item_draw(pos);

	v = tokenize(mstrlist);

	//display the friendly string if the current value matches
//	if (!(item_flags & IF_KFOCUSED) && (!cfriend || !(cfriend.item_flags & IF_KFOCUSED)))
	{
		for (i = 0; i < v; i+=2)
		{
			if (argv(i) == curval)
			{
				curval = argv(i+1);
				break;
			}
		}
	}

	pos_x += item_size_x / 2;


/*	//border
	ui.drawfill(pos, [item_size_x/2, 1], TD_BOT, item_alpha, 0);
	ui.drawfill(pos, [1, item_size_y - 1], TD_RGT, item_alpha, 0);
	ui.drawfill(pos + [item_size_x/2-1, 1], [1, item_size_y - 1], TD_LFT, item_alpha, 0);
	ui.drawfill(pos + [0, item_size_y-1], [item_size_x/2, 1], TD_TOP, item_alpha, 0);
*/
	//silly strings need to get cut off properly.
	ui.setcliparea(pos[0], pos[1], item_size_x/2, item_size_y);
	pos_y += (item_size_y - item_scale)*0.5;
	pos_x += 1;
	ui.drawstring(pos, curval, '1 1 0' * item_scale, rgb, item_alpha, 0);
	ui.setcliparea(ui.drawrectmin[0], ui.drawrectmin[1], ui.drawrectmax[0] - ui.drawrectmin[0], ui.drawrectmax[1] - ui.drawrectmin[1]);
};
void(mitem newfocus, float flag) mitem_combo::item_focuschange =
{
	if (!cpopup || !(flag & IF_KFOCUSED))
		return;	//don't care
	if (newfocus != (mitem)this && newfocus != (mitem)cpopup)
	{
		cpopup.item_size = cpopup.maxs = '0 0';
		cpopup.item_flags &~= IF_SELECTABLE;
	}
	spos = -1;
};
void(mitem newfocus, float flag) mitem_combo_popup::item_focuschange =
{
	pcombo.item_focuschange(newfocus, flag);
};
void(vector pos) mitem_combo_popup::item_draw =
{
	vector col;
	if (item_size_y < 1)
		return;

	local mitem_combo f = pcombo;
	item_command = f.item_command;
	local string curval = f.get(f.item_command);
	local float i, m, c, v;

	vector popupsize = item_size;

	

	if (!((f.item_flags | item_flags) & IF_KFOCUSED))
	{
		item_size = maxs = '0 0';
		item_flags &~= IF_SELECTABLE;
		return;
	}

	ui.drawfill(pos, popupsize, item_rgb, item_alpha, 0);

/*	//border
	ui.drawfill(pos, [popupsize_x, 1], TD_BOT, item_alpha, 0);
	ui.drawfill(pos, [1, popupsize_y - 1], TD_RGT, item_alpha, 0);
	ui.drawfill(pos + [popupsize_x-1, 1], [1, popupsize_y - 1], TD_LFT, item_alpha, 0);
	ui.drawfill(pos + [0, popupsize_y-1], [popupsize_x, 1], TD_TOP, item_alpha, 0);
*/	pos_x += 1;

	v = tokenize(f.mstrlist);
	for (c = 0; c < v; c += 2)
		if (argv(c) == curval)
			break;
	if (c >= v)
		c = -1;

	i = f.firstrow;
	i = i*2;
	if (!f.visrows)
		i = 0;
	else if (c >= 0)
	{
		//bound the displayed position 
		if (c < i)
			i = c;
		if (i < c - (f.visrows-1)*2)
			i = c - (f.visrows-1)*2;
	}
	f.firstrow = floor(i*0.5);

	if (v > f.visrows*2)
	{
		if (!pslider)
		{
			slidercache = -2;
			pslider = spawn(mitem_vslider);
		}
	}
	else if (pslider)
	{
		pslider.item_remove();
		pslider = __NULL__;
	}

	if (pslider)
	{
		popupsize_x -= pslider.item_size_x;
		if (slidercache != c)
		{	//current index changed, force the slider to the active item.
			slidercache = c;
			pslider.val = f.firstrow;
		}
		pslider.maxv = v/2 - f.visrows;
		pslider.item_size_y = popupsize_y;
		pslider.item_draw(pos + [popupsize_x, 0]);
	}

	//constrain the drawing so it doesn't overflow the popup
	ui.setcliparea(pos[0], pos[1], popupsize[0], popupsize[1]);

	if (pslider)
	{
		f.firstrow = i = floor(pslider.val);
		m = ((i!=pslider.val) + i + f.visrows)*2;
		i *= 2;
		pos[1] -= (pslider.val-i/2) * item_scale;
	}
	else m = i + f.visrows*2;

	for (; i < m && i < v ; i+=2)
	{
		col = f.item_rgb;
		if (item_flags & IF_MFOCUSED)
			if (mouseinbox(pos, [popupsize_x, item_scale]))
				col_z = 0;
		if (c == i)
			col_x = 0;

		ui.drawstring(pos, argv(i+1), '1 1 0' * item_scale, col, f.item_alpha, 0);
		pos_y += item_scale;
	}

	//reset the clip area
	ui.setcliparea(ui.drawrectmin[0], ui.drawrectmin[1], ui.drawrectmax[0] - ui.drawrectmin[0], ui.drawrectmax[1] - ui.drawrectmin[1]);
};
float(vector pos, float scan, float char, float down) mitem_combo_popup::item_keypress =
{
	if (pslider && (scan == K_TOUCHTAP || scan == K_MOUSE1))
	{
		vector sliderpos = pos + [item_size_x-pslider.item_size_x,0];
		if (mouseinbox(pos + [item_size_x-pslider.item_size_x,0], pslider.item_size))
			return pslider.item_keypress(sliderpos, scan, char, down);
	}
			
	return pcombo.item_keypress(pos - [0, pcombo.item_size_y], scan, char, down);
};
float(vector pos, float scan, float char, float down) mitem_combo::item_keypress =
{
	if (!down)
		return FALSE;

	local string curval = get(item_command);
	local float i, c;
	local float f;
	c = tokenize(mstrlist);

	//find which one is active
	for (i = 0; i < c; i+=2)
	{
		if (argv(i) == curval)
		{
			break;
		}
	}

	if (scan == K_ESCAPE || scan == K_MOUSE2 || scan == K_GP_B || scan == K_GP_BACK)
	{
		if (cpopup)
		{
			cpopup.item_remove();
			return TRUE;
		}
		return FALSE;
	}
	else if (scan == K_MWHEELUP || (cpopup && ISUPARROW(scan)) || (!cpopup && spos < 0 && ISLEFTARROW(scan)))
	{
		i -= 2;
		if (i < 0)
			i = c - 2;
		curval = argv(i);
	}
	else if (scan == K_MWHEELDOWN || (cpopup && ISDOWNARROW(scan)) || (!cpopup && spos < 0 && ISRIGHTARROW(scan)))
	{
		i += 2;
		if (i >= c)
			i = 0;
		curval = argv(i);
	}
	else if (scan == K_TOUCHTAP || scan == K_MOUSE1 || ISCONFIRMKEY(scan))
	{
		if (ISCONFIRMKEY(scan) && cpopup)
		{
			cpopup.item_remove();
			return TRUE;
		}

		visrows = ((c>18)?18/2:c/2);
		if (!cpopup)
		{
			cpopup = spawn(mitem_combo_popup);
			cpopup.pcombo = this;
			cpopup.item_scale = 8;
			cpopup.item_rgb = MENUBACK_RGB;
			cpopup.item_alpha = MENUBACK_ALPHA;
			pos = item_position;
			mitem_frame fr = item_parent;
			while (fr.item_parent)
			{	//try to inject the combo thingie into the desktop item. this is to avoid scissoring.
				pos += fr.item_position;
				fr = fr.item_parent;
			}
			fr.addr(cpopup, RS_X_MIN_PARENT_MIN | RS_X_MAX_OWN_MIN | RS_Y_MIN_PARENT_MIN | RS_Y_MAX_OWN_MIN, pos + [self.item_size_x / 2, self.item_size_y], [item_size_x*0.5, item_size_y*visrows]);
		}
		cpopup.item_size = cpopup.maxs = [item_size_x*0.5, item_size_y*visrows];
		cpopup.item_flags |= IF_SELECTABLE;
		cpopup.totop();

		if ((scan == K_TOUCHTAP || scan == K_MOUSE1) && (cpopup.item_flags & IF_MFOCUSED))
		{
			//if they clicked inside the popup, change the selected item.
			f = ui.mousepos[1] - (pos_y + item_size_y);
			f /= cpopup.item_scale;
			f += firstrow;
			i = floor(f) * 2;
			if (i < c)
			{
				curval = argv(i);
				cpopup.item_flags &~= IF_SELECTABLE;
				cpopup.item_size = cpopup.maxs = '0 0';
				item_parent.item_focuschange(this, IF_MFOCUSED|IF_KFOCUSED);

				cpopup.item_remove();
			}
		}
	}
	else
	{
		if (spos < 0)
		{
			spos = strlen(curval);
			if (!super::item_keypress(pos, scan, char, down))
			{
				spos = -1;
				return FALSE;
			}
			return TRUE;
		}
		else return super::item_keypress(pos, scan, char, down);
	}

	set(item_command, curval);
	return TRUE;
};
mitem_combo(string text, string command, vector sz, string valuelist) menuitemcombo_spawn =
{
	mitem_combo n = spawn(mitem_combo, item_text:text, item_command:command);
	n.item_scale = sz_y;
	n.item_size = sz;
	n.mstrlist = strzone(valuelist);

	if (n.isvalid(command))
		n.item_flags |= IF_SELECTABLE;
	return n;
};
