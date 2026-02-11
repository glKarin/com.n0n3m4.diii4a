
typedef struct
{
	int numverts;
	int numidx;
	vector *p;
	int *i;
	
	int selectedvert1;
	int selectedvert2;
	
	int model;
	int brush;
} vertsoup_t;
vertsoup_t vertedit;

static patchinfo_t patchinfo;
static patchvert_t *patchvert;
static int maxcp = 64*64;


//take a brush apart and generate trisoup from it.
//note that the trisoup does not have textures. they will be reconciled when reforming the brush.
void(vertsoup_t *vertedit, int model, int brush) Debrushify =
{
	int *ni;
	int i, j, k;
	int points;
	vector p;
	vector *np;
	static int fi[64];
	vertedit->model = model;
	vertedit->brush = brush;
	vertedit->numidx = 0;
	vertedit->numverts = patch_getcp(vertedit->model, vertedit->brush, __NULL__, 0, __NULL__);
	if (vertedit->numverts)
	{	//okay, this is a patch. one logical face.
		memfree(vertedit->i);
		memfree(vertedit->p);

		vertedit->p = memalloc(sizeof(*vertedit->p) * vertedit->numverts);

		if (!patchvert)
			patchvert = memalloc(sizeof(*patchvert)*maxcp);
		vertedit->numverts = patch_getcp(vertedit->model, vertedit->brush, patchvert, maxcp, &patchinfo);
		for (i = 0; i < vertedit->numverts; i++)
			vertedit->p[i] = patchvert[i].xyz;

		//some of the rest of the code assumes we have indexes. oh well.
		k = (patchinfo.cpwidth-1)*(patchinfo.cpheight-1);
		vertedit->numidx = k*6;
		vertedit->i = ni = memalloc(sizeof(*ni) * vertedit->numidx);
		for (i = 0; i < k; i++, ni+=6)
		{
			ni[0] = i;
			ni[1] = i+1;
			ni[2] = i+patchinfo.cpwidth;

			ni[3] = i+1;
			ni[4] = i+patchinfo.cpwidth+1;
			ni[5] = i+patchinfo.cpwidth;
		}
		return;
	}

	//brush.
	for (i = 0; ; )
	{
		points = brush_getfacepoints(vertedit->model, vertedit->brush, ++i, facepoints, facepoints.length);
		if (!points)
			break;
		
		//allocate a few new indexes
		ni = memalloc(sizeof(*ni) * (vertedit->numidx+3*(points-2)));
		memcpy(ni, vertedit->i, sizeof(*ni) * vertedit->numidx);
		memfree(vertedit->i);
		vertedit->i = ni;
		ni += vertedit->numidx;
		vertedit->numidx += 3*(points-2);

		for (j = 0; j < points; j++)
		{
			p = facepoints[j];
			p_x = floor(p_x + 0.5);	//gah, bloomin inprecision.
			p_y = floor(p_y + 0.5);
			p_z = floor(p_z + 0.5);
			for (k = 0; k < vertedit->numverts; k++)
			{	//try to be nice and re-use verts.
				if (vertedit->p[k] == p)
					break;
			}
			if (k == vertedit->numverts)
			{
				//if it wasn't found, we need to allocate a new one
				np = memalloc(sizeof(*np) * (vertedit->numverts+1));
				memcpy(np, vertedit->p, sizeof(*np) * vertedit->numverts);
				memfree(vertedit->p);
				vertedit->p = np;
				np += vertedit->numverts;
				vertedit->numverts += 1;
				*np = p;
			}
			fi[j] = k;
		}
		for (j = 2; j < points; j++)
		{
			*ni++ = fi[0];
			*ni++ = fi[j-1];
			*ni++ = fi[j];
		}
	}
};

//determines only the various points of the brush, without duplicates. doesn't care about indexes.
void(void) DebrushifyLite =
{
	int points, k;
	vector p;
	vector *np;
	int fi[64];
	vertedit.numidx = 0;

	vertedit->numverts = patch_getcp(vertedit->model, vertedit->brush, __NULL__, 0, __NULL__);
	if (vertedit->numverts)
	{	//okay, this is a patch. one logical face.
		memfree(vertedit->i);
		memfree(vertedit->p);

		vertedit->i = __NULL__;	//don't bother.
		vertedit->p = memalloc(sizeof(*vertedit->p) * vertedit->numverts);

		if (!patchvert)
			patchvert = memalloc(sizeof(*patchvert)*maxcp);

		vertedit->numverts = patch_getcp(vertedit->model, vertedit->brush, patchvert, maxcp, &patchinfo);

		for (int i = 0; i < vertedit->numverts; i++)
			vertedit->p[i] = patchvert[i].xyz;
		return;
	}

	for (int i = 0; ; )
	{
		points = brush_calcfacepoints(++i, tmp.faces, tmp.numfaces, facepoints, facepoints.length);
		if (!points)
			break;
		
		for (int j = 0; j < points; j++)
		{
			p = facepoints[j];
			p_x = floor(p_x + 0.5);	//gah, bloomin inprecision.
			p_y = floor(p_y + 0.5);
			p_z = floor(p_z + 0.5);
			for (k = 0; k < vertedit.numverts; k++)
			{	//try to be nice and re-use verts.
				if (vertedit.p[k] == p)
					break;
			}
			if (k == vertedit.numverts)
			{
				//if it wasn't found, we need to allocate a new one
				np = memalloc(sizeof(*np) * (vertedit.numverts+1));
				memcpy(np, vertedit.p, sizeof(*np) * vertedit.numverts);
				memfree(vertedit.p);
				vertedit.p = np;
				np += vertedit.numverts;
				vertedit.numverts += 1;
				*np = p;
			}
			fi[j] = k;
		}
	}
};




static float(vertsoup_t *soup, int *idx, __out vector norm, __out float dist) planenormal =
{
	vector v1 = soup->p[idx[0]];
	vector v2 = soup->p[idx[1]];
	vector v3 = soup->p[idx[2]];
	vector d1 = v3 - v1;
	vector d2 = v2 - v1;
	vector d3 = v3 - v2;
	norm = normalize(crossproduct(d1, d2));
	dist = norm * v1;

	if (!d1 || !d2 || !d3 || !norm)
		return FALSE;
	return TRUE;
};

void(vertsoup_t *soup, int drawit) Rebrushify =
{
	static brushface_t faces[MAX_BRUSHFACES];
	int numfaces, f, point;
	string shader = 0;
	
	vector n;
	float d;
	int o=0;
		
	f = patch_getcp(soup->model, soup->brush, patchvert, maxcp, &patchinfo);
	if (f)
	{
		for (o = 0; o < f; o++)
			patchvert[o].xyz = soup.p[o];
		if (drawit)
		{
			const int maxtessverts=64*64;
			static patchinfo_t tessinfo;
			patchvert_t *tessverts = memalloc(sizeof(*tessverts)*maxtessverts);
			tessinfo = patchinfo;
			if (patch_evaluate(patchvert, tessverts, maxtessverts, &tessinfo) <= maxtessverts)
			{
				//draw textured preview (yay for fullbright lighting being overbright)
				DrawQCPatchTextured(tessverts, tessinfo, '0.7 0.7 0.7', 0.7);		//display the patch. in-place so you know what it'll actually look like. stoopid tessellation.
				DrawQCPatchWireframe(tessverts, tessinfo, "chop", [0.3,0,0.3], 1);	//show a somewhat faded indication of how many quads are actually being used.
			}
			memfree(tessverts);
			
			//draw it wireframe without depth testing
			DrawQCPatchWireframe(patchvert, patchinfo, "chop", [0,0,1], 1);
		}
		else
		{
			patch_history_edit(soup->model, soup->brush, patchvert, patchinfo); 
			
			soup->numidx = 0;
			soup->numverts = 0;
		}	
		return;
	}
	
	tmp.numfaces = brush_get(soup->model, soup->brush, tmp.faces, tmp.faces.length, &tmp.contents);
	
	//if any triangle's neighbour opposes it, reform the edge across the quad to try to keep it convex.
	for(point = 0; point+2 < soup->numidx; point+=3)
	{
		int p1 = soup->i[point+0];
		int p2 = soup->i[point+1];
		int p3 = soup->i[point+2];
		if (!planenormal(soup, &soup->i[point], n, d))
			continue;	//degenerate
		d += EPSILON;
		for(f = point+3; f+2 < soup->numidx; f+=3)
		{
			int o1 = soup->i[f+0];
			int o2 = soup->i[f+1];
			int o3 = soup->i[f+2];
p1p2edge:
			if (o2 == p1 && o1 == p2)
				o = o3;
			else if (o3 == p1 && o2 == p2)
				o = o1;
			else if (o1 == p1 && o3 == p2)
				o = o2;
			else
				goto p2p3edge;
			if (soup->p[o] * n > d)
			{
				soup->i[f+0] = p3;
				soup->i[f+1] = o;
				soup->i[f+2] = p2;
				p2 = o;
			}
			continue;
p2p3edge:
			if (o2 == p2 && o1 == p3)
				o = o3;
			else if (o3 == p2 && o2 == p3)
				o = o1;
			else if (o1 == p2 && o3 == p3)
				o = o2;
			else
				goto p3p1edge;
			if (soup->p[o] * n > d)
			{
				soup->i[f+0] = p1;
				soup->i[f+1] = o;
				soup->i[f+2] = p3;
				p3 = o;
			}
			continue;
p3p1edge:
			if (o2 == p3 && o1 == p1)
				o = o3;
			else if (o3 == p3 && o2 == p1)
				o = o1;
			else if (o1 == p3 && o3 == p1)
				o = o2;
			else
				continue;
			if (soup->p[o] * n > d)
			{
				soup->i[f+0] = p2;
				soup->i[f+1] = o;
				soup->i[f+2] = p1;
				p1 = o;
			}
			continue;
		}
		
		soup->i[point+0] = p1;
		soup->i[point+1] = p2;
		soup->i[point+2] = p3;
	}
	
	//generate the plane info
	numfaces = 0;
	for(point = 0; point+2 < soup->numidx; point+=3)
	{
		if (!planenormal(soup, &soup->i[point], n, d))
			continue;	//a degenerate triangle is one that probably got merged or something

		for (f = 0; f < numfaces; f++)
		{
			if (faces[f].planenormal == n && faces[f].planedist == d)
				break;
		}
		if (f < numfaces)
			continue;	//duplicate plane
			
		for (f = 0; f < tmp.numfaces; f++)
		{
			if (tmp.faces[f].planenormal * n > 0.999) //stupid inprecisions
			{
				if (numfaces == 64)
					return;
					
				//note that we only care about the normal, not the dist. this means you can move the entire face forward+back without loosing textures.
				faces[numfaces].shadername = shader = tmp.faces[f].shadername;
				faces[numfaces].planenormal = n;
				faces[numfaces].planedist = d;
				faces[numfaces].sdir = tmp.faces[f].sdir;
				faces[numfaces].sbias = tmp.faces[f].sbias;
				faces[numfaces].tdir = tmp.faces[f].tdir;
				faces[numfaces].tbias = tmp.faces[f].tbias;
				numfaces++;
				break;
			}
		}
		if (f < tmp.numfaces)
			continue;	//matched a plane in the original brush
			
		if (numfaces == 64)
			return;
			
		//FIXME: find aproximate faces to give corners or something

		//okay, it appears to be new. that's a pain.
		faces[numfaces].shadername = 0;
		faces[numfaces].planenormal = n;
		faces[numfaces].planedist = d;
		reset_texturecoords(&faces[numfaces]);
		numfaces++;
	}
	
	//any surface without a texture/shader yet should inherit one from some other surface
	if (!shader)
		shader = autocvar_ca_newbrushtexture;
	for(f = 0; f < numfaces; f++)
	{
		if (!faces[f].shadername)
			faces[f].shadername = shader;
	}

	int internals = 0;
	//If we have a point outside a plane, then we know we have a concave volume.
	//chop the points 
	for(f = 0; f < numfaces; f++)
	{
		n = faces[f].planenormal;
		d = faces[f].planedist;
		for (point = 0; point < soup->numidx; point++)	//would ideally use points, but I want to cover dead ones
		{
			if (soup->p[soup->i[point]] * n > d + EPSILON)
			{
				internals++;
				break;
			}
		}
	}
//	cprint(sprintf("%i internal splits, %i faces\n", internals, numfaces));
	
	if (numfaces <= 3)
		return;	//can't possibly be valid.
	
	if (drawit)
	{	
		//draw textured preview (yay for block lighting)
		DrawQCBrushTextured(faces, numfaces, '0.7 0.7 0.7', 1);
		
		//draw it wireframe without depth testing
		DrawQCBrushWireframe(faces, numfaces, "chop", internals?[1,0,0]:[0,0,1], 1);
	}
	else
	{
		brush_history_edit(soup->model, soup->brush, faces, numfaces, 1i); 
		
		soup->numidx = 0;
		soup->numverts = 0;
	}	
};