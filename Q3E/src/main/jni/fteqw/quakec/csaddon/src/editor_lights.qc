/*FTE has some special light editing builtins, I don't ever expect them to be standard or anything, but they're handy for this*/

/*you probably want to change this if you're running hexen2*/
var string autocvar_cg_editor_lightmodel = "progs/s_light.spr";

var float autocvar_r_editlights_import_ambient = 0;
var float autocvar_r_editlights_import_diffuse = 1;
var float autocvar_r_editlights_import_specular = 1;

static float selectedlight;
static float editfield;
static string editvalue;
static entity tempent;
static float helphidden;
void() editor_lights_addentities =
{
	float l;
	if (!tempent)
		tempent = spawn();

	l = (float)dynamiclight_get(-1, -1);
	precache_model(autocvar_cg_editor_lightmodel); /*just to silence it*/
	setmodel(tempent, autocvar_cg_editor_lightmodel);
	while(l > 0)
	{
		l = l-1.0;
		if (l == selectedlight)
		{
			if (gettime(0)*5f & 1)
				continue;
			tempent.effects |= 8192f;
		}
		else
			tempent.effects &= ~8192f;
		if (!(float)dynamiclight_get(l, LFIELD_RADIUS))
			continue;
		setorigin(tempent, dynamiclight_get(l, LFIELD_ORIGIN));
		addentity(tempent);
	}
};

#define NUMLFIELDS 15
#define NUMCMDS 7
static var string fldname[NUMLFIELDS+1+NUMCMDS] = {	"bad",
	"  num",
	"  org",
	"  rgb",
	"  rad",
	"  flg",
	"  sty",
	"  ang",
	"  fov",
	"  cmp",
	 " cor",
	 " csc",
	 " amb",
	 " dif",
	 " spc",
	 " avl",

	"Save",
	"World Lights",
	"Dynamic Lights",
	"Toggle Help",
	"Wipe All",
	"Import",
	"Reload"
};
static string(int fld, float foredit) readfield =
{
	switch(fld)
	{
	case 1:
		if (foredit)
			return ftos(selectedlight);
		return strcat(ftos(selectedlight), " / ", ftos(dynamiclight_get(-1f, -1f)));
	case 2:
		return sprintf("%v", (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN));
	case 3:
		return sprintf("%v", (vector)dynamiclight_get(selectedlight, LFIELD_COLOUR));
	case 4:
		return ftos(dynamiclight_get(selectedlight, LFIELD_RADIUS));
	case 5:
		float fl = (float)dynamiclight_get(selectedlight, LFIELD_FLAGS);
		string ret;
		ret = strcat(ret, (fl & LFLAG_NORMALMODE)?"d":"");
		ret = strcat(ret, (fl & LFLAG_REALTIMEMODE)?"w":"");
		ret = strcat(ret, (fl & LFLAG_LIGHTMAP)?"l":"");
		ret = strcat(ret, (fl & LFLAG_FLASHBLEND)?"f":"");
		ret = strcat(ret, (fl & LFLAG_NOSHADOWS)?"c":"");
		ret = strcat(ret, (fl & LFLAG_SHADOWMAP)?"s":"");
		ret = strcat(ret, (fl & LFLAG_CREPUSCULAR)?"r":"");
		ret = strcat(ret, (fl & LFLAG_ORTHOSUN)?"o":"");
		return ret;
	case 6:
		return ftos(dynamiclight_get(selectedlight, LFIELD_STYLE));
	case 7:
		return sprintf("%v", (vector)dynamiclight_get(selectedlight, LFIELD_ANGLES));
	case 8:
		return ftos(dynamiclight_get(selectedlight, LFIELD_FOV));
	case 9:
		return (string)dynamiclight_get(selectedlight, LFIELD_CUBEMAPNAME);
	case 10:
		return ftos(dynamiclight_get(selectedlight, LFIELD_CORONA));
	case 11:
		return ftos(dynamiclight_get(selectedlight, LFIELD_CORONASCALE));
	case 12:
		return ftos(dynamiclight_get(selectedlight, LFIELD_AMBIENTSCALE));
	case 13:
		return ftos(dynamiclight_get(selectedlight, LFIELD_DIFFUSESCALE));
	case 14:
		return ftos(dynamiclight_get(selectedlight, LFIELD_SPECULARSCALE));
	case 15:
		return sprintf("%v", (vector)dynamiclight_get(selectedlight, LFIELD_ROTATION));
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
		var float fl = 0;
		if (strstrofs(newval, "d")>=0) fl |= LFLAG_NORMALMODE;
		if (strstrofs(newval, "w")>=0) fl |= LFLAG_REALTIMEMODE;
		if (strstrofs(newval, "l")>=0) fl |= LFLAG_LIGHTMAP;
		if (strstrofs(newval, "f")>=0) fl |= LFLAG_FLASHBLEND;
		if (strstrofs(newval, "c")>=0) fl |= LFLAG_NOSHADOWS;
		if (strstrofs(newval, "s")>=0) fl |= LFLAG_SHADOWMAP;
		if (strstrofs(newval, "r")>=0) fl |= LFLAG_CREPUSCULAR;
		if (strstrofs(newval, "o")>=0) fl |= LFLAG_ORTHOSUN;
		dynamiclight_set(selectedlight, LFIELD_FLAGS, (float)fl);
		return;
	case 6:
		dynamiclight_set(selectedlight, LFIELD_STYLE, stof(newval));
		return;
	case 7:
		dynamiclight_set(selectedlight, LFIELD_ANGLES, stov(newval));
		return;
	case 8:
		dynamiclight_set(selectedlight, LFIELD_FOV, stof(newval));
		return;
	case 9:
		dynamiclight_set(selectedlight, LFIELD_CUBEMAPNAME, newval);
		return;
	case 10:
		dynamiclight_set(selectedlight, LFIELD_CORONA, stof(newval));
		return;
	case 11:
		dynamiclight_set(selectedlight, LFIELD_CORONASCALE, stof(newval));
		return;
	case 12:
		dynamiclight_set(selectedlight, LFIELD_AMBIENTSCALE, stof(newval));
		return;
	case 13:
		dynamiclight_set(selectedlight, LFIELD_DIFFUSESCALE, stof(newval));
		return;
	case 14:
		dynamiclight_set(selectedlight, LFIELD_SPECULARSCALE, stof(newval));
		return;
	case 15:
		dynamiclight_set(selectedlight, LFIELD_ROTATION, stov(newval));
		return;
	default:
		return;
	}
};

void(vector m) editor_lights_overlay =
{
	int i;
	string s;
	vector col;

	m_y = floor((m_y - 32f) / 8f);


	fldname[NUMLFIELDS+2] = strcat("realtime world: ", cvar("r_shadow_realtime_world")?"on":"off");
	fldname[NUMLFIELDS+3] = strcat("realtime dlight: ", cvar("r_shadow_realtime_dlight")?"on":"off");

	for (i = 1; i <= NUMLFIELDS; i++)
	{
		if (editfield == i)
			s = editvalue;
		else
			s = readfield(i, 0);
		s = strcat(ftos(i), " ", fldname[i], ": ", s);

		if (editfield == i)
			col = '1 0 0';
		else if (m_y == i && m_x < 64)
			col = '0 0 1';
		else
			col = '1 1 1';
			
		drawrawstring('0 32 0' + '0 8 0' * (float)i, s, '8 8 0', col, 1);
	}

	for (i = NUMLFIELDS+1f; i <= NUMLFIELDS+NUMCMDS; i+=1)
	{
		s = strcat(ftos(i), " ", fldname[i]);

		if (editfield == i)
			col = '1 0 0';
		else if (m_y == i && m_x < 64)
			col = '0 0 1';
		else
			col = '1 1 1';
			
		drawrawstring('0 32 0' + '0 8 0' * (float)i, s, '8 8 0', col, 1);
	}
	i+=1i;

	if (helphidden)
		return;

	if (editfield == 5)
	{
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "d: dynamic mode\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "w: realtime world lights mode\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "l: lightmap hack (not valid above index 32)\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "f: flashblend coronas\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "c: does not cast shadows\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "s: shadowmapped light\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "r: crepuscular rays\n", '8 8 0', '1 1 1', 1); i+=1i;
	}
	else
	{
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "+/- change selected light\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "mouse also selects lights\n", '8 8 0', '1 1 1', 1);	i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "m moves the light to the cursor\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "i inserts new light\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "@ sets the light's radius to the cursor\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "# changes the light's shadow flag\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "p points the light to aim at the cursor\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "[ and ] move the light towards/away from indicated plane\n", '8 8 0', '1 1 1', 1); i+=1i;
		drawrawstring('0 32 0' + '0 8 0' * (float)i, "don't forget to save\n", '8 8 0', '1 1 1', 1); i+=1i;
	}
};

static void(vector fwd, vector vorg) selectbestlight =
{
	float l, b=selectedlight, d, bd;
	vector ldir;
	l = (float)dynamiclight_get(-1f, -1f);
	bd = 0;
	while(l > 0)
	{
		l-=1;
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

float(float keyc, float unic, vector m) editor_lights_key =
{
	vector t = unproject(m + '0 0 8192');
	vector o = unproject(m);
	string ns;
	if (keyc == 512 || (!editfield && unic == 13 && m_x < 64))
	{
		if (editfield)
		{
			writefield(editfield, editvalue);
			strunzone(editvalue);
			editfield = 0;
		}
		editfield = floor((m_y - 32f) / 8f);
		if (editfield <= 0 || editfield > NUMLFIELDS+NUMCMDS || m_x >= 64)
		{
			editfield = 0;

			selectbestlight(t - o, o);
		}
		else if (editfield > NUMLFIELDS)
		{
			switch(editfield - (NUMLFIELDS+1))
			{
			case 0:
				localcmd("r_editlights_save\n");
				break;
			case 1:
				localcmd("toggle r_shadow_realtime_world\n");
				break;
			case 2:
				localcmd("toggle r_shadow_realtime_dlight\n");
				break;
			case 3:
				helphidden = !helphidden;
				break;
			case 4:
				localcmd("r_editlights_reload none\n");
				break;
			case 5:
				localcmd("r_editlights_reload bsp\n");
				break;
			case 6:
				localcmd("r_editlights_reload\n");
				break;
			}
			editfield = 0;
		}
		else
			editvalue = strzone(readfield(editfield, 1));
	}
	else if (editfield)
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
	else if (unic >= '0' && unic <= '9')
	{
		editfield = unic - '0';
		editvalue = strzone(readfield(editfield, 1));
	}
	else if (unic == '=')
		selectedlight++;
	else if (unic == '-')
		selectedlight-=1;
	else if (unic == 'n' || unic == 'N')
		localcmd("noclip\n");
//	else if (unic == 's' || unic == 'S')
//	{
//		selectbestlight(t - o, o);
//	}
	else if (unic == 'm' || unic == 'M')
	{
		traceline(o, t, TRUE, world);
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, trace_endpos + trace_plane_normal*4);
	}
	else if (unic == 'p' || unic == 'P')
	{
		traceline(o, t, TRUE, world);
		vector ang = vectoangles((trace_endpos + trace_plane_normal*4f) - (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN));
		ang_x *= -1f;
		dynamiclight_set(selectedlight, LFIELD_ANGLES, ang);

		/*if we're pointing the light at something, it should probably have a fov*/
		if (!(float)dynamiclight_get(selectedlight, LFIELD_FOV))
			dynamiclight_set(selectedlight, LFIELD_FOV, 90f);
	}
	else if (unic == 'i' || unic == 'I')
	{
		traceline(o, t, TRUE, world);
		float oldl = selectedlight;
		selectedlight = dynamiclight_spawnstatic(trace_endpos + trace_plane_normal*4f, 300, '1 1 1');

		if ((float)dynamiclight_get(oldl, LFIELD_RADIUS) != 0f && oldl != selectedlight)
		{
			//dupe the current light
			dynamiclight_set(selectedlight, LFIELD_RADIUS,		dynamiclight_get(oldl, LFIELD_RADIUS));
			dynamiclight_set(selectedlight, LFIELD_COLOUR,		dynamiclight_get(oldl, LFIELD_COLOUR));
			dynamiclight_set(selectedlight, LFIELD_FOV,		dynamiclight_get(oldl, LFIELD_FOV));
			dynamiclight_set(selectedlight, LFIELD_STYLE,		dynamiclight_get(oldl, LFIELD_STYLE));
			dynamiclight_set(selectedlight, LFIELD_ANGLES,		dynamiclight_get(oldl, LFIELD_ANGLES));
			dynamiclight_set(selectedlight, LFIELD_FLAGS,		dynamiclight_get(oldl, LFIELD_FLAGS));
			dynamiclight_set(selectedlight, LFIELD_AMBIENTSCALE,	dynamiclight_get(oldl, LFIELD_AMBIENTSCALE));
			dynamiclight_set(selectedlight, LFIELD_DIFFUSESCALE,	dynamiclight_get(oldl, LFIELD_DIFFUSESCALE));
			dynamiclight_set(selectedlight, LFIELD_SPECULARSCALE,	dynamiclight_get(oldl, LFIELD_SPECULARSCALE));
			dynamiclight_set(selectedlight, LFIELD_CUBEMAPNAME,	dynamiclight_get(oldl, LFIELD_CUBEMAPNAME));
			dynamiclight_set(selectedlight, LFIELD_CORONA,		dynamiclight_get(oldl, LFIELD_CORONA));
			dynamiclight_set(selectedlight, LFIELD_CORONASCALE,	dynamiclight_get(oldl, LFIELD_CORONASCALE));
			dynamiclight_set(selectedlight, LFIELD_ROTATION,	dynamiclight_get(oldl, LFIELD_ROTATION));
		}else
		{
			/*reset the light's properties*/
			dynamiclight_set(selectedlight, LFIELD_RADIUS, 300);
			dynamiclight_set(selectedlight, LFIELD_COLOUR, '1 1 1');
			dynamiclight_set(selectedlight, LFIELD_FOV, 0);
			dynamiclight_set(selectedlight, LFIELD_STYLE, 0);
			dynamiclight_set(selectedlight, LFIELD_ANGLES, '0 0 0');
			dynamiclight_set(selectedlight, LFIELD_FLAGS, LFLAG_REALTIMEMODE);
			dynamiclight_set(selectedlight, LFIELD_AMBIENTSCALE, autocvar_r_editlights_import_ambient);
			dynamiclight_set(selectedlight, LFIELD_DIFFUSESCALE, autocvar_r_editlights_import_diffuse);
			dynamiclight_set(selectedlight, LFIELD_SPECULARSCALE, autocvar_r_editlights_import_specular);
			dynamiclight_set(selectedlight, LFIELD_CUBEMAPNAME,	"");
			dynamiclight_set(selectedlight, LFIELD_CORONA,		0);
			dynamiclight_set(selectedlight, LFIELD_CORONASCALE,	1);
			dynamiclight_set(selectedlight, LFIELD_ROTATION,	'0 0 0');
		}
	}
	else if (unic == '[')
	{
		traceline(o, t, TRUE, world);
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN) - trace_plane_normal);
	}
	else if (unic == ']')
	{
		traceline(o, t, TRUE, world);
		dynamiclight_set(selectedlight, LFIELD_ORIGIN, (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN) + trace_plane_normal);
	}
	else if (unic == '\'' || unic == '@')
	{
		traceline(o, t, TRUE, world);
		dynamiclight_set(selectedlight, LFIELD_RADIUS, 1.5*vlen(trace_endpos - (vector)dynamiclight_get(selectedlight, LFIELD_ORIGIN)));
	}
	else if (unic == '#')
	{
		float fl;
		fl = (float)dynamiclight_get(selectedlight, LFIELD_FLAGS);
		if (fl & LFLAG_NOSHADOWS)
			fl -= LFLAG_NOSHADOWS;
		else
			fl += LFLAG_NOSHADOWS;
		dynamiclight_set(selectedlight, LFIELD_FLAGS, (float)fl);
	}
	else if (unic == '{')
		dynamiclight_set(selectedlight, LFIELD_RADIUS, dynamiclight_get(selectedlight, LFIELD_RADIUS) - 15);
	else if (unic == '}')
		dynamiclight_set(selectedlight, LFIELD_RADIUS, dynamiclight_get(selectedlight, LFIELD_RADIUS) + 15);
	else if (keyc == 127)
		dynamiclight_set(selectedlight, LFIELD_RADIUS, 0);
	else
		return FALSE;
	return TRUE;
};
