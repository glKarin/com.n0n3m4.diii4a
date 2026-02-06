/***************************************************************************
key binding item.
interactable - queries binds for a command, and accepts new scan codes to bind for its given command.
*/
class mitem_bind : mitem
{
	virtual void(vector pos) item_draw;
	virtual float(vector pos, float scan, float char, float down) item_keypress;
	virtual void(mitem newfocus, float flag) item_focuschange;

	void() mitem_bind =
	{
		item_scale = item_size[1];

		item_flags |= IF_SELECTABLE;
		
		item_text = strzone(item_text);
		item_command = strzone(item_command);
	};
	virtual void() item_remove =
	{
		strunzone(item_text);
		strunzone(item_command);
	};
};
#define menuitembind_spawn(text,command,sz)	\
	spawn(mitem_bind,			\
		item_text:		text,		\
		item_command:	command,	\
		item_size:		sz		\
		)

void(vector pos) mitem_bind::item_draw =
{
#ifdef CSQC
	tokenize(findkeysforcommandex(self.item_command));
	string key1 = argv(0);
	string key2 = argv(1);
#else
	/*this is not my API...*/
	tokenize(findkeysforcommand(self.item_command));
	string key1 = argv(0);
	string key2 = argv(1);
	if (key1 != "") key1 = (key1=="-1")?"":keynumtostring(stof(key1));
	if (key2 != "") key2 = (key2=="-1")?"":keynumtostring(stof(key2));
#endif

	super::item_draw(pos);
	pos_x += self.item_size_x / 2;

	if (self.item_flags & IF_INTERACT)
	{
		ui.drawstring(pos, "Please press a key", '1 1 0' * self.item_scale, menuitem_textcolour(self), self.item_alpha, 0);
	}
	else
	{
		ui.drawstring(pos, key1, '1 1 0' * self.item_scale, menuitem_textcolour(self), self.item_alpha, 0);
		pos_x += stringwidth(key1, TRUE, '1 1 0'*self.item_scale);

		if (key2 != "")
		{
			ui.drawstring(pos, "  or  ", '1 1 0' * self.item_scale, menuitem_textcolour(self), self.item_alpha, 0);
			pos_x += stringwidth("  or  ", TRUE, '1 1 0'*self.item_scale);

			ui.drawstring(pos, key2, '1 1 0' * self.item_scale, menuitem_textcolour(self), self.item_alpha, 0);
//			pos_x += stringwidth(key2, TRUE, '1 1 0'*self.item_scale);
		}
	}
};
float(vector pos, float scan, float char, float down) mitem_bind::item_keypress =
{
	if (!down)
		return FALSE;

	if (self.item_flags & IF_INTERACT)
	{
		if (scan == K_ESCAPE || scan == K_GP_START)	//keys that cannot/shouldnot be bound can be used to cancel here.
		{
		}
		else if (scan)
			localcmd(sprintf("bind \"%s\" \"%s\"\n", keynumtostring(scan), self.item_command));
		else
			return FALSE;
		self.item_flags -= IF_INTERACT;
		return TRUE;
	}
	else
	{
		if (scan == K_ENTER || scan == K_GP_A || ((scan == K_TOUCHTAP || scan == K_MOUSE1) && mouseinbox(pos, self.item_size)))
		{
			self.item_flags |= IF_INTERACT;
			return TRUE;
		}
		if (scan == K_DEL || scan == K_GP_Y || scan == K_BACKSPACE)
		{
#ifdef CSQC
			float c = tokenize(findkeysforcommandex(self.item_command));
			for (float i = 0; i < c; i++)
				localcmd(sprintf("bind \"%s\" \"\"\n", argv(i)));
#else
			/*again, this is not my API...*/
			tokenize(findkeysforcommand(self.item_command));
			string key1 = argv(0);
			string key2 = argv(1);
			if (key1 != "") key1 = (key1=="-1")?"":keynumtostring(stof(key1));
			if (key2 != "") key2 = (key2=="-1")?"":keynumtostring(stof(key2));
			if (key1 != "") localcmd(sprintf("bind \"%s\" \"\"\n", key1));
			if (key2 != "") localcmd(sprintf("bind \"%s\" \"\"\n", key2));
#endif
			return TRUE;
		}
		return FALSE;
	}
};
void(mitem newfocus, float flag) mitem_bind::item_focuschange =
{
	if (!(self.item_flags & IF_KFOCUSED))
		self.item_flags = self.item_flags - (self.item_flags & IF_INTERACT);
};

