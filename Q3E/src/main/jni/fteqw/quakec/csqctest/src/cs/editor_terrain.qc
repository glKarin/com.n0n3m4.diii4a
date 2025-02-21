enum
{
	ter_reload = 0,
	ter_save = 1,
	ter_set = 2,
	ter_smooth = 3,
	ter_raise = 4,
	ter_lower = 5
};



float(float keyc, float unic) editor_terrain_key =
{
	vector v, o;
	__using terrain_edit;
	if (keyc == '[')
	{
		v = (vector)getviewprop(VF_ANGLES);
		o = (vector)getviewprop(VF_ORIGIN);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);

		terrain_edit(ter_lower, trace_endpos, 256, 8);
	}
	else if (keyc == '=')
	{
		v = (vector)getviewprop(VF_ANGLES);
		o = (vector)getviewprop(VF_ORIGIN);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);

		terrain_edit(ter_set, trace_endpos, 256, 8);
	}

	else if (keyc == 13)
	{
		v = (vector)getviewprop(VF_ANGLES);
		o = (vector)getviewprop(VF_ORIGIN);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);

		terrain_edit(ter_smooth, trace_endpos, 256, 8);
	}
	else if (keyc == ']')
	{
		v = (vector)getviewprop(VF_ANGLES);
		o = (vector)getviewprop(VF_ORIGIN);
		makevectors(v);
		traceline(o, o + v_forward*8192, true, world);

		terrain_edit(ter_raise, trace_endpos, 256, 8);
	}
	else return false;

	return true;
};
