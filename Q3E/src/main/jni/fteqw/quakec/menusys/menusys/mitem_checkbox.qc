/***************************************************************************
checkbox, directly linked to a cvar.
*/
class mitem_check : mitem
{
	virtual float(vector pos, float scan, float char, float down) item_keypress =
	{
		if (!down)
			return FALSE;
	
		if (ISCONFIRMKEY(scan) || scan == K_SPACE || ISLEFTARROW(scan) || ISRIGHTARROW(scan) || scan == K_MOUSE1 || scan == K_TOUCHTAP)
		{
			pos_x += this.item_size_x / 2;
//			if (ui.mousepos[0] > pos_x || scan != K_MOUSE1)	//don't do anything if they clicked the bit on the left to select it
				set(item_command, ftos(!stof(get(item_command))));
			return TRUE;
		}
		else if ((scan == K_DEL || scan == K_GP_Y) && down && cvar_type(item_command))
			set(item_command, cvar_defstring(item_command));
		return FALSE;
	};
	virtual void(vector pos) item_draw =
	{
		vector rgb = item_rgb;
		if (!(item_flags & IF_SELECTABLE))
			rgb *= 0.2;
		local float truth = stof(get(item_command));

		super::item_draw(pos);
		pos_x += item_size_x / 2;

		if (dp_workarounds)
		{	//lame, but whatever
			ui.drawstring(pos, chr2str(0xe080, 0xe082), '1 1 0' * this.item_scale, rgb, this.item_alpha, 0);
			if (truth)
				ui.drawstring(pos + '4 0', chr2str(0xe083), '1 1 0' * this.item_scale, rgb, this.item_alpha, 0);
		}
		else
		{
			ui.drawstring(pos, "^{e080}^{e082}", '1 1 0' * this.item_scale, rgb, this.item_alpha, 0);
			if (truth)
				ui.drawstring(pos + '4 0', "^{e083}", '1 1 0' * this.item_scale, rgb, this.item_alpha, 0);
		}
	};
	void() mitem_check =
	{
		if (!item_scale)
			item_scale = 8;
		if (!item_size_y)
			item_size_y = item_scale;
		if (isvalid(item_command))
			item_flags |= IF_SELECTABLE;
	};
	
	virtual void() item_resized =
	{
		if (isvalid(item_command))
			item_flags |= IF_SELECTABLE;		
		else
			item_flags &= ~IF_SELECTABLE;
		super::item_resized();
	};
};

//optional, can spawn direcly
mitem_check(string text, string command, vector sz) menuitemcheck_spawn =
{
	return spawn(mitem_check, item_scale: sz_y, item_text: text, item_size: sz, item_command: command);
};
