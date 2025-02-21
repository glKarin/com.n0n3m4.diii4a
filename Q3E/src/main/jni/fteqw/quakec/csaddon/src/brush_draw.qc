void(int mod, int id) DrawEngineBrushWireframe =
{
	const vector col = '1 0 0';
	for(int facenum = 0;;)
	{
		int points = brush_getfacepoints(mod, id, ++facenum, &facepoints[0], facepoints.length);
		if (!points)
			break;	//end of face list, I guess

		R_BeginPolygon("chop");
		R_PolygonVertex(facepoints[0], '0 0', col, 1);
		for (int point = 1; point < points; point++)
		{
			R_PolygonVertex(facepoints[point], '0 0', col, 1);
			R_EndPolygon();
			R_PolygonVertex(facepoints[point], '0 0', col, 1);
		}
		R_PolygonVertex(facepoints[0], '0 0', col, 1);
		R_EndPolygon();
	}
};
void(patchvert_t *cp, patchinfo_t info, vector col, float alpha) DrawQCPatchTextured;
void(int mod, int id, float alpha) DrawEngineBrushFaded =
{	//draw one of the engine's brushes, but faded.
	const vector col = '1 1 1';
	static int contents;
	static brushface_t faces[MAX_BRUSHFACES];
	int numfaces = brush_get(mod, id, &faces[0], faces.length, &contents);
	if (!numfaces)	
	{
		numfaces = patch_getmesh(mod, id, __NULL__, 0, __NULL__);	//ask how much space we need
		if (numfaces)
		{
			patchvert_t *v = memalloc(sizeof(*v)*numfaces);
			static patchinfo_t patchinfo;
			patch_getmesh(mod, id, v, numfaces, &patchinfo);	//now we can actually get it
			DrawQCPatchTextured(v, patchinfo, col, alpha);
			memfree(v);
		}
		
		return;
	}

	for(int f = 0; f < numfaces; f++)
	{
		int points = brush_getfacepoints(mod, id, 1+f, &facepoints[0], facepoints.length);
		if (!points)
			continue;

		//this is unfortunate. the built in shaders expect to use lightmaps. we don't have those.
		//because lightmaps are special things, we end up in a real mess. so lets just make sure there's a shader now, because we can.
		//FIXME: we don't manage to pick up the size of the original wad image
		
		string materialname = strcat("textures/", faces[f].shadername);
		shaderforname(materialname,
			sprintf("{"
				"{\n"
					"map \"%s\"\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
				"}\n"
			"}", faces[f].shadername));

		vector sz = drawgetimagesize(materialname);
		R_BeginPolygon(materialname, 3);
		for (int point = 0; point < points; point++)
			R_PolygonVertex(facepoints[point], [(facepoints[point] * faces[f].sdir + faces[f].sbias)/sz_x, (facepoints[point] * faces[f].tdir + faces[f].tbias)/sz_y], col, alpha);
		R_EndPolygon();
	}
};

void(patchvert_t *vert, patchinfo_t info, string shader, vector col, float alpha) DrawQCPatchWireframe =
{
	int x,y,cp;
	R_BeginPolygon(shader);
	for (y = 0, cp = 0; y < info.cpheight; y++, cp++)
		for (x = 0; x < info.cpwidth-1; x++)
		{
			R_PolygonVertex(vert[cp].xyz, [vert[cp].s, vert[cp].t], col, alpha);
			cp++;
			R_PolygonVertex(vert[cp].xyz, [vert[cp].s, vert[cp].t], col, alpha);
			R_EndPolygon();
		}
	for (x = 0; x < info.cpwidth; x++)
		for (y = 0, cp=x; y < info.cpheight-1; y++)
		{
			R_PolygonVertex(vert[cp].xyz, [vert[cp].s, vert[cp].t], col, alpha);
			cp += info.cpwidth;
			R_PolygonVertex(vert[cp].xyz, [vert[cp].s, vert[cp].t], col, alpha);
			R_EndPolygon();
		}
};

void(brushface_t *faces, int numfaces, string shader, vector col, float alpha) DrawQCBrushWireframe =
{
	int f;
	int point, points;
	for(f = 0; f < numfaces;)
	{
		points = brush_calcfacepoints(++f, faces, numfaces, facepoints, MAX_FACEPOINTS);
		if (!points)
			continue;	//should probably warn somehow about this
		R_BeginPolygon(shader);
		R_PolygonVertex(facepoints[0], '0 0', col, alpha);
		for (point = 0; point < points-1; )
		{
			point++;
			R_PolygonVertex(facepoints[point], '0 0', col, alpha);
			R_EndPolygon();
			R_PolygonVertex(facepoints[point], '0 0', col, alpha);
		}
		R_PolygonVertex(facepoints[0], '0 0', col, alpha);
		R_EndPolygon();
	}
};
void(brushface_t *faces, int numfaces, string shader, vector col, float alpha) DrawQCBrushSolid =
{
	int f;
	int point, points;
	for(f = 0; f < numfaces;)
	{
		points = brush_calcfacepoints(++f, faces, numfaces, facepoints, MAX_FACEPOINTS);
		if (!points)
			continue;	//should probably warn somehow about this
		R_BeginPolygon(shader);
		for (point = 0; point < points; point++)
			R_PolygonVertex(facepoints[point], '0 0', col, alpha);
		R_EndPolygon();
	}
};
void(brushface_t *faces, int numfaces, vector col, float alpha) DrawQCBrushTextured =
{
	int f;
	int point, points;
	for(f = 0; f < numfaces; f++)
	{
		points = brush_calcfacepoints(1+f, faces, numfaces, facepoints, MAX_FACEPOINTS);
		if (points)
		{
			//this is unfortunate. the built in shaders expect to use lightmaps. we don't have those.
			//because lightmaps are special things, we end up in a real mess. so lets just make sure there's a shader now, because we can.
			//FIXME: we don't manage to pick up the size of the original wad image
			string materialname = strcat("textures/", faces[f].shadername);
			shaderforname(materialname,
				sprintf("{"
					"{\n"
						"map \"%s\"\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
					"}\n"
				"}", faces[f].shadername));

			vector sz = drawgetimagesize(materialname);
			R_BeginPolygon(materialname, 3);
			for (point = 0; point < points; point++)
				R_PolygonVertex(facepoints[point], [(facepoints[point] * faces[f].sdir + faces[f].sbias)/sz_x, (facepoints[point] * faces[f].tdir + faces[f].tbias)/sz_y], col, alpha);
			R_EndPolygon();
		}
	}
};
void(patchvert_t *cp, patchinfo_t info, vector col, float alpha) DrawQCPatchTextured =
{
	int x, y;

	string materialname = strcat("textures/", info.shadername);
	shaderforname(materialname,
		sprintf("{"
			"{\n"
				"map \"%s\"\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}", info.shadername));

	R_BeginPolygon(materialname);
	for (y = 0; y < info.cpheight-1; y++, cp++)
	for (x = 0; x < info.cpwidth-1; x++, cp++)
	{
#if 1
		R_PolygonVertex(cp[0].xyz, [cp[0].s,cp[0].t], col, alpha);
		R_PolygonVertex(cp[1].xyz, [cp[1].s,cp[1].t], col, alpha);
		R_PolygonVertex(cp[1+info.cpwidth].xyz, [cp[1+info.cpwidth].s,cp[1+info.cpwidth].t], col, alpha);
		R_PolygonVertex(cp[0+info.cpwidth].xyz, [cp[0+info.cpwidth].s,cp[0+info.cpwidth].t], col, alpha);
#else
		R_PolygonVertex(cp[0].xyz, [cp[0].s,cp[0].t], cp[0].rgb, cp[0].a);
		R_PolygonVertex(cp[1].xyz, [cp[1].s,cp[1].t], cp[1].rgb, cp[1].a);
		R_PolygonVertex(cp[1+info.cpwidth].xyz, [cp[1+info.cpwidth].s,cp[1+info.cpwidth].t], cp[1+info.cpwidth].rgb, cp[1+info.cpwidth].a);
		R_PolygonVertex(cp[0+info.cpwidth].xyz, [cp[0+info.cpwidth].s,cp[0+info.cpwidth].t], cp[0+info.cpwidth].rgb, cp[0+info.cpwidth].a);
#endif
		R_EndPolygon();
	}
};

void(vector *p, int points, string shader, vector col, float alpha) DrawAxisExtensions =
{
	R_BeginPolygon(shader);
	for (int point = 0; point < points; point++)
	{
		R_PolygonVertex(p[point] + [ 64, 0, 0], '0 0', col, alpha);
		R_PolygonVertex(p[point] + [-64, 0, 0], '0 0', col, alpha);
		R_EndPolygon();
		R_PolygonVertex(p[point] + [0,  64, 0], '0 0', col, alpha);
		R_PolygonVertex(p[point] + [0, -64, 0], '0 0', col, alpha);
		R_EndPolygon();
		R_PolygonVertex(p[point] + [0, 0,  64], '0 0', col, alpha);
		R_PolygonVertex(p[point] + [0, 0, -64], '0 0', col, alpha);
		R_EndPolygon();
	}
};


void(int brushid) editor_drawbbox =
{
	static vector bbox[2];
	int p = brush_getfacepoints(selectedbrushmodel, brushid, 0, bbox, bbox.length);
	if (p == 2)
	{
		R_BeginPolygon("chop");
#define line(x,y) 			R_PolygonVertex(x, '0 0', '1 0 0', 1);	R_PolygonVertex(y, '0 0', '1 0 0', 1);	R_EndPolygon()
		line(bbox[0], ([bbox[1][0], bbox[0][1], bbox[0][2]]));
		line(bbox[0], ([bbox[0][0], bbox[1][1], bbox[0][2]]));
		line(bbox[0], ([bbox[0][0], bbox[0][1], bbox[1][2]]));
		line(bbox[1], ([bbox[0][0], bbox[1][1], bbox[1][2]]));
		line(bbox[1], ([bbox[1][0], bbox[0][1], bbox[1][2]]));
		line(bbox[1], ([bbox[1][0], bbox[1][1], bbox[0][2]]));

		line(([bbox[1][0], bbox[0][1], bbox[0][2]]), ([bbox[1][0], bbox[1][1], bbox[0][2]]));
		line(([bbox[1][0], bbox[0][1], bbox[0][2]]), ([bbox[1][0], bbox[0][1], bbox[1][2]]));
		line(([bbox[0][0], bbox[1][1], bbox[0][2]]), ([bbox[0][0], bbox[1][1], bbox[1][2]]));
		line(([bbox[0][0], bbox[1][1], bbox[0][2]]), ([bbox[1][0], bbox[1][1], bbox[0][2]]));
		line(([bbox[0][0], bbox[0][1], bbox[1][2]]), ([bbox[0][0], bbox[1][1], bbox[1][2]]));
		line(([bbox[0][0], bbox[0][1], bbox[1][2]]), ([bbox[1][0], bbox[0][1], bbox[1][2]]));
#undef line
	}
};
