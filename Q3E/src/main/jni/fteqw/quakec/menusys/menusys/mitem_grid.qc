#ifndef MITEM_GRID_QC__
#define MITEM_GRID_QC__

//simple key/value grid (with editing).
class mitem_grid : mitem
{
	//virtual void(mitem newfocus, float changedflag)			item_focuschange;
	virtual float(vector pos, float scan, float char, float down)	item_keypress;
	virtual void(vector pos)	item_draw;
	virtual void() 			item_resized;

	//first child is determined from the vslider.
	float		grid_numchildren;
	float		grid_mactive;
	float		grid_kactive;

	mitem_vslider	vslider;	//displayed if any client item's max[y] > our item_size[y]

	virtual void(vector pos, float idx) grid_draw = 0;
	virtual float(vector pos, float scan, float char, float down, float idx) grid_keypress = {return FALSE;};
	virtual void(float olditem, float newitem) grid_selectionchanged = {};	//just key focus
};

//does NOT wrap.
//does NOT pass go.
static mitem(mitem item, float upwards) menu_simplenextitem =
{
	mitem_frame menu = item.item_parent;
	mitem prev;
	if (upwards)
	{
		if (item)
		{
			item = item.item_next;
			if (!item)
				return __NULL__;
			return item;
		}
		else
			return __NULL__;
	}
	else
	{
		for(prev = menu.item_children; prev.item_next; prev = prev.item_next)
		{
			if (prev.item_next == item)
				return prev;
		}
		return __NULL__;
	}
};

float(vector pos, float scan, float char, float down) mitem_grid::item_keypress =
{
	float ch;
	float handled = FALSE;

	if (scan >= K_MOUSE1 && scan <= K_MOUSE5 && scan != K_MWHEELUP && scan != K_MWHEELDOWN && scan != K_TOUCHTAP)
	{
		ch = grid_mactive;
		if (ch != grid_kactive)
		if (down && (scan == K_TOUCHTAP || scan == K_MOUSE1))	//keyboard focus follows on mouse click.
		{
			item_focuschange((ch>=0)?__NULL__:vslider, IF_KFOCUSED);
			grid_selectionchanged(grid_kactive, ch);
			grid_kactive = ch;
			handled = TRUE;
		}
	}
	else
	{
		ch = grid_kactive;
		if (ch<0 && down)
		{
			//if there's no key child active, then go and pick one so you can just start using keyboard access etc.
			if (grid_mactive)
				ch = grid_mactive;
			else
				ch = 0;
			item_focuschange((ch>=0)?__NULL__:vslider, IF_KFOCUSED);
			grid_selectionchanged(grid_kactive, ch);
			grid_kactive = ch;
		}
	}

	if (vslider)
		pos[1] -= vslider.val;

	if (ch<0&&vslider)
	{
		if (vslider.item_keypress)
			handled = vslider.item_keypress(pos + vslider.item_position, scan, char, down);
	}
	else if (!handled && ch >= 0 && ch < grid_numchildren)
		handled = grid_keypress(pos + vslider.item_position + [0,ch]*item_scale, scan, char, down, ch);
	if (!handled && down)
	{
		ch = grid_kactive;
		switch(scan)
		{
		case K_MWHEELUP:
		case K_MWHEELDOWN:
			if (vslider)	//give mwheel to the slider if its visible.
				handled = vslider.item_keypress(pos + vslider.item_position, scan, char, down);
			break;
		case K_HOME:
			ch = 0;
			break;
		case K_END:
			ch = grid_numchildren-1;
			break;
		case K_PGUP:
			ch = max(0, grid_kactive-5);
			break;
		case K_PGDN:
			ch = min(grid_numchildren-1, grid_kactive+5);
			break;
		case K_UPARROW:
		case K_GP_DPAD_UP:
		case K_GP_LTHUMB_UP:
		case K_GP_RTHUMB_UP:
			ch = max(grid_kactive-1, 0);
			break;
		case K_DOWNARROW:
		case K_GP_DPAD_DOWN:
		case K_GP_LTHUMB_DOWN:
		case K_GP_RTHUMB_DOWN:
			ch = min(grid_kactive+1, grid_numchildren-1);
			break;
		}
		if (ch != grid_kactive)
		{
			grid_selectionchanged(grid_kactive, ch);
			grid_kactive = ch;
			handled = TRUE;
		}
	}

	return handled;
};
void() mitem_grid::item_resized =
{
	float clientheight = grid_numchildren * item_scale;
	if (clientheight > item_size[1])
	{
		if (!vslider)
			vslider = spawn(mitem_vslider, val:0, stride:8);
		vslider.maxv = clientheight - item_size[1];
	}
	else if (vslider)
	{
		vslider.item_remove();
		vslider = (mitem_vslider)__NULL__;
	}
}

void(vector pos) mitem_grid::item_draw =
{
	local vector omin = ui.drawrectmin, omax = ui.drawrectmax;
	local vector clientpos;
	local vector clientsize;

	clientpos = pos;
	clientsize = this.item_size;

	if (vslider)
	{
		//scroll+shrink the client area to fit the slider on it.
		clientpos[1] -= vslider.val;
		clientsize[0] -= vslider.item_size[0];
	}

	if (ui.mousepos != ui.oldmousepos)
	{
		local mitem newfocus = __NULL__;
		grid_mactive = floor((ui.mousepos[1]-clientpos_y)/item_scale);

		if (vslider)
			if (mouseinbox(pos + [clientsize[0], 0], vslider.item_size))
			{
				newfocus = vslider;
				grid_mactive = -1;
			}
		this.item_focuschange(newfocus, IF_MFOCUSED);
	}

	//clip the draw rect to our area, so children don't draw outside it. don't draw if its inverted.
	if (pos_x > ui.drawrectmin[0])
		ui.drawrectmin[0] = pos_x;
	if (pos_y > ui.drawrectmin[1])
		ui.drawrectmin[1] = pos_y;
	if (pos_x+clientsize_x < ui.drawrectmax[0])
		ui.drawrectmax[0] = pos_x+clientsize_x;
	if (pos_y+clientsize_y < ui.drawrectmax[1])
		ui.drawrectmax[1] = pos_y+clientsize[1];
	if (ui.drawrectmax[0] > ui.drawrectmin[0] && ui.drawrectmax[1] > ui.drawrectmin[1])
	{
		ui.setcliparea(ui.drawrectmin[0], ui.drawrectmin[1], ui.drawrectmax[0] - ui.drawrectmin[0], ui.drawrectmax[1] - ui.drawrectmin[1]);

		float c = max(0,floor((ui.drawrectmin[1]-clientpos_y)/item_scale));	//calculate how many we can skip
		for (clientpos_y += item_scale * c; c < grid_numchildren; c++, clientpos_y += item_scale)
		{
			if (clientpos_y >= ui.drawrectmax[1])
				break;
			grid_draw(clientpos, c);
		}
		ui.setcliparea(omin_x, omin_y, omax_x - omin_x, omax_y - omin_y);

		if (vslider)
		{
			vslider.item_size[1] = clientsize[1];
			vslider.item_draw(pos + [clientsize[0], 0]);
		}
	}
	ui.drawrectmin = omin;
	ui.drawrectmax = omax;
};
#endif
