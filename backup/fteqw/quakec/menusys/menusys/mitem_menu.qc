/***************************************************************************
window-like menu.
interactable - basically just a container for the items in the menu, but also handles killing them+itself when the user presses escape.

these items will automatically be added to the desktop/workspace thing
Call it.item_remove() to pop the menu.
Regular items can be added to the menu by first spawning them, then calling menu_additem to actually add it to the desired menu in the right place.
*/

class mitem_menu : mitem_frame
{
	vector draginfo;	//x, y, resizeflags={0:none, 1:left, 2:bottom, 4:right, 8:top, 16:move}

	virtual float(vector pos, float scan, float char, float down) item_keypress;
	virtual void(mitem newfocus, float flag) item_focuschange;
	virtual void(vector pos) item_draw;

	virtual void() item_remove =
	{
		strunzone(item_text);
		super::item_remove();
	};

	nonvirtual float(vector pos) whichgrabhandle;
	nonvirtual void(vector pos, float handle) drawgrabhandle;
	void() mitem_menu =
	{
		item_text = strzone(item_text);
		item_scale = 8;
		item_rgb = MENUBACK_RGB;
		item_alpha = MENUBACK_ALPHA;

		if (item_framesize == '0 0 0')
			item_framesize = '4 16 4';

		item_size[0] += item_framesize[0];
		item_size[1] += item_framesize[1]+item_framesize[2];

		item_flags |= IF_SELECTABLE;
		
#ifdef CSQC
		cprint("");
#endif
	};

	//same as mitem, but does not propogate to parent.
	virtual string(string key) get =
	{
		return cvar_string(key);
	};
	virtual void(string key, string newval) set =
	{
		cvar_set(key, newval);
	};
};

void(mitem newfocus, float flag) mitem_menu::item_focuschange =
{
	if (flag & IF_KFOCUSED)
	{
		//move the window to the top of the z-order when it gets focus.
		if (item_flags & IF_KFOCUSED)
			totop();
		//release grabs if someone else took focus.	
		else if (ui.mgrabs == this)
		{
			ui.mgrabs = __NULL__;
			draginfo[2] = 0;
		}
	}

	super::item_focuschange(newfocus, flag);
};

void(vector pos, float handle) mitem_menu::drawgrabhandle =
{
	if ((handle & 3) == 3)
	{
		//bottom left
		drawfill(pos + [0, item_size[1]-item_framesize[2]], [item_framesize[0], item_framesize[2]], '1 1 0', 1, 0);
		handle &~= 3;
	}
	if ((handle & 6) == 6)
	{
		//bottom right
		drawfill(pos + [item_size[0]-item_framesize[0], item_size[1]-item_framesize[2]], [item_framesize[0], item_framesize[2]], '1 1 0', 1, 0);
		handle &~= 6;
	}
	if (handle & 1)
	{
		//left
		drawfill(pos + [0, 0], [item_framesize[0], item_size[1]], '1 1 0', 1, 0);
	}
	if (handle & 2)
	{
		//bottom
		drawfill(pos + [0, item_size[1]-item_framesize[2]], [item_size[0], item_framesize[2]], '1 1 0', 1, 0);
	}
	if (handle & 4)
	{
		//right
		drawfill(pos + [item_size[0]-item_framesize[0], 0], [item_framesize[0], item_size[1]], '1 1 0', 1, 0);
	}
};

float(vector pos) mitem_menu::whichgrabhandle =
{
	//handle dragging and resizing.
	if (mouseinbox(pos, [item_size[0]-item_framesize[1], item_framesize[1]]))
	{
		//drag
		return 16;
	}
	else if (this.item_flags & IF_RESIZABLE)
	{
		if (mouseinbox(pos + [0, item_size[1]-item_framesize[2]], [item_framesize[0], item_framesize[2]]))
		{
			//bottom-left
			return 3;
		}
		else if (mouseinbox(pos + [item_size[0]-item_framesize[0], item_size[1]-item_framesize[2]], [item_framesize[0], item_framesize[2]]))
		{
			//bottom-right
			return 6;
		}
		else if (mouseinbox(pos + [0, 0], [item_framesize[0], item_size[1]]))
		{
			//left
			return 1;
		}
		else if (mouseinbox(pos + [0, item_size[1]-item_framesize[2]], [item_size[0], item_framesize[2]]))
		{
			//bottom
			return 2;
		}
		else if (mouseinbox(pos + [item_size[0]-item_framesize[0], 0], [item_framesize[0], item_size[1]]))
		{
			//right
			return 4;
		}
	}
	return 0;
};

float(vector pos, float scan, float char, float down) mitem_menu::item_keypress =
{
	if (scan == K_MOUSE1 && (down&1))
	{
		if (!(item_flags & IF_NOKILL) && mouseinbox(pos + [item_size[0]-item_framesize[1], 0], item_framesize[1] * '1 1'))
		{
			localcmd(strcat(item_command, "\n"));	//console command to exec if someone clicks the close button.
			this.item_remove();
			return TRUE;
		}
		draginfo[2] = this::whichgrabhandle(pos);
		if (draginfo[2])
			ui.mgrabs = this;
		if (draginfo[2] & (1|16))
			draginfo[0] = ui.mousepos[0] - pos_x;
		if (draginfo[2] & 2)
			draginfo[1] = (ui.mousepos[1] - pos_y - item_size[1]) + item_position[1];
		if (draginfo[2] & 4)
			draginfo[0] = ui.mousepos[0] - pos_x - item_size[0] + item_position[0];
		if (draginfo[2] & (8|16))
			draginfo[1] = ui.mousepos[1] - pos_y;
		if (draginfo[2])
			return TRUE;
	}
	else if (scan == K_MOUSE1 && draginfo[2])
	{
		if (!draginfo[2] && (item_flags & IF_RESIZABLE))
		{
			//if they dropped the window at the top of the screen, make sure its fullscreened to match how it was drawn...
			queryscreensize();
			if (ui.mousepos[1] < 8)
				item_size = ui.screensize;
			if (this.item_resized)
				item_resized();
		}
		ui.mgrabs = __NULL__;
		draginfo[2] = 0;
		return TRUE;
	}
	
	if (!super::item_keypress(pos, scan, char, down))
	{
		down &= 1;
		if (scan == K_ESCAPE && down)
		{
			if (this.item_flags & IF_NOKILL)
				return FALSE;
			else
			{
				localcmd(strcat(this.item_command, "\n"));	//console command to exec if someone clicks the close button.
				this.item_remove();
				return TRUE;
			}
		}
		else if (scan == K_UPARROW && down)
			menu_selectnextitem(this, TRUE);
		else if (scan == K_DOWNARROW && down)
			menu_selectnextitem(this, FALSE);
		else if (scan == K_MOUSE2 && down && !(this.item_flags & IF_NOKILL))
		{	//unhandled right click closes menus, if we're allowed
			localcmd(strcat(item_command, "\n"));	//console command to exec if someone clicks the close button.
			this.item_remove();
			return TRUE;
		}
		else	if (scan >= K_MOUSE1 && scan <= K_MWHEELDOWN)
			return TRUE;	//don't let the parent take unhandled mouse events.
		else
			return FALSE;
	}
	return TRUE;
};
void(vector pos) mitem_menu::item_draw =
{
	float inset;
	vector efsize = item_size;
	if (draginfo[2])
	{
		if (draginfo[2] & 1)
		{	//resize left
			local float nmax = item_position[0] + item_size[0];
			local float nmin = item_position[0] + item_size[0];
			nmin = ui.mousepos[0] - (pos_x - item_position[0]) - draginfo[0];
			if (nmax-nmin < 128)
				nmin = nmax - 128;
			pos_x -= item_position[0];
			item_position[0] = nmin;
			pos_x += item_position[0];
			item_size[0] = nmax - nmin;
			if (item_resized)
				item_resized();
		}
		if (draginfo[2] & 2)
		{	//resize bottom
			item_size[1] = ui.mousepos[1] - (pos_y - item_position[1]) - draginfo[1];
			if (item_size[1] < 16)
				item_size[1] = 16;
			if (item_resized)
				item_resized();
		}
		if (draginfo[2] & 4)
		{	//resize right
			item_size[0] = ui.mousepos[0] - (pos_x - item_position[0]) - draginfo[0];
			if (item_size[0] < 128)
				item_size[0] = 128;
			if (item_resized)
				item_resized();
		}
		efsize = item_size;
		if (draginfo[2]&16)
		{
			queryscreensize();
			if (ui.mousepos[1] < 8 && (item_flags & IF_RESIZABLE))
			{
				//make it look as though its fullscreen, without destroying its actual size.
				//yes, we will fake-resize everything inside.
				item_position = '0 0';

				item_size = ui.screensize;
				if (this.item_resized)
					this.item_resized();
				item_size = efsize;
				efsize = ui.screensize;
			}
			else if (item_size == ui.screensize && (item_flags & IF_RESIZABLE))
			{
				item_size = item_size*0.5;
				draginfo[0] = draginfo[0]*0.5;
				if (this.item_resized)
					this.item_resized();
				efsize = item_size;
			}
			else
			{
				item_position = ui.mousepos - (pos - item_position) - draginfo;
				item_position[2] = 0;
				if (item_flags & IF_RESIZABLE)
					if (item_resized)
						item_resized();
			}
		}
	}

	//bound it to the screen... just in case the screen got resized or something
	if (item_position[0] > ui.screensize[0] - item_size[0]*0.25)
		item_position[0] = ui.screensize[0] - item_size[0]*0.25;
	if (item_position[1] > ui.screensize[1] - item_framesize[1])
		item_position[1] = ui.screensize[1] - item_framesize[1];
	if (item_position[0] < item_size[0]*-0.75)
		item_position[0] = item_size[0]*-0.75;
	if (item_position[1] < 0)
		item_position[1] = 0;

	local float drag = 0;
	if (draginfo[2])
		drag = draginfo[2];
	else if (item_flags & IF_MFOCUSED)
		drag = this::whichgrabhandle(pos);
	if (drag)
		this::drawgrabhandle(pos, drag);

	ui.drawfill(pos, efsize, item_rgb, item_alpha, 0);
	local vector col = '1 1 1';
	if (item_flags & IF_KFOCUSED)
		col_x = 0;
	if ((item_flags & IF_MFOCUSED) && mouseinbox(pos, [efsize_x-item_framesize[1], item_framesize[1]]))
		col_z = 0;
	inset = (item_framesize[1]-item_scale)*0.5;
	ui.drawstring(pos + '1 1'*inset, item_text, item_scale*'1 1', col, 1, 0);
	if (!(item_flags & IF_NOKILL))
	{
		if (mouseinbox(pos + [efsize_x-item_framesize[1], 0], item_framesize[1]*'1 1'))
			col = '1 1 0';
		else
			col = '1 1 1';
		ui.drawstring(pos + [efsize_x+inset-item_framesize[1], inset], "X", item_scale*'1 1', col, 1, 0);
	}
	super::item_draw(pos);
};
mitem_menu(mitem_frame desktop, string mname, vector sz) menu_spawn =
{
	mitem_menu n = spawn(mitem_menu, item_text:mname, item_framesize:'4 16 4', item_size:sz);

//	n.item_flags |= IF_RESIZABLE;

	if (desktop)
		desktop.addr(n, RS_X_MIN_PARENT_MIN | RS_X_MAX_OWN_MIN | RS_Y_MIN_PARENT_MIN | RS_Y_MAX_OWN_MIN, (desktop.item_size - n.item_size) * 0.5, n.item_size);
	return n;
};
