/***************************************************************************
tabs/tab widgets.
the 'tabs' widget is simply a tab-selection control. horizontal multiple choice. it draws only its currently active child.
the 'tab' widget is merely a container of other widgets, no different from a standard frame object, just has a name and a specific size.
*/
class mitem_tabs : mitem_frame	/*frame... but not really*/
{
	virtual void(vector pos) item_draw;
	virtual float(vector pos, float scan, float char, float down) item_keypress;
//	virtual void() item_resize;

	void() mitem_tabs =
	{
		item_framesize = '2 16 2';
		item_flags |= IF_SELECTABLE|IF_RESIZABLE;
	};
};

class mitem_tab : mitem_frame
{
	virtual float(vector pos, float scan, float char, float down) item_keypress =
	{
		if (down && ISUPARROW(scan))
			menu_selectnextitem(this, TRUE);
		else if (down && ISDOWNARROW(scan))
			menu_selectnextitem(this, FALSE);
		else if (super::item_keypress(pos, scan, char, down))
			return TRUE;
		else
			return FALSE;
		return TRUE;
	};

	void() mitem_tab =
	{
		item_framesize = '0 0 0';
		item_flags |= IF_SELECTABLE|IF_RESIZABLE;
	};
};

void(vector pos) mitem_tabs::item_draw =
{
	local mitem ch;
	local vector tpos = pos;
	local float w;
	local vector col;

	//to highlight the active tab, we draw the top line 1 pixel higher, and no bottom line
	for (ch = item_children; ch; ch = ch.item_next)
	{
		w = stringwidth(ch.item_text, TRUE, '8 8') + 8;

		ui.drawfill(tpos + '0 1', [1, 15], TD_LFT, item_alpha, 0);
		ui.drawfill(tpos + [w-1, 1], [1, 14], TD_RGT, item_alpha, 0);
		if (ch == item_kactivechild)
		{
			//top line
			ui.drawfill(tpos, [w, 1], TD_TOP, item_alpha, 0);
		}
		else
		{
			//top line
			ui.drawfill(tpos + '0 1', [w, 1], TD_TOP, item_alpha, 0);
			//bottom
			ui.drawfill(tpos + '0 15', [w, 1], TD_TOP, item_alpha, 0);
		}

		col = item_rgb;
		if (!(ch.item_flags & IF_SELECTABLE))
			col *= 0.2;
		else
		{
			if (!item_kactivechild)
				item_focuschange(ch, IF_KFOCUSED);
			if (mouseinbox(tpos, [w, 16]))
				col_z = 0;
			if (ch.item_flags & IF_KFOCUSED)
				col_x = 0;
		}

		ui.drawstring(tpos + '4 4', ch.item_text, '8 8', col, item_alpha, 0);
		tpos_x += w;
	}
	ui.drawfill(tpos + '0 15', [pos_x + item_size_x - tpos_x, 1], TD_TOP, item_alpha, 0);		//top
	ui.drawfill(pos + '0 16', [1, item_size_y - 16], TD_LFT, item_alpha, 0);					//left
	ui.drawfill(pos + [item_size_x-1, 16], [1, item_size_y - 17], TD_RGT, item_alpha, 0);	//right
	ui.drawfill(pos + [1, item_size_y-1], [item_size_x-1, 1], TD_BOT, item_alpha, 0);		//bottom

	if (item_mactivechild != item_kactivechild)
		item_focuschange(item_kactivechild, IF_MFOCUSED);	//give the tab full focus.
	ch = item_kactivechild;
	if (ch)
		ch.item_draw(pos + ch.item_position + [item_framesize[0], item_framesize[1]]);
};
float(vector pos, float scan, float char, float down) mitem_tabs::item_keypress =
{
	local mitem ch;
	local vector tpos = pos;
	local vector sz;
	local float result;

	if (down && (scan == K_TOUCHTAP || scan == K_MOUSE1 || scan == K_MOUSE2 || scan == K_MOUSE3))
	{
		sz = [8,16];
		//to highlight the active tab, we draw the top line 1 pixel higher, and no bottom line
		for (ch = this.item_children; ch; ch = ch.item_next)
		{
			sz_x = stringwidth(ch.item_text, TRUE, '8 8') + 8;
			if (mouseinbox(tpos, sz))
			{
				item_focuschange(ch, IF_KFOCUSED);	//give the tab full focus.
				return TRUE;
			}
			
			tpos_x += sz_x;
		}
	}
	ch = item_kactivechild;
	if (ch)
	{
		result = ch.item_keypress(pos + [item_framesize[0], item_framesize[1]] + ch.item_position, scan, char, down);
		if (!result && down)
		{
			if ((scan == K_TAB && !ui.shiftheld) || scan == K_GP_RSHOULDER || scan == K_RIGHTARROW)
			{
				ch = ch.item_next;
				if (!ch)
					ch = item_children;
				item_focuschange(ch, IF_KFOCUSED);
				result = TRUE;
			}
//			else if ((scan == K_TAB && ui.shiftheld) || scan == K_GP_LSHOULDER || scan == K_LEFTARROW)
//			{
//				ch = ch.item_next;
//				if (!ch)
//					ch = item_children;
//				item_focuschange((ch.item_next?ch.item_next:this.item_children), IF_KFOCUSED);
//				result = TRUE;
//			}
		}
	}
	else
		result = FALSE;
	return result;
};
/*void() mitem_tabs::item_resize =
{
	local mitem ch;
	for (ch = this.item_children; ch; ch = ch.item_next)
	{
		ch.item_size = this.item_size;
		if (ch.item_resized)
			ch.item_resized();
	}
};*/

mitem_tabs(vector sz) menuitemtabs_spawn =
{
	return spawn(mitem_tabs, item_size:sz);
};
mitem_tab(mitem_tabs tabs, string itname) menuitemtab_spawn =
{
	//a tab itself is little different from a frame, just has no implicit focus, and has a title
	mitem_tab n = spawn(mitem_tab, item_text:itname, frame_hasscroll:TRUE);

	tabs.addr(n, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	return n;
};
