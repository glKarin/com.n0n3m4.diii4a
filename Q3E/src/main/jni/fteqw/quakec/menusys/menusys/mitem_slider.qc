/***************************************************************************
slider item, directly attached to a cvar.
interactable - executes a given console command.
*/
class mitem_hslider : mitem
{
	virtual void(vector pos) item_draw;
	virtual float(vector pos, float scan, float char, float down) item_keypress;

	vector item_slidercontrols;	//min, max, step
	
	virtual void() item_resized =
	{
		if (isvalid(item_command))
			item_flags |= IF_SELECTABLE;		
		else
			item_flags &= ~IF_SELECTABLE;
		super::item_resized();
	};
};
void(vector pos) mitem_hslider::item_draw =
{
	local float curval;
	vector rgb = self.item_rgb;
	if (!(item_flags & IF_SELECTABLE))
		rgb *= 0.2;

	super::item_draw(pos);
	pos_x += item_size_x / 2;

	if (ui.mgrabs == this)
	{
		//if we're sliding it, update the value
		curval = (ui.mousepos[0] - pos_x-8) / (10*8);
		curval = bound(0, curval, 1);
		curval = curval * (item_slidercontrols_y - item_slidercontrols_x);
		if (!ui.shiftheld)
			curval = rint(curval / item_slidercontrols_z) * item_slidercontrols_z;	//round it.
		curval += item_slidercontrols_x;
		set(item_command, sprintf("%g", curval));
	}
	curval = stof(get(item_command));

	if (dp_workarounds)
	{	//no ^U markup support. chr2str avoids warnings about non-utf8 strings which at least allows bi-compat to work.
		string s = strcat(chr2str(0xe080, 0xe081, 0xe081, 0xe081, 0xe081, 0xe081), chr2str(0xe081, 0xe081, 0xe081, 0xe081, 0xe081, 0xe082));
		//slider background uses the fallback quake chars
		ui.drawstring(pos, sprintf("%s (%g)", s, curval), '1 1 0' * self.item_scale, rgb, self.item_alpha, 0);
		//now draw an indicater char in the right place.
		//the inner side of the boundary is 4 pixels wide so we can overlap the ends by that many pixels.
		curval = (curval - self.item_slidercontrols_x) / (self.item_slidercontrols_y - self.item_slidercontrols_x);	//fractionize it
		curval = bound(0, curval, 1);
		ui.drawstring(pos + [4 + curval*10*8, 0], chr2str(0xe083), '1 1 0' * self.item_scale, rgb, self.item_alpha, 0);
	}
	else
	{
		//slider background uses the fallback quake chars
		ui.drawstring(pos, sprintf("^{e080}^{e081}^{e081}^{e081}^{e081}^{e081}^{e081}^{e081}^{e081}^{e081}^{e081}^{e082} (%g)", curval), '1 1 0' * self.item_scale, rgb, self.item_alpha, 0);
		//now draw an indicater char in the right place.
		//the inner side of the boundary is 4 pixels wide so we can overlap the ends by that many pixels.
		curval = (curval - self.item_slidercontrols_x) / (self.item_slidercontrols_y - self.item_slidercontrols_x);	//fractionize it
		curval = bound(0, curval, 1);
		ui.drawstring(pos + [4 + curval*10*8, 0], "^{e083}", '1 1 0' * self.item_scale, rgb, self.item_alpha, 0);
	}
};
float(vector pos, float scan, float char, float down) mitem_hslider::item_keypress =
{
	if (down&2)
	{
		//we have grabs, and mouse was released?
		if ((scan == K_TOUCHTAP || scan == K_MOUSE1) && !(down&1))
		{	//we're done here.
			ui.mgrabs = __NULL__;
			return TRUE;
		}
		return FALSE;	//not handled, don't inhibit
	}
	if (down)
	{
		local float curval = stof(get(item_command));
		if (scan == K_MWHEELUP || scan == K_MWHEELDOWN)
		{
			if (ui.mousepos[0] > pos_x + item_size[0]/2)
				scan = ((scan == K_MWHEELDOWN)?K_LEFTARROW:K_RIGHTARROW);
		}
		if ((scan == K_TOUCHTAP || scan == K_MOUSE1) && down)
		{
			pos_x += item_size_x / 2;
			if (ui.mousepos[0] < pos_x)
				return TRUE;//goto keyenter;
			curval = (ui.mousepos[0] - pos_x-8) / (10*8);
			if (curval < 0 || curval > 1)
				return FALSE;
			curval = curval * (item_slidercontrols_y - item_slidercontrols_x) + item_slidercontrols_x;
			set(item_command, sprintf("%g", curval));
			ui.mgrabs = this;
		}
		else if ((scan == K_DEL || scan == K_GP_Y) && down && cvar_type(item_command))
			set(item_command, cvar_defstring(item_command));
		else if ((ISLEFTARROW(scan) || scan == K_MWHEELUP) && down)
		{
			if (item_slidercontrols_x > item_slidercontrols_y)
				set(item_command, sprintf("%g", min(curval - item_slidercontrols_z, item_slidercontrols_x)));
			else
				set(item_command, sprintf("%g", max(curval - item_slidercontrols_z, item_slidercontrols_x)));
		}
		else if ((ISRIGHTARROW(scan) || scan == K_MWHEELDOWN) && down)
		{
			if (item_slidercontrols_x > item_slidercontrols_y)
				set(item_command, sprintf("%g", max(curval + item_slidercontrols_z, item_slidercontrols_y)));
			else
				set(item_command, sprintf("%g", min(curval + item_slidercontrols_z, item_slidercontrols_y)));
		}
		else if ((ISCONFIRMKEY(scan) || scan == K_SPACE) && down)
		{
//keyenter:
			if (item_slidercontrols_x > item_slidercontrols_y)
			{
				if (curval-0.001 <= item_slidercontrols_y)
					set(item_command, sprintf("%g", item_slidercontrols_x));
				else
					set(item_command, sprintf("%g", max(curval + item_slidercontrols_z, item_slidercontrols_y)));
			}
			else
			{
				if (curval+0.001 >= item_slidercontrols_y)
					set(item_command, sprintf("%g", item_slidercontrols_x));
				else
					set(item_command, sprintf("%g", min(curval + item_slidercontrols_z, item_slidercontrols_y)));
			}
		}
		else
			return FALSE;
		return TRUE;
	}
	else if ((scan == K_TOUCHTAP || scan == K_MOUSE1) && ui.mgrabs == this)
		ui.mgrabs = __NULL__;
	return FALSE;
};
mitem_hslider(string text, string command, vector controls, vector sz) menuitemslider_spawn =
{
	mitem_hslider n = spawn(mitem_hslider);
	n.item_scale = sz_y;
	n.item_text = text;
	n.item_size = sz;

	n.item_slidercontrols = controls;

	n.item_command = command;
	if (n.isvalid(command))
		n.item_flags |= IF_SELECTABLE;
	return n;
};
