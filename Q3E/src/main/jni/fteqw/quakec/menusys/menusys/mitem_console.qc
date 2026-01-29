/***************************************************************************
Embed a (sub) console in a menu item thing.

you can print to these consoles with con_print("consolename", "text to be printed\n");
they can be detached from the system console with con_getset("consolename", "hidden", "1"). be sure to getset(con, "close", "1") or unhide afterwards.
you can enumerate the active consoles with con="";while((con=con_getset(con,"next"))!=""){print(con_getset(con,"title"));}  this is useful for finding xmpp conversations. you might then want to hide them afterwards.
*/

class mitem_console : mitem
{
	virtual void() item_remove =
	{
		strunzone(item_command);
		super::item_remove();
	};
	virtual float(vector pos, float scan, float char, float down) item_keypress =
	{
		local float ret;
		if (scan == K_ESCAPE)	//let the owning menu take it.
			return FALSE;
		if (down)
		{
			ret = con_input(item_command, IE_KEYDOWN, scan, char, 0);
			if (scan == K_MOUSE1 || scan == K_MOUSE2)	//grab mouse, so we still receive mouse up events.
			{
				ui.mgrabs = this;
				ret = TRUE;
				local mitem_frame p = item_parent;
				p.item_focuschange(this, IF_KFOCUSED);
			}
		}
		else
		{
			ret = con_input(item_command, IE_KEYUP, scan, char, 0);	//note the engine never tries to cancel key up events anyway.
			if (ui.mgrabs == this)
				ui.mgrabs = __NULL__;
		}
		return ret;
	};
	virtual void(mitem newfocus, float flag) item_focuschange =
	{
		con_input(item_command, IE_FOCUS, !!(item_flags&IF_MFOCUSED), !!(item_flags&IF_KFOCUSED), 0);	
	};
	virtual void(vector pos) item_draw =
	{
		con_input(item_command, IE_MOUSEABS, ui.mousepos[0] - pos_x, ui.mousepos[1] - pos_y, 0);
		con_draw(item_command, pos, item_size, 12);	//use a 12-point font, if we can.
	};
	void() mitem_console =
	{
		item_flags |= IF_SELECTABLE;
		item_command = strzone(item_command);
	};
};

#define menuitemconsole_spawn(conname) spawn(mitem_console, item_command:conname)


