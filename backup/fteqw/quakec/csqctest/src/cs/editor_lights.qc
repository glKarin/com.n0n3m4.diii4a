/*NOTE: csaddon is more up to date, with more stuff! use that instead!*/
/*FTE has some special light editing builtins, I don't ever expect them to be standard or anything, but they're handy for this*/
/*If you want to edit lights for hexen2, edit this cvar, use models/s_light.spr or something.*/
DEFCVAR_STRING(cg_editor_lightmodel, "progs/s_light.spr")

static float selectedlight;
static float editfield;
static string editvalue;
static entity tempent;
void() editor_lights_add =
{
	__using dynamiclight_get;

	float l;
	if (!tempent)
		tempent = spawn();

	l = dynamiclight_get(-1, -1);
	setmodel(tempent, CVARS(cg_editor_lightmodel));
	while(l > 0)
	{
		l = l-1;
		if (l == selectedlight)
		{
			if (time*5 & 1)
				continue;
			tempent.effects |= 8192;
		}
		else
			tempent.effects (-) 8192;
		if (!(float)dynamiclight_get(l, LFIELD_RADIUS))
			continue;
		setorigin(tempent, (vector)dynamiclight_get(l, LFIELD_ORIGIN));
		addentity(tempent);
	}
};

static string fldname[8] = {
	"bad",
	"num",
	"org",
	"rgb",
	"rad",
	"flg",
	"sty",
	"???"
};
static string(float fld, float foredit) readfield =
{
	__using dynamiclight_get;

	switch(fld)
	{
	case 1:
		if (foredit)
			return ftos(selectedlight);
		return strcat(ftos(selectedlight), " / ", ftos(dynamiclight_get(-1, -1)));
	case 2:
		return vtos((vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN));
	case 3:
		return vtos((vector)dynamiclight_get(selectedlight, LFIELD_COLOUR));
	case 4:
		return ftos(dynamiclight_get(selectedlight, LFIELD_RADIUS));
	case 5:
		float fl = dynamiclight_get(selectedlight, LFIELD_FLAGS);
		if (foredit)
			return ftos(fl);
		return strcat(	((fl & 1)?" alwaysvisible":""),
					((fl & 2)?" realtime":""),
					((fl & 65536)?" no-shadows":"")
				 );
	case 6:
		return ftos(dynamiclight_get(selectedlight, LFIELD_STYLE));
	default:
		return "";
	}
};

static void(float fld, string newval) writefield =
{
	switch(fld)
	{
	case 1:
		selectedlight = stof(newval);
		return;
	case 2:
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, stov(newval));
		return;
	case 3:
		dynamiclight_set(selectedlight, LFIELD_COLOUR, stov(newval));
		return;
	case 4:
		dynamiclight_set(selectedlight, LFIELD_RADIUS, stof(newval));
		return;
	case 5:
		dynamiclight_set(selectedlight, LFIELD_FLAGS, stof(newval));
		return;
	case 6:
		dynamiclight_set(selectedlight, LFIELD_STYLE, stof(newval));
		return;
	default:
		return;
	}
};

void() editor_lights_overlay =
{
	float i;
	string s;
//void(vector position, string text, vector scale, vector rgb, float alpha, ...) drawrawstring
	drawrawstring('0 0 0', "LIGHTS EDITOR", '8 8 0', '1 1 1', 1);
	for (i = 1; i < 8; i++)
	{
		if (editfield == i)
			s = editvalue;
		else
			s = readfield(i, 0);
		s = strcat(ftos(i), " ", fldname[i], ": ", s);
		drawrawstring('0 32 0' + '0 8 0' * i, s, '8 8 0', ((editfield == i)?'1 0 0':'1 1 1'), 1);
	}
};

static void(vector fwd, vector vorg) selectbestlight =
{
	float l, b=selectedlight, d, bd = 0;
	vector ldir;
	l = (float)dynamiclight_get(-1, -1);
	while(l > 0)
	{
		l--;
		ldir = (vector)dynamiclight_get(l, LFIELD_ORIGIN);
		ldir = normalize(ldir - vorg);
		d = fwd*ldir;
		if (d > bd)
		{
			bd = d;
			b = l;
		}
	}
	selectedlight = b;
};

float(float keyc, float unic) editor_lights_key =
{
	vector v, o;
	string ns;
	if (editfield)
	{
		ns = strcat(editvalue);
		if (keyc == 10 || keyc == 13)
		{
			writefield(editfield, ns);
			editfield = 0;
		}
		else if (keyc == 127)
		{
			if (ns != "")
				ns = substring(ns, 0, -2);
		}
		else if (keyc == 8)
		{
			ns = "";
		}
		else
			ns = strcat(ns, chr2str(unic));

		ns = strzone(ns);
		strunzone(editvalue);
		editvalue = ns;

		writefield(editfield, ns);
	}
	else if (keyc >= '0' && keyc <= '9')
	{
		editfield = keyc - '0';
		editvalue = strzone(readfield(editfield, 1));
	}
	else if (keyc == '=')
		selectedlight++;
	else if (keyc == '-')
		selectedlight--;
	else if (keyc == 'n')
		localcmd("noclip\n");
	else if (keyc == 's')
	{
		v = (vector)getviewprop(15);
		o = (vector)getviewprop(11);
		makevectors(v);
		selectbestlight(v_forward, o);
	}
	else if (keyc == 'm')
	{
		v = (vector)getviewprop(15);
		o = (vector)getviewprop(11);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, trace_endpos + trace_plane_normal*4);
	}
	else if (keyc == 'i')
	{
		for (selectedlight = 32; ; selectedlight++)
		{
			if (!(float)dynamiclight_get(selectedlight, LFIELD_RADIUS))
			{
				dynamiclight_set(selectedlight, LFIELD_RADIUS, 300);
				v = (vector)getviewprop(15);
				o = (vector)getviewprop(11);
				makevectors(v);
				traceline(o, o + v_forward*8192, true, world);
				dynamiclight_set(selectedlight, LFIELD_ORIGIN, trace_endpos + trace_plane_normal*4);
				break;
			}
		}
	}
	else if (keyc == '[')
	{
		v = (vector)getviewprop(15);
		o = (vector)getviewprop(11);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN) - trace_plane_normal);
	}
	else if (keyc == ']')
	{
		v = (vector)getviewprop(15);
		o = (vector)getviewprop(11);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN) + trace_plane_normal);
	}
	else if (keyc == 127)
		dynamiclight_set(selectedlight, LFIELD_RADIUS, 0);
	else
		return false;
	return true;
};
