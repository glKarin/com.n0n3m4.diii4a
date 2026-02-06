/*reusable code to draw/edit/etc some generic text object*/
/*consult the brief blurb above each function to see what it does*/

/*this file deals with textfield_t pointers. allocate your textfield somewhere yourself (probably as a global, this is QC after all), before invoking this code with a pointer to it*/
#define TEXTFIELD_LINES 200i
typedef struct
{
	string lines[TEXTFIELD_LINES];
	int cursorline;
	int cursorcol;
	int linecount;
	int dirty;
} textfield_t;

/*this is a text description thing*/
typedef struct
{
	string name;
	string desc;
} textfield_linedesc_t;

/*destroys the contents of a text field, releasing strings to the system (and clears ready for reuse)*/
void(textfield_t *fld) textfield_clean =
{
	while(fld->linecount)
	{
		fld->linecount = fld->linecount - 1i;
		strunzone(fld->lines[fld->linecount]);
		fld->lines[fld->linecount] = (string)__NULL__;
	}
	fld->cursorline = 0i;
	fld->cursorcol = 0i;
	fld->dirty = TRUE; //because its changed
};

/*fill a text field with data from a string. new lines are supported, but leading tabs and spaces will be stripped*/
void(textfield_t *fld, string text) textfield_fill =
{
	string line;
	float start, end, term;

	while(fld->linecount)
	{
		fld->linecount = fld->linecount - 1i;
		strunzone(fld->lines[fld->linecount]);
		fld->lines[fld->linecount] = (string)__NULL__;
	}

	fld->cursorline = -1i;
	fld->cursorcol = -1i;
	start = 0;
	for(;;)
	{
		end = strstrofs(text, "\n", start);
		if (end < 0)
			break;
		while(text[start] == '\t')
			start+=1;
term = end;
if (term) if (text[term-1] == '\r') term-=1;
		line = substring(text, start, term - start);
		end += 1;

		if (fld->linecount == TEXTFIELD_LINES)
			break;
		fld->lines[fld->linecount] = strzone(line);
		fld->linecount = fld->linecount + 1i;

		start = end;
	}
	fld->dirty = TRUE;
};


/*draws the text field on the screen at a given position.
there's no size argument, so make sure data doesn't cause the display to overflow intended area (personally I'm just going to put it at the bottom+right of the screen)*/
void(vector pos, textfield_t *fld, textfield_linedesc_t *fields) textfield_draw =
{
	string t;
	int l;
	int nummatches = 0;
	int lastmatch = 0;
	for (l = 0i; l < fld->linecount; l+=1i, pos_y += 8)
	{
		if (l == fld->cursorline && ((cltime * 4) & 1))
		{
			t = substring(fld->lines[l], 0, fld->cursorcol);
			drawrawstring(pos, t, '8 8 0', '1 1 1', 1, 0);
			drawrawstring(pos + (0+fld->cursorcol) * '8 0 0', "\x0b", '8 8 0', '1 0 0', 1, 0);
			t = substring(fld->lines[l], fld->cursorcol+1, -1);
			drawrawstring(pos + (1+fld->cursorcol) * '8 0 0', t, '8 8 0', '1 1 1', 1, 0);
		}
		else
			drawrawstring(pos, fld->lines[l], '8 8 0', '1 1 1', 1, 0);
	}

	if (fld->cursorline < 0)
		return;

	pos_y += 8;
	pos_x += 64;
	string cmd = fld->lines[fld->cursorline];
	float llen = strstrofs(cmd, " ");
	if (llen < 0f)
		llen = strlen(cmd);
	cmd = substring(cmd, 0, llen);

	/*show command completion box*/
	for (l = 0; fields[l].name; l+=1)
	{
		if (!strncmp(fields[l].name, cmd, llen))
		{
			nummatches+=1;
			lastmatch = l;
			/*if its an exact match, then stop here*/
			if (cmd == fields[l].name)
			{
				nummatches = 1;
				break;
			}
		}
	}

	if (nummatches == 1 && strlen(fields[lastmatch].name) == llen)
	{
		drawrawstring(pos, fields[lastmatch].desc, '8 8 0', '0 1 1', 1, 0);
	}
	else
	{
		/*show command completion box*/
		for (l = 0; fields[l].name; l+=1)
		{
			if (!strncmp(fields[l].name, cmd, llen))
			{
				drawrawstring(pos, fields[l].name, '8 8 0', '0 1 1', 1, 0);
				pos_y += 8;
			}
		}
	}
};

/*send the entire text field into the localcmd builtin, newlines and all*/
void(textfield_t *fld) textfield_localcmd =
{
	int l;
	for (l = 0i; l < fld->linecount; l+=1i)
	{
		localcmd(fld->lines[l]);
		localcmd("\n");
	}
};

/*returns the current contents of the text field as a single tempstring (strcat, woo)*/
string(textfield_t *fld) textfield_strcat =
{
	int l;
	string res = "";
	for (l = 0i; l < fld->linecount; l+=1i)
	{
		res = strcat(res, fld->lines[l], "\n");
	}
	return res;
};

/*if the user clicks+presses+etc upon the text field, do the magic here
the caller will need to track selected wigit themselves.*/
float(textfield_t *fld, float keyc, float unic, vector m) textfield_key
{
	int i;
	string l,r;
	/*mouse*/
	if (keyc == 512)
	{
		fld->cursorline = floor(m_y / 8);
		fld->cursorcol = floor(m_x / 8);
		if (fld->cursorline > fld->linecount && fld->linecount)
			fld->cursorline = -1i;
		return TRUE;
	}
	else if (fld->cursorline == -1i)
	{
		/*if its not active, do nothing*/
		return FALSE;
	}
	/*escape*/
	else if (keyc == 27)
	{
		fld->cursorline = -1i;
		return TRUE;
	}
	/*backspace*/
	else if (keyc == 127)
	{
		if (fld->cursorline >= fld->linecount)
		{
			/*we're beyond the bounds of the buffer*/
			if (fld->cursorline)
				fld->cursorline = fld->cursorline - 1i;
		}
		else if (fld->cursorcol)
		{
			/*simple delete*/
			l = substring(fld->lines[fld->cursorline], 0, fld->cursorcol-1);
			r = substring(fld->lines[fld->cursorline], fld->cursorcol, -1);
			strunzone(fld->lines[fld->cursorline]);
			fld->lines[fld->cursorline] = strzone(strcat(l, r));
			fld->cursorcol = fld->cursorcol - 1i;
		}
		else if (fld->cursorline)
		{
			/*at the start of the line, merge the line with thee previous*/
			l = fld->lines[(fld->cursorline - 1i)];
			r = fld->lines[fld->cursorline];
			fld->cursorcol = strlen(l);
			l = strcat(l, r);
			strunzone(fld->lines[fld->cursorline - 1i]);
			fld->lines[fld->cursorline - 1i] = strzone(l);
			for (i = fld->cursorline; i < fld->linecount - 1i; i+=1)
			{
				fld->lines[i] = fld->lines[i + 1i];
			}
			fld->cursorline = fld->cursorline - 1i;
			fld->linecount = fld->linecount - 1i;
		}
	}
	/*delete*/
	else if (keyc == 8)
	{
		if (fld->cursorline >= fld->linecount)
		{
			/*we're beyond the bounds of the buffer*/
			if (fld->cursorline)
				fld->cursorline = fld->cursorline - 1i;
		}
		else if (fld->cursorcol >= strlen(fld->lines[fld->cursorline]))
		{
			/*we're at the end of the line, merge next line in to this one*/
			l = fld->lines[(fld->cursorline)];
			r = fld->lines[fld->cursorline + 1i];
			fld->cursorcol = strlen(l);
			l = strcat(l, r);
			strunzone(fld->lines[fld->cursorline]);
			fld->lines[fld->cursorline] = strzone(l);
			for (i = fld->cursorline + 1i; i < fld->linecount - 1i; i+=1)
			{
				fld->lines[i] = fld->lines[i + 1i];
			}
			fld->linecount = fld->linecount - 1i;
		}
		else
		{
			/*simple delete*/
			l = substring(fld->lines[fld->cursorline], 0, fld->cursorcol);
			r = substring(fld->lines[fld->cursorline], fld->cursorcol + 1, -1);
			strunzone(fld->lines[fld->cursorline]);
			fld->lines[fld->cursorline] = strzone(strcat(l, r));
			fld->cursorcol = fld->cursorcol - 1i;
		}
	}
	/*home*/
	else if (keyc == 151)
	{
		fld->cursorcol = 0;
	}
	/*end*/
	else if (keyc == 152)
	{
		fld->cursorcol = strlen(fld->lines[fld->cursorline]);
	}
	/*leftarrow*/
	else if (keyc == 130)
	{
		if (fld->cursorcol)
			fld->cursorcol = fld->cursorcol - 1i;
	}
	/*rightarrow*/
	else if (keyc == 131)
	{
		fld->cursorcol = fld->cursorcol + 1i;
	}
	/*uparrow*/
	else if (keyc == 128)
	{
		if (fld->cursorline)
			fld->cursorline = fld->cursorline - 1i;
	}
	/*downarrow*/
	else if (keyc == 129)
	{
		if (fld->cursorline < fld->linecount)
			fld->cursorline = fld->cursorline + 1i;
	}
	/*return*/
	else if (keyc == 13)
	{
		if (fld->linecount < TEXTFIELD_LINES)
		{
			/*shift trailing lines down*/
			fld->linecount = fld->linecount + 1i;
			for (i = fld->linecount - 1i; i > fld->cursorline; i-=1)
			{
				fld->lines[i] = fld->lines[i - 1i];
			}
			/*set the new line to the second half of the current line*/
			fld->lines[fld->cursorline + 1i] = strzone(substring(fld->lines[fld->cursorline], fld->cursorcol, -1));
			/*update the old line to be the first half only*/
			r = strzone(substring(fld->lines[fld->cursorline], 0, fld->cursorcol));
			strunzone(fld->lines[fld->cursorline]);
			fld->lines[fld->cursorline] = r;
			/*and set the cursor properly*/
			fld->cursorline = fld->cursorline + 1i;
			fld->cursorcol = 0;
		}
	}
	/*printable char*/
	else if (unic)
	{
		if (fld->cursorline >= fld->linecount)
		{
			fld->cursorline = fld->linecount;
			fld->linecount += 1i;
			fld->lines[fld->cursorline] = strzone(chr2str(unic));
			fld->cursorcol = 1i;
		}
		else
		{
			l = substring(fld->lines[fld->cursorline], 0, fld->cursorcol);
			r = substring(fld->lines[fld->cursorline], fld->cursorcol, -1);
			strunzone(fld->lines[fld->cursorline]);
			fld->lines[fld->cursorline] = strzone(strcat(l, chr2str(unic), r));
			fld->cursorcol = fld->cursorcol+1i;
		}

	}
	else
		return FALSE;
	fld->dirty = TRUE;
	return TRUE;
}
