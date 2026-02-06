/*
a)
b) Make a version of Mix Paint that just draws the texture onto tiles and replaces any other texture there instead of mixing, call it Tex Paint Single
b2) Rename Mix Paint to Tex Paint Mix
c) Add 2 buttons to incr/decrease Percentage setting
d) Add option to switch from circular selection reticle to square
e) Make reticle follow mouse properly (I can see it moving on the sonar map up above, but it's definitely not where I'm pointing the mouse)
f) Make Tex Kill choose the circular or square reticle you selected (suggestion D), right now it only uses square
g) Possibly an option to manually shrink the world bounds from the outside in (for eliminating unwanted space)
*/

enum
{
	ter_reload,		//reload the entire thing
	ter_save,			//save the entire heightmap
	ter_sethole,		//punch a hole in the terrain
	ter_height_set,		//set heights to a specific value
	ter_height_smooth,	//smooth the heights slightly (non-destructive)
	ter_height_spread,	//smooth the heights slightly (leaves center as-is)
	ter_height_flatten,	//smooth the heights slightly (non-destructive), such that the middle becomes flatter faster than the edges
	ter_height_raise,	//raise the terrain in a bell (negative value to lower)
	ter_height_lower,	//lower the terrain in a bell (negative value to raise)
	ter_tex_kill,		//set section texture
	ter_tex_get,		//get section texture
	ter_tex_paint,		//paint a specific texture with gracefulish blending.
	ter_tex_paint_single,	//paint a texture with 100% opacity and no attenuation (other than radius)
	ter_mixconcentrate,	//figure out which is the strongest mixed texture and make it stronger
	ter_mixnoise,		//add random noise to the affected samples
	ter_mixblur,		//blur the texture mixture
	ter_water_set,		//lower the terrain in a bell (negative value to raise)
	ter_mesh_add,		//add a mesh
	ter_mesh_kill,		//remove meshes within the radius
	ter_tint,			//paints new colour modifiers/tints
	ter_reset,			//destroy's the entire section completely, resetting it to default.
	ter_reloadsect,		//reload a section, reverting changes.
	ter_blank,
	ter_radius,
	ter_quant,
	ter_strength,
	ter_mesh,
	ter_tintval,
	ter_tex,
	ter_roundpegsquarehole,
	ter_count
};
static var float eradius = 256;
static var float equant = 8;
static var float epercent = 40;
static var float squaretool = 0;
static string tex[8];
static var string tint[8] = {"1 1 1", "1.2 0.9 0.9", "0 1 0"};
static string meshname;
static var float curtool = ter_blank;
static var float lasttool = ter_blank;
static int painttex;
static float mautorepeattime;
static entity tempent;
static var searchhandle texturesearch = (searchhandle)-1;
static float texturesearchfirst;
static string texturesearchhighlighted;

float autocvar_mod_terrain_networked;

vector vidsize;
float mousedown;

static string toolname[ter_count] =
{
	"reload",
	"save",
	"holes",
	"height set",
	"height smooth",
	"height spread",
	"height flatten",
	"height raise",
	"height lower",
	"tex kill",
	"tex get",
	"tex paint blend",
	"tex paint single",
	"tex concentrate",
	"tex noise",
	"tex blur",
	"water set",
	"mesh add",
	"mesh kill",
	"tex tint",
	"revert to default",
	"reload single section",
	"",
	"rad",
	"quant",
	"str",
	"mesh",
	"tex"
};

#define terrain_edit terrain_editfoo

__variant(float action, ...) terrain_edit = #278;

void(vector m, float repeated) editor_do =
{
	vector t = mousefar;
	vector o = mousenear;
	if (vlen(o - t) > 8192)
		t = o + normalize(t)*8192;
	traceline(o, t, TRUE, world);

	self = world;

	switch(curtool)
	{
	case ter_reload:
		if (repeated)	//don't autorepeat that...
			return;
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "f", TEREDIT_RELOAD);
		else if (!(float)terrain_edit(TEREDIT_RELOAD))
			print("Unable to reload terrain.\n");
		else
			print("Terrain changes discarded.\n");
		break;
	case ter_save:
		if (repeated)	//don't autorepeat that...
			return;
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "f", TEREDIT_SAVE);
		else
			print(sprintf("Saved %g chunks\n", (float)terrain_edit(TEREDIT_SAVE)));
		break;
	case ter_sethole:	//use view center instead of targetted - you cannot target that which is not there
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_SETHOLE, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_SETHOLE, trace_endpos, eradius, equant);
		break;
	case ter_height_smooth:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_HEIGHT_SMOOTH, trace_endpos, eradius, epercent/100.0);
		else
			terrain_edit(TEREDIT_HEIGHT_SMOOTH, trace_endpos, eradius, epercent/100.0);
		break;
	case ter_height_spread:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_HEIGHT_SPREAD, trace_endpos, eradius, epercent/100.0);
		else
			terrain_edit(TEREDIT_HEIGHT_SPREAD, trace_endpos, eradius, epercent/100.0);
		break;
	case ter_height_flatten:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_HEIGHT_FLATTEN, trace_endpos, eradius, epercent/100.0);
		else
			terrain_edit(TEREDIT_HEIGHT_FLATTEN, trace_endpos, eradius, epercent/100.0);
		break;
	case ter_water_set:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_WATER_SET, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_WATER_SET, trace_endpos, eradius, equant);
		break;
	case ter_height_set:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_HEIGHT_SET, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_HEIGHT_SET, trace_endpos, eradius, equant);
		break;
	case ter_height_raise:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_HEIGHT_RAISE, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_HEIGHT_RAISE, trace_endpos, eradius, equant);
		break;
	case ter_height_lower:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_HEIGHT_LOWER, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_HEIGHT_LOWER, trace_endpos, eradius, equant);
		break;
	case ter_tex_get:
		strunzone(tex[0]);
		strunzone(tex[1]);
		strunzone(tex[2]);
		strunzone(tex[3]);
		tex[0] = strzone(terrain_edit(TEREDIT_TEX_GET, trace_endpos, 0, 0));
		tex[1] = strzone(terrain_edit(TEREDIT_TEX_GET, trace_endpos, 0, 1));
		tex[2] = strzone(terrain_edit(TEREDIT_TEX_GET, trace_endpos, 0, 2));
		tex[3] = strzone(terrain_edit(TEREDIT_TEX_GET, trace_endpos, 0, 3));
		break;
//	case ter_mixset:
//		terrain_edit(curtool, trace_endpos, eradius, equant, emix);
//		break;
	case ter_tex_paint:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvffs", TEREDIT_TEX_BLEND, trace_endpos, eradius, epercent/100.0, tex[painttex]);
		else
			terrain_edit(TEREDIT_TEX_BLEND, trace_endpos, eradius, epercent/100.0, tex[painttex]);
		break;
	case ter_tex_paint_single:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvfs", TEREDIT_TEX_REPLACE, trace_endpos, eradius, tex[painttex]);
		else
			terrain_edit(TEREDIT_TEX_REPLACE, trace_endpos, eradius, tex[painttex]);
		break;
	case ter_tex_kill:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvffs", TEREDIT_TEX_KILL, trace_endpos, eradius, equant, tex[painttex]);
		else
			terrain_edit(TEREDIT_TEX_KILL, trace_endpos, eradius, equant, tex[painttex]);
		break;
	case ter_reset:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvf", TEREDIT_RESET_SECT, trace_endpos, eradius);
		else
			terrain_edit(TEREDIT_RESET_SECT, trace_endpos, eradius);
		break;
	case ter_reloadsect:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvf", TEREDIT_RELOAD_SECT, trace_endpos, eradius);
		else
			terrain_edit(TEREDIT_RELOAD_SECT, trace_endpos, eradius);
		break;
	case ter_mixconcentrate:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_TEX_UNIFY, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_TEX_UNIFY, trace_endpos, eradius, equant);
		break;
	case ter_mixnoise:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_TEX_NOISE, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_TEX_NOISE, trace_endpos, eradius, equant);
		break;
	case ter_mixblur:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvff", TEREDIT_TEX_BLUR, trace_endpos, eradius, equant);
		else
			terrain_edit(TEREDIT_TEX_BLUR, trace_endpos, eradius, equant);
		break;

	case ter_tint:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvffvf", TEREDIT_TINT, trace_endpos, eradius, epercent/100.0, stov(tint[painttex]), 1);
		else
			terrain_edit(TEREDIT_TINT, trace_endpos, eradius, epercent/100.0, stov(tint[painttex]), 1);
		break;

	case ter_mesh_add:
		if (repeated)	//don't autorepeat this one.
			break;
		terrain_edit(TEREDIT_MESH_ADD, tempent);
		break;
	case ter_mesh_kill:
		if (autocvar_mod_terrain_networked && !isserver())
			sendevent("teredit", "fvf", TEREDIT_MESH_KILL, trace_endpos, eradius);
		else
			terrain_edit(TEREDIT_MESH_KILL, trace_endpos, eradius);
		break;
	}
};

float(float keyc, float unic, vector m) editor_terrain_key =
{
	float nt;
	if (curtool >= ter_radius && curtool <= ter_tex)
	{
		string txt = "";
		nt = curtool;
		if (curtool == ter_tex)
		{
			if (keyc == 512 && m_x > 128)
			{
				txt = texturesearchhighlighted;
				nt = ter_tex_paint;
			}
			if (keyc == 515)
				texturesearchfirst += floor((vidsize_x)/128) - 1;
			if (keyc == 516)
				texturesearchfirst -= floor((vidsize_x)/128) - 1;
		}

		if (curtool == ter_radius)
			txt = itos((int)fabs(eradius));
		if (curtool == ter_quant)
			txt = itos((int)equant);
		if (curtool == ter_strength)
			txt = itos((int)epercent);

		if (curtool == ter_mesh)
			txt = meshname;
		if (curtool == ter_tintval)
			txt = tint[painttex];
		if (curtool == ter_tex)
			txt = tex[painttex];
			
		if (keyc == 10 || keyc == 13)
			nt = lasttool;
		else if (keyc == 127)
			txt = substring(txt, 0, -2);
		else if (keyc == 8)
			txt = "";
		else if (unic)
			txt = strcat(txt, chr2str(unic));

		if (curtool == ter_radius)
		{
			eradius = fabs(stof(txt));
			if (squaretool)
				eradius *= -1;
		}
		if (curtool == ter_quant)
			equant = stof(txt);
		if (curtool == ter_strength)
			epercent = stof(txt);
		if (curtool == ter_mesh)
		{
			txt = strzone(txt);
			strunzone(meshname);
			meshname = txt;
		}
		if (curtool == ter_tex)
		{
			txt = strzone(txt);
			strunzone(tex[painttex]);
			tex[painttex] = txt;
		}
		if (curtool == ter_tintval)
		{
			txt = strzone(txt);
			strunzone(tint[painttex]);
			tint[painttex] = txt;
		}

		if (curtool != nt)
		{
			lasttool = curtool;
			curtool = nt;
		}
	}
	else if (keyc == 13 || (keyc == 512 && m_x < 128))
	{
		if (m_x < 128)
		{
			nt = floor((m_y-16) / 8);
			if (nt != curtool)
			{
				if (nt == ter_roundpegsquarehole)
				{
					squaretool = !squaretool;
					eradius = fabs(eradius);
					if (squaretool) eradius *= -1;
				}
				else
				{
					lasttool = curtool;
					curtool = nt;
				}
			}
		}
		else if (keyc == 13)
		{
			editor_do(m, FALSE);
		}
	}
	else if (unic == '+' || unic == '=')
		eradius += squaretool?-16:16;
	else if (unic == '-')
	{
		eradius = fabs(eradius) - 16;
		if (eradius < 0)
			eradius = 0;
		if (squaretool) eradius *= -1;
	}
	else if (curtool == ter_mesh_add && tempent)
	{
		if (unic == '[')
			tempent.angles_y -= 12.5;
		else if (unic == ']')
			tempent.angles_y += 12.5;
		else if (unic == '{')
			tempent.angles_x -= 12.5;
		else if (unic == '}')
			tempent.angles_x += 12.5;
		else if (unic == '(')
			tempent.angles_z -= 12.5;
		else if (unic == ')')
			tempent.angles_z += 12.5;
		else if (unic == '@')
		{
			tempent.angles_x = gettime(5)*360*360;
			tempent.angles_y = gettime(5)*360;
			tempent.angles_z = gettime(5)*360*360*360;
		}
		else if (keyc == 127 || keyc == 8)
		{
			tempent.angles_x = 0;
			tempent.angles_y = 0;
			tempent.angles_z = 0;
		}
		else
			return FALSE;
	}
	else if (unic == '(')
		epercent -= 10;
	else if (unic == ')')
		epercent += 10;
	else if (unic == '[')
		equant -= 1;
	else if (unic == ']')
		equant += 1;
	else if (unic == '{')
		painttex = (painttex + 7) & 7;
	else if (unic == '}')
		painttex = (painttex + 1) & 7;
	else if (unic == '1')
		painttex = 0;
	else if (unic == '2')
		painttex = 1;
	else if (unic == '3')
		painttex = 2;
	else if (unic == '4')
		painttex = 3;
	else if (unic == '5')
		painttex = 4;
	else if (unic == '6')
		painttex = 5;
	else if (unic == '7')
		painttex = 6;
	else if (unic == '8')
		painttex = 7;
	else
		return FALSE;

	if (curtool == ter_save || curtool == ter_reload)
	{
		editor_do('0 0 0', FALSE);
		curtool = -1;
	}
	return TRUE;
};

void(vector mousepos) editor_terrain_addentities =
{
	float s,c;
	float r;
	vector tx, p, col;
	float a;
	
	if (curtool == ter_tex_paint || curtool == ter_tex_paint_single || curtool == ter_tex)
		terrain_edit(TEREDIT_TEX_SETMASK, tex[painttex]);
	
	if (mousepos_x < 128)
		return;

	vector t = mousefar;
	vector o = mousenear;
	if (vlen(o - t) > 8192)
		t = o + normalize(t)*8192;
	traceline(o, t, TRUE, world);

	if (curtool == ter_mesh_add || curtool == ter_mesh)
	{
		if (!tempent)
			tempent = spawn();

		precache_model(meshname); /*just to silence it*/
		setmodel(tempent, meshname);
		setorigin(tempent, trace_endpos);
		tempent.scale = eradius/256;
		addentity(tempent);
	}
	else
	{
		shaderforname("terrainedit",
		"{"
			"{\n"
				"map terrainedit\n"
				"blendfunc add\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}");

		r = eradius;//sqrt(eradius*eradius/2);

		s = sin(gettime(5)) * r;
		c = cos(gettime(5)) * r;

		col = [(sin(gettime(5))+1.5)*0.1,  0,  0];

		t = trace_endpos;
		R_BeginPolygon("terrainedit");
		tx_z = 0;
		for (a = 0; a < 3.14*2; a += 3.14*2/8)
		{
			tx_x = sin(a);
			tx_y = cos(a);

			//-1 -1
			p_x = t_x + tx_x*c - tx_y*s;
			p_y = t_y + tx_y*c + tx_x*s;
			p_z = t_z + 5;
			tx = (tx*0.5)+'0.5 0.5 0';
			R_PolygonVertex(p, tx, col, 1);
		}
		R_EndPolygon();
	}
};

void(vector mousepos) editor_terrain_overlay =
{
	float i;
	vector pos;
	vector colour;
	
//	if (curtool == ter_tex_paint || curtool == ter_tex_paint_single || curtool == ter_tex)
		terrain_edit(TEREDIT_TEX_SETMASK, 0);

	float ctime = gettime(5);
	if (mautorepeattime)
	{
		if (mousedown != 1)
			mautorepeattime = 0;
		else if (mautorepeattime < ctime && ter_mesh_add)
		{
			mautorepeattime = ctime + 0.05;
			editor_do(mousepos, TRUE);
		}
	}
	else if (mousedown == 1)
	{
		mautorepeattime = ctime + 0.5;
		editor_do(mousepos, FALSE);
	}

	pos = '128 0 0';
	pos_y = vidsize_y - 32;
	drawfill('0 16 0', pos, '0 0 0', 0.3);

	pos = '0 16 0';
	for (i = 0; i < ter_count; i+=1)
	{
		if (curtool == i)
			colour = '1 0 0';
		else if (mousepos_x < 128 && mousepos_y >= pos_y && mousepos_y < pos_y + 8)
			colour = '0 0 1';
		else
			colour = '1 1 1';

		if (i == ter_radius)
			drawstring(pos, sprintf("radius: %g", fabs(eradius)), '8 8 0', colour, 1, 0);
		else if (i == ter_quant)
			drawstring(pos, sprintf("quantity: %g", equant), '8 8 0', colour, 1, 0);
		else if (i == ter_strength)
			drawstring(pos, sprintf("percent: %g%%", epercent), '8 8 0', colour, 1, 0);
		else if (i == ter_mesh)
			drawstring(pos, sprintf("mesh: %s", meshname), '8 8 0', colour, 1, 0);
		else if (i == ter_roundpegsquarehole)
			drawstring(pos, sprintf("shape: %s", (squaretool?"square peg":"round")), '8 8 0', colour, 1, 0);
		else if (i == ter_tex)
		{
			if (curtool == ter_tex_get)
			{
				drawstring(pos, sprintf("Tex0: %s", tex[0]), '8 8 0', colour, 1, 0);	pos_y += 8;
				drawstring(pos, sprintf("Tex1: %s", tex[1]), '8 8 0', colour, 1, 0);	pos_y += 8;
				drawstring(pos, sprintf("Tex2: %s", tex[2]), '8 8 0', colour, 1, 0);	pos_y += 8;
				drawstring(pos, sprintf("Tex3: %s", tex[3]), '8 8 0', colour, 1, 0);
			}
			else
				drawstring(pos, sprintf("Tex%1i: %s", painttex, tex[painttex]), '8 8 0', colour, 1, 0);
			if (tex[painttex] != "")
				if (whichpack(strcat("textures/", tex[painttex], ".tga")) != "")
					drawpic(pos + '0 8 0', tex[painttex], '128 128 0', '1 1 1', 1);
		
		}
		else if (i == ter_tintval)
			drawstring(pos, sprintf("Tint%1i: %s", painttex, tint[painttex]), '8 8 0', colour, 1, 0);
		else
			drawstring(pos, toolname[i], '8 8 0', colour, 1, 0);
		pos_y += 8;
	}

	if notnull(texturesearchhighlighted)
	{
		strunzone(texturesearchhighlighted);
		texturesearchhighlighted = __NULL__;
	}
	if (curtool == ter_tex)
	{
		if ((float)texturesearch < 0)
			texturesearch = search_begin("textures/*", FALSE, TRUE);
		if (texturesearchfirst > search_getsize(texturesearch)-4)
			texturesearchfirst = search_getsize(texturesearch)-4;
		if (texturesearchfirst < 0)
			texturesearchfirst = 0;
		i = texturesearchfirst;
		pos = '128 16';
		local float x;
		for (pos_y = 8; pos_y < vidsize_y; pos_y+=128+8)
		for (x = 0; x < floor(vidsize_x / 128)-1; )
		{
			string fname = search_getfilename(texturesearch, i);
			i+=1;
			if (fname == "")
				break;
			if (substring(fname, -1, 1) == "/")
				continue;
			drawpic(pos + [x*128, 8], fname, '128 128', '1 1 1', 1, 0);
			if (substring(fname, 0, 9) == "textures/")
				fname = substring(fname, 9, -1);
			string ext = substring(fname, -4, 4);
			if (ext == ".png" || ext == ".tga" || ext == ".jpg")
				fname = substring(fname, 0, -5);
			drawstring(pos + [x*128, 0], fname, '8 8 0', '1 1 1', 1, 0);
			x+=1;

			if (mousepos_x > pos_x+x*128 && mousepos_y > pos_y && mousepos_x < pos_x+(x+1)*128 && mousepos_y < pos_y+(128+8))
			{
				if notnull(texturesearchhighlighted)
					strunzone(texturesearchhighlighted);
				texturesearchhighlighted = strzone(fname);
			}
		}
	}
};
