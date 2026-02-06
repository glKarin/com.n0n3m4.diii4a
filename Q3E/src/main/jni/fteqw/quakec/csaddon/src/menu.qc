#define MF_DS_VALUE		(1i << 0i)

typedef struct
{
	float *currentf;
	int *currenti;
	float key;
	vector *mousepos;
	vector mousediff;
	int flags;
} menu_t;

int (vector topleft, vector bottomright, vector cursor) cursor_in_widget
{
	if (cursor_x <= bottomright_x && cursor_x >= topleft_x)
		if (cursor_y <= bottomright_y && cursor_y >= topleft_y)
			return 1;
	return 0;
}

void(menu_t *menu, string fieldname, vector pos, float *value, float minv, float maxv) sliderf_widgit =
{
	local vector rpos, spos, col;
	local float f;
	local int render_only;
	local vector mousepos = *menu->mousepos;
	local vector mp = *menu->mousepos;
	local vector mousediff = menu->mousediff;
	local float key = menu->key;

	render_only = 0;

	rpos = pos;
	spos = rpos;
	spos_x = pos_x + strlen(fieldname) * 8 + 4;
	/*if they're dragging us, update*/
	if (menu->currentf == value)
	{
		if (mousedown == 1)
		{
			*value = *value + mousediff_x/10;
			*value = bound(minv, *value, maxv);
			f = *value;
			mp_y = spos_y + 4;
			mp_x = spos_x + 9 * 8 * (f - minv) / (maxv - minv) + 4;
			*menu->mousepos = mp;
		}
		else
		{
			menu->currentf = (float*)__NULL__;
		}
	}
	else
		if (mousedown == 1)
			render_only = 1;

	if (key == -1 || render_only == 1 )
	{
		if (mousepos_y >= pos_y && mousepos_y < pos_y + 8 && render_only == 0)
		{
			col_x = (sin(cltime*3.14)+1) / 2;
			col_y = col_x;
			col_z = 1;
		}
		else
			col = '1 1 1';

		f = *value;
		if (f < minv)
			f = minv;
		if (f > maxv)
			f = maxv;
		drawstring(pos, fieldname, '8 8 0', col, 1, 0);
		drawstring(spos, "^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", '8 8 0', '1 1 1', 1, 0);
		if (!(menu->flags & MF_DS_VALUE))
			drawstring(spos + '88 0', ftos(f), '8 8 0', col, 1, 0);
		spos_x += 4 + 9*8*(f - minv) / (maxv - minv);
		drawstring(spos, "^Ue083", '8 8 0', '1 1 1', 1, 0);
	}
	else 	if (mousepos_y >= pos_y && mousepos_y < pos_y + 8)
	{
		if (key == 512)
		{
			f = *value;
			menu->currentf = value;
			mp_x = spos_x + 9 * 8 * (f - minv) / (maxv - minv) + 4;
			*menu->mousepos = mp;
		}
		else if (key == 515)
			*value = bound(minv, *value + (maxv - minv) / 10, maxv);
		else if (key == 516)
			*value = bound(minv, *value - (maxv - minv) / 10, maxv);
	}
};

void(menu_t *menu, string fieldname, vector pos, float *value) sliderf_fixed_widgit =
{
	local vector rpos, spos, epos, col;
	local float f;
	local int render_only = 0;
	local vector mousepos = *menu->mousepos;
	local vector mp = *menu->mousepos;
	local vector mousediff = menu->mousediff;
	local vector *mpp = menu->mousepos;
	local float key = menu->key;

	rpos = pos;
	spos = rpos;
	spos_x = pos_x + strlen(fieldname) * 8 + 4;
	epos = spos + '20 8';
	if (!(menu->flags & MF_DS_VALUE))
		epos_x += strlen(sprintf("%f", *value)) * 8;
	/*if they're dragging us, update*/
	if (menu->currentf == value)
	{
		if (mousedown == 1)
		{
			*value = *value + mousediff_x/10;
			f = *value;
			mp_y = spos_y + 4;
			mp_x = spos_x + 4;
			*menu->mousepos = mp;
		}
		else
		{
			menu->currentf = (float*)__NULL__;
		}
	}
	else
		if (mousedown == 1)
			render_only = 1;

	if (key == -1 || render_only == 1 )
	{
		if (cursor_in_widget(pos, epos, mousepos) && render_only == 0)
		{
			col_x = (sin(cltime*3.14)+1) / 2;
			col_y = col_x;
			col_z = 1;
		}
		else
			col = '1 1 1';

		f = *value;
		drawstring(pos, fieldname, '8 8 0', col, 1, 0);
		
		drawstring(spos, "^Ue080^Ue082", '8 8 0', '1 1 1', 1, 0);
		if (!(menu->flags & MF_DS_VALUE))
			drawstring(spos + '20 0', ftos(f), '8 8 0', col, 1, 0);
		drawstring(spos + '4 0', "^Ue083", '8 8 0', '1 1 1', 1, 0);
	}
	else 	if (cursor_in_widget(pos, epos, mousepos))
	{
		if (key == 512)
		{
			f = *value;
			menu->currentf = value;
			mp_x = spos_x + 4;
			*mpp = mp;
		}
		else if (key == 515)
			*value = *value + 0.1;
		else if (key == 516)
			*value = *value - 0.1;
	}
};

void(menu_t *menu, string fieldname, vector pos, int *value, int minv, int maxv) slideri_widgit =
{
	local vector rpos, spos, epos, col;
	local int f;
	local int render_only;
	local vector mousepos = *menu->mousepos;
	local vector mp = *menu->mousepos;
	local vector mousediff = menu->mousediff;
	local float key;

	key = menu->key;

	render_only = 0;

	rpos = pos;
	spos = rpos;
	spos_x = pos_x + strlen(fieldname) * 8 + 4;
	epos = spos + '88 8';
	if (!(menu->flags & MF_DS_VALUE))
		epos_x = strlen(sprintf("%i", *value)) * 8;
	/*if they're dragging us, update*/
	if (menu->currenti == value)
	{
		if (mousedown == 1)
		{
			*value = *value + mousediff_x/10;
			*value = bound(minv, *value, maxv);
			f = *value;
			mp_y = spos_y + 4;
			mp_x = spos_x + 9 * 8 * (f - minv) / (maxv - minv) + 4;
			*menu->mousepos = mp;
		}
		else
		{
			menu->currenti = (int*)__NULL__;
		}
	}
	else
		if (mousedown == 1)
			render_only = 1;

	if (key == -1 || render_only == 1 )
	{
		if (mousepos_y >= pos_y && mousepos_y < pos_y + 8 && render_only == 0)
		{
			col_x = (sin(cltime*3.14)+1) / 2;
			col_y = col_x;
			col_z = 1;
		}
		else
			col = '1 1 1';

		f = *value;
		if (f < minv)
			f = minv;
		if (f > maxv)
			f = maxv;
		drawstring(pos, fieldname, '8 8 0', col, 1, 0);
		drawstring(spos, "^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", '8 8 0', '1 1 1', 1, 0);
		if (!(menu->flags & MF_DS_VALUE))
			drawstring(spos + '88 0', ftos(f), '8 8 0', col, 1, 0);
		spos_x += 9*8*(f - minv) / (maxv - minv);
		drawstring(spos, "^Ue083", '8 8 0', '1 1 1', 1, 0);
	}
	else 	if (mousepos_y >= pos_y && mousepos_y < pos_y + 8)
	{
		if (key == 512)
		{
			f = *value;
			menu->currenti = value;
			mp_x = spos_x + 9 * 8 * (f - minv) / (maxv - minv) + 4;
			*menu->mousepos = mp;
		}
		else if (key == 515)
			*value = bound(minv, *value + 1i, maxv);
		else if (key == 516)
			*value = bound(minv, *value - 1i, maxv);
	}
};


void(string fieldname, vector pos, vector *mousepos_in, float *value, float key) checkboxf_widgit =
{
	local vector rpos, col;
	local int render_only;
	local vector mousepos = *mousepos_in;
	rpos = pos + '132 0 0';

	render_only = 0;
	// fix this
	if (mousedown == 1)
		render_only = 1;

	if (key == -1 || render_only == 1)
	{
		if (mousepos_y >= pos_y && mousepos_y < pos_y + 8 && render_only == 0)
		{
			col_x = (sin(cltime*3.14)+1) / 2;
			col_y = col_x;
			col_z = 1;
		}
		else
			col = '1 1 1';

		drawstring(pos, fieldname, '8 8 0', col, 1, 0);
		rpos_x -= 4;
		drawstring(rpos, "^Ue080^Ue082", '8 8 0', '1 1 1', 1, 0);
		drawstring(rpos + '88 0', ((*value) ?"TRUE":"FALSE"), '8 8 0', col, 1, 0);
		rpos_x += 4;
		if (*value)
			drawstring(rpos, "^Ue083", '8 8 0', '1 1 1', 1, 0);
	}
	else 	if (mousepos_y >= pos_y && mousepos_y < pos_y + 8)
	{
		if (key == 512)
			*value = !*value;
		else if (key == 515)
			*value = !*value;
		else if (key == 516)
			*value = !*value;
	}
};

void(menu_t *menu, string fieldname, vector pos, int *value) checkboxi_widgit =
{
	local vector epos, spos, col;
	local int render_only = 0;
	local vector mousepos = *menu->mousepos;
	local vector mousediff = menu->mousediff;
	local float key = menu->key;

	spos = pos;
	spos_x += strlen(fieldname) * 8 + 4;
	epos = spos;
	epos_x += 16;
	epos_y += 8;
	if (!(menu->flags & MF_DS_VALUE))
		epos_x += strlen((*value) ? "TRUE":"FALSE") *8;

	render_only = 0;

	if (key == -1)
	{
		if (cursor_in_widget(pos, epos, mousepos) && render_only == 0)
		{
			col_x = (sin(cltime*3.14)+1) / 2;
			col_y = col_x;
			col_z = 1;
		}
		else
			col = '1 1 1';

		drawstring(pos, fieldname, '8 8 0', col, 1, 0);
		drawstring(spos, "^Ue080^Ue082", '8 8 0', '1 1 1', 1, 0);
		if (!(menu->flags & MF_DS_VALUE))
			drawstring(spos + '88 0', ((*value) ?"TRUE":"FALSE"), '8 8 0', col, 1, 0);
		if (*value)
			drawstring(spos + '4 0', "^Ue083", '8 8 0', '1 1 1', 1, 0);
	}

	if (cursor_in_widget(pos, epos, mousepos) && key != -1)
	{
		if (key == 512)
			*value = !*value;
		else if (key == 515)
			*value = !*value;
		else if (key == 516)
			*value = !*value;
	}
};

int (menu_t *menu, string fieldname, vector pos) button_widget =
{
	local vector epos, spos, col;
	local int render_only = 0;
	local vector mousepos = *menu->mousepos;
	local vector mousediff = menu->mousediff;
	local float key = menu->key;

	spos = pos;
	spos_x += strlen(fieldname) * 8;
	epos = spos;
	epos_y += 8;

	render_only = 0;

	if (key == -1)
	{
		if (cursor_in_widget(pos, epos, mousepos) && render_only == 0)
		{
			col_x = (sin(cltime*3.14)+1) / 2;
			col_y = col_x;
			col_z = 1;
		}
		else
			col = '1 1 1';

		drawstring(pos, fieldname, '8 8 0', col, 1, 0);
		return 0;
	}

	if (cursor_in_widget(pos, epos, mousepos) && key != -1)
	{
		if (key == 512)
			return 1;
	//	else if (key == 515)
	//	else if (key == 516)
	}

	return 0;
};

void(string shader, vector org, vector s, vector t, string text, vector col, float alph) drawlame3dtext =
{
	/*the shader must be a 16*16 grid of 256 chars*/
	vector st = draw_getimagesize(shader);
	vector pos = org;
	float chr;
	float idx = 0;
	vector tc0,tc1,tc2,tc3;

	//with GL_LINEAR sampling, the sample value is interpolated from the nearby two pixels
	//this means that we must move the texture coord inwards by half of a pixel to avoid any bleed through into neighbouring chars
	//(with nearest sampling, this isn't needed)
	if (st != '0 0 0')
	{
		st_x = 0.5 / st_x;
		st_y = 0.5 / st_y;
	}

	//precompute the st offset for each vertex
	tc0 = [(0.0/16) + st_x, (0.0/16) + st_y];
	tc1 = [(1.0/16) - st_x, (0.0/16) + st_y];
	tc2 = [(1.0/16) - st_x, (1.0/16) - st_y];
	tc3 = [(0.0/16) + st_x, (1.0/16) - st_y];

	/*begin looks up the shader and is thus potentially expensive, fte requires it only once per batch of polygons.*/
	R_BeginPolygon(shader);

	while((chr = text[idx]))
	{
		idx+=1;

		/*handle new lines*/
		if (chr == '\n')
		{
			org += t;
			pos = org;
			continue;
		}

		/*skip spaces*/
		if (chr != ' ')
		{
			/*its a grid, so the top-left texturecoord should be easy to calculate*/
			/*weirdly though, qc has no (direct) modulo functionality, not even as a builtin, so this code is ugly*/
			st_x = (chr & 15) / 16;
			st_y = floor(chr/16) / 16;

			/*plot the poly's point and emit the quad*/
			R_PolygonVertex(pos    , st + tc0, col, alph);
			R_PolygonVertex(pos + s, st + tc1, col, alph);
			R_PolygonVertex(pos+s+t, st + tc2, col, alph);
			R_PolygonVertex(pos + t, st + tc3, col, alph);
			R_EndPolygon();
		}
		pos += s;
	}
};
