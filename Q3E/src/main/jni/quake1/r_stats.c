#include "quakedef.h"
#include "r_stats.h"

cvar_t r_speeds_graph = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph", "0", "display a graph of renderer statistics "};
cvar_t r_speeds_graph_filter[8] =
{
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_r", "timedelta", "Red - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_g", "batch_batches", "Green - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_b", "batch_triangles", "Blue - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_y", "fast_triangles", "Yellow - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_c", "copytriangles_triangles", "Cyan - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_m", "dynamic_triangles", "Magenta - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_w", "animcache_shade_vertices", "White - display the specified renderer statistic"},
	{CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_filter_o", "animcache_shape_vertices", "Orange - display the specified renderer statistic"},
};
cvar_t r_speeds_graph_length = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_length", "1024", "number of frames in statistics graph, can be from 4 to 8192"};
cvar_t r_speeds_graph_seconds = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_seconds", "2", "number of seconds in graph, can be from 0.1 to 120"};
cvar_t r_speeds_graph_x = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_x", "0", "position of graph"};
cvar_t r_speeds_graph_y = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_y", "0", "position of graph"};
cvar_t r_speeds_graph_width = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_width", "256", "size of graph"};
cvar_t r_speeds_graph_height = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_height", "128", "size of graph"};
cvar_t r_speeds_graph_maxtimedelta = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_maxtimedelta", "16667", "maximum timedelta to display in the graph (this value will be the top line)"};
cvar_t r_speeds_graph_maxdefault = {CF_CLIENT | CF_ARCHIVE, "r_speeds_graph_maxdefault", "100", "if the minimum and maximum observed values are closer than this, use this value as the graph range (keeps small numbers from being big graphs)"};


const char *r_stat_name[r_stat_count] =
{
	"timedelta",
	"quality",
	"renders",
	"entities",
	"entities_surfaces",
	"entities_triangles",
	"world_leafs",
	"world_portals",
	"world_surfaces",
	"world_triangles",
	"lightmapupdates",
	"lightmapupdatepixels",
	"particles",
	"drawndecals",
	"totaldecals",
	"draws",
	"draws_vertices",
	"draws_elements",
	"lights",
	"lights_clears",
	"lights_scissored",
	"lights_lighttriangles",
	"lights_shadowtriangles",
	"lights_dynamicshadowtriangles",
	"bouncegrid_lights",
	"bouncegrid_particles",
	"bouncegrid_traces",
	"bouncegrid_hits",
	"bouncegrid_splats",
	"bouncegrid_bounces",
	"photoncache_animated",
	"photoncache_cached",
	"photoncache_traced",
	"bloom",
	"bloom_copypixels",
	"bloom_drawpixels",
	"rendertargets_used",
	"rendertargets_pixels",
	"indexbufferuploadcount",
	"indexbufferuploadsize",
	"vertexbufferuploadcount",
	"vertexbufferuploadsize",
	"framedatacurrent",
	"framedatasize",
	"bufferdatacurrent_vertex", // R_BUFFERDATA_ types are added to this index
	"bufferdatacurrent_index16",
	"bufferdatacurrent_index32",
	"bufferdatacurrent_uniform",
	"bufferdatasize_vertex", // R_BUFFERDATA_ types are added to this index
	"bufferdatasize_index16",
	"bufferdatasize_index32",
	"bufferdatasize_uniform",
	"animcache_skeletal_count",
	"animcache_skeletal_bones",
	"animcache_skeletal_maxbones",
	"animcache_shade_count",
	"animcache_shade_vertices",
	"animcache_shade_maxvertices",
	"animcache_shape_count",
	"animcache_shape_vertices",
	"animcache_shape_maxvertices",
	"batch_batches",
	"batch_withgaps",
	"batch_surfaces",
	"batch_vertices",
	"batch_triangles",
	"fast_batches",
	"fast_surfaces",
	"fast_vertices",
	"fast_triangles",
	"copytriangles_batches",
	"copytriangles_surfaces",
	"copytriangles_vertices",
	"copytriangles_triangles",
	"dynamic_batches",
	"dynamic_surfaces",
	"dynamic_vertices",
	"dynamic_triangles",
	"dynamicskeletal_batches",
	"dynamicskeletal_surfaces",
	"dynamicskeletal_vertices",
	"dynamicskeletal_triangles",
	"dynamic_batches_because_cvar",
	"dynamic_surfaces_because_cvar",
	"dynamic_vertices_because_cvar",
	"dynamic_triangles_because_cvar",
	"dynamic_batches_because_lightmapvertex",
	"dynamic_surfaces_because_lightmapvertex",
	"dynamic_vertices_because_lightmapvertex",
	"dynamic_triangles_because_lightmapvertex",
	"dynamic_batches_because_deformvertexes_autosprite",
	"dynamic_surfaces_because_deformvertexes_autosprite",
	"dynamic_vertices_because_deformvertexes_autosprite",
	"dynamic_triangles_because_deformvertexes_autosprite",
	"dynamic_batches_because_deformvertexes_autosprite2",
	"dynamic_surfaces_because_deformvertexes_autosprite2",
	"dynamic_vertices_because_deformvertexes_autosprite2",
	"dynamic_triangles_because_deformvertexes_autosprite2",
	"dynamic_batches_because_deformvertexes_normal",
	"dynamic_surfaces_because_deformvertexes_normal",
	"dynamic_vertices_because_deformvertexes_normal",
	"dynamic_triangles_because_deformvertexes_normal",
	"dynamic_batches_because_deformvertexes_wave",
	"dynamic_surfaces_because_deformvertexes_wave",
	"dynamic_vertices_because_deformvertexes_wave",
	"dynamic_triangles_because_deformvertexes_wave",
	"dynamic_batches_because_deformvertexes_bulge",
	"dynamic_surfaces_because_deformvertexes_bulge",
	"dynamic_vertices_because_deformvertexes_bulge",
	"dynamic_triangles_because_deformvertexes_bulge",
	"dynamic_batches_because_deformvertexes_move",
	"dynamic_surfaces_because_deformvertexes_move",
	"dynamic_vertices_because_deformvertexes_move",
	"dynamic_triangles_because_deformvertexes_move",
	"dynamic_batches_because_tcgen_lightmap",
	"dynamic_surfaces_because_tcgen_lightmap",
	"dynamic_vertices_because_tcgen_lightmap",
	"dynamic_triangles_because_tcgen_lightmap",
	"dynamic_batches_because_tcgen_vector",
	"dynamic_surfaces_because_tcgen_vector",
	"dynamic_vertices_because_tcgen_vector",
	"dynamic_triangles_because_tcgen_vector",
	"dynamic_batches_because_tcgen_environment",
	"dynamic_surfaces_because_tcgen_environment",
	"dynamic_vertices_because_tcgen_environment",
	"dynamic_triangles_because_tcgen_environment",
	"dynamic_batches_because_tcmod_turbulent",
	"dynamic_surfaces_because_tcmod_turbulent",
	"dynamic_vertices_because_tcmod_turbulent",
	"dynamic_triangles_because_tcmod_turbulent",
	"dynamic_batches_because_nogaps",
	"dynamic_surfaces_because_nogaps",
	"dynamic_vertices_because_nogaps",
	"dynamic_triangles_because_nogaps",
	"dynamic_batches_because_derived",
	"dynamic_surfaces_because_derived",
	"dynamic_vertices_because_derived",
	"dynamic_triangles_because_derived",
	"entitycache_count",
	"entitycache_surfaces",
	"entitycache_vertices",
	"entitycache_triangles",
	"entityanimate_count",
	"entityanimate_surfaces",
	"entityanimate_vertices",
	"entityanimate_triangles",
	"entityskeletal_count",
	"entityskeletal_surfaces",
	"entityskeletal_vertices",
	"entityskeletal_triangles",
	"entitystatic_count",
	"entitystatic_surfaces",
	"entitystatic_vertices",
	"entitystatic_triangles",
	"entitycustom_count",
	"entitycustom_surfaces",
	"entitycustom_vertices",
	"entitycustom_triangles",
};

char r_speeds_timestring[4096];
int speedstringcount, r_timereport_active;
double r_timereport_temp = 0, r_timereport_current = 0, r_timereport_start = 0;
int r_speeds_longestitem = 0;

void R_TimeReport(const char *desc)
{
	char tempbuf[256];
	int length;
	int t;

	if (r_speeds.integer < 2 || !r_timereport_active)
		return;

	CHECKGLERROR
	if (r_speeds.integer == 2)
		GL_Finish();
	CHECKGLERROR
	r_timereport_temp = r_timereport_current;
	r_timereport_current = Sys_DirtyTime();
	t = (int) ((r_timereport_current - r_timereport_temp) * 1000000.0 + 0.5);

	length = dpsnprintf(tempbuf, sizeof(tempbuf), "%8i %s", t, desc);
	if (length < 0)
		length = (int)sizeof(tempbuf) - 1;
	if (r_speeds_longestitem < length)
		r_speeds_longestitem = length;
	for (;length < r_speeds_longestitem;length++)
		tempbuf[length] = ' ';
	tempbuf[length] = 0;

	if (speedstringcount + length > (vid_conwidth.integer / 8))
	{
		dp_strlcat(r_speeds_timestring, "\n", sizeof(r_speeds_timestring));
		speedstringcount = 0;
	}
	dp_strlcat(r_speeds_timestring, tempbuf, sizeof(r_speeds_timestring));
	speedstringcount += length;
}

void R_TimeReport_BeginFrame(void)
{
	speedstringcount = 0;
	r_speeds_timestring[0] = 0;
	r_timereport_active = false;
	memset(&r_refdef.stats, 0, sizeof(r_refdef.stats));

	if (r_speeds.integer >= 2)
	{
		r_timereport_active = true;
		r_timereport_start = r_timereport_current = Sys_DirtyTime();
	}
}

static int R_CountLeafTriangles(const model_t *model, const mleaf_t *leaf)
{
	int i, triangles = 0;
	for (i = 0;i < leaf->numleafsurfaces;i++)
		triangles += model->data_surfaces[leaf->firstleafsurface[i]].num_triangles;
	return triangles;
}

#define R_SPEEDS_GRAPH_COLORS 8
#define R_SPEEDS_GRAPH_TEXTLENGTH 64
static float r_speeds_graph_colors[R_SPEEDS_GRAPH_COLORS][4] = {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 1, 1}, {1, 0, 1, 1}, {1, 1, 1, 1}, {1, 0.5f, 0, 1}};

extern float viewscalefpsadjusted;
void R_TimeReport_EndFrame(void)
{
	int j, lines;
	cl_locnode_t *loc;
	char string[1024+4096];
	mleaf_t *viewleaf;
	static double oldtime = 0;

	r_refdef.stats[r_stat_timedelta] = (int)((host.realtime - oldtime) * 1000000.0);
	oldtime = host.realtime;
	r_refdef.stats[r_stat_quality] = (int)(100 * r_refdef.view.quality);

	string[0] = 0;
	if (r_speeds.integer)
	{
		// put the location name in the r_speeds display as it greatly helps
		// when creating loc files
		loc = CL_Locs_FindNearest(cl.movement_origin);
		viewleaf = (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.PointInLeaf) ? r_refdef.scene.worldmodel->brush.PointInLeaf(r_refdef.scene.worldmodel, r_refdef.view.origin) : NULL;
		dpsnprintf(string, sizeof(string),
"%6ius time delta %s%s %.3f cl.time%2.4f brightness\n"
"%3i renders org:'%+8.2f %+8.2f %+8.2f' dir:'%+2.3f %+2.3f %+2.3f'\n"
"%5i viewleaf%5i cluster%3i area%4i brushes%4i surfaces(%7i triangles)\n"
"%7i surfaces%7i triangles %5i entities (%7i surfaces%7i triangles)\n"
"%5i leafs%5i portals%6i/%6i particles%6i/%6i decals %3i%% quality\n"
"%7i lightmap updates (%7i pixels)%8i/%8i framedata\n"
"%4i lights%4i clears%4i scissored%7i light%7i shadow%7i dynamic\n"
"bouncegrid:%4i lights%6i particles%6i traces%6i hits%6i splats%6i bounces\n"
"photon cache efficiency:%6i cached%6i traced%6ianimated\n"
"%6i draws%8i vertices%8i triangles bloompixels%8i copied%8i drawn\n"
"%3i rendertargets%8i pixels\n"
"updated%5i indexbuffers%8i bytes%5i vertexbuffers%8i bytes\n"
"animcache%5ib gpuskeletal%7i vertices (%7i with normals)\n"
"fastbatch%5i count%5i surfaces%7i vertices %7i triangles\n"
"copytris%5i count%5i surfaces%7i vertices %7i triangles\n"
"dynamic%5i count%5i surfaces%7i vertices%7i triangles\n"
"%s"
, r_refdef.stats[r_stat_timedelta], loc ? "Location: " : "", loc ? loc->name : "", cl.time, r_refdef.view.colorscale
, r_refdef.stats[r_stat_renders], r_refdef.view.origin[0], r_refdef.view.origin[1], r_refdef.view.origin[2], r_refdef.view.forward[0], r_refdef.view.forward[1], r_refdef.view.forward[2]
, viewleaf ? (int)(viewleaf - r_refdef.scene.worldmodel->brush.data_leafs) : -1, viewleaf ? viewleaf->clusterindex : -1, viewleaf ? viewleaf->areaindex : -1, viewleaf ? viewleaf->numleafbrushes : 0, viewleaf ? viewleaf->numleafsurfaces : 0, viewleaf ? R_CountLeafTriangles(r_refdef.scene.worldmodel, viewleaf) : 0
, r_refdef.stats[r_stat_world_surfaces], r_refdef.stats[r_stat_world_triangles], r_refdef.stats[r_stat_entities], r_refdef.stats[r_stat_entities_surfaces], r_refdef.stats[r_stat_entities_triangles]
, r_refdef.stats[r_stat_world_leafs], r_refdef.stats[r_stat_world_portals], r_refdef.stats[r_stat_particles], cl.num_particles, r_refdef.stats[r_stat_drawndecals], r_refdef.stats[r_stat_totaldecals], r_refdef.stats[r_stat_quality]
, r_refdef.stats[r_stat_lightmapupdates], r_refdef.stats[r_stat_lightmapupdatepixels], r_refdef.stats[r_stat_framedatacurrent], r_refdef.stats[r_stat_framedatasize]
, r_refdef.stats[r_stat_lights], r_refdef.stats[r_stat_lights_clears], r_refdef.stats[r_stat_lights_scissored], r_refdef.stats[r_stat_lights_lighttriangles], r_refdef.stats[r_stat_lights_shadowtriangles], r_refdef.stats[r_stat_lights_dynamicshadowtriangles]
, r_refdef.stats[r_stat_bouncegrid_lights], r_refdef.stats[r_stat_bouncegrid_particles], r_refdef.stats[r_stat_bouncegrid_traces], r_refdef.stats[r_stat_bouncegrid_hits], r_refdef.stats[r_stat_bouncegrid_splats], r_refdef.stats[r_stat_bouncegrid_bounces]
, r_refdef.stats[r_stat_photoncache_cached], r_refdef.stats[r_stat_photoncache_traced], r_refdef.stats[r_stat_photoncache_animated]
, r_refdef.stats[r_stat_draws], r_refdef.stats[r_stat_draws_vertices], r_refdef.stats[r_stat_draws_elements] / 3, r_refdef.stats[r_stat_bloom_copypixels], r_refdef.stats[r_stat_bloom_drawpixels]
, r_refdef.stats[r_stat_rendertargets_used], r_refdef.stats[r_stat_rendertargets_pixels]
, r_refdef.stats[r_stat_indexbufferuploadcount], r_refdef.stats[r_stat_indexbufferuploadsize], r_refdef.stats[r_stat_vertexbufferuploadcount], r_refdef.stats[r_stat_vertexbufferuploadsize]
, r_refdef.stats[r_stat_animcache_skeletal_bones], r_refdef.stats[r_stat_animcache_shape_vertices], r_refdef.stats[r_stat_animcache_shade_vertices]
, r_refdef.stats[r_stat_batch_fast_batches], r_refdef.stats[r_stat_batch_fast_surfaces], r_refdef.stats[r_stat_batch_fast_vertices], r_refdef.stats[r_stat_batch_fast_triangles]
, r_refdef.stats[r_stat_batch_copytriangles_batches], r_refdef.stats[r_stat_batch_copytriangles_surfaces], r_refdef.stats[r_stat_batch_copytriangles_vertices], r_refdef.stats[r_stat_batch_copytriangles_triangles]
, r_refdef.stats[r_stat_batch_dynamic_batches], r_refdef.stats[r_stat_batch_dynamic_surfaces], r_refdef.stats[r_stat_batch_dynamic_vertices], r_refdef.stats[r_stat_batch_dynamic_triangles]
, r_speeds_timestring);
	}

	speedstringcount = 0;
	r_speeds_timestring[0] = 0;
	r_timereport_active = false;

	if (r_speeds.integer >= 2)
	{
		r_timereport_active = true;
		r_timereport_start = r_timereport_current = Sys_DirtyTime();
	}

	if (string[0])
	{
		int i, y;
		if (string[strlen(string)-1] == '\n')
			string[strlen(string)-1] = 0;
		lines = 1;
		for (i = 0;string[i];i++)
			if (string[i] == '\n')
				lines++;
		y = vid_conheight.integer - sb_lines - lines * 8;
		i = j = 0;
		r_draw2d_force = true;
		DrawQ_Fill(0, y, vid_conwidth.integer, lines * 8, 0, 0, 0, 0.5, 0);
		while (string[i])
		{
			j = i;
			while (string[i] && string[i] != '\n')
				i++;
			if (i - j > 0)
				DrawQ_String(0, y, string + j, i - j, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);
			if (string[i] == '\n')
				i++;
			y += 8;
		}
		r_draw2d_force = false;
	}

	if (r_speeds_graph_length.integer != bound(4, r_speeds_graph_length.integer, 8192))
		Cvar_SetValueQuick(&r_speeds_graph_length, bound(4, r_speeds_graph_length.integer, 8192));
	if (fabs(r_speeds_graph_seconds.value - bound(0.1f, r_speeds_graph_seconds.value, 120.0f)) > 0.01f)
		Cvar_SetValueQuick(&r_speeds_graph_seconds, bound(0.1f, r_speeds_graph_seconds.value, 120.0f));
	if (r_speeds_graph.integer)
	{
		// if we currently have no graph data, reset the graph data entirely
		int i;
		if (!cls.r_speeds_graph_data)
			for (i = 0;i < r_stat_count;i++)
				cls.r_speeds_graph_datamin[i] = cls.r_speeds_graph_datamax[i] = 0;
		if (cls.r_speeds_graph_length != r_speeds_graph_length.integer)
		{
			int stat, index, d, graph_length, *graph_data;
			cls.r_speeds_graph_length = r_speeds_graph_length.integer;
			cls.r_speeds_graph_current = 0;
			if (cls.r_speeds_graph_data)
				Mem_Free(cls.r_speeds_graph_data);
			cls.r_speeds_graph_data = (int *)Mem_Alloc(cls.permanentmempool, cls.r_speeds_graph_length * sizeof(r_refdef.stats));
			// initialize the graph to have the current values throughout history
			graph_data = cls.r_speeds_graph_data;
			graph_length = cls.r_speeds_graph_length;
			index = 0;
			for (stat = 0;stat < r_stat_count;stat++)
			{
				d = r_refdef.stats[stat];
				if (stat == r_stat_timedelta)
					d = 0;
				for (i = 0;i < graph_length;i++)
					graph_data[index++] = d;
			}
		}
	}
	else
	{
		if (cls.r_speeds_graph_length)
		{
			cls.r_speeds_graph_length = 0;
			Mem_Free(cls.r_speeds_graph_data);
			cls.r_speeds_graph_data = NULL;
			cls.r_speeds_graph_current = 0;
		}
	}

	if (cls.r_speeds_graph_length)
	{
		char legend[128];
		int i;
		const int *data;
		float x, y, width, height, scalex, scaley;
		int range_default = max(r_speeds_graph_maxdefault.integer, 1);
		int color, stat, stats, index, range_min, range_max;
		int graph_current, graph_length, *graph_data;
		int statindex[R_SPEEDS_GRAPH_COLORS];
		int sum;

		// add current stats to the graph_data
		cls.r_speeds_graph_current++;
		if (cls.r_speeds_graph_current >= cls.r_speeds_graph_length)
			cls.r_speeds_graph_current = 0;
		// poke each new stat into the current offset of its graph
		graph_data = cls.r_speeds_graph_data;
		graph_current = cls.r_speeds_graph_current;
		graph_length = cls.r_speeds_graph_length;
		for (stat = 0;stat < r_stat_count;stat++)
			graph_data[stat * graph_length + graph_current] = r_refdef.stats[stat];

		// update the graph ranges
		for (stat = 0;stat < r_stat_count;stat++)
		{
			if (cls.r_speeds_graph_datamin[stat] > r_refdef.stats[stat])
				cls.r_speeds_graph_datamin[stat] = r_refdef.stats[stat];
			if (cls.r_speeds_graph_datamax[stat] < r_refdef.stats[stat])
				cls.r_speeds_graph_datamax[stat] = r_refdef.stats[stat];
		}

		// force 2D drawing to occur even if r_render is 0
		r_draw2d_force = true;

		// position the graph
		width = r_speeds_graph_width.value;
		height = r_speeds_graph_height.value;
		x = bound(0, r_speeds_graph_x.value, vid_conwidth.value - width);
		y = bound(0, r_speeds_graph_y.value, vid_conheight.value - height);

		// fill background with a pattern of gray and black at one second intervals
		scalex = (float)width / (float)r_speeds_graph_seconds.value;
		for (i = 0;i < r_speeds_graph_seconds.integer + 1;i++)
		{
			float x1 = x + width - (i + 1) * scalex;
			float x2 = x + width - i * scalex;
			if (x1 < x)
				x1 = x;
			if (i & 1)
				DrawQ_Fill(x1, y, x2 - x1, height, 0.0f, 0.0f, 0.0f, 0.5f, 0);
			else
				DrawQ_Fill(x1, y, x2 - x1, height, 0.2f, 0.2f, 0.2f, 0.5f, 0);
		}

		// count how many stats match our pattern
		stats = 0;
		color = 0;
		for (color = 0;color < R_SPEEDS_GRAPH_COLORS;color++)
		{
			// look at all stat names and find ones matching the filter
			statindex[color] = -1;
			if (!r_speeds_graph_filter[color].string)
				continue;
			for (stat = 0;stat < r_stat_count;stat++)
				if (!strcmp(r_stat_name[stat], r_speeds_graph_filter[color].string))
					break;
			if (stat >= r_stat_count)
				continue;
			// record that this color is this stat for the line drawing loop
			statindex[color] = stat;
			// draw the legend text in the background of the graph
			dpsnprintf(legend, sizeof(legend), "%10i :%s", graph_data[stat * graph_length + graph_current], r_stat_name[stat]);
			DrawQ_String(x, y + stats * 8, legend, 0, 8, 8, r_speeds_graph_colors[color][0], r_speeds_graph_colors[color][1], r_speeds_graph_colors[color][2], r_speeds_graph_colors[color][3] * 1.00f, 0, NULL, true, FONT_DEFAULT);
			// count how many stats we need to graph in vertex buffer
			stats++;
		}

		if (stats)
		{
			// legend text is drawn after the graphs
			// render the graph lines, we'll go back and render the legend text later
			scalex = (float)width / (1000000.0 * r_speeds_graph_seconds.value);
			stats = 0;
			for (color = 0;color < R_SPEEDS_GRAPH_COLORS;color++)
			{
				// look at all stat names and find ones matching the filter
				stat = statindex[color];
				if (stat < 0)
					continue;
				// prefer to graph stats with 0 base, but if they are
				// negative we have no choice
				range_min = cls.r_speeds_graph_datamin[stat];
				range_max = max(cls.r_speeds_graph_datamax[stat], range_min + range_default);
				// some stats we specifically override the graph scale on
				if (stat == r_stat_timedelta)
					range_max = r_speeds_graph_maxtimedelta.integer;
				scaley = height / (range_max - range_min);
				// generate lines (2 vertices each)
				// to deal with incomplete data we walk right to left
				data = graph_data + stat * graph_length;
				index = graph_current;
				sum = 0;
				for (i = 0;i < graph_length - 1;)
				{
					float x1, y1, x2, y2;
					x1 = max(x, x + width - sum * scalex);
					y1 = y + height - (data[index] - range_min) * scaley;
					sum += graph_data[r_stat_timedelta * graph_length + index];
					index--;
					if (index < 0)
						index = graph_length - 1;
					i++;
					x2 = max(x, x + width - sum * scalex);
					y2 = y + height - (data[index] - range_min) * scaley;
					DrawQ_Line(1, x1, y1, x2, y2, r_speeds_graph_colors[color][0], r_speeds_graph_colors[color][1], r_speeds_graph_colors[color][2], r_speeds_graph_colors[color][3], 0);
				}
			}
		}

		// return to not drawing anything if r_render is 0
		r_draw2d_force = false;
	}

	memset(&r_refdef.stats, 0, sizeof(r_refdef.stats));
}
