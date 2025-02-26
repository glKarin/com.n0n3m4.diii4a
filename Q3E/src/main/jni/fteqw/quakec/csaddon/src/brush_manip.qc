
//basic cube plane normals, for selection.
static nosave const vector axis[6] = {
	'-1 0 0',
	'0 -1 0',
	'0 0 -1',
	'1 0 0',
	'0 1 0',
	'0 0 1'
};
float dists[6];


//generates default quakeed-style texture mapping for the given surface.
//this sucks for cylinders, but keeps slopes and things easy.
void(brushface_t *fa) reset_texturecoords =
{
	//figure out some default texture coords	
	fa->sdir = '0 0 0';
	fa->sbias = 0;
	fa->tdir = '0 0 0';
	fa->tbias = 0;
	float a=fabs(fa->planenormal[0]),b=fabs(fa->planenormal[1]),c=fabs(fa->planenormal[2]);
	if (a>=b&&a>=c)
		fa->sdir[1] = 1;
	else
		fa->sdir[0] = 1;
	if (c>a&&c>b)
		fa->tdir[1] = -1;
	else
		fa->tdir[2] = -1;
};

int(brushface_t *fa, int famax, vector *points, int numpoints, float height) BrushFromPoints =
{
	int count = 0;
	
	fa->planenormal = normalize(crossproduct(points[2] - points[0], points[1] - points[0]));
	fa->planedist = bt_point[0] * fa->planenormal + height;
	fa->shadername = autocvar_ca_newbrushtexture;
	reset_texturecoords(fa);
	fa++;

	fa->planenormal = -normalize(crossproduct(points[2] - points[0], points[1] - points[0]));
	fa->planedist = (bt_point[0] * fa->planenormal);
	fa->shadername = autocvar_ca_newbrushtexture;
	reset_texturecoords(fa);
	fa++;
	count = 2;

	for (int p = 0; p < numpoints; p++)
	{
		int n = p + 1;
		if (n == numpoints)
			n = 0;
		fa->planenormal = normalize(crossproduct(points[p] - bt_point[n], tmp.faces[0].planenormal));
		fa->planedist = points[p] * fa->planenormal;
		fa->shadername = autocvar_ca_newbrushtexture;
		reset_texturecoords(fa);
		fa++;
		count++;
	}

	return count;
}


vector(vector guess) brush_snappoint =
{
	if (nogrid || autocvar_ca_grid <= 0)
		return guess;
	int facenum, points;
	float bestdist = autocvar_ca_grid*autocvar_ca_grid;	//worst case value so we don't snap to grid when there's a vertex 0.001qu away from the grid.
	vector best = guess * (1.0/autocvar_ca_grid);
	best_x = rint(best_x);	//snap to grid
	best_y = rint(best_y);
	best_z = rint(best_z);
	best *= autocvar_ca_grid;

	//find surfaces within 32qu of the point (via plane volume). use a tetrahedron instead if you want something more circular
	for (facenum = 0; facenum < axis.length; facenum++)
		dists[facenum] = (guess * axis[facenum]) + autocvar_ca_grid;
	int numbrushes = brush_findinvolume(selectedbrushmodel, axis, dists, 6, brushlist, __NULL__, brushlist.length);

	for (int brushnum = 0; brushnum < numbrushes; brushnum++)
	{
		for (facenum = 0; ; )
		{
			points = brush_getfacepoints(selectedbrushmodel, brushlist[brushnum], ++facenum, facepoints, MAX_FACEPOINTS);
			if (!points)
				break;	//end of face list, I guess

			//walk the faces we found and use the point if its nearer than our previous guess.
			for (int point = 0; point < points; point++)
			{
				vector disp = facepoints[point] - guess;
				float dist = disp*disp;
				if (dist < bestdist)
					best = facepoints[point];
			}
		}
	}

	return best;
};



//move a brush so that its planes all move without any translations in positions or texcoords
void brushface_translate(vector displacement)
{
	int i;
	if (tmp.numcp)
	{
		for (i = 0; i < tmp.numcp; i++)
			tmp.cp[i].xyz += displacement;
	}
	else
	{
		for (i = 0; i < tmp.numfaces; i++)
		{
			tmp.faces[i].planedist += tmp.faces[i].planenormal * displacement;
			tmp.faces[i].sbias -= tmp.faces[i].sdir * displacement;
			tmp.faces[i].tbias -= tmp.faces[i].tdir * displacement;
		}
	}
};
//rotates a list of faces by the current v_* vectors, around the origin.
//translate before+after first in order to deal with pivots.
void brushface_rotate(void)
{
	if (tmp.numcp)
	{
		for (int i = 0; i < tmp.numcp; i++)
		{
			vector orig = tmp.cp[i].xyz;
			tmp.cp[i].xyz = [orig * v_forward,
					orig * -v_right,
					orig * v_up];
			//don't need to touch tcs
		}
	}
	else
	{
		brushface_t *fa = tmp.faces;
		int numfaces = tmp.numfaces;
		for (int i = 0; i < numfaces; i++, fa++)
		{
			vector orig = fa->planenormal;
			fa->planenormal[0] = orig * v_forward;
			fa->planenormal[1] = orig * -v_right;	//quake just isn't right...
			fa->planenormal[2] = orig * v_up;

			orig = fa->sdir;
			fa->sdir[0] = orig * v_forward;
			fa->sdir[1] = orig * -v_right;	//quake just isn't right...
			fa->sdir[2] = orig * v_up;

			orig = fa->tdir;
			fa->tdir[0] = orig * v_forward;
			fa->tdir[1] = orig * -v_right;	//quake just isn't right...
			fa->tdir[2] = orig * v_up;
		}
	}
};

vector(vector dir) axialize =
{
	vector a;
	a_x = fabs(dir_x);
	a_y = fabs(dir_y);
	a_z = fabs(dir_z);
	if (a_x > a_y && a_x > a_z)
		return (dir_x > 0)?[1,0,0]:[-1,0,0];
	if (a_y > a_x && a_y > a_z)
		return (dir_y > 0)?[0,1,0]:[0,-1,0];
	return (dir_z > 0)?[0,0,1]:[0,0,-1];
};

vector(vector in) channelizeangle =
{
	in_x = anglemod(in_x);
	in_y = anglemod(in_y);
	in_z = anglemod(in_z);
	if (in_x > 180)
		in_x -= 360;
	if (in_y > 180)
		in_y -= 360;
	if (in_z > 180)
		in_z -= 360;

	float fx = fabs(in_x);
	float fy = fabs(in_y);
	float fz = fabs(in_z);
	cprint(sprintf("%v", in));
	if (fx > fy && fx > fz)
		return [in_x,0,0];
	if (fy > fz)
		return [0,in_y,0];
	return [0,0,in_z];
}

vector(vector p1, vector p2, vector norm, float dist) planelinepoint =
{
	float d1 = p1*norm - dist;
	float d2 = p2*norm - dist;
	float frac = d1 / (d2-d1);

//	frac = bound(0, frac, 1);

	return p2 + (p1-p2)*frac;	//convert that frac into an actual position
};


int(brushface_t *faces, int numfaces, vector *points, int numpoints) isconcave =
{
	int result = 0;

	//if any of the points are outside the brush, then we know that one of the planes cut straight through in a concavey kind of way
	for(int f = 0; f < numfaces; f++)
	{
		vector n = faces[f].planenormal;
		float d = faces[f].planedist + EPSILON; //epsilon to cover precision issues
		for (int p = 0; p < numpoints; p++)
		{
			if (points[p] * n > d)
			{
				result++;
				break;
			}
		}
	}

	return result;
};

//returns the resulting brush id
int(int model, int brush1, int brush2, int face1, int face2) mergebrushes =
{
	int extrafaces;
	float found = FALSE;
	brushface_t *fa;
	brushface_t *infa;
	int i;

	if (brush1 == brush2)
	{
		print("cannot merge brush with itself\n");
		return 0;
	}
	if (!brush1 || !brush2)
	{
		print("no brush targetted\n");
		return 0;
	}

	if (patch_getcp(model, brush1, __NULL__, 0, __NULL__) || patch_getcp(model, brush2, __NULL__, 0, __NULL__))
	{	//fixme: should be possible if they're reasonably adjacent with the same numbers of CPs
		print("unable to merge patches\n");
		return 0;
	}

	tmp.numfaces = brush_get(model, brush1, tmp.faces, tmp.faces.length, &tmp.contents);
	tmp.numcp = 0;
	infa = &tmp.faces[tmp.numfaces];
	extrafaces = brush_get(model, brush2, &tmp.faces[tmp.numfaces], tmp.faces.length-tmp.numfaces, &tmp.contents);

	//merge the two sets of planes together.
	for(infa = &tmp.faces[tmp.numfaces]; extrafaces > 0; infa++, extrafaces--)
	{
		for (fa = tmp.faces, i = 0; i < tmp.numfaces; i++, fa++)
		{
//fixme: needs some tolerance / epsilon
			if (fa->planenormal == -infa->planenormal && fa->planedist == -infa->planedist)
			{
				//inverted. this is the plane we're merging over, so strip it out
				memcpy(fa, &fa[1], sizeof(brushface_t) * ((tmp.numfaces-i)-1));
				tmp.numfaces--;
				i--;
				fa--;
				found = TRUE;
				break;
			}
			else if (fa->planenormal * infa->planenormal > 0.999 && fa->planedist == infa->planedist)
			{
//print("coplanar\n");
				//coplanar surfaces are considered safe to ignore. we use the selected brush's texturing info
				break;
			}
		}
		if (i == tmp.numfaces)
		{	//okay, this plane is important, merge it into the selected brush.
//print("merge plane\n");
			memcpy(fa, infa, sizeof(brushface_t));
			tmp.numfaces++;
		}
	}
	if (!found)
	{
		print("Brushes do not share a plane\n");
#if 0
		//try to find a surface to move to match to the given plane
		float val;
		float bestval = -0.707;	//must be within 45 degrees
		int bestface = -1;
		tmp_numfaces = brush_get(model, brush1, tmp_faces, tmp_faces.length, &tmp_contents);
		brush_get(model, brush2, &tmp_faces[tmp_numfaces], tmp_faces.length-tmp_numfaces, &tmp_contents);
		infa = &tmp_faces[tmp_numfaces + face2-1i];
		for (i = 0; i < tmp_numfaces; i++)
		{
			val = tmp_faces[i].planenormal * infa->planenormal;
			if (val < bestval)
			{
				bestval = val;
				bestface = i;
			}
		}
		if (bestface != -1)
		{
			//FIXME: determine volume and reject it if we shrink?
			tmp_faces[bestface].planenormal = -infa->planenormal;
			tmp_faces[bestface].planedist = -infa->planedist;

		//	if (isconcave(tmp_faces, tmp_numfaces))
		//	{
		//		print("Resulting brush would be concave\n");
		//		return 0;
		//	}

			brush_history_delete(model, brush1);
			return brush_history_create(model, tmp_faces, tmp_numfaces, tmp_contents);
		}
		else
#endif
			return 0;
	}
	else
	{
		vector *points = memalloc(sizeof(vector)*64*64);
		int numpoints = 0, f;
		for(f = 0; (i = brush_getfacepoints(model, brush1, ++f, points+numpoints, 64*64-numpoints)); )
			numpoints += i;
		for(f = 0; (i = brush_getfacepoints(model, brush2, ++f, points+numpoints, 64*64-numpoints)); )
			numpoints += i;

		if (isconcave(tmp.faces, tmp.numfaces, points, numpoints))
		{
			print("Resulting brush would be concave\n");
			memfree(points);
			return 0;
		}
		memfree(points);

	//FIXME: verify that no planes got dropped, as this indicates that the region wasn't convex, and we probably just destroyed the entire thing.
		brush_history_delete(model, brush1);
		brush_history_delete(model, brush2);

		return brush_history_create(model, tmp.faces, tmp.numfaces, tmp.contents, TRUE);
	}
};




/*
void(vector org, vector ang) editor_brushes_simpleclone =
{
	vector midpoint;
	if (!selectedbrush)
		return;

	tmp.numfaces = brush_get(selectedbrushmodel, selectedbrush, tmp.faces, tmp.faces.length, &tmp.contents);
	if (ang != '0 0 0')
	{
		brush_getfacepoints(selectedbrushmodel, selectedbrush, 0, &midpoint, 1);
		brushface_translate(tmp.faces, tmp_numfaces, -midpoint);
		makevectors(ang);
		brushface_rotate(tmp_faces, tmp_numfaces);
		brushface_translate(tmp.faces, tmp_numfaces, midpoint + org);
	}
	else
		brushface_translate(tmp.faces, tmp_numfaces, org);
	brush_history_create(selectedbrushmodel, tmp.faces, tmp.numfaces, tmp.contents, TRUE);
};
*/

void() brushedit_subtract =
{
	static int discard;
	static vector selnormals[tmp.faces.length];
	static float seldists[tmp.faces.length];
	
	for (int sb = 0; sb < selectedbrushcount; sb++)
	{
		int mod = selectedbrushes[sb].model;
		int brush = selectedbrushes[sb].id;
		int planecount = brush_get(mod, brush, tmp.faces, tmp.faces.length, &tmp.contents);
		if (!planecount)
			continue; //csg can't subtract patches
		for (int i = 0; i < planecount; i++)
		{
			selnormals[i] = tmp.faces[i].planenormal;
			seldists[i] = tmp.faces[i].planedist;
		}
		int numbrushes = brush_findinvolume(mod, selnormals, seldists, planecount, brushlist, __NULL__, brushlist.length);

		while (numbrushes --> 0)
		{
			int br = brushlist[numbrushes];
			
			if (brush_isselected(mod, br))
				continue;	//ignore other selected brushes. race conditions suck

			int counto = brush_get(mod, br, tmp.faces, tmp.faces.length, &tmp.contents);
			if (!counto)
				continue; //don't cut into patches.
			int counts = counto + brush_get(mod, brush, tmp.faces+counto, tmp.faces.length-counto, &discard);

			brush_history_delete(mod, br);
			while(counts --> counto)
			{
				//only consider the resulting brush if the new face actually contributed anything.
				//this reduces dupes.
				if (brush_calcfacepoints(1+counts, tmp.faces, counts+1, facepoints, MAX_FACEPOINTS))
				{
					//determine the brush defined by this plane
					tmp.faces[counts].planenormal *= -1;
					tmp.faces[counts].planedist *= -1;
					brush_history_create(mod, tmp.faces, counts+1, tmp.contents, FALSE);
				}
			}
		}
	}
};

void() brushedit_resettextures =
{
	for (int sb = 0; sb < selectedbrushcount; sb++)
	{
		int model = selectedbrushes[sb].model;
		int brush = selectedbrushes[sb].id;
		__uint64 facemask = selectedbrushes[sb].facemask;
		
		int planecount = brush_get(model, brush, tmp.faces, tmp.faces.length, &tmp.contents);		
		if (planecount)
		{
			for (int i = 0; i < planecount; i++)
				if (facemask & (1lu << i))
					reset_texturecoords(&tmp.faces[i]);

			brush_history_edit(model, brush, tmp.faces, planecount, tmp.contents);
		}
	}
};

void(string newtexture) brushedit_settexture =
{
	for (int sb = 0; sb < selectedbrushcount; sb++)
	{
		int model = selectedbrushes[sb].model;
		int brush = selectedbrushes[sb].id;
		__uint64 facemask = selectedbrushes[sb].facemask;
		
		int planecount = brush_get(model, brush, tmp.faces, tmp.faces.length, &tmp.contents);		
		if (planecount)
		{
			for (int i = 0; i < planecount; i++)
				if (facemask & (1lu << i))
					tmp.faces[i].shadername = newtexture;

			brush_history_edit(model, brush, tmp.faces, planecount, tmp.contents);
		}
	}
};