/*traceline/tracebox returns trace_brush_id and trace_brush_faceid


mod should track selected brush list instead of polling.
to move 50 brushes, mod needs to get+delete+transform+create
brush ids are ints. this allows different clients to use different ranges without float problems.
*/





//when we're drawing sideviews, we normally need them wireframe in order to actually see anything
static void(vector sizes) drawwireframeview =
{
	//figure out the planes that we care about.
	vector org = getviewprop(VF_ORIGIN);
	static vector vaxis[6];
	vaxis[0] = v_right;
	vaxis[1] = v_up;
	vaxis[2] = v_forward;
	for (int i = 0; i < 3; i++)
		dists[i] = (org*vaxis[i]) + sizes[i];
	for (int i = 0; i < 3; i++)
	{
		vaxis[i+3] = -vaxis[i];
		dists[i+3] = (org*vaxis[i+3]) + sizes[i];
	}

	int numbrushes = brush_findinvolume(selectedbrushmodel, vaxis, dists, dists.length, &brushlist[0], __NULL__, brushlist.length);
	
	//this can be a bit slow with lots of brushes. would be nice to batch them a bit better.
	while(numbrushes --> 0)
		DrawEngineBrushWireframe(selectedbrushmodel, brushlist[numbrushes]);
	setviewprop(VF_DRAWWORLD, FALSE);
};

static void() editor_brushes_drawselected =
{
	float intensity = (sin(gettime(5)*4)+1)*0.05;
	int facenum, point, points;
	vector col;

	for (int sb = 0; sb < selectedbrushcount; sb++)
	{
		int model = selectedbrushes[sb].model;
		int brush = selectedbrushes[sb].id;
		
		//draw a faded version of all the brushes.
		DrawEngineBrushFaded(model, brush, 0.5);
	}
	
	//draw all selected brush faces.
	for (int sb = 0; sb < selectedbrushcount; sb++)
	{
		int model = selectedbrushes[sb].model;
		int brush = selectedbrushes[sb].id;
		__uint64 facemask = selectedbrushes[sb].facemask;
		for(facenum = 0;; facenum++)
		{
			points = brush_getfacepoints(model, brush, 1+facenum, facepoints, MAX_FACEPOINTS);
			if (!points)
				break;	//end of face list, I guess

			if (facemask & (1ul<<facenum))
				col = [0,intensity,0];
			else
				col = [intensity,0,0];

			R_BeginPolygon("terrainedit");
			for (point = 0; point < points; point++)
				R_PolygonVertex(facepoints[point], '0 0', col, 1);
			R_EndPolygon();
		}
	}
	
	for (int sb = 0; sb < selectedbrushcount; sb++)
	{
		int model = selectedbrushes[sb].model;
		int brush = selectedbrushes[sb].id;
		
		//now draw wireframe
		DrawEngineBrushWireframe(model, brush);
	}
	
//	tmp.numfaces = brush_get(selectedbrushmodel, selectedbrush, tmp.faces, tmp.faces.length, &tmp.contents);
//	DebrushifyLite(tmp.faces, tmp.numfaces);
	DrawAxisExtensions(vertedit.p, vertedit.numverts, "terrainedit", '0 0 0.3', 1);
};

void(vector mousepos) editor_brushes_addentities =
{
	vector col = '0 0 0';
	int points, point;
	int facenum;
	float intensity = (sin(gettime(5)*4)+1)*0.05;
	vector displace = 0;
	static vector mid;
	
	autocvar_ca_brush_view = fabs(autocvar_ca_brush_view) % 4f;
	if (autocvar_ca_brush_view)
	{
		vector vmn = (vector)getproperty(VF_MIN);
		vector vsz = (vector)getproperty(VF_SIZE);
		vector ang = vectoangles([autocvar_ca_brush_view==1,autocvar_ca_brush_view==2,autocvar_ca_brush_view==3]);
		vector fov;
		if (vsz_x > vsz_y)
			fov = [autocvar_ca_brush_viewsize, autocvar_ca_brush_viewsize * (vsz_y/vsz_x), 8192];
		else
			fov = [autocvar_ca_brush_viewsize * (vsz_x/vsz_y), autocvar_ca_brush_viewsize, 8192];
		makevectors(ang);
		drawfill(vmn, vsz, '0 0 0', 1, 0);
		setviewprop(VF_VIEWENTITY, 0);
		setviewprop(VF_PERSPECTIVE, FALSE);
		setviewprop(VF_ANGLES, ang);
		setviewprop(VF_MAXDIST, fov_z*0.5);
		setviewprop(VF_MINDIST, -fov_z*0.5);	//I'm not entirely sure where the near clip plane should be. oh well.
		if (vsz_x > vsz_y)
			setviewprop(VF_FOV, fov);
		else
			setviewprop(VF_FOV, fov);

		if (curmousepos_x >= vmn_x && curmousepos_x <= vmn_x+vsz_x && curmousepos_y >= vmn_y && curmousepos_y <= vmn_y+vsz_y)
		{
			mousefar = unproject(curmousepos + '0 0 1');
			mousenear = unproject(curmousepos);
		}

		drawwireframeview(fov*0.5);
	}
	else
		makevectors(input_angles);
	
	if ((mousetool == BT_VERTEXEDIT || mousetool == BT_CREATEDRAG || brushtool == BT_PUSHFACE || brushtool == BT_CLONEDISPLACE || brushtool == BT_MOVE || brushtool == BT_MOVETEXTURE) && bt_points)
	{
		vector fwd = v_forward;
		vector rgt = v_right;
		vector up = v_up;
		vector farpos;

		fwd = axialize(fwd);
		rgt -= (rgt * fwd) * rgt;
		up -= (up * fwd) * up;
		rgt = axialize(rgt);
		up = axialize(up);

		farpos = normalize(mousefar-mousenear) + mousenear;
		farpos = planelinepoint(mousenear, farpos, v_forward, bt_point[0] * v_forward); //find where the cursor impacts the screen grid (moved along the view vector to match where the drag was last frame)
		displace = farpos - bt_point[0];
		if (mousetool != BT_VERTEXEDIT && mousetool != BT_CREATEDRAG)
			displace = brush_snappoint(displace);
		if (altdown)
			bt_displace_x = displace * fwd;
		else
		{
			bt_displace_y = displace * rgt;
			bt_displace_z = displace * up;
		}
		displace = bt_displace_x * fwd + bt_displace_y * rgt + bt_displace_z * up;
		
		if (mousetool == BT_VERTEXEDIT)
			displace = brush_snappoint(displace+bt_point[0]);
	}
	else if (brushtool == BT_ROTATE)
	{
		te_lightning2(world, bt_point[0], bt_point[0]+-bt_displace);
		col = vectoangles(bt_point[0]+bt_displace) - vectoangles(bt_point[0]);
		col = channelizeangle([col_x*-1,col_y,col_z]);
	}
	
	if (vertedit.numidx && mousetool != BT_VERTEXEDIT)
	{
		vertedit.numidx = 0;
		vertedit.numverts = 0;
	}

	//make sure this shader was generated already.
	shaderforname("terrainedit",
		"{"
			"{\n"
				"map terrainedit\n"
				"blendfunc add\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}");

	//make sure this shader was generated already.
	shaderforname("chop",
		"{"
			"cull disable\n"
			"{\n"
				"map terrainedit\n"
				"blendfunc add\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"sort nearest\n"
				"nodepthtest\n"
			"}\n"
		"}");

	if (mousetool == BT_VERTEXEDIT && vertedit.numidx)
	{
#if 0
		//draw it solid WITH depth testing
		R_BeginPolygon("terrainedit");
		for(point = 0; point+2 < vertedit.numidx; point+=3)
		{
			col = '0 0 0.1';
			R_PolygonVertex(vertedit.p[vertedit.i[point+0]], '0 0', col, 1);
			R_PolygonVertex(vertedit.p[vertedit.i[point+1]], '0 0', col, 1);
			R_PolygonVertex(vertedit.p[vertedit.i[point+2]], '0 0', col, 1);
			R_EndPolygon();
		}
#endif
#if 1
		//draw the wires (no depth)
		R_BeginPolygon("chop");
		for(point = 0; point+2 < vertedit.numidx; point+=3)
		{
			col = '0 0.1 0';
			R_PolygonVertex(vertedit.p[vertedit.i[point+0]], '0 0', col, 1);
			R_PolygonVertex(vertedit.p[vertedit.i[point+1]], '0 0', col, 1);
			R_EndPolygon();
			R_PolygonVertex(vertedit.p[vertedit.i[point+1]], '0 0', col, 1);
			R_PolygonVertex(vertedit.p[vertedit.i[point+2]], '0 0', col, 1);
			R_EndPolygon();
			R_PolygonVertex(vertedit.p[vertedit.i[point+2]], '0 0', col, 1);
			R_PolygonVertex(vertedit.p[vertedit.i[point+0]], '0 0', col, 1);
			R_EndPolygon();
		}
#endif

		DrawAxisExtensions(vertedit.p, vertedit.numverts, "terrainedit", '1 0.3 0.3', 1);

		if (mousedown)
		{
			if (bt_points == 1 && vertedit.selectedvert1 != -1)
			{
				if (vertedit.selectedvert2 != -1)
					vertedit.p[vertedit.selectedvert2] = displace + bt_point[2];
				vertedit.p[vertedit.selectedvert1] = displace + bt_point[1];
			}
		}
		else
			bt_points = 0;

		Rebrushify(&vertedit, TRUE);
	}
	else if (brushtool == BT_CREATEBRUSH || brushtool == BT_CREATEPATCH)
	{
		editor_brushes_drawselected();
		
		if (bt_points > 2)
		{
			tmp.numfaces = BrushFromPoints(tmp.faces, tmp.faces.length, bt_point, bt_points, autocvar_ca_newbrushheight);
			tmp.numcp = 0;
			
			DrawQCBrushWireframe(tmp.faces, tmp.numfaces, "chop", '1 0 0', 1);
		}
	}
	else if (mousetool == BT_CREATEDRAG)
	{
		if (bt_points)
		{
			displace -= axialize(v_forward)*autocvar_ca_grid;

			mid = bt_point[0];
			displace = brush_snappoint(mid + displace);
			mid = brush_snappoint(mid);
			displace = displace - mid;

			tmp.faces[0].planenormal = '1 0 0';
			tmp.faces[1].planenormal = '-1 0 0';
			tmp.faces[2].planenormal = '0 1 0';
			tmp.faces[3].planenormal = '0 -1 0';
			tmp.faces[4].planenormal = '0 0 1';
			tmp.faces[5].planenormal = '0 0 -1';
			tmp.faces[0].planedist = mid[0];
			tmp.faces[1].planedist = -mid[0];
			tmp.faces[2].planedist = mid[1];
			tmp.faces[3].planedist = -mid[1];
			tmp.faces[4].planedist = mid[2];
			tmp.faces[5].planedist = -mid[2];
			tmp.numfaces = 6;
			tmp.contents = 1;
			tmp.numcp = 0;
			
			tmp.faces[0 + (displace_x<0.0)].planedist += fabs(displace_x);
			tmp.faces[2 + (displace_y<0.0)].planedist += fabs(displace_y);
			tmp.faces[4 + (displace_z<0.0)].planedist += fabs(displace_z);
			
			for (facenum = 0; facenum < tmp.numfaces; facenum++)
			{
				tmp.faces[facenum].shadername = autocvar_ca_newbrushtexture;
				reset_texturecoords(&tmp.faces[facenum]);
			}

			DrawQCBrushWireframe(tmp.faces, tmp.numfaces, "chop", '1 0 0', 1);

			if (!mousedown)
			{
				brush_history_create(selectedbrushmodel, tmp.faces, tmp.numfaces, tmp.contents, TRUE);
				bt_points = 0;
				mousetool = BT_NONE;
			}
		}
		else
			mousedown = FALSE;
	}
	else if (brushtool == BT_SLICE)
	{
		editor_brushes_drawselected();
		if (bt_points)
		{
			for (int sb = 0; sb < selectedbrushcount; sb++)
			{
				int model = selectedbrushes[sb].model;
				int brush = selectedbrushes[sb].id;
		
				tmp.numfaces = brush_get(model, brush, tmp.faces, tmp.faces.length, &tmp.contents);
				tmp.numcp = 0;
				tmp.faces[tmp.numfaces].planenormal = normalize(crossproduct(bt_point[2] - bt_point[0], bt_point[1] - bt_point[0]));
				tmp.faces[tmp.numfaces].planedist = bt_point[0] * tmp.faces[tmp.numfaces].planenormal;

				//draw it wireframe
				DrawQCBrushWireframe(tmp.faces, tmp.numfaces+1, "chop", '1 0 0', 1);

				//flip the split
				tmp.faces[tmp.numfaces].planenormal *= -1;
				tmp.faces[tmp.numfaces].planedist *= -1;

				//draw the other side wireframe
				DrawQCBrushWireframe(tmp.faces, tmp.numfaces+1, "chop", '0 1 0', 1);
			}
		}
	}
	else if ((mousetool == BT_PUSHFACE || mousetool == BT_MOVETEXTURE || mousetool == BT_MOVE || mousetool == BT_CLONEDISPLACE || mousetool == BT_ROTATE) && bt_points)
	{
		int oldselectedbrushcount = selectedbrushcount;
		selbrush_t *oldselectedbrushes = memalloc(sizeof(selbrush_t)*selectedbrushcount);
		memcpy(oldselectedbrushes, selectedbrushes, selectedbrushcount*sizeof(selbrush_t));
		for (int sb = 0; sb < oldselectedbrushcount; sb++)
		{
			int model = oldselectedbrushes[sb].model;
			int brush = oldselectedbrushes[sb].id;
			__uint64 facemask = oldselectedbrushes[sb].facemask;
			if (!brush)
				continue;

			tmp.numfaces = brush_get(model, brush, tmp.faces, tmp.faces.length, &tmp.contents);
			tmp.numcp = patch_getcp(model, brush, tmp.cp, tmp.maxcp, &tmp.patchinfo);

			if (mousetool == BT_PUSHFACE)
			{
				for (facenum = 0; facenum < tmp.numfaces; facenum++)
				{
					if (facemask & (1ul<<facenum))
						tmp.faces[facenum].planedist += tmp.faces[facenum].planenormal * displace;
				}
			}
			else if (mousetool == BT_MOVETEXTURE)
			{
				for (facenum = 0; facenum < tmp.numfaces; facenum++)
				{
					if (facemask & (1ul<<facenum))
					{
						tmp.faces[facenum].sbias -= tmp.faces[facenum].sdir * displace;
						tmp.faces[facenum].tbias -= tmp.faces[facenum].tdir * displace;
					}
				}
			}
			else if (mousetool == BT_MOVE || mousetool == BT_CLONEDISPLACE)
				brushface_translate(displace);		
			else if (mousetool == BT_ROTATE)
			{
				//find the brush's middle (based on its bbox)
				brush_getfacepoints(model, brush, 0, &mid, 1);

				makevectors(col);

				//move it so its pivot is at the origin
				brushface_translate(-mid);
				//rotate it (by v_forward etc)
				brushface_rotate();
				//reposition it around its pivot again
				brushface_translate(mid);
			}
			else
				continue;

			if (0)//mousetool == BT_MOVETEXTURE)
			{
				DrawQCBrushTextured(tmp.faces, tmp.numfaces, '1 1 1', 0.3);	//faded to give an idea, without stopping you from seeing what you're aligning it to.

				/*points = brush_calcfacepoints(model, tmp.faces, tmp.numfaces, facepoints, MAX_FACEPOINTS);
				if (points)
				{
					//this is unfortunate. the built in shaders expect to use lightmaps. we don't have those.
					//because lightmaps are special things, we end up in a real mess. so lets just make sure there's a shader now, because we can.
					shaderforname(tmp.faces[face-1].shadername,
						sprintf("{"
							"{\n"
								"map \"%s.lmp\"\n"
								"rgbgen vertex\n"
								"alphagen vertex\n"
							"}\n"
						"}", tmp.faces[face-1].shadername));

					col = '0.7 0.7 0.7'; //fullbright is typically TOO bright. overbrights? meh!
					vector sz = drawgetimagesize(tmp.faces[face-1].shadername);
					R_BeginPolygon(tmp.faces[face-1].shadername);
					for (point = 0; point < points; point++)
						R_PolygonVertex(facepoints[point] + tmp.faces[face-1].planenormal*0.1, [(facepoints[point] * tmp.faces[face-1].sdir + tmp.faces[face-1].sbias)/sz_x, (facepoints[point] * tmp.faces[face-1].tdir + tmp.faces[face-1].tbias)/sz_y], col, 1);
					R_EndPolygon();
				}*/
			}
			else
			{
				//draw it wireframe WITH depth testing
				if (tmp.numcp)
					DrawQCPatchWireframe(tmp.cp, tmp.patchinfo, "terrainedit", '0 0 1', 1);
				else
				{
					//draw it wireframe
					for(facenum = 0; facenum < tmp.numfaces;)
					{
						points = brush_calcfacepoints(++facenum, tmp.faces, tmp.numfaces, facepoints, MAX_FACEPOINTS);
						if (!points)
							continue;	//should probably warn somehow about this
						//should we use two colour channels? one depth one not?
						if (facemask & (1ul<<facenum))
							col = '1 0 0';
						else
							col = '0 0.5 0';
						R_BeginPolygon("chop");
						R_PolygonVertex(facepoints[0], '0 0', col, 1);
						for (point = 0; point < points-1; )
						{
							point++;
							R_PolygonVertex(facepoints[point], '0 0', col, 1);
							R_EndPolygon();
							R_PolygonVertex(facepoints[point], '0 0', col, 1);
						}
						R_PolygonVertex(facepoints[0], '0 0', col, 1);
						R_EndPolygon();
					}

					DrawQCBrushTextured(tmp.faces, tmp.numfaces, '1 1 1', 0.3);	//faded to give an idea, without stopping you from seeing what you're aligning it to.
					DrawQCBrushWireframe(tmp.faces, tmp.numfaces, "terrainedit", '0 0 1', 1);
				}
			}

			DebrushifyLite();
			DrawAxisExtensions(vertedit.p, vertedit.numverts, "terrainedit", '0 0 0.3', 1);

			if (!mousedown)
			{
				if (mousetool == BT_CLONEDISPLACE)	//doesn't affect the original brush.
				{
					if (displace*displace > 1)
						brush_history_create(model, tmp.faces, tmp.numfaces, tmp.contents, TRUE);
				}
				else if (tmp.numcp)
					patch_history_edit(model, brush, tmp.cp, tmp.patchinfo);
				else
					brush_history_edit(model, brush, tmp.faces, tmp.numfaces, tmp.contents);
				bt_points = 0;
				mousetool = BT_NONE;
			}
		}
		memfree(oldselectedbrushes);
	}
	else if (selectedbrushcount)
	{
		/*if (brushtool == BT_PUSHFACE)
		{	//selected face (not brush) follows cursor when we're not actively dragging a face
			for(facenum = 0;;)
			{
				points = brush_getfacepoints(selectedbrushmodel, selectedbrush, ++facenum, facepoints, MAX_FACEPOINTS);
				if (!points)
					break;	//end of face list, I guess

				mid = 0;
				for (point = 0; point < points; point++)
					mid += facepoints[point];	//FIXME: find the centroid for greater accuracy
				mid = project(mid * (1.0/points));
				dist = (mousepos_x - mid_x) * (mousepos_x - mid_x) + (mousepos_y - mid_y) * (mousepos_y - mid_y);
				if (facenum == 1 || dist < bestdist)
				{
					bestdist = dist;
					selectedbrushface = facenum;
				}	
			}
		}*/
	
		editor_brushes_drawselected();
	}

//	editor_drawbbox(selectedbrush);

	vector t = mousefar;
	vector o = mousenear;
	if (vlen(o - t) > 8192 && !autocvar_ca_brush_view)
		t = o + normalize(t)*8192;
	*world_hitcontentsmaski = ~0;	//I want to be able to select water...
	traceline(o, t, TRUE, world);
	*world_hitcontentsmaski = 0;	//don't break stuff

#if 0
	//find all the brushes within 32qu of the mouse cursor's impact point
	//you can tweak this to find the nearest brush vertex efficiently (or snap it to a grid or so).
	//you can then slice through a brush by generating a plane between three points found this way and inserting it into the brush, clipping away the extra part.
	//(remember, the engine will automatically discard any degenerate planes)
	col = '0 0 1';

	//generate the volume
	for (facenum = 0; facenum < 6; facenum++)
		dists[facenum] = (trace_endpos * axis[facenum]) + 32;

	int brushnum, numbrushes = brush_findinvolume(selectedbrushmodel, axis, dists, dists.length, &brushlist[0], __NULL__, brushlist.length);
	for (brushnum = 0; brushnum < numbrushes; brushnum++)
	{
		for (facenum = 0; ; )
		{
			points = brush_getfacepoints(selectedbrushmodel, brushlist[brushnum], ++facenum, facepoints, MAX_FACEPOINTS);
			if (!points)
				break;	//end of face list, I guess

			R_BeginPolygon("terrainedit");
			for (point = 0; point < points; point++)
				R_PolygonVertex(facepoints[point], '0 0', col, 1);
			R_EndPolygon();
		}
	}
#endif

	//draw a line/triangle to show the selection.
	int showpoints = bt_points;
	if (brushtool == BT_SLICE && bt_points)
	{
//		bt_point[showpoints++] = brush_snappoint(trace_endpos);
		showpoints = 3;
	}
	if (brushtool == BT_CREATEBRUSH || brushtool == BT_CREATEPATCH)
		bt_point[showpoints++] = brush_snappoint(trace_endpos);

	//FIXME: if slicing, draw the intersection

	if (showpoints > 1)
	{
		col = '0 0 1';

		R_BeginPolygon("chop");
		for (point = 0; point < showpoints; point++)
			R_PolygonVertex(bt_point[point], '0 0', col, 1);
		R_EndPolygon();
	}

	//FIXME: if creating, draw a wireframe brush.
};

void(vector mousepos) editor_brushes_overlay =
{
	int point;
	vector mid;
	float dist, bestdist;
	if (vertedit.numidx)
	{
		if (!mousedown)
		{
			for(vertedit.selectedvert1 = -1, vertedit.selectedvert2 = -1, point = 0, bestdist = 16*16; point < vertedit.numverts; point++)
			{
				mid = project(vertedit.p[point]);
				dist = (mousepos_x - mid_x) * (mousepos_x - mid_x) + (mousepos_y - mid_y) * (mousepos_y - mid_y);
				if (dist < bestdist && mid_z > 0)
				{
					bestdist = dist;
					vertedit.selectedvert1 = point;
				}	
			}
			for(point = 0; point < vertedit.numidx; point+=3)
			{
				mid = project(0.5*(vertedit.p[vertedit.i[point+0]] + vertedit.p[vertedit.i[point+1]]));
				dist = (mousepos_x - mid_x) * (mousepos_x - mid_x) + (mousepos_y - mid_y) * (mousepos_y - mid_y);
				if (dist < bestdist && mid_z > 0)
				{
					bestdist = dist;
					vertedit.selectedvert1 = vertedit.i[point+0];
					vertedit.selectedvert2 = vertedit.i[point+1];
				}
				
				mid = project(0.5*(vertedit.p[vertedit.i[point+1]] + vertedit.p[vertedit.i[point+2]]));
				dist = (mousepos_x - mid_x) * (mousepos_x - mid_x) + (mousepos_y - mid_y) * (mousepos_y - mid_y);
				if (dist < bestdist && mid_z > 0)
				{
					bestdist = dist;
					vertedit.selectedvert1 = vertedit.i[point+1];
					vertedit.selectedvert2 = vertedit.i[point+2];
				}
				
				mid = project(0.5*(vertedit.p[vertedit.i[point+2]] + vertedit.p[vertedit.i[point+0]]));
				dist = (mousepos_x - mid_x) * (mousepos_x - mid_x) + (mousepos_y - mid_y) * (mousepos_y - mid_y);
				if (dist < bestdist && mid_z > 0)
				{
					bestdist = dist;
					vertedit.selectedvert1 = vertedit.i[point+2];
					vertedit.selectedvert2 = vertedit.i[point+0];
				}
			}
		}
		if (vertedit.selectedvert1 != -1)
		{
			mid = project(vertedit.p[vertedit.selectedvert1]);
			if (mid_z >= 0)
				drawfill([mid_x,mid_y] - '2 2', [4,4], [0.2,0.2,1], 1, 0);
		}
		if (vertedit.selectedvert2 != -1)
		{
			mid = project(vertedit.p[vertedit.selectedvert2]);
			if (mid_z >= 0)
				drawfill([mid_x,mid_y] - '2 2', [4,4], [0.2,0.2,1], 1, 0);
		}
		
		drawrawstring('0 32 0', "Vertex Editor", '8 8 0', '1 1 1', 1);
		return;
	}

	switch(brushtool)
	{
	case BT_CLONEDISPLACE:
		drawrawstring('0 32 0', "Clone", '8 8 0', '1 1 1', 1);
		break;
	case BT_CREATEDRAG:
		drawrawstring('0 32 0', "Drag+Create", '8 8 0', '1 1 1', 1);
		break;
	case BT_CREATEPATCH:
		if (autocvar_ca_explicitpatch)
			drawrawstring('0 32 0', sprintf("Create Explicit %g*%g Patch", autocvar_ca_cpwidth, autocvar_ca_cpheight), '8 8 0', '1 1 1', 1);
		else
			drawrawstring('0 32 0', sprintf("Create Auto %g*%g Patch", autocvar_ca_cpwidth, autocvar_ca_cpheight), '8 8 0', '1 1 1', 1);
		break;
	case BT_CREATEBRUSH:
		drawrawstring('0 32 0', "Paint+Create", '8 8 0', '1 1 1', 1);
		break;
	case BT_SLICE:
		drawrawstring('0 32 0', "Slice Tool", '8 8 0', '1 1 1', 1);
		if (!selectedbrushcount)
			brushtool = BT_NONE;
		break;
	case BT_PUSHFACE:
		drawrawstring('0 32 0', "Push Face", '8 8 0', '1 1 1', 1);
		break;
	case BT_MOVE:
		drawrawstring('0 32 0', "Move Brush", '8 8 0', '1 1 1', 1);
		break;
	case BT_ROTATE:
		drawrawstring('0 32 0', "Rotate Brush", '8 8 0', '1 1 1', 1);
		break;
	case BT_MOVETEXTURE:
		drawrawstring('0 32 0', "Move Texture", '8 8 0', '1 1 1', 1);
		break;
	case BT_VERTEXEDIT:
		drawrawstring('0 32 0', "Vertex Editor", '8 8 0', '1 1 1', 1);
		break;
	}
};

#define brusheditormodes
//#append brusheditormodes brusheditormode("move", BT_MOVE)
#append brusheditormodes brusheditormode("clone", BT_CLONEDISPLACE)
#append brusheditormodes brusheditormode("draw", BT_CREATEDRAG)
#append brusheditormodes brusheditormode("rotate", BT_ROTATE)
#append brusheditormodes brusheditormode("pushface", BT_PUSHFACE)
#append brusheditormodes brusheditormode("scrolltex", BT_MOVETEXTURE)
#append brusheditormodes brusheditormode("vertex", BT_VERTEXEDIT)

void() editor_brushes_registercommands =
{
	tmp.maxcp = 64*64;
	tmp.cp = memalloc(sizeof(*tmp.cp)*tmp.maxcp);
#define brusheditormode(n,v) registercommand("+brushedit_"n);registercommand("-brushedit_"n);
brusheditormodes
#undef brusheditormode
	registercommand("brushedit_undo");
	registercommand("brushedit_redo");
	registercommand("brushedit_delete");
	registercommand("brushedit_create");
	registercommand("brushedit_createpatch");
	registercommand("brushedit_slice");
	registercommand("brushedit_matchface");
	registercommand("brushedit_resettexcoords");
	registercommand("brushedit_settexture");
	registercommand("brushedit_scrolltex");
	registercommand("brushedit_subtract");
	registercommand("brushedit_binds");
	registercommand("brushedit_binds_default");
	registercommand("+brushedit_nogrid");
	registercommand("-brushedit_nogrid");
};

float() editor_brushes_command =
{
	switch(argv(0))
	{
#define brusheditormode(n,v) case "+brushedit_"n: brushtool = v; bt_points = 0; break;  case "-brushedit_"n: if (brushtool == v) brushtool = BT_NONE; break;
brusheditormodes
#undef brusheditormode

	case "brushedit_undo":
		brush_undo();
		break;
	case "brushedit_redo":
		brush_undo();
		break;
	case "brushedit_create":
		brushtool = BT_CREATEBRUSH;
		bt_points = 0;
		break;
	case "brushedit_createpatch":
		brushtool = BT_CREATEPATCH;
		bt_points = 0;
		break;
	case "brushedit_slice":
		brushtool = BT_SLICE;
		bt_points = 0;
		break;
	case "brushedit_scrolltex":
		brushtool = BT_MOVETEXTURE;
		bt_points = 0;
		break;
		
	case "+brushedit_nogrid": nogrid = TRUE; break;
	case "-brushedit_nogrid": nogrid = FALSE; break;

/*	case "brushedit_matchface":
		brushtool = BT_NONE;
		bt_points = 0;
		
		
		{
			vector t = mousefar;
			vector o = mousenear;
			int i;
			if (vlen(o - t) > 8192 && !autocvar_ca_brush_view)
				t = o + normalize(t)*8192;
		
			traceline(o, t, TRUE, world);
			
			i = mergebrushes(selectedbrushmodel, selectedbrush, trace_brush_id, selectedbrushface, trace_brush_faceid);
			if (i)
			{
				brush_selected(selectedbrushmodel, selectedbrush, -1, FALSE);
				selectedbrush = i;
				selectedbrushface = 0;
				brush_selected(selectedbrushmodel, selectedbrush, selectedbrushface, TRUE);
			}
		}
		break;
*/
	case "brushedit_resettexcoords":
		brushedit_resettextures();
		break;
	case "brushedit_settexture":
		brushedit_settexture(argv(1));
		break;
	case "brushedit_subtract":
		brushedit_subtract();
		break;
	case "brushedit_delete":
		while(selectedbrushcount)
		{
			brush_history_delete(selectedbrushes[0].model, selectedbrushes[0].id);
		}
		break;
	case "brushedit_binds_default":
		localcmd("echo Setting default brusheditor bindings\n");
		localcmd("bind ctrl+z		brushedit_undo\n");
		localcmd("bind ctrl+y		brushedit_redo\n");
		localcmd("bind ctrl+i		brushedit_create\n");
		localcmd("bind ctrl+p		brushedit_createpatch\n");
		localcmd("bind ctrl+s		brushedit_slice\n");
		localcmd("bind ctrl+m		brushedit_matchface\n");
		localcmd("bind ctrl+del		brushedit_delete\n");
		localcmd("bind ctrl+backspace	brushedit_subtract\n");
		localcmd("bind ctrl+c		+brushedit_clone\n");
		localcmd("bind ctrl+d		+brushedit_draw\n");
		localcmd("bind ctrl+r		+brushedit_rotate\n");
		localcmd("bind ctrl+f		+brushedit_pushface\n");
		localcmd("bind ctrl+t		brushedit_scrolltex\n");
		localcmd("bind ctrl+v		+brushedit_vertex\n");
		break;
	case "brushedit_binds":
		localcmd("conmenu \"\"\n");
		float foo = 0;
		localcmd(sprintf("menutext 0 %g \"Set Default Bindings\" \"brushedit_binds_default\"\n", foo+=8));
		foo+=8;
		localcmd(sprintf("menubind 0 %g \"brushedit_create\" \"Creation Tool\"\n", foo+=8));
		localcmd(sprintf("menubind 0 %g \"brushedit_slice\" \"Slice Tool\"\n", foo+=8));
		localcmd(sprintf("menubind 0 %g \"brushedit_delete\" \"Delete\"\n", foo+=8));
		localcmd(sprintf("menubind 0 %g \"brushedit_subtract\" \"Subtract\"\n", foo+=8));
		localcmd(sprintf("menubind 0 %g \"brushedit_undo\" \"Undo\"\n", foo+=8));
		localcmd(sprintf("menubind 0 %g \"brushedit_redo\" \"Redo\"\n", foo+=8));
		localcmd(sprintf("menubind 0 %g \"brushedit_nogrid\" \"Disable Grid\"\n", foo+=8));
		
#define brusheditormode(n,v) localcmd(sprintf("menubind 0 %g \"+brushedit_"n"\" \""n"\"\n", foo+=8));
brusheditormodes
#undef brusheditormode
		break;
	default:
		return FALSE;
	}

	//just in case.
	cvar_set("ca_show", "1");
	cvar_set("ca_editormode", ftos(MODE_BRUSHEDIT));
	return TRUE;
};

void(entity e) editor_brush_set_entity
{
//	faesf = e;
};

float(float key, float unic, vector mousepos) editor_brushes_key =
{
	brushface_t *fa;
	vector t = mousefar;
	vector o = mousenear;
	int i;

	if (vlen(o - t) > 8192 && !autocvar_ca_brush_view)
		t = o + normalize(t)*8192;
	if (key == K_ESCAPE)
	{
		bt_points = 0;
		vertedit.numidx = 0;
		vertedit.numverts = 0;
		if (brushtool)
			brushtool = BT_NONE;
		else
			brush_deselectall();
		return TRUE;
	}
	if (key == K_MOUSE1)
	{
		if (vertedit.numidx)
		{
			if (vertedit.selectedvert1 != -1)
			{
				mousedown |= 1;
				mousetool = BT_VERTEXEDIT;
				if (vertedit.selectedvert2 != -1)
				{
					bt_point[0] = 0.5 * (vertedit.p[vertedit.selectedvert1] + vertedit.p[vertedit.selectedvert2]);
					bt_point[1] = vertedit.p[vertedit.selectedvert1] - bt_point[0];
					bt_point[2] = vertedit.p[vertedit.selectedvert2] - bt_point[0];
				}
				else
					bt_point[0] = vertedit.p[vertedit.selectedvert1];
					bt_point[1] = vertedit.p[vertedit.selectedvert1] - bt_point[0];
				bt_points = 1;
				bt_displace = '0 0 0';
			}
			return TRUE;
		}
		
		*world_hitcontentsmaski = ~0;	//I want to be able to select water...
		traceline(o, t, TRUE, world);
		*world_hitcontentsmaski = 0;	//don't break stuff.
		float tracemodel = trace_ent.modelindex;

		/*if (brushtool == BT_CREATEDRAG || (brushtool == BT_PUSHFACE && selectedbrushface))
		{
			trace_brush_faceid = selectedbrushface;
			trace_brush_id = selectedbrush;
			tracemodel = selectedbrushmodel;
		}
		else*/ if (brushtool != BT_PUSHFACE && brushtool != BT_MOVETEXTURE)
			trace_brush_faceid = 0;
			
		if (brushtool == BT_SLICE || brushtool == BT_CREATEBRUSH || brushtool == BT_CREATEPATCH)
		{
			if (brushtool == BT_SLICE && bt_points > 2)
				bt_points = 2;
			if (brushtool == BT_CREATEPATCH && bt_points > 3)
				bt_points = 3;

//FIXME: BT_CREATE is planar. so keep the points planar too.
//FIXME: need a way to move points already placed
//FIXME: create needs to ensure verts are clockwise and convex.
			if (bt_points < bt_point.length)
			{
				bt_point[bt_points] = brush_snappoint(trace_endpos);
				bt_points++;
			}

			if (brushtool == BT_SLICE && bt_points == 1)
			{	//slice makes assumptions about the brush, so that you don't have to move to the other side of it first.
				//it ALWAYS draws 3 points if any are defined, so make sure 2+3 have valid locations once point 1 is defined.
				int majoraxis;
				traceline(o, t, TRUE, world);

				static vector bbox[2];
				brush_getfacepoints(tracemodel, trace_brush_id, 0, bbox, bbox.length);
	
				t[0] = fabs(trace_plane_normal[0]);
				t[1] = fabs(trace_plane_normal[1]);
				t[2] = fabs(trace_plane_normal[2]);
				if (t[2] > t[0] && t[2] > t[1]) 
					majoraxis = 2;
				else if (t[1] > t[0])
					majoraxis = 1;
				else
					majoraxis = 0;

				bt_point[1] = bt_point[0];
				bt_point[1][majoraxis] = bbox[trace_plane_normal[majoraxis]<0][majoraxis];
				majoraxis = !majoraxis;
				bt_point[2] = bt_point[0];
				bt_point[2][majoraxis] = bbox[trace_plane_normal[majoraxis]<0][majoraxis];
			}
		}
		//FIXME: selecting a brush by face should select either the front or the back. ideally depending on which one is closest to its respective face center, I suppose.
		else
		{
			if (shiftdown)
			{	//simple selection toggle of the clicked brush
				if (!tracemodel || !trace_brush_id)
				{
				}
				else if (brushface_isselected(tracemodel, trace_brush_id, trace_brush_faceid))
					brushface_deselect(tracemodel, trace_brush_id, trace_brush_faceid);
				else
					brushface_select(tracemodel, trace_brush_id, trace_brush_faceid);
			}
			else
			{	//deselect everything but the targetted brush.
				if (!tracemodel || !trace_brush_id)
					brush_deselectall();
				else if (!brush_isselected(tracemodel, trace_brush_id))
				{
					brush_deselectall();
					brushface_select(tracemodel, trace_brush_id, trace_brush_faceid);
				}
				else
				{	//its already selected, start doing whatever the current mouse action is meant to be
					mousedown |= 1;
					mousetool = brushtool;
					vertedit.selectedvert1 = -1;
					vertedit.selectedvert2 = -1;
					bt_point[0] = brush_snappoint(trace_endpos);
					bt_points = 1;
					bt_displace = '0 0 0';
				}
				
				if (brushtool == BT_VERTEXEDIT && !vertedit.numidx && trace_brush_id)
				{
					mousedown = FALSE;
					if (brush_isselected(tracemodel, trace_brush_id))
						Debrushify(&vertedit, tracemodel, trace_brush_id);
				}
			}
/*
			
			 if (trace_brush_id != selectedbrush || selectedbrushface != trace_brush_faceid || selectedbrushmodel != tracemodel)
			{
				brush_selected(selectedbrushmodel, selectedbrush, -1, FALSE);
				if (!shiftdown)
				{
					if (!brush_isselected(tracemodel, trace_brush_id))
						brush_deselectall();
				}
				else if (brush_deselect(tracemodel, trace_brush_id))
					return TRUE;
				brush_select(tracemodel, trace_brush_id);
				selectedbrush = trace_brush_id;
				selectedbrushface = trace_brush_faceid;
				selectedbrushmodel = tracemodel;
				if (trace_brush_faceid)
					brush_selected(selectedbrushmodel, selectedbrush, selectedbrushface, TRUE);
				
				vertedit.numidx = 0;
				vertedit.numverts = 0;
				
				if (selectedbrushface && (brushtool == BT_PUSHFACE || brushtool == BT_MOVETEXTURE))
				{
					mousedown |= 1;
					mousetool = brushtool;
					vertedit.selectedvert1 = -1;
					vertedit.selectedvert2 = -1;
					bt_point[0] = brush_snappoint(trace_endpos);
					bt_points = 1;
					bt_displace = '0 0 0';
				}
			}
			else if (selectedbrush == trace_brush_id && selectedbrushface == trace_brush_faceid && selectedbrushmodel == tracemodel)
			{
				mousedown = TRUE;
				mousetool = brushtool;
				vertedit.selectedvert1 = -1;
				vertedit.selectedvert2 = -1;
				bt_point[0] = brush_snappoint(trace_endpos);
				bt_points = 1;
				bt_displace = '0 0 0';
			}
*/
		}
		return TRUE;
	}

	if (key == K_ENTER)
	{
		if (vertedit.numidx)
		{
			Rebrushify(&vertedit, FALSE);
			mousetool = BT_NONE;
			bt_points = 0;
			vertedit.numidx = 0;
			vertedit.numverts = 0;
			mousedown = FALSE;
			return TRUE;
		}
		else if (brushtool == BT_CREATEPATCH)
		{
			if (bt_points == 4)
			{
				int x,y;
				patchvert_t *vert;
				patchvert_t *cp;
				patchinfo_t patchinfo = {autocvar_ca_newbrushtexture, CONTENTBIT_SOLID, max(2,autocvar_ca_cpwidth), max(2,autocvar_ca_cpheight), 0, 0, '0 0 0'};
				tmp.numfaces = 0;
				tmp.numcp = 0;
				bt_points = 0;

				if (!autocvar_ca_explicitpatch)
				{	//must be odd sizes. cos grr.
					patchinfo.cpwidth|=1;
					patchinfo.cpheight|=1;
					patchinfo.tesswidth = patchinfo.tessheight = -1; //proper q3 patch, engine decides according to smoothness (which means nothing quite fits).
				}
				if (patchinfo.cpwidth*patchinfo.cpheight>64*64)
				{
					cprint(sprintf("%i*%i patch dimensions too large", patchinfo.cpwidth, patchinfo.cpheight));
					return TRUE;
				}

				//populate the CPs, averaging from the 4 specified coords.
				cp = vert = memalloc(sizeof(*vert) * patchinfo.cpwidth*patchinfo.cpheight);
				for (y = 0; y < patchinfo.cpheight; y++)
				for (x = 0; x < patchinfo.cpwidth; x++)
				{
					cp->s = x/(patchinfo.cpwidth-1);
					cp->t = y/(patchinfo.cpheight-1);
					vector top = bt_point[0]+(bt_point[1]-bt_point[0])*cp->s;
					vector bot = bt_point[3]+(bt_point[2]-bt_point[3])*cp->s;
					cp->xyz = top+(bot-top)*cp->t;
					cp->rgb = '1 1 1'; cp->a = 1;
					cp++;
				}

				brush_deselectall();
				patch_history_create(selectedbrushmodel, vert, patchinfo, TRUE);
				memfree(vert);
			}
			return TRUE;
		}
		else if (brushtool == BT_CREATEBRUSH)
		{
			if (bt_points >= 3)
			{
				tmp.numfaces = BrushFromPoints(tmp.faces, tmp.faces.length, bt_point, bt_points, autocvar_ca_newbrushheight);
				tmp.numcp = 0;
				bt_points = 0;
				brush_deselectall();
				brush_history_create(selectedbrushmodel, tmp.faces, tmp.numfaces, CONTENTBIT_SOLID, TRUE);
			}
			return TRUE;
		}
		else if (brushtool == BT_SLICE)
		{	//save off the selected brushes so we can butcher the list, and select ONLY the sliced brushes on one side of the slice plane.
			int oldselectedbrushcount = selectedbrushcount;
			selbrush_t *oldselectedbrushes = memalloc(sizeof(selbrush_t)*selectedbrushcount);
			memcpy(oldselectedbrushes, selectedbrushes, selectedbrushcount*sizeof(selbrush_t));
			brush_deselectall();
			for (int sb = 0; sb < oldselectedbrushcount; sb++)
			{
				int model = oldselectedbrushes[sb].model;
				int brush = oldselectedbrushes[sb].id;
				//get the current faces
				tmp.numfaces = brush_get(model, brush, tmp.faces, tmp.faces.length-1, &tmp.contents);
				if (!tmp.numfaces)
					continue;	//can't slice patches

				//generate a new face plane
				fa = &tmp.faces[tmp.numfaces];
				fa->planenormal = normalize(crossproduct(bt_point[2] - bt_point[0], bt_point[1] - bt_point[0]));
				fa->planedist = bt_point[0] * fa->planenormal;
				fa->shadername = tmp.faces[0].shadername;	//find a neighbour?

				//make sure its not coplanar with some other surface. such slices are silly
				for (i = 0; i < tmp.numfaces; i++)
				{
					if (tmp.faces[i].planenormal == fa->planenormal && tmp.faces[i].planedist == fa->planedist)
						break;
				}
				if (i < tmp.numfaces)
					continue;
				tmp.numfaces++;

				reset_texturecoords(fa);		

				//delete the old one and insert the new
				brush_history_edit(model, brush, tmp.faces, tmp.numfaces, tmp.contents);

				//and insert another new one too, because inserting a plane like this generates two fragments and I'm too lazy to work out which is the front and which is the back.
				fa->planenormal *= -1;
				fa->planedist *= -1;
				brush_history_create(model, tmp.faces, tmp.numfaces, tmp.contents, TRUE);
			}
			memfree(oldselectedbrushes);
			bt_points = 0;
			return TRUE;
		}
		else return FALSE;
	}

	return FALSE;
};
