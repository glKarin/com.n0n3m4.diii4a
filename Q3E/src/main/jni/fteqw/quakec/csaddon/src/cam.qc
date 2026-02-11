#ifdef CAMQUAKE
typedef struct
{
	int numpoints;
	int type;
	vector pos[64];
	float totallength;
	vector color;
	int selected_point;
	float position;
} spline_t;

typedef struct
{
	float starttime;
	float stoptime;
	spline_t spline;
} cam_t;

typedef struct
{
	int ver;
	int position_count;
	int position_selected;
	int view_count;
	int view_selected;
	cam_t position[64];
	cam_t view[64];
} camdata_t;
#define VER 1i	/*bump when the above structs change*/

static camdata_t *camdata;
static var float splinefile = -1;
static var int edit_type = 0;
static vector submenu_position;
var float autocvar_cl_demospeed = 1;

void(int *dest, int bytes) memclr =
{
	bytes = bytes>>2i;
	while(bytes > 0i)
	{
		bytes = bytes - 1i;
		dest[bytes] = 0i;
	}
};

void() spline_init =
{
	string camfilename;
	if (isdemo())
		camfilename = serverkey("ip");	//this always contains the name of the demo rather than the original ip, which is not known.
	else
		camfilename = mapname;	//otherwise just use the current map (though really we should just disable this)
	camfilename = strcat(camfilename, ".cam");

	/*precache the shader*/
	shaderforname("camsplineshader", "{\n{\nmap construction.tga\nblendfunc blend\nrgbgen vertex\nalphagen vertex\n}\n}\n");

	splinefile = fopen(camfilename, FILE_MMAP_RW, sizeof(camdata_t));
	if (splinefile < 0)
	{
		/*too lazy to create a file, just use it as a malloc*/
		splinefile = fopen("", FILE_MMAP_RW, sizeof(camdata_t));
	}

	camdata = (camdata_t*)(fgets(splinefile));

	if (!camdata)
		error("unable to mmap spline.dat\n");

	if (camdata->ver != VER)
	{
		print(sprintf("%s version is outdated, wiping.\n", camfilename));
		memclr ((int*)camdata, sizeof(camdata_t));
		camdata->ver = VER;
	}
};
void() spline_shutdown =
{
	fclose(splinefile);
	splinefile = -1;
};

static void spline_calclength(spline_t *s)
{
	int i;
	s->totallength = 0;
	for (i = 0; i < s->numpoints - 1; i+=1)
	{
		s->totallength = s->totallength + vlen(s->pos[i] - s->pos[i+1]);
	}
};

static vector(spline_t *s, float frac) spline_pos =
{
	vector v1, v2;
	if (s->numpoints < 2)
	{
		return '0 0 0';
	}
	frac *= s->numpoints;
	if (frac < 0)
	{
		v1 = s->pos[0];
		v2 = s->pos[1];
	}
	else if (frac >= s->numpoints)
	{
		v1 = s->pos[s->numpoints-2];
		v2 = s->pos[s->numpoints-1];
		frac -= s->numpoints;
	}
	else
	{
		v1 = s->pos[s->numpoints];
		v2 = s->pos[s->numpoints+1];
		frac = frac - (float)(int)frac;
	}

	return v1 * (1-frac) + v2 * frac;
};

static vector(spline_t *s, float frac) spline_dir =
{
	return normalize(spline_pos(s, frac+0.001) - spline_pos(s, frac-0.001));
};

static vector(vector pos, vector dir, vector view) beamdir =
{
	vector result;
	view = normalize(view - pos);
	//crossproduct view+dir
	result_x = dir_y*view_z - dir_z*view_y;
	result_y = dir_z*view_x - dir_x*view_z;
	result_z = dir_x*view_y - dir_y*view_x;
	return result;
};

void(vector p1, vector p2, vector color1, vector color2) fakeline = {
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(p1+'0 0 1', '0 0', color1,1);
	R_PolygonVertex(p1-'0 0 1', '0 1', color1,1);
	R_PolygonVertex(p2-'0 0 1', '1 1', color2,1);
	R_PolygonVertex(p2+'0 0 1', '1 0', color2,1);
	R_PolygonVertex(p2+'0 0 1', '0 0', color2,1);
	R_PolygonVertex(p2-'0 0 1', '0 1', color2,1);
	R_PolygonVertex(p1-'0 0 1', '1 1', color1,1);
	R_PolygonVertex(p1-'0 0 1', '1 0', color1,1);
	R_EndPolygon();
};

void (vector p1, vector p2, vector color1, vector color2) tube
{
	local vector s;
	R_BeginPolygon("camsplineshader");
	R_EndPolygon();
};

vector (vector p1, vector p2, float frac) interpolate
{
	return (p2 - p1) * frac + p1;
};

void (vector pos, float point_size, vector color) draw_point
{
	local int i;
	local vector offsets[5];
	local vector points[8];
	local vector point, point1;

	if (point_size == 0)
		return;

	point_x = -(point_size/2);
	point_y = -(point_size/2);
	offsets[0] = point;
	point_x = point_size/2;
	point_y = -(point_size/2);
	offsets[1] = point;
	point_x = -(point_size/2);
	point_y = (point_size/2);
	offsets[2] = point;
	point_x = (point_size/2);
	point_y = (point_size/2);
	offsets[3] = point;

	point1_z = -(point_size/2);
	for (i=0; i<4; i+=1)
		points[i] = offsets[i] + point1 + pos;

	point1_z = (point_size/2);
	for (i=4; i<8; i+=1)
		points[i] = offsets[i-4] + point1 + pos;

	/*
	 * bottom
	 * 2 3
	 * 0 1
	 *
	 * top
	 * 6 7
	 * 4 5
	 */

	// bottom
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(points[3], '0 0', color ,1);
	R_PolygonVertex(points[2], '0 1', color ,1);
	R_PolygonVertex(points[0], '1 1', color,1);
	R_PolygonVertex(points[1], '1 0', color,1);
	R_EndPolygon();

	// top
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(points[5], '0 0', color ,1);
	R_PolygonVertex(points[4], '0 1', color ,1);
	R_PolygonVertex(points[6], '1 1', color,1);
	R_PolygonVertex(points[7], '1 0', color,1);
	R_EndPolygon();

	// front
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(points[4], '0 0', color ,1);
	R_PolygonVertex(points[5], '0 1', color ,1);
	R_PolygonVertex(points[1], '1 1', color,1);
	R_PolygonVertex(points[0], '1 0', color,1);
	R_EndPolygon();

	//back
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(points[7], '0 0', color ,1);
	R_PolygonVertex(points[6], '0 1', color ,1);
	R_PolygonVertex(points[2], '1 1', color,1);
	R_PolygonVertex(points[3], '1 0', color,1);
	R_EndPolygon();

	//left
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(points[6], '0 0', color ,1);
	R_PolygonVertex(points[4], '0 1', color ,1);
	R_PolygonVertex(points[0], '1 1', color,1);
	R_PolygonVertex(points[2], '1 0', color,1);
	R_EndPolygon();

	//right
	R_BeginPolygon("camsplineshader");
	R_PolygonVertex(points[5], '0 0', color ,1);
	R_PolygonVertex(points[7], '0 1', color ,1);
	R_PolygonVertex(points[3], '1 1', color,1);
	R_PolygonVertex(points[1], '1 0', color,1);
	R_EndPolygon();
};

void (spline_t *s, vector p) spline_move
{
	local int i;

	for (i=0; i<s->numpoints; i+=1i)
	{
		s->pos[i] = s->pos[i] + p;
	}
};

// time has to be between 0 and 1
vector(spline_t *s, float frac_time) spline_calculate_position =
{
	local vector r, point1, point2, point3, point4;
	local float t1, t2, t3, t4, t5, t_scaled;
	local int i;

	if (s->numpoints == 0)
	{
		return '0 0 0';
	}

	// point
	if (s->numpoints == 1)
	{
		return s->pos[0];
	}

	// straight line
	if (s->numpoints == 2)
	{
		r = s->pos[1] - s->pos[0];
		r = r * frac_time;
		r = r + s->pos[0];
		return r;
	}

	// cubic spline
	if (s->numpoints == 3)// || (s->numpoints > 3 && s->type == 1))
	{
		t1 = 1-frac_time;
		t2 = pow(t1, s->numpoints - 1i);
		t3 = pow(frac_time, s->numpoints - 1i);

		for (i=0; i<s->numpoints; i+=1)
		{
			if (i == 0i)
			{
				r = s->pos[i] * t2;
			}
			else if (i == s->numpoints - 1i)
			{
				r = r + s->pos[i] * t3;
			}
			else
			{
				t4 = (s->numpoints-1i) * pow(frac_time, i) * pow(t1, s->numpoints - 1i - i);
				r = r + s->pos[i] * t4;
			}
		}
		return r;
	}

	// catmull rom
	if (s->numpoints > 3)
	{
		// getting the start point -- might change if we add times to points
		// first point is always an entry point last point also
		t5 = 1 / (float)(s->numpoints - 3i);
		for (i=1, t4=t5; t4<frac_time; t4 = t4 + t5)
		{
			i+=1;
		}

		if (i >= s->numpoints)
		{
			print("something went wrong while getting the start point!\n");
			return s->pos[0];
		}


		point1 = s->pos[i - 1i];
		point2 = s->pos[i];
		point3 = s->pos[i+1i];
		point4 = s->pos[i+2i];

		// scale the time so its 0 -> 1 between point1 and 2
		/*
			1		2		3		4		5		6
					0		1
					0						1
		*/

		t_scaled = (frac_time - (t4 - t5))/t5; // hope this works :P

		// would be really nice if i could access vector components via [0] [1] ... here
		// warning RETARDED!
		t1 = point2_x;
		t2 = -0.5 * point1_x + 0.5 * point3_x;
		t3 = point1_x + (-2.5 * point2_x) + 2.0* point3_x +(-0.5 *point4_x);
		t4 = -0.5 * point1_x + 1.5 * point2_x + (-1.5 * point3_x) + 0.5 * point4_x;
		r_x = ((t4 * t_scaled + t3)*t_scaled + t2)*t_scaled + t1;

		t1 = point2_y;
		t2 = -0.5 * point1_y + 0.5 * point3_y;
		t3 = point1_y + (-2.5 * point2_y) + 2.0* point3_y +(-0.5 *point4_y);
		t4 = -0.5 * point1_y + 1.5 * point2_y + (-1.5 * point3_y) + 0.5 * point4_y;
		r_y = ((t4 * t_scaled + t3)*t_scaled + t2)*t_scaled + t1;

		t1 = point2_z;
		t2 = -0.5 * point1_z + 0.5 * point3_z;
		t3 = point1_z + (-2.5 * point2_z) + 2.0* point3_z +(-0.5 *point4_z);
		t4 = -0.5 * point1_z + 1.5 * point2_z + (-1.5 * point3_z) + 0.5 * point4_z;
		r_z = ((t4 * t_scaled + t3)*t_scaled + t2)*t_scaled + t1;

		return r;
	}

	return s->pos[0];

};

static void(spline_t *s) spline_draw =
{
	local float step, frac;
	local vector pos, pos1, pos2, vpos;
	local vector bdir;
	local int i;
	local vector color, color1, color2;
	local float psize;

	/*example of drawing convex polygons*/
	///R_BeginPolygon("camsplineshader");

	vpos = getviewprop(VF_ORIGIN);

	if (s->numpoints < 3)
	{
		step = 1; // should be fine
	}
	else
	{
		step = 0.01;
	}

	for (frac=0; frac<=1; frac = frac + step)
	{
		pos = spline_calculate_position(s, frac);
		if (frac == 0)
		{
			color2 = interpolate('0.1 0.3 0.1' , '0.5 0 0', frac);
			pos2 = pos;
			continue;
		}

		color1 = interpolate('0.1 0.3 0.1' , '0.5 0 0', frac);
		fakeline(pos2, pos, color2, color1);
		pos2 = pos;
		color2 = color1;
	}

	//fakeline(pos2, s->pos[s->numpoints-1]);

//	draw points
	for (i=0; i<s->numpoints; i+=1)
	{
		psize = 5;
		color = '0.25 0.25 0.25';
		if (i == s->selected_point)
		{
			psize = 10;
			color = '1 1 1';
		}
		draw_point(s->pos[i], psize, color);
	}
	// draw position
	pos = spline_calculate_position(s, s->position);
	draw_point(pos, 3, '1 0 0');
};

void(float attime) spline_overrides =
{
	local vector position, view;
	local cam_t *cpos;
	local cam_t *cview;
	local float stime;
	local int i;

	if (splinefile < 0)
		spline_init();

	if (camdata->position_count == 0 || camdata->position_count < 0 || camdata->position_count > 64)
		return;

	if (camdata->view_count == 0 || camdata->view_count < 0 || camdata->view_count > 64)
		return;

	for (i=0i; i < camdata->position_count; i=i+1i)
	{
		cpos = &camdata->position[i];
		if (cpos->starttime <= attime && cpos->stoptime > attime)
			break;
	}

	if (i == camdata->position_count)
		return;

	for (i=0i; i < camdata->view_count; i=i+1i)
	{
		cview = &camdata->view[i];
		if (cview->starttime <= attime && cview->stoptime > attime)
			break;
	}

	if (i == camdata->view_count)
		return;
		
	stime = (attime - cpos->starttime) / (cpos->stoptime - cpos->starttime);
	position = spline_calculate_position(&cpos->spline, stime);

	stime = (attime - cview->starttime) / (cview->stoptime - cview->starttime);
	view = spline_calculate_position(&cview->spline, stime);

	setviewprop(VF_ORIGIN, position);
	setviewprop(VF_ANGLES, vectoangles(view - position));
};

void() editor_spline_addentities =
{
	local int i;
	local float ctime;
	//local float stime;

	local vector tdir = [16 * cos(time), 16 * sin(time)];
//	drawlame3dtext("charset", '300 -500 50', tdir, '0 0 -16', "hello jogi\ndoes this string appear for you?", '1 1 1', 1);
//	drawlame3dtext("charset", '300 -500 50', -tdir, '0 0 -16', "hello jogi\ndoes this string appear for you?", '1 1 1', 1);

	if (splinefile < 0)
		spline_init();

	ctime = gettime(5);

	/*add visible splines to the scene*/
	for (i = 0; i < camdata->position_count; i+=1i)
		spline_draw(&camdata->position[i].spline);
	for (i = 0; i < camdata->view_count; i+=1i)
		spline_draw(&camdata->view[i].spline);

	/*sort out the overrides*/
//	spline_overrides(simtime);
};
//float slidertestfloat1, slidertestfloat2, slidertestfloat3;
//static float *curslider;
static menu_t menu;

void (vector *curmousepos, vector mousediff, float key) submenu =
{
	local vector pos, cpos;
	local spline_t *s;
	local cam_t *c;
	local vector point, point1;
	string str;

	pos = submenu_position;

	menu->key = key;
	menu->mousepos = curmousepos;
	menu->mousediff = mousediff;

	if (edit_type == 0)
	{
		if (camdata.position_count > 0)
		{
			c = &camdata.position[camdata.position_selected];
			s = &camdata.position[camdata.position_selected].spline;
		}
	}
	else
	{
		if (camdata.view_count > 0)
		{
			c = &camdata.view[camdata.view_selected];
			s = &camdata.view[camdata.view_selected].spline;
		}
	}

	menu->flags = MF_DS_VALUE;
	if (edit_type == 0)
	{
		if (camdata.position_count > 1)
		{
			str = sprintf("%2i/%2i", camdata.position_selected+1i, camdata.position_count);
			cpos = pos;
			cpos_x = strlen(str) * 8 +4;
			drawrawstring(pos, str, '8 8 0', '1 0 0', 1);
			slideri_widgit(&menu, "", cpos, &camdata.position_selected, 0, camdata.position_count-1i);
			pos_y += 8;
		}
		else if (camdata.position_count > 0)
		{
			str = sprintf("starttime: %f - ", camdata.position[camdata.position_selected].starttime);
			drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
			cpos = pos;
			cpos_x += strlen(str) * 8;
			sliderf_fixed_widgit(&menu, "", cpos, &camdata.position[camdata.position_selected].starttime);
			cpos_x += 32;
			if (button_widget(&menu, "set", cpos))
				camdata.position[camdata.position_selected].starttime = gettime(5);
			pos_y += 8;
			str = sprintf("stoptime : %f - ", camdata.position[camdata.position_selected].stoptime);
			drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
			cpos = pos;
			cpos_x += strlen(str) * 8;
			sliderf_fixed_widgit(&menu, "", cpos, &camdata.position[camdata.position_selected].stoptime);
			cpos_x += 32;
			if (button_widget(&menu, "set", cpos))
				camdata.position[camdata.position_selected].stoptime = gettime(5);
			pos_y += 8;
		}
	}
	else
	{
		if (camdata.view_count > 1)
		{
			str = sprintf("%2i/%2i", camdata.view_selected+1i, camdata.view_count);
			cpos = pos;
			cpos_x = strlen(str) * 8 +4;
			drawrawstring(pos, str, '8 8 0', '1 0 0', 1);
			slideri_widgit(&menu, "", cpos, &camdata.view_selected, 0, camdata.view_count);
			pos_y += 8;
		}
		else if (camdata.view_count > 0)
		{
			str = sprintf("starttime: %f - ", camdata.view[camdata.view_selected].starttime);
			drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
			cpos = pos;
			cpos_x += strlen(str) * 8;
			sliderf_fixed_widgit(&menu, "", cpos, &camdata.view[camdata.view_selected].starttime);
			cpos_x += 32;
			if (button_widget(&menu, "set", cpos))
				camdata.view[camdata.view_selected].starttime = gettime(5);
			pos_y += 8;
			str = sprintf("stoptime : %f - ", camdata.view[camdata.view_selected].stoptime);
			drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
			cpos = pos;
			cpos_x += strlen(str) * 8;
			sliderf_fixed_widgit(&menu, "", cpos, &camdata.view[camdata.view_selected].stoptime);
			cpos_x += 32;
			if (button_widget(&menu, "set", cpos))
				camdata.view[camdata.view_selected].stoptime = gettime(5);
			pos_y += 8;
		}
	}
	menu->flags = 0;

	pos_y += 8;

	if (s && s->numpoints > 0)
	{
		menu->flags = MF_DS_VALUE;
		str = sprintf("selected point: %2i/%2i", s->selected_point + 1i, s->numpoints);
		drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
		cpos = pos;
		cpos_x += strlen(str) * 8 + 4;
		slideri_widgit(&menu, "",  cpos, &s->selected_point, 0, s->numpoints - 1i);pos_y+=8;
		menu->flags = 0;
		point = s->pos[s->selected_point];
		sliderf_fixed_widgit(&menu, "x:", pos, &point_x);pos_y+=8;
		sliderf_fixed_widgit(&menu, "y:", pos, &point_y);pos_y+=8;
		sliderf_fixed_widgit(&menu, "z:", pos, &point_z);pos_y+=8;
		s->pos[s->selected_point] = point;
		pos_y += 8;
	}

	if (c && s)
	{
		menu->flags = MF_DS_VALUE;
		point1 = '0 0 0';	// << problem with using a global value identifier i cant reuse point
		str = "move spline: ";
		drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
		pos_y += 8;
		sliderf_fixed_widgit(&menu, " x:", pos, &point1_x); pos_y += 8;
		sliderf_fixed_widgit(&menu, " y:", pos, &point1_y); pos_y += 8;
		sliderf_fixed_widgit(&menu, " z:", pos, &point1_z); pos_y += 8;
		spline_move(s, point1);
		menu->flags = 0;
	}

//	drawrawstring(pos, sprintf("isdemo: %f", isdemo()), '8 8 0', '1 1 1', 1);
}

int (vector *curmousepos, vector mousediff, float key) testmenu =
{
	local vector pos;
	local int reset_mouse = 0;

 	pos = '0 80';
	/*
	sliderf_widgit ("test float a", pos, curmousepos, mousediff, &slidertestfloat1, 100, 200);pos_y += 8;
	sliderf_widgit ("test float b", pos, curmousepos, mousediff, &slidertestfloat2, 0, 1);pos_y += 8;

	checkboxf_widgit ("checkbox test", pos, curmousepos, &slidertestfloat2, key);pos_y += 32;
	sliderf_fixed_widgit("test float c", pos, curmousepos, mousediff, &slidertestfloat3, key, 1i);pos_y += 32;
	*/

	//curmousepos = '0 0 0';

	return reset_mouse;
}

int (vector *curmousepos, vector mousediff) editor_spline_overlay =
{
	local vector pos, v;
//	local spline_t *s;
	local int ri;
	local string str;
	local float minv, sec;

	if (splinefile < 0)
		spline_init();

	/*draw menu*/
	/*dunno if the light editor has any convienient code*/
	pos_y = 40;
	str = sprintf("cam(%i)", camdata->position_count);
	if (edit_type == 0)
		drawrawstring(pos, str, '8 8 0', '1 0 0', 1);
	else
		drawrawstring(pos, str, '8 8 0', '1 1 1', 1);
	v = pos;
	v_x += strlen(str) * 8;
	ri = strlen(str) + 1;
	drawrawstring(v , "/", '8 8 0', '1 1 1', 1);
	v_x += 8;
	str = sprintf("view(%i)", camdata->view_count);
	if (edit_type == 1)
		drawrawstring(v, str, '8 8 0', '1 0 0', 1);
	else
		drawrawstring(v, str, '8 8 0', '1 1 1', 1);
	ri += (int)(strlen(str) + 1);
	v_x += 4 + strlen(str) * 8;

	menu->flags = MF_DS_VALUE;
	checkboxi_widgit(&menu, "", v, &edit_type);

	v_x = 0;
	v_y += 8;
	sliderf_widgit(&menu, "demo speed", v, &autocvar_cl_demospeed, 0, 1);
	cvar_set("cl_demospeed", ftos(autocvar_cl_demospeed));

	v_y += 8;
	sec = time;
	minv = floor(sec/60);
	sec -= minv*60;
	drawrawstring(v, sprintf("time: %g:%02.0f", min, sec), '8 8 0', '1 1 1', 1);

	submenu_position = pos + '0 8';

	submenu(curmousepos, mousediff, -1);

	return ri;
};

int (spline_t *s, vector point) spline_add_point = 
{
	if (s->numpoints + 1 >= 64)
	{
		return 1;
	}

	s->pos[s->numpoints] = point;
	s->selected_point = s->numpoints;
	s->numpoints = s->numpoints + 1i; //oldskool!
	return 0;
};

void (spline_t *s, vector point) spline_set_point =
{
	s->pos[s->selected_point] = point;
};

void (int type) position_add_point =
{
	vector v;
	int i;
	local cam_t *c;
	local spline_t *s;

	i = camdata->position_selected;
	c = &camdata->position[i];
	s = &c->spline;
	v = getviewprop(VF_ORIGIN);
	if (type == 0)
	{
		if (spline_add_point(s, v))
		{
//			print(sprintf("point could not be added.\n"));
		}
		else
		{
//			print(sprintf("added point: %i %f %f %f\n", s->selected_point, v_x, v_y, v_z));
		}
	}
}

void (int type) view_add_point =
{
	vector v;
	int i;
	local cam_t *c;
	local spline_t *s;

	i = camdata->view_selected;
	c = &camdata->view[i];
	s = &c->spline;
	v = getviewprop(VF_ORIGIN);
	if (type == 0)
	{
		if (spline_add_point(s, v))
		{
//			print(sprintf("point could not be added.\n"));
		}
		else
		{
//			print(sprintf("added point: %i %f %f %f\n", s->selected_point, v_x, v_y, v_z));
		}
	}
}


void(void) position_add
{
	if (camdata->position_count >= 64)
	{
		return;
	}

	camdata->position_selected = camdata->position_count;
	camdata->position_count = camdata->position_count + 1i; //oldskool
}

void(void) view_add
{
	if (camdata->view_count >= 64)
	{
		return;
	}
	camdata->view_selected = camdata->view_count;
	camdata->view_count = camdata->view_count + 1i;
}

void(spline_t *s, vector change) spline_change_point_position
{
	if (s->numpoints == 0)
		return;
	s->pos[s->selected_point] = s->pos[s->selected_point] + change;
};

void (cam_t *c, float starttime) cam_set_start_time
{
	c->starttime = starttime;
	if (starttime > c->starttime)
		c->starttime = starttime;
}

void (cam_t *c, float starttime) cam_set_stop_time
{
	c->starttime = starttime;
	if (starttime < c->starttime)
		c->starttime = starttime;
}

void (spline_t *s, float change) spline_change_position
{
	s->position = s->position + change;
	if (s->position < 0)
		s->position = 0;
	else if (s->position > 1)
		s->position = 1;
}

void(spline_t *s, int change) spline_change_selected_point
{
	if (s->numpoints == 0)
		return;

	if (change + s->selected_point < 0)
		s->selected_point = s->numpoints -1;
	else if (change + s->selected_point >= s->numpoints)
		s->selected_point = 0;
	else
		s->selected_point = s->selected_point + change;
};

float(float keycode, float unicode, vector *curmousepos_in, vector mousediff) editor_spline_key
{
	local vector curmousepos = *curmousepos_in;

	if (splinefile < 0)
		spline_init();

	if (keycode == 'j')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				position_add();
			position_add_point(0);
		}
		else
		{
			if (camdata->view_count == 0)
				view_add();
			view_add_point(0);
		}
		return TRUE;
	}

	if (keycode == 'k')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_selected_point(&camdata->position[camdata->position_selected].spline, -1);
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_selected_point(&camdata->view[camdata->view_selected].spline, -1);
		}
		return TRUE;
	}

	if (keycode == 'l')
	{
		if (camdata->position_count == 0)
			return FALSE;
		spline_change_selected_point(&camdata->position[camdata->position_selected].spline, 1);
		return TRUE;
	}

	if (keycode == '9')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '0 0 -1');
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '0 0 -1');
		}
		return TRUE;
	}

	if (keycode == '0')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '0 0 1');
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '0 0 1');
		}
		return TRUE;
	}

	if (keycode == '7')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '0 -1 0');
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '0 -1 0');
		}
		return TRUE;
	}

	if (keycode == '8')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '0 1 0');
		}
		else
		{
			if (camdata->view_count== 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '0 1 0');
		}
		return TRUE;
	}

	if (keycode == '5')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '-1 0 0');
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '-1 0 0');
		}
		return TRUE;
	}

	if (keycode == '6')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '1 0 0');
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '1 0 0');
		}
		return TRUE;
	}

	if (keycode == '6')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->position[camdata->position_selected].spline, '1 0 0');
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_point_position(&camdata->view[camdata->view_selected].spline, '1 0 0');
		}
		return TRUE;
	}

	if (keycode == '-')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_position(&camdata->position[camdata->position_selected].spline, -0.01);
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_position(&camdata->view[camdata->view_selected].spline, -0.01);
		}
		return TRUE;
	}

	if (keycode == '=')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			spline_change_position(&camdata->position[camdata->position_selected].spline, 0.01);
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			spline_change_position(&camdata->view[camdata->view_selected].spline, 0.01);
		}
		return TRUE;
	}

	if (keycode == '1')
	{
		edit_type = !edit_type;
		return TRUE;
	}

	if (keycode == '2')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			cam_set_start_time(&camdata->position[camdata->position_selected], gettime(5));
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			cam_set_start_time(&camdata->view[camdata->view_selected], gettime(5));
		}
		return TRUE;
	}

	if (keycode == '2')
	{
		if (edit_type == 0)
		{
			if (camdata->position_count == 0)
				return FALSE;
			cam_set_stop_time(&camdata->position[camdata->position_selected], gettime(5));
		}
		else
		{
			if (camdata->view_count == 0)
				return FALSE;
			cam_set_stop_time(&camdata->view[camdata->view_selected], gettime(5));
		}
		return TRUE;
	}

	if (keycode == 'n')
	{
		if (edit_type == 0)
			position_add();
		else
			view_add();
		return TRUE;
	}

	submenu(curmousepos_in, mousediff, keycode);

	return FALSE;
};

float(string str) CSQC_ConsoleCommand =
{
	local float numargs;
	numargs = tokenize(str);
	switch(argv(0))
	{
	case "examplecommandforjogi":
		print("Hello jogi, did this work?\n");
		print(argv(1), "\n");
		return TRUE;
	case "crashandcatchfire":
		print("Yeah, you're probably going to regret that\n");
		localcmd("unbindall\ncfg_save\nalias quit say ahaha\n");
		return TRUE;
	}
	return FALSE;
};

void() Cam_Init =
{
	registercommand("examplecommandforjogi");
//	registercommand("crashandcatchfire");
};
#endif
