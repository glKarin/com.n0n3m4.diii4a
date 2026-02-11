/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_light.c

#include "quakedef.h"
#ifndef SERVERONLY
#include "glquake.h"
#include "shader.h"

extern cvar_t r_shadow_realtime_world, r_shadow_realtime_world_lightmaps;
extern cvar_t r_hdr_irisadaptation, r_hdr_irisadaptation_multiplier, r_hdr_irisadaptation_minvalue, r_hdr_irisadaptation_maxvalue, r_hdr_irisadaptation_fade_down, r_hdr_irisadaptation_fade_up;
extern cvar_t mod_lightpoint_distance;

int	r_dlightframecount;
int		d_lightstylevalue[MAX_NET_LIGHTSTYLES];	// 8.8 fraction of base light value

void R_BumpLightstyles(unsigned int maxstyle)
{
	int style = cl_max_lightstyles;
	if (maxstyle >= style)
	{
		Z_ReallocElements((void**)&cl_lightstyle, &cl_max_lightstyles, maxstyle+1, sizeof(*cl_lightstyle));
		for (; style < cl_max_lightstyles; style++)
			VectorSet(cl_lightstyle[style].colours, 1,1,1);
	}
}
void R_UpdateLightStyle(unsigned int style, const char *stylestring, float r, float g, float b)
{
	if (style >= MAX_NET_LIGHTSTYLES)
		return;

	R_BumpLightstyles(style);

	if (!stylestring)
		stylestring = "";

	Q_strncpyz (cl_lightstyle[style].map,  stylestring, sizeof(cl_lightstyle[style].map));
	cl_lightstyle[style].length = Q_strlen(cl_lightstyle[style].map);
	if (!cl_lightstyle[style].length)
	{
		d_lightstylevalue[style] = 256;
		VectorSet(cl_lightstyle[style].colours, 1,1,1);
	}
	else
		VectorSet(cl_lightstyle[style].colours, r,g,b);
	cl_lightstyle[style].colourkey = (int)(cl_lightstyle[style].colours[0]*0x400) ^ (int)(cl_lightstyle[style].colours[1]*0x100000) ^ (int)(cl_lightstyle[style].colours[2]*0x40000000);
}

void Sh_CalcPointLight(vec3_t point, vec3_t light);
void R_UpdateHDR(vec3_t org)
{
	if (r_hdr_irisadaptation.ival && cl.worldmodel && !(r_refdef.flags & RDF_NOWORLDMODEL))
	{
		//fake and lame, but whatever.
		vec3_t ambient, diffuse, dir;
		float lev = 0;

#ifdef RTLIGHTS
		Sh_CalcPointLight(org, ambient);
		lev += VectorLength(ambient);


		if (!r_shadow_realtime_world.ival || r_shadow_realtime_world_lightmaps.value)
#endif
		{
			cl.worldmodel->funcs.LightPointValues(cl.worldmodel, org, ambient, diffuse, dir);
			lev += (VectorLength(ambient) + VectorLength(diffuse))/256;
		}

		lev += 0.001;	//no division by 0!
		lev = r_hdr_irisadaptation_multiplier.value / lev;
		lev = bound(r_hdr_irisadaptation_minvalue.value, lev, r_hdr_irisadaptation_maxvalue.value);
		if (lev > r_refdef.playerview->hdr_last + r_hdr_irisadaptation_fade_up.value*host_frametime)
			lev = r_refdef.playerview->hdr_last + r_hdr_irisadaptation_fade_up.value*host_frametime;
		else if (lev < r_refdef.playerview->hdr_last - r_hdr_irisadaptation_fade_down.value*host_frametime)
			lev = r_refdef.playerview->hdr_last - r_hdr_irisadaptation_fade_down.value*host_frametime;
		lev = bound(r_hdr_irisadaptation_minvalue.value, lev, r_hdr_irisadaptation_maxvalue.value);
		r_refdef.playerview->hdr_last = lev;
		r_refdef.hdr_value = lev;
	}
	else
		r_refdef.hdr_value = 1;
}

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i,j;
	float f;
	static int fbmodcount;


	//if (r_lightstylescale.value > 2)
		//r_lightstylescale.value = 2;
	
//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	f = (cl.time*r_lightstylespeed.value);
	if (f < 0)
		f = 0;
	i = (int)f;
	f -= i;	//this can require updates at 1000 times a second.. Depends on your framerate of course

	if (r_fullbright.value)
	{
		for (j=0 ; j<cl_max_lightstyles ; j++)
			d_lightstylevalue[j] = r_fullbright.value*255;
	}
	else for (j=0 ; j<cl_max_lightstyles ; j++)
	{
		int v1, v2, vd;
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = ('m'-'a')*22 * r_lightstylescale.value;
			continue;
		}

		if (cl_lightstyle[j].map[0] == '=')
		{
			d_lightstylevalue[j] = atof(cl_lightstyle[j].map+1)*256*r_lightstylescale.value;
			continue;
		}

		v1 = i % cl_lightstyle[j].length;
		v1 = cl_lightstyle[j].map[v1] - 'a';

		v2 = (i+1) % cl_lightstyle[j].length;
		v2 = cl_lightstyle[j].map[v2] - 'a';

		vd = v1 - v2;
		if (!r_lightstylesmooth.ival || vd < -r_lightstylesmooth_limit.ival || vd > r_lightstylesmooth_limit.ival)
			d_lightstylevalue[j] = v1*22*r_lightstylescale.value;
		else
			d_lightstylevalue[j] = (v1*(1-f) + v2*(f))*22*r_lightstylescale.value;
	}

	if (r_fullbright.modified != fbmodcount)
	{
		fbmodcount = r_fullbright.modified;
		for (j=0 ; j<cl_max_lightstyles ; j++)
		{
			if (r_fullbright.value)
				cl_lightstyle[j].colourkey = 0xff;
			else
				cl_lightstyle[j].colourkey = (int)(cl_lightstyle[j].colours[0]*0x400) ^ (int)(cl_lightstyle[j].colours[1]*0x100000) ^ (int)(cl_lightstyle[j].colours[2]*0x40000000);
		}
	}
}

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void AddLightBlend (float r, float g, float b, float a2)
{
	float	a;
	float *sw_blend = r_refdef.playerview->screentint;

	r = bound(0, r, 1);
	g = bound(0, g, 1);
	b = bound(0, b, 1);

	sw_blend[3] = a = sw_blend[3] + a2*(1-sw_blend[3]);

	a2 = a2/a;

	sw_blend[0] = sw_blend[0]*(1-a2) + r*a2;
	sw_blend[1] = sw_blend[1]*(1-a2) + g*a2;
	sw_blend[2] = sw_blend[2]*(1-a2) + b*a2;
//Con_Printf("AddLightBlend(): %4.2f %4.2f %4.2f %4.6f\n", v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
}

#define FLASHBLEND_VERTS 16
static float bubble_sintable[FLASHBLEND_VERTS+1], bubble_costable[FLASHBLEND_VERTS+1];

static void R_InitBubble(void)
{
	float a;
	int i;
	float *bub_sin, *bub_cos;

	bub_sin = bubble_sintable;
	bub_cos = bubble_costable;

	for (i=FLASHBLEND_VERTS ; i>=0 ; i--)
	{
		a = i/(float)FLASHBLEND_VERTS * M_PI*2;
		*bub_sin++ = sin(a);
		*bub_cos++ = cos(a);
	}
}

avec4_t flashblend_colours[FLASHBLEND_VERTS+1]; 
vecV_t flashblend_vcoords[FLASHBLEND_VERTS+1];
vec2_t flashblend_tccoords[FLASHBLEND_VERTS+1];
index_t flashblend_indexes[FLASHBLEND_VERTS*3];
index_t flashblend_fsindexes[6] = {0, 1, 2, 0, 2, 3};
mesh_t flashblend_mesh;
mesh_t flashblend_fsmesh;
shader_t *occluded_shader;
shader_t *flashblend_shader;
shader_t *deferredlight_shader[LSHADER_MODES];

void R_GenerateFlashblendTexture(void)
{
	float dx, dy;
	int x, y, a;
	unsigned char pixels[32][32][4];
	for (y = 0;y < 32;y++)
	{
		dy = (y - 15.5f) * (1.0f / 16.0f);
		for (x = 0;x < 32;x++)
		{
			dx = (x - 15.5f) * (1.0f / 16.0f);
			a = (int)(((1.0f / (dx * dx + dy * dy + 0.2f)) - (1.0f / (1.0f + 0.2))) * 32.0f / (1.0f / (1.0f + 0.2)));
			a = bound(0, a, 255);
			pixels[y][x][0] = a;
			pixels[y][x][1] = a;
			pixels[y][x][2] = a;
			pixels[y][x][3] = 255;
		}
	}
	R_LoadReplacementTexture("***flashblend***", NULL, IF_LINEAR, pixels, 32, 32, TF_RGBA32);
}
void R_InitFlashblends(void)
{
	int i;
	R_InitBubble();
	for (i = 0; i < FLASHBLEND_VERTS; i++)
	{
		flashblend_indexes[i*3+0] = 0;
		if (i+1 == FLASHBLEND_VERTS)
			flashblend_indexes[i*3+1] = 1;
		else
			flashblend_indexes[i*3+1] = i+2;
		flashblend_indexes[i*3+2] = i+1;

		flashblend_tccoords[i+1][0] = 0.5 + bubble_sintable[i]*0.5;
		flashblend_tccoords[i+1][1] = 0.5 + bubble_costable[i]*0.5;
	}
	flashblend_tccoords[0][0] = 0.5;
	flashblend_tccoords[0][1] = 0.5;
	flashblend_mesh.numvertexes = FLASHBLEND_VERTS+1;
	flashblend_mesh.xyz_array = flashblend_vcoords;
	flashblend_mesh.st_array = flashblend_tccoords;
	flashblend_mesh.colors4f_array[0] = flashblend_colours;
	flashblend_mesh.indexes = flashblend_indexes;
	flashblend_mesh.numindexes = FLASHBLEND_VERTS*3;
	flashblend_mesh.istrifan = true;

	flashblend_fsmesh.numvertexes = 4;
	flashblend_fsmesh.xyz_array = flashblend_vcoords;
	flashblend_fsmesh.st_array = flashblend_tccoords;
	flashblend_fsmesh.colors4f_array[0] = flashblend_colours;
	flashblend_fsmesh.indexes = flashblend_fsindexes;
	flashblend_fsmesh.numindexes = 6;
	flashblend_fsmesh.istrifan = true;

	R_GenerateFlashblendTexture();

	flashblend_shader = R_RegisterShader("flashblend", SUF_NONE,
		"{\n"
			"program defaultadditivesprite\n"
			"{\n"
				"map ***flashblend***\n"	
				"blendfunc gl_one gl_one\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"nodepth\n"
			"}\n"
		"}\n"
		);
	occluded_shader = R_RegisterShader("flashblend_occlusiontest", SUF_NONE,
		"{\n"
			"program defaultadditivesprite\n"
			"{\n"
				"maskcolor\n"
				"maskalpha\n"
			"}\n"
		"}\n"
		);
	memset(deferredlight_shader, 0, sizeof(deferredlight_shader));
}

static qboolean R_BuildDlightMesh(dlight_t *light, float colscale, float radscale, int dtype)
{
	int		i, j;
//	float	a;
	vec3_t	v;
	float	rad;
	float	*bub_sin, *bub_cos;
	vec3_t colour;

	bub_sin = bubble_sintable;
	bub_cos = bubble_costable;
	rad = light->radius * radscale;

	VectorCopy(light->color, colour);

	if (light->fov)
	{
		float a = -DotProduct(light->axis[0], vpn);
		colour[0] *= a;
		colour[1] *= a;
		colour[2] *= a;
		rad *= a;
		rad *= 0.33;
	}
	if (light->style>=0 && light->style < countof(d_lightstylevalue))
	{
		colscale *= d_lightstylevalue[light->style]/255.0f;
	}

	VectorSubtract (light->origin, r_origin, v);
	if (dtype != 1 && Length (v) < rad + r_refdef.mindist*2)
	{	// view is inside the dlight
		return false;
	}

	flashblend_colours[0][0] = colour[0]*colscale;
	flashblend_colours[0][1] = colour[1]*colscale;
	flashblend_colours[0][2] = colour[2]*colscale;
	flashblend_colours[0][3] = 1;

	VectorCopy(light->origin, flashblend_vcoords[0]);
	for (i=FLASHBLEND_VERTS ; i>0 ; i--)
	{
		for (j=0 ; j<3 ; j++)
			flashblend_vcoords[i][j] = light->origin[j] + (vright[j]*(*bub_cos) +
				+ vup[j]*(*bub_sin)) * rad;
		bub_sin++; 
		bub_cos++;
	}
	if (dtype == 0)
	{
		//flashblend 3d-ish
		VectorMA(flashblend_vcoords[0], -rad/1.5, vpn, flashblend_vcoords[0]);
	}
	else if (dtype != 1)
	{
		//prepass lights needs to be fully infront of the light. the glsl is a fullscreen-style effect, but we can benefit from early-z and scissoring
		vec3_t diff;
		VectorSubtract(r_origin, light->origin, diff);
		VectorNormalize(diff);
		for (i=0 ; i<=FLASHBLEND_VERTS ; i++)
			VectorMA(flashblend_vcoords[i], rad, diff, flashblend_vcoords[i]);
	}
	return true;
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights (void)
{
	int		i;
	dlight_t	*l;
	vec3_t waste1, waste2;
	unsigned int beflags = 0;
	float intensity, cscale;
	qboolean coronastyle;
	qboolean flashstyle;
	float dist;

	if (!r_coronas.value && !r_flashblend.value)
		return;

	l = cl_dlights+rtlights_first;
	for (i=rtlights_first; i<rtlights_max; i++, l++)
	{
		if (!l->radius)
			continue;

		if (l->corona <= 0)
			continue;

		//dlights emitting from the local player are not visible as flashblends
		if (l->key == r_refdef.playerview->viewentity)
			continue;	//was a glow
		if (l->key == -(r_refdef.playerview->viewentity))
			continue;	//was a muzzleflash

		coronastyle = (l->flags & (LFLAG_NORMALMODE|LFLAG_REALTIMEMODE)) && r_coronas.value;
		flashstyle = ((l->flags & LFLAG_FLASHBLEND) && r_flashblend.value);

		if (!coronastyle && !flashstyle)
			continue;
		if (coronastyle && flashstyle)
			flashstyle = false;

		cscale = l->coronascale;
		intensity = l->corona;// * 0.25;
		if (coronastyle)
			intensity *= r_coronas.value * r_coronas_intensity.value;
		else
			intensity *= r_flashblend.value;
		if (intensity <= 0 || cscale <= 0)
			continue;

		//prevent the corona from intersecting with the near clip plane by just fading it away if its too close
		VectorSubtract(l->origin, r_refdef.vieworg, waste1);
		dist = VectorLength(waste1);
		if (dist < r_coronas_mindist.value+r_coronas_fadedist.value)
		{
			if (dist <= r_coronas_mindist.value)
				continue;
			intensity *= (dist-r_coronas_mindist.value) / r_coronas_fadedist.value;
		}

		/*coronas use depth testing to compute visibility*/
		if (coronastyle)
		{
			int method;
			if (r_refdef.recurse)
				method = 1;	//don't confuse queries... FIXME: splitscreen/PIP will still have issues.
			else if (!*r_coronas_occlusion.string)
				method = 4;	//default to using hardware queries where possible.
			else
				method = r_coronas_occlusion.ival;

			switch(method)
			{
			case 0:
				break;
			case 3:
#ifdef GLQUAKE
				if (qrenderer == QR_OPENGL)
				{
					float depth;
					vec3_t out;
					float v[4], tempv[4];
					float mvp[16];

					v[0] = l->origin[0];
					v[1] = l->origin[1];
					v[2] = l->origin[2];
					v[3] = 1;

					Matrix4_Multiply(r_refdef.m_projection_std, r_refdef.m_view, mvp);
					Matrix4x4_CM_Transform4(mvp, v, tempv);

					tempv[0] /= tempv[3];
					tempv[1] /= tempv[3];
					tempv[2] /= tempv[3];

					out[0] = (1+tempv[0])/2;
					out[1] = (1+tempv[1])/2;
					out[2] = (1+tempv[2])/2;

					out[0] = out[0]*r_refdef.pxrect.width + r_refdef.pxrect.x;
					out[1] = out[1]*r_refdef.pxrect.height + r_refdef.pxrect.y;
					if (tempv[3] < 0)
						out[2] *= -1;

					if (out[2] < 0)
						continue;

					//FIXME: in terms of performance, mixing reads+draws is BAD BAD BAD. SERIOUSLY BAD
					//it would be an improvement to calculate all of these at once.
					qglReadPixels(out[0], out[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
					if (depth < out[2])
						continue;
					break;
				}
#endif
				//other renderers fall through
			case 4:
#ifdef GLQUAKE
				if (qrenderer == QR_OPENGL && qglGenQueriesARB)
				{
					GLuint res;
					qboolean requery = true;
					if (r_refdef.recurse)
						requery = false;
					else if (l->coronaocclusionquery)
					{
						qglGetQueryObjectuivARB(l->coronaocclusionquery, GL_QUERY_RESULT_AVAILABLE_ARB, &res);
						if (res)
							qglGetQueryObjectuivARB(l->coronaocclusionquery, GL_QUERY_RESULT_ARB, &l->coronaocclusionresult);
						else if (!l->coronaocclusionresult)
							continue;	//query still running, nor currently visible.
						else
							requery = false;
					}
					else
					{
						qglGenQueriesARB(1, &l->coronaocclusionquery);
					}

					if (requery)
					{
						qglBeginQueryARB(GL_SAMPLES_PASSED_ARB, l->coronaocclusionquery);
						R_BuildDlightMesh (l, intensity*10, cscale*.1, coronastyle);
						BE_DrawMesh_Single(occluded_shader, &flashblend_mesh, NULL, beflags);
						qglEndQueryARB(GL_SAMPLES_PASSED_ARB);
					}

					if (!l->coronaocclusionresult)
						continue;
					break;
				}
#endif
				//other renderers fall through
			default:
			case 1:	//bsp-only
			case 2:	//non-bsp too
				if (TraceLineR(r_refdef.vieworg, l->origin, waste1, waste2, method!=2))
					continue;
				break;
			}
		}

		if (!R_BuildDlightMesh (l, intensity, cscale, coronastyle) && !coronastyle)
			AddLightBlend (l->color[0], l->color[1], l->color[2], l->radius * 0.0003);
		else
			BE_DrawMesh_Single(flashblend_shader, &flashblend_mesh, NULL, (coronastyle?BEF_FORCENODEPTH|BEF_FORCEADDITIVE:0)|beflags);
	}
}


qboolean Sh_GenerateShadowMap(dlight_t *l, int lightflags);
qboolean Sh_CullLight(dlight_t *dl, qbyte *vvis);
void R_GenDlightMesh(struct batch_s *batch)
{
	static mesh_t *meshptr;
	dlight_t	*l = cl_dlights + batch->user.dlight.lightidx;
	vec3_t colour;

	int lightflags = batch->user.dlight.lightmode;

	VectorCopy(l->color, colour);
	if (l->style>=0 && l->style < cl_max_lightstyles)
	{
		colour[0] *= cl_lightstyle[l->style].colours[0] * d_lightstylevalue[l->style]/255.0f;
		colour[1] *= cl_lightstyle[l->style].colours[1] * d_lightstylevalue[l->style]/255.0f;
		colour[2] *= cl_lightstyle[l->style].colours[2] * d_lightstylevalue[l->style]/255.0f;
	}
	else
	{
		colour[0] *= r_lightstylescale.value;
		colour[1] *= r_lightstylescale.value;
		colour[2] *= r_lightstylescale.value;
	}

	if (colour[0] < 0.001 && colour[1] < 0.001 && colour[2] < 0.001)
	{	//just switch these off.
		batch->meshes = 0;
		return;
	}

	BE_SelectDLight(l, colour, l->axis, lightflags);
#ifdef RTLIGHTS
	if (lightflags & LSHADER_SMAP)
	{
		if (!Sh_GenerateShadowMap(l, lightflags))
		{
			batch->meshes = 0;
			return;
		}
		BE_SelectEntity(&r_worldentity);
		BE_SelectMode(BEM_STANDARD);
	}
	else if (Sh_CullLight(l, r_refdef.scenevis))
	{
		batch->meshes = 0;
		return;
	}
#endif

	if (!R_BuildDlightMesh (l, 2, 1, 2))
	{
		int i;
		static vec2_t s[4] = {{1, -1}, {-1, -1}, {-1, 1}, {1, 1}};
		for (i = 0; i < 4; i++)
		{
			VectorMA(r_origin, 32, vpn, flashblend_vcoords[i]);
			VectorMA(flashblend_vcoords[i], s[i][0]*320, vright, flashblend_vcoords[i]);
			VectorMA(flashblend_vcoords[i], s[i][1]*320, vup, flashblend_vcoords[i]);
		}

		meshptr = &flashblend_fsmesh;
	}
	else
	{
		meshptr = &flashblend_mesh;
	}
	batch->mesh = &meshptr;

	RQuantAdd(RQUANT_RTLIGHT_DRAWN, 1);
}
void R_GenDlightBatches(batch_t *batches[])
{
#ifdef RTLIGHTS
	int i, j, sort;
	dlight_t	*l;
	batch_t		*b;
	int lmode;
	unsigned modes;
	extern cvar_t r_shadow_realtime_dlight;
	extern cvar_t r_shadow_realtime_world;
	if (!r_lightprepass)
		return;

	modes = 0;
	if (r_shadow_realtime_dlight.ival)
		modes |= LFLAG_NORMALMODE;
	if (r_shadow_realtime_world.ival)
		modes |= LFLAG_REALTIMEMODE;
	if (!modes)
		return;


	if (!deferredlight_shader[0])
	{
		const char *deferredlight_shader_code = 
						"{\n"
							"deferredlight\n"
							"surfaceparm nodlight\n"
							"{\n"
								"program lpp_light#USE_ARB_SHADOW\n"
								"blendfunc gl_one gl_one\n"
								"nodepthtest\n"
								"map $gbuffer0\n"	//depth
								"map $gbuffer1\n"	//normals.rgb specexp.a
							"}\n"
						"}\n"
			;
		deferredlight_shader[0] = R_RegisterShader("deferredlight", SUF_NONE, deferredlight_shader_code);
#ifdef RTLIGHTS
		deferredlight_shader[LSHADER_SMAP] = R_RegisterShader("deferredlight#PCF", SUF_NONE, deferredlight_shader_code);
#endif
	}

	l = cl_dlights+rtlights_first;
	for (i=rtlights_first; i<rtlights_max; i++, l++)
	{
		if (!l->radius)
			continue;

		if (!(modes & l->flags))
			continue;

		if (R_CullSphere(l->origin, l->radius))
		{
			RQuantAdd(RQUANT_RTLIGHT_CULL_FRUSTUM, 1);
			continue;
		}

		lmode = 0;
#ifdef RTLIGHTS
		if (!(((i >= RTL_FIRST)?!r_shadow_realtime_world_shadows.ival:!r_shadow_realtime_dlight_shadows.ival) || l->flags & LFLAG_NOSHADOWS))
			lmode |= LSHADER_SMAP;
#endif
//		if (TEXLOADED(l->cubetexture))
//			lmode |= LSHADER_CUBE;

		b = BE_GetTempBatch();
		if (!b)
			return;

		b->flags = 0;
		b->shader = deferredlight_shader[lmode];
		sort = b->shader->sort;
		b->buildmeshes = R_GenDlightMesh;
		b->ent = &r_worldentity;
		b->mesh = NULL;
		b->firstmesh = 0;
		b->meshes = 1;
		b->skin = NULL;
		b->texture = NULL;
		for (j = 0; j < MAXRLIGHTMAPS; j++)
			b->lightmap[j] = -1;
		b->user.dlight.lightidx = i;
		b->user.dlight.lightmode = lmode;
		b->flags |= BEF_NOSHADOWS|BEF_NODLIGHT;	//that would be weeird
		b->vbo = NULL;
		b->next = batches[sort];
		batches[sort] = b;
	}
#endif
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	r_dlightframecount++;	// because the count hasn't
											//  advanced yet for this frame

#ifdef RTLIGHTS
	/*if we're doing full rtlighting only, then don't bother calculating old-style dlights as they won't be visible anyway*/
	if (r_shadow_realtime_world.ival && r_shadow_realtime_world_lightmaps.value < 0.1)
		return;
#endif

	if (!r_dlightlightmaps || !r_worldentity.model)
		return;

	if (r_worldentity.model->loadstate != MLS_LOADED)
		return;

	if (!r_worldentity.model->nodes)
		return;

	currentmodel = r_worldentity.model;
	if (!currentmodel->funcs.MarkLights)
		return;
	
	l = cl_dlights+rtlights_first;
	for (i=rtlights_first ; i <= DL_LAST ; i++, l++)
	{
		if (!l->radius || !(l->flags & LFLAG_LIGHTMAP))
			continue;
		if ((l->flags & LFLAG_NORMALMODE) && r_shadow_realtime_dlight.ival)
			continue;	//don't draw both. its redundant and a waste of cpu.
		if ((l->flags & LFLAG_REALTIMEMODE) && r_shadow_realtime_world.ival)
			continue;	//also don't draw both.
		currentmodel->funcs.MarkLights( l, (dlightbitmask_t)1u<<i, currentmodel->nodes );
	}
}



/////////////////////////////////////////////////////////////
//rtlight loading

#ifdef RTLIGHTS
//These affect importing
static cvar_t r_editlights_import_radius			= CVARAD ("r_editlights_import_radius", "1", "r_editlights_quakelightsizescale", "changes size of light entities loaded from a map");
static cvar_t r_editlights_import_ambient			= CVARD ("r_editlights_import_ambient", "0", "ambient light scaler for imported lights");
static cvar_t r_editlights_import_diffuse			= CVARD ("r_editlights_import_diffuse", "1", "diffuse light scaler for imported lights");
static cvar_t r_editlights_import_specular			= CVARD ("r_editlights_import_specular", "1", "specular light scaler for imported lights");	//excessive, but noticable. its called stylized, okay? shiesh, some people

//these are just for the crappy editor
static cvar_t r_editlights							= CVARD ("r_editlights", "0", "enables .rtlights file editing mode. Consider using csaddon/equivelent instead.");
static cvar_t r_editlights_cursordistance			= CVARD ("r_editlights_cursordistance", "1024", "maximum distance of cursor from eye");
static cvar_t r_editlights_cursorpushoff			= CVARD ("r_editlights_cursorpushoff", "4", "how far to push the cursor off the impacted surface");
static cvar_t r_editlights_cursorpushback			= CVARD ("r_editlights_cursorpushback", "0", "how far to pull the cursor back toward the eye, for some reason");
static cvar_t r_editlights_cursorgrid				= CVARD ("r_editlights_cursorgrid", "1", "snaps cursor to this grid size");

//internal settings
static qboolean r_editlights_locked = false;		//don't change the selected light
static int r_editlights_selected = -1;				//the light closest to the cursor
static vec3_t r_editlights_cursor;					//the position of the crosshair/cursor (new lights will be spawned here)
static dlight_t r_editlights_copybuffer;			//written by r_editlights_copyinfo, read by r_editlights_pasteinfo. FIXME: use system clipboard?

qboolean R_ImportRTLights(const char *entlump, int importmode)
{
	typedef enum lighttype_e {LIGHTTYPE_MINUSX, LIGHTTYPE_RECIPX, LIGHTTYPE_RECIPXX, LIGHTTYPE_INFINITE, LIGHTTYPE_LOCALMIN, LIGHTTYPE_RECIPXX2, LIGHTTYPE_SUN} lighttype_t;

	/*I'm using the DP code so I know I'll get the DP results*/
	int entnum, style, islight, skin, pflags, n;
	lighttype_t type;
	float origin[3], angles[3], mangle[3], radius, color[3], light[4], fadescale, lightscale, originhack[3], overridecolor[3], colourscales[3], vec[4];
	char key[256], value[8192];
	char targetname[256], target[256];
	int nest;
	qboolean okay = false;
	infobuf_t targets;
	const char *lmp;
	qboolean rerelease = false;
	float fade[2];
	memset(&targets, 0, sizeof(targets));

	//a quick note about tenebrae:
	//by default, tenebrae's rtlights come from the server via static entities, which is all fancy and posh and actually fairly nice... if all servers actually did it.
	//(the tenebrae gamecode uses spawnflag 2048 for static lights. note the pflags_fulldynamic fte/dp vs tenebrae difference)
	//failing that, it will insert lights with some crappy fixed radius around only all 'classname light' entities, without any colours or anything, vanilla only.
	//such lights are ONLY created if they're not near some other existing light (like a static entity one).
	//this can result in FTE having noticably more and bigger lights than tenebrae. shadowmapping doesn't help performance either.

	//handle doom3's header
	COM_Parse(entlump);
	if (!strcmp(com_token, "Version"))
	{
		entlump = COM_Parse(entlump);
		entlump = COM_Parse(entlump);
	}

	//find targetnames, and store their origins so that we can deal with spotlights.
	for (lmp = entlump; ;)
	{
		lmp = COM_Parse(lmp);
		if (com_token[0] != '{')
			break;

		*targetname = 0;
		VectorClear(origin);

		nest = 1;
		while (1)
		{
			lmp = COM_ParseOut(lmp, key, sizeof(key));
			if (!lmp)
				break; // error
			if (key[0] == '{')
			{
				nest++;
				continue;
			}
			if (key[0] == '}')
			{
				nest--;
				if (!nest)
					break; // end of entity
				continue;
			}
			if (nest!=1)
				continue;
			if (key[0] == '_')
			{
				if (!strcmp(key+1, "shadowlight"))
					rerelease = true;
				memmove(key, key+1, strlen(key));
			}
			while (key[strlen(key)-1] == ' ') // remove trailing spaces
				key[strlen(key)-1] = 0;
			lmp = COM_ParseOut(lmp, value, sizeof(value));
			if (!lmp)
				break; // error

			// now that we have the key pair worked out...
			if (!strcmp("targetname", key))
				Q_strncpyz(targetname, value, sizeof(targetname));
			else if (!strcmp("origin", key))
				sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
		}
		//if we found an ent with a targetname and an origin, then record where it was.
		if (*targetname && (origin[0] || origin[1] || origin[2]))
			InfoBuf_SetStarKey(&targets, targetname, va("%f %f %f", origin[0], origin[1], origin[2]));
	}

	if (!importmode && !rerelease)
	{
		InfoBuf_Clear(&targets, true);
		return false;	//don't make it up from legacy ents.
	}

	for (entnum = 0; ;entnum++)
	{
		entlump = COM_Parse(entlump);
		if (com_token[0] != '{')
			break;

		type = LIGHTTYPE_MINUSX;
		origin[0] = origin[1] = origin[2] = 0;
		originhack[0] = originhack[1] = originhack[2] = 0;
		angles[0] = angles[1] = angles[2] = 0;
		mangle[0] = mangle[1] = mangle[2] = 0;
		color[0] = color[1] = color[2] = 1;
		light[0] = light[1] = light[2] = 1;light[3] = 300;
		overridecolor[0] = overridecolor[1] = overridecolor[2] = 1;
		fadescale = 1;
		lightscale = 1;
		*target = 0;
		style = 0;
		skin = 0;
		pflags = 0;
		fade[0] = fade[1] = 0;
		VectorSet(colourscales, r_editlights_import_ambient.value, r_editlights_import_diffuse.value, r_editlights_import_specular.value);
		//effects = 0;
		islight = false;
		nest = 1;
		while (1)
		{
			entlump = COM_Parse(entlump);
			if (!entlump)
				break; // error
			if (com_token[0] == '{')
			{
				nest++;
				continue;
			}
			if (com_token[0] == '}')
			{
				nest--;
				if (!nest)
					break; // end of entity
				continue;
			}
			if (nest!=1)
				continue;
			if (com_token[0] == '_')
				Q_strncpyz(key, com_token + 1, sizeof(key));
			else
				Q_strncpyz(key, com_token, sizeof(key));
			while (key[strlen(key)-1] == ' ') // remove trailing spaces
				key[strlen(key)-1] = 0;
			entlump = COM_Parse(entlump);
			if (!entlump)
				break; // error
			Q_strncpyz(value, com_token, sizeof(value));

			if (rerelease)
			{
				if (!strcmp("color", key))
					sscanf(value, "%f %f %f", &light[0], &light[1], &light[2]);
				else if (!strcmp("shadowlightcull", key))
					;//sscanf(value, "%f %f %f", &color[0], &color[1], &color[2]);
				else if (!strcmp("shadowlightresolution", key))
					;//sscanf(value, "%f %f %f", &color[0], &color[1], &color[2]);
				else if (!strcmp("shadowlightradius", key))
					light[3] = atof(value);
				else if (!strcmp("shadowlightstartfadedistance", key))
					fade[0] = atof(value);
				else if (!strcmp("shadowlightendfadedistance", key))
					fade[1] = atof(value);
				else if (!strcmp("shadowlightintensity", key))
				{
					colourscales[0] *= atof(value);
					colourscales[1] *= atof(value);
					colourscales[2] *= atof(value);
				}
				else if (!strcmp("shadowlight", key))
					islight = atoi(value);
				else if (!strcmp("shadowlightstyle", key))
					style = atoi(value);
				else if (!strcmp("shadowlightconeangle", key))
					angles[1] = atof(value)*2;
				else if (!strcmp("origin", key))
					sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
				else if (!strcmp("target", key))
					Q_strncpyz(target, value, sizeof(target));
			}
			else
			{
				// now that we have the key pair worked out...
				if (!strcmp("light", key))
				{
					n = sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
					if (n == 1)
					{
						// quake
						light[0] = vec[0] * (1.0f / 256.0f);
						light[1] = vec[0] * (1.0f / 256.0f);
						light[2] = vec[0] * (1.0f / 256.0f);
						light[3] = vec[0];
					}
					else if (n == 4)
					{
						// halflife
						light[0] = vec[0] * (1.0f / 255.0f);
						light[1] = vec[1] * (1.0f / 255.0f);
						light[2] = vec[2] * (1.0f / 255.0f);
						light[3] = vec[3];
					}
				}
				else if (!strcmp("delay", key))
					type = atoi(value);
				else if (!strcmp("origin", key))
					sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
				else if (!strcmp("angle", key))	//orientation for cubemaps (or angle of spot lights)
					angles[0] = 0, angles[1] = atof(value), angles[2] = 0;
				else if (!strcmp("mangle", key))	//orientation for cubemaps (or angle of spot lights)
				{
					sscanf(value, "%f %f %f", &mangle[1], &mangle[0], &mangle[2]);	//FIXME: order is fucked.
					mangle[0] = 360-mangle[0];	//FIXME: pitch is fucked too.
				}
				//_softangle -- the inner cone angle of a spotlight.
				else if (!strcmp("angles", key))	//richer cubemap orientation.
					sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
				else if (!strcmp("color", key))
					sscanf(value, "%f %f %f", &color[0], &color[1], &color[2]);
				else if (!strcmp("wait", key))
					fadescale = atof(value);
				else if (!strcmp("target", key))
					Q_strncpyz(target, value, sizeof(target));
				else if (!strcmp("classname", key))
				{
					if (!strncmp(value, "light", 5))
					{
						islight = true;
						if (!strcmp(value, "light_fluoro"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 1;
							overridecolor[2] = 1;
						}
						if (!strcmp(value, "light_fluorospark"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 1;
							overridecolor[2] = 1;
						}
						if (!strcmp(value, "light_globe"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 0.8;
							overridecolor[2] = 0.4;
						}
						if (!strcmp(value, "light_flame_large_yellow"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 0.5;
							overridecolor[2] = 0.1;
						}
						if (!strcmp(value, "light_flame_small_yellow"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 0.5;
							overridecolor[2] = 0.1;
						}
						if (!strcmp(value, "light_torch_small_white"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 0.5;
							overridecolor[2] = 0.1;
						}
						if (!strcmp(value, "light_torch_small_walltorch"))
						{
							originhack[0] = 0;
							originhack[1] = 0;
							originhack[2] = 0;
							overridecolor[0] = 1;
							overridecolor[1] = 0.5;
							overridecolor[2] = 0.1;
						}
					}
				}
				else if (!strcmp("style", key))
					style = atoi(value);
				else if (!strcmp("skin", key))
					skin = (int)atof(value);
				else if (!strcmp("pflags", key))
					pflags = (int)atof(value);
				//else if (!strcmp("effects", key))
					//effects = (int)atof(value);

				else if (!strcmp("scale", key))
					lightscale = atof(value);
				else if (!strcmp("fade", key))
					fadescale = atof(value);

#ifdef MAP_PROC
				else if (!strcmp("nodynamicshadows", key))	//doom3
					;
				else if (!strcmp("noshadows", key))	//doom3
				{
					if (atof(value))
						pflags |= PFLAGS_NOSHADOW;
				}
				else if (!strcmp("nospecular", key))//doom3
				{
					if (atof(value))
						colourscales[2] = 0;
				}
				else if (!strcmp("nodiffuse", key))	//doom3
				{
					if (atof(value))
						colourscales[1] = 0;
				}
#endif
				else if (!strcmp("light_radius", key))
				{
					light[0] = 1;
					light[1] = 1;
					light[2] = 1;
					light[3] = atof(value);
				}
				else if (entnum == 0 && !strcmp("noautolight", key))
				{
					//tenebrae compat. don't generate rtlights automagically if the world entity specifies this.
					if (atoi(value))
					{
						okay = true;
						InfoBuf_Clear(&targets, true);
						return okay;
					}
				}
				else if (entnum == 0 && !strcmp("lightmapbright", key))
				{
					//tenebrae compat. this overrides r_shadow_realtime_world_lightmap
					r_shadow_realtime_world_lightmaps_force = atof(value);
				}
			}
		}
		if (!islight)
			continue;
		if (lightscale <= 0)
			lightscale = 1;
		if (fadescale <= 0)
			fadescale = 1;
		if (color[0] >= 16 || color[1] >= 16 || color[2] >= 16)	//_color 255 255 255 should be identity, not super-oversaturated.
			VectorScale(color, 1/255.0, color);					//if only there were standards for this sort of thing.
		if (color[0] == color[1] && color[0] == color[2])
		{
			color[0] *= overridecolor[0];
			color[1] *= overridecolor[1];
			color[2] *= overridecolor[2];
		}
		radius = light[3] * r_editlights_import_radius.value * lightscale / fadescale;
		color[0] = color[0] * light[0];
		color[1] = color[1] * light[1];
		color[2] = color[2] * light[2];
#define CUTOFF (128.0/255)
		switch (type)
		{
		case LIGHTTYPE_MINUSX:
			break;
		case LIGHTTYPE_RECIPX:
#if 1
			radius *= 2;
//			VectorScale(color, (1.0f / 16.0f), color);
#else
			//light util uses something like: cutoff == light/((scaledist*fadescale*radius)/128)
			//radius = light/(cutoff*128*scaledist*fadescale)
			radius = lightscale*r_editlights_import_radius.value*256/(1*fadescale);
			radius = min(radius, 300);
			VectorScale(color, 255/light[3], color);
#endif
			break;
		case LIGHTTYPE_RECIPXX:
		case LIGHTTYPE_RECIPXX2:
#if 1
			radius *= 2;
//			VectorScale(color, (1.0f / 16.0f), color);
#else
			//light util uses something like: cutoff == light/((scaledist*scaledist*fadescale*fadescale*radius*radius)/(128*128))
			radius = lightscale*r_editlights_import_radius.value*sqrt(1/CUTOFF*128*128*1*1*fadescale*fadescale);
			radius = min(radius, 300);
			VectorScale(color, 255/light[3], color);
#endif
			break;
		default:
		case LIGHTTYPE_INFINITE:
			radius = FLT_MAX;	//close enough
			break;
		case LIGHTTYPE_LOCALMIN:	//can't support, treat like LIGHTTYPE_MINUSX
			break;
		case LIGHTTYPE_SUN:
			break;
		}
		
		if (rerelease)
		{
			if (r_shadow_realtime_world_lightmaps_force < 0)
				r_shadow_realtime_world_lightmaps_force = 1;
		}
		else if (radius < 50)	//some mappers insist on many tiny lights. such lights can usually get away with no shadows..
			pflags |= PFLAGS_NOSHADOW;

		VectorAdd(origin, originhack, origin);
		if (radius >= 1 && !(cl.worldmodel->funcs.PointContents(cl.worldmodel, NULL, origin) & FTECONTENTS_SOLID))
		{
			dlight_t *dl = CL_AllocSlight();
			if (!dl)
				break;

			VectorCopy(origin, dl->origin);
			VectorCopy(angles, dl->angles);
			AngleVectors(dl->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
			VectorInverse(dl->axis[1]);
			dl->radius = radius;
			VectorCopy(color, dl->color);
			dl->flags = 0;
			dl->flags |= rerelease?LFLAG_REALTIMEMODE|LFLAG_NORMALMODE:LFLAG_REALTIMEMODE;
			dl->flags |= (pflags & PFLAGS_CORONA)?LFLAG_FLASHBLEND:0;
			dl->flags |= (pflags & PFLAGS_NOSHADOW)?LFLAG_NOSHADOWS:0;
			dl->style = style;
			dl->fade[0] = fade[0];
			dl->fade[1] = fade[1];
			VectorCopy(colourscales, dl->lightcolourscales);

			//handle spotlights.
			if (mangle[0] || mangle[1] || mangle[2])
			{
				dl->fov = angles[1];
				if (!dl->fov)	//default is 40, supposedly
					dl->fov = 40;

				VectorCopy(mangle, dl->angles);
				AngleVectors(dl->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
				VectorInverse(dl->axis[1]);
			}
			else if (*target)
			{
				lmp = InfoBuf_ValueForKey(&targets, target);
				if (*lmp)
				{
					dl->fov = angles[1];
					if (!dl->fov)	//default is 40, supposedly
						dl->fov = 40;
					sscanf(lmp, "%f %f %f", &angles[0], &angles[1], &angles[2]);
					VectorSubtract(angles, origin, dl->axis[0]);
					VectorNormalize(dl->axis[0]);
					VectorVectors(dl->axis[0], dl->axis[1], dl->axis[2]);
					VectorInverse(dl->axis[1]);
					//we don't have any control over the inner cone.

					//so queries work properly
					VectorAngles(dl->axis[0], dl->axis[2], dl->angles, false);
					dl->angles[0] = anglemod(dl->angles[0]);
				}
			}

			if (skin >= 16)
				R_LoadNumberedLightTexture(dl, skin);

			okay = true;
		}
	}

	InfoBuf_Clear(&targets, true);

	return okay;
}

qboolean R_LoadRTLights(void)
{
	dlight_t *dl;
	char fname[MAX_QPATH];
	char cubename[MAX_QPATH];
	char customstyle[1024];
	char *file;
	char *end;
	int style;

	vec3_t org;
	float radius;
	vec3_t rgb;
	vec3_t avel;
	float fov, nearclip;
	unsigned int flags;

	float coronascale;
	float corona;
	float ambientscale, diffusescale, specularscale;
	vec3_t angles;
	float fade[2];

	//delete all old lights, even dynamic ones
	rtlights_first = RTL_FIRST;
	rtlights_max = RTL_FIRST;

	COM_StripExtension(cl.worldmodel->name, fname, sizeof(fname));
	strncat(fname, ".rtlights", MAX_QPATH-1);

	file = COM_LoadTempFile(fname, 0, NULL);
	if (file)
	while(1)
	{
		end = strchr(file, '\n');
		if (!end)
			end = file + strlen(file);
		if (end == file)
			break;
		*end = '\0';

		while(*file == ' ' || *file == '\t')
			file++;
		if (*file == '#')
		{
			file++;
			while(*file == ' ' || *file == '\t')
				file++;
			file = COM_Parse(file);
			if (!Q_strcasecmp(com_token, "lightmaps"))
			{
				file = COM_Parse(file);
				//foo = atoi(com_token);
			}
			else
				Con_DPrintf("Unknown directive: %s\n", com_token);
			file = end+1;
			continue;
		}
		else if (*file == '!')
		{
			flags = LFLAG_NOSHADOWS;
			file++;
		}
		else
			flags = 0;

		file = COM_Parse(file);
		org[0] = atof(com_token);
		file = COM_Parse(file);
		org[1] = atof(com_token);
		file = COM_Parse(file);
		org[2] = atof(com_token);

		file = COM_Parse(file);
		radius = atof(com_token);

		file = COM_Parse(file);
		rgb[0] = file?atof(com_token):1;
		file = COM_Parse(file);
		rgb[1] = file?atof(com_token):1;
		file = COM_Parse(file);
		rgb[2] = file?atof(com_token):1;

		file = COM_Parse(file);
		style = file?atof(com_token):0;

		file = COM_Parse(file);
		//cubemap
		Q_strncpyz(cubename, com_token, sizeof(cubename));

		file = COM_Parse(file);
		//corona
		corona = file?atof(com_token):0;

		file = COM_Parse(file);
		angles[0] = file?atof(com_token):0;
		file = COM_Parse(file);
		angles[1] = file?atof(com_token):0;
		file = COM_Parse(file);
		angles[2] = file?atof(com_token):0;

		file = COM_Parse(file);
		//corona scale
		coronascale = file?atof(com_token):0.25;

		file = COM_Parse(file);
		//ambient
		ambientscale = file?atof(com_token):0;

		file = COM_Parse(file);
		//diffuse
		diffusescale = file?atof(com_token):1;

		file = COM_Parse(file);
		//specular
		specularscale = file?atof(com_token):1;

		file = COM_Parse(file);
		flags |= file?atoi(com_token):LFLAG_REALTIMEMODE;

		nearclip = fov = avel[0] = avel[1] = avel[2] = fade[0] = fade[1] = 0;
		*customstyle = 0;
		while(file)
		{
			file = COM_Parse(file);
			if (!strncmp(com_token, "rotx=", 5))
				avel[0] = file?atof(com_token+5):0;
			else if (!strncmp(com_token, "roty=", 5))
				avel[1] = file?atof(com_token+5):0;
			else if (!strncmp(com_token, "rotz=", 5))
				avel[2] = file?atof(com_token+5):0;
			else if (!strncmp(com_token, "fov=", 4))
				fov = file?atof(com_token+4):0;
			else if (!strncmp(com_token, "fademin=", 8))
				fade[0] = file?atof(com_token+8):0;
			else if (!strncmp(com_token, "fademax=", 8))
				fade[1] = file?atof(com_token+4):0;
			else if (!strncmp(com_token, "nearclip=", 9))
				nearclip = file?atof(com_token+9):0;
			else if (!strncmp(com_token, "nostencil=", 10))
				flags |= atoi(com_token+10)?LFLAG_SHADOWMAP:0;
			else if (!strncmp(com_token, "crepuscular=", 12))
				flags |= atoi(com_token+12)?LFLAG_CREPUSCULAR:0;
			else if (!strncmp(com_token, "ortho=", 6))
				flags |= atoi(com_token+6)?LFLAG_ORTHO:0;
			else if (!strncmp(com_token, "stylestring=", 12))
				Q_strncpyz(customstyle, com_token+12, sizeof(customstyle));
			else if (file)
				Con_DPrintf("Unknown .rtlights arg \"%s\"\n", com_token);
		}

		if (radius)
		{
			dl = CL_AllocSlight();
			if (!dl)
				break;

			VectorCopy(org, dl->origin);
			dl->radius = radius;
			VectorCopy(rgb, dl->color);
			dl->corona = corona;
			dl->coronascale = coronascale;
			dl->die = 0;
			dl->flags = flags;
			dl->fov = fov;
			dl->nearclip = nearclip;
			dl->lightcolourscales[0] = ambientscale;
			dl->lightcolourscales[1] = diffusescale;
			dl->lightcolourscales[2] = specularscale;
			dl->fade[0] = fade[0];
			dl->fade[1] = fade[1];
			VectorCopy(angles, dl->angles);
			AngleVectorsFLU(angles, dl->axis[0], dl->axis[1], dl->axis[2]);
			VectorCopy(avel, dl->rotation);

			Q_strncpyz(dl->cubemapname, cubename, sizeof(dl->cubemapname));
			if (*dl->cubemapname)
				dl->cubetexture = R_LoadReplacementTexture(dl->cubemapname, "", IF_TEXTYPE_CUBE, NULL, 0, 0, TF_INVALID);
			else
				dl->cubetexture = r_nulltex;

			dl->style = style;
			dl->customstyle = (*customstyle)?Z_StrDup(customstyle):NULL;
		}
		file = end+1;
	}
	return !!file;
}

static void R_SaveRTLights_f(void)
{
	dlight_t *light;
	vfsfile_t *f;
	unsigned int i;
	char fname[MAX_QPATH];
	char displayname[MAX_OSPATH];
	vec3_t ang;
	int ver = 0;
	COM_StripExtension(cl.worldmodel->name, fname, sizeof(fname));
	strncat(fname, ".rtlights", MAX_QPATH-1);

	FS_CreatePath(fname, FS_GAMEONLY);
	f = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
	if (!f)
	{
		Con_Printf("couldn't open %s\n", fname);
		return;
	}

//	VFS_PUTS(f, va("#lightmap %f\n", foo));

	for (light = cl_dlights+rtlights_first, i=rtlights_first; i<rtlights_max; i++, light++)
	{
		if (light->die)
			continue;
		if (!light->radius)
			continue;
		VectorAngles(light->axis[0], light->axis[2], ang, false);

		//the .rtlights format is defined by DP, the first few parts cannot be changed without breaking wider compat.
		//it got extended a few times. only write what we need for greater compat, just in case.
		if ((light->flags & (LFLAG_SHADOWMAP|LFLAG_CREPUSCULAR|LFLAG_ORTHO)) || light->rotation[0] || light->rotation[1] || light->rotation[2] || light->fov || light->customstyle)
			ver = 2;	//one of our own flags. always spew the full DP stuff to try to avoid confusion
		else if (light->coronascale!=0.25 || light->lightcolourscales[0]!=0 || light->lightcolourscales[1]!=1 || light->lightcolourscales[2]!=1 || (light->flags&~LFLAG_NOSHADOWS) != LFLAG_REALTIMEMODE)
			ver = 2;
		else if (*light->cubemapname || light->corona || ang[0] || ang[1] || ang[2])
			ver = 1;
		else
			ver = 0;
		VFS_PRINTF(f,
			"%s%f %f %f "
			"%f %f %f %f "
			"%i",
			(light->flags & LFLAG_NOSHADOWS)?"!":"", light->origin[0], light->origin[1], light->origin[2],
			light->radius, light->color[0], light->color[1], light->color[2],
			light->style);
		if (ver > 0)
			VFS_PRINTF(f, " \"%s\" %f %f %f %f", light->cubemapname, light->corona, ang[0], ang[1], ang[2]);
		if (ver > 1)
			VFS_PRINTF(f, " %f %f %f %f %i", light->coronascale, light->lightcolourscales[0], light->lightcolourscales[1], light->lightcolourscales[2], light->flags&(LFLAG_NORMALMODE|LFLAG_REALTIMEMODE));

		//our weird flags
		if (light->flags&LFLAG_SHADOWMAP)
			VFS_PRINTF(f, " nostencil=1");
		if (light->flags&LFLAG_CREPUSCULAR)
			VFS_PRINTF(f, " crepuscular=1");
		if (light->flags&LFLAG_ORTHO)
			VFS_PRINTF(f, " ortho=1");
		//spinning lights (for cubemaps)
		if (light->rotation[0] || light->rotation[1] || light->rotation[2])
			VFS_PRINTF(f, " rotx=%g roty=%g rotz=%g", light->rotation[0],light->rotation[1],light->rotation[2]);
		//spotlights
		if (light->fov)
			VFS_PRINTF(f, " fov=%g", light->fov); //aka: outer cone
		if (light->nearclip)
			VFS_PRINTF(f, " nearclip=%g", light->nearclip); //aka: distance into a wall, for lights that are meant to appear to come from a texture
		if (light->customstyle)
			VFS_PRINTF(f, " \"stylestring=%s\"", light->customstyle); //aka: outer cone
		if (light->fade[1]>0)
			VFS_PRINTF(f, " \"fademin=%g\" \"fademax=%g\"", light->fade[0], light->fade[1]);

		VFS_PUTS(f, "\n");
	}
	VFS_CLOSE(f);

	FS_DisplayPath(fname, FS_GAMEONLY, displayname, sizeof(displayname));
	Con_Printf("rtlights saved to %s\n", displayname);
}

void R_StaticEntityToRTLight(int i)
{
	entity_state_t *state = &cl_static_entities[i].state;
	dlight_t *dl;
	if (!(state->lightpflags&(PFLAGS_FULLDYNAMIC|PFLAGS_CORONA)))
		return;
	dl = CL_AllocSlight();
	if (!dl)
		return;
	VectorCopy(state->origin, dl->origin);
	AngleVectors(state->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
	VectorInverse(dl->axis[1]);
	dl->radius = state->light[3];
	if (!dl->radius)
		dl->radius = 350;
	VectorScale(state->light, 1.0/1024, dl->color);
	if (!state->light[0] && !state->light[1] && !state->light[2])
		VectorSet(dl->color, 1, 1, 1);
	dl->flags = 0;
	dl->flags |= LFLAG_NORMALMODE|LFLAG_REALTIMEMODE;
	dl->flags |= (state->lightpflags & PFLAGS_NOSHADOW)?LFLAG_NOSHADOWS:0;
	if (state->lightpflags & PFLAGS_CORONA)
		dl->corona = 1;
	dl->style = state->lightstyle;
	if (state->lightpflags & PFLAGS_FULLDYNAMIC)
	{
		dl->lightcolourscales[0] = r_editlights_import_ambient.value;
		dl->lightcolourscales[1] = r_editlights_import_diffuse.value;
		dl->lightcolourscales[2] = r_editlights_import_specular.value;
	}
	else
	{	//corona-only light
		dl->lightcolourscales[0] = 0;
		dl->lightcolourscales[1] = 0;
		dl->lightcolourscales[2] = 0;
	}
	if (state->skinnum >= 16)
		R_LoadNumberedLightTexture(dl, state->skinnum);
}

static void R_ReloadRTLights_f(void)
{
	int i;

	if (!cl.worldmodel)
	{
		Con_Printf("Cannot reload lights at this time\n");
		return;
	}
	rtlights_first = RTL_FIRST;
	rtlights_max = RTL_FIRST;
	r_shadow_realtime_world_lightmaps_force = -1;
	if (!strcmp(Cmd_Argv(1), "bsp"))
		R_ImportRTLights(Mod_GetEntitiesString(cl.worldmodel), 1);
	else if (!strcmp(Cmd_Argv(1), "rtlights"))
		R_LoadRTLights();
	else if (!strcmp(Cmd_Argv(1), "statics"))
	{
		for (i = 0; i < cl.num_statics; i++)
			R_StaticEntityToRTLight(i);
	}
	else if (!strcmp(Cmd_Argv(1), "none"))
		;
	else
	{
		//try to load .rtlights file
		if (rtlights_first == rtlights_max)
			R_LoadRTLights();
		//if there's a static entity with rtlights set, then assume the mod is taking care of it for us.
		if (rtlights_first == rtlights_max)
			for (i = 0; i < cl.num_statics; i++)
				R_StaticEntityToRTLight(i);
		//otherwise try to import.
		if (rtlights_first == rtlights_max)
			R_ImportRTLights(Mod_GetEntitiesString(cl.worldmodel), r_shadow_realtime_world_importlightentitiesfrommap.ival);
	}

	if (r_shadow_realtime_world_lightmaps_force >= 0)
		r_shadow_realtime_world_lightmaps.value = r_shadow_realtime_world_lightmaps_force;
	else
		r_shadow_realtime_world_lightmaps.value = atof(r_shadow_realtime_world_lightmaps.string);
}

//-1 for arg error
static int R_EditLight(dlight_t *dl, const char *cmd, int argc, const char *x, const char *y, const char *z)
{
	if (argc == 1)
	{
		y = x;
		z = x;
	}
	if (!strcmp(cmd, "origin"))
	{
		dl->origin[0] = atof(x);
		dl->origin[1] = atof(y);
		dl->origin[2] = atof(z);
	}
	else if (!strcmp(cmd, "originscale"))
	{
		dl->origin[0] *= atof(x);
		dl->origin[1] *= atof(y);
		dl->origin[2] *= atof(z);
	}
	else if (!strcmp(cmd, "originx"))
		dl->origin[0] = atof(x);
	else if (!strcmp(cmd, "originy"))
		dl->origin[1] = atof(x);
	else if (!strcmp(cmd, "originz"))
		dl->origin[2] = atof(x);
	else if (!strcmp(cmd, "move"))
	{
		dl->origin[0] += atof(x);
		dl->origin[1] += atof(y);
		dl->origin[2] += atof(z);
	}
	else if (!strcmp(cmd, "movex"))
		dl->origin[0] += atof(x);
	else if (!strcmp(cmd, "movey"))
		dl->origin[1] += atof(x);
	else if (!strcmp(cmd, "movez"))
		dl->origin[2] += atof(x);

	else if (!strcmp(cmd, "angles"))
	{
		dl->angles[0] = atof(x);
		dl->angles[1] = atof(y);
		dl->angles[2] = atof(z);

		AngleVectors(dl->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
		VectorInverse(dl->axis[1]);
	}
	else if (!strcmp(cmd, "anglesx"))
	{
		dl->angles[0] = atof(x);
		AngleVectors(dl->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
		VectorInverse(dl->axis[1]);
	}
	else if (!strcmp(cmd, "anglesy"))
	{
		dl->angles[1] = atof(x);
		AngleVectors(dl->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
		VectorInverse(dl->axis[1]);
	}
	else if (!strcmp(cmd, "anglesz"))
	{
		dl->angles[2] = atof(x);
		AngleVectors(dl->angles, dl->axis[0], dl->axis[1], dl->axis[2]);
		VectorInverse(dl->axis[1]);
	}

	else if (!strcmp(cmd, "avel") || !strcmp(cmd, "spin"))
	{
		dl->rotation[0] = atof(x);
		dl->rotation[1] = atof(y);
		dl->rotation[2] = atof(z);
	}
	else if (!strcmp(cmd, "avelx"))
		dl->rotation[0] = atof(x);
	else if (!strcmp(cmd, "avey"))
		dl->rotation[1] = atof(x);
	else if (!strcmp(cmd, "avelz"))
		dl->rotation[2] = atof(x);

	else if (!strcmp(cmd, "outercone") || !strcmp(cmd, "fov") || !strcmp(cmd, "cone"))
		dl->fov = atof(x);
	else if (!strcmp(cmd, "nearclip"))
		dl->nearclip = atof(x);
	else if (!strcmp(cmd, "color") || !strcmp(cmd, "colour"))
	{
		dl->color[0] = atof(x);
		dl->color[1] = atof(y);
		dl->color[2] = atof(z);
	}
	else if (!strcmp(cmd, "colorscale") || !strcmp(cmd, "colourscale"))
	{
		dl->color[0] *= atof(x);
		dl->color[1] *= atof(y);
		dl->color[2] *= atof(z);
	}
	else if (!strcmp(cmd, "radius"))
		dl->radius = atof(x);
	else if (!strcmp(cmd, "radiusscale") || !strcmp(cmd, "sizescale"))
		dl->radius *= atof(x);
	else if (!strcmp(cmd, "style"))
		dl->style = atoi(x);
	else if (!strcmp(cmd, "stylestring"))
	{
		Z_Free(dl->customstyle);
		dl->customstyle = x?Z_StrDup(x):NULL;
	}
	else if (!strcmp(cmd, "cubemap"))
	{
		Q_strncpyz(dl->cubemapname, x, sizeof(dl->cubemapname));
		if (*dl->cubemapname)
			dl->cubetexture = R_LoadReplacementTexture(dl->cubemapname, "", IF_TEXTYPE_CUBE, NULL, 0, 0, TF_INVALID);
		else
			dl->cubetexture = r_nulltex;
	}
	else if (!strcmp(cmd, "shadows"))
		dl->flags = (dl->flags&~LFLAG_NOSHADOWS) | ((*x=='y'||*x=='Y'||*x=='t'||atoi(x))?0:LFLAG_NOSHADOWS);
	else if (!strcmp(cmd, "nostencil"))
		dl->flags = (dl->flags&~LFLAG_SHADOWMAP) | ((*x=='y'||*x=='Y'||*x=='t'||atoi(x))?0:LFLAG_SHADOWMAP);
	else if (!strcmp(cmd, "crepuscular"))
		dl->flags = (dl->flags&~LFLAG_CREPUSCULAR) | ((*x=='y'||*x=='Y'||*x=='t'||atoi(x))?LFLAG_CREPUSCULAR:0);
	else if (!strcmp(cmd, "ortho"))
		dl->flags = (dl->flags&~LFLAG_ORTHO) | ((*x=='y'||*x=='Y'||*x=='t'||atoi(x))?LFLAG_ORTHO:0);
	else if (!strcmp(cmd, "corona"))
		dl->corona = atof(x);
	else if (!strcmp(cmd, "coronasize"))
		dl->coronascale = atof(x);
	else if (!strcmp(cmd, "ambient"))
		dl->lightcolourscales[0] = atof(x);
	else if (!strcmp(cmd, "diffuse"))
		dl->lightcolourscales[1] = atof(x);
	else if (!strcmp(cmd, "specular"))
		dl->lightcolourscales[2] = atof(x);
	else if (!strcmp(cmd, "normalmode"))
		dl->flags = (dl->flags&~LFLAG_NORMALMODE) | ((*x=='y'||*x=='Y'||*x=='t'||atoi(x))?LFLAG_NORMALMODE:0);
	else if (!strcmp(cmd, "realtimemode"))
		dl->flags = (dl->flags&~LFLAG_REALTIMEMODE) | ((*x=='y'||*x=='Y'||*x=='t'||atoi(x))?LFLAG_REALTIMEMODE:0);
	else
		return -2;
	dl->rebuildcache = true;	//mneh, lets just flag it for everything.
	return 1;
}

void R_EditLights_DrawInfo(void)
{
	float fontscale[2] = {8,8};
	float x = vid.width - 320;
	float y = 0;
	const char *s;
	if (!r_editlights.ival)
		return;

	if (r_editlights_selected >= RTL_FIRST && r_editlights_selected < rtlights_max)
	{
		dlight_t *dl = &cl_dlights[r_editlights_selected];
		s = va(	"      Origin : %.0f %.0f %.0f\n"
				"      Angles : %.0f %.0f %.0f\n"
				"      Colour : %.2f %.2f %.2f\n"
				"      Radius : %.0f\n"
				"      Corona : %.0f\n"
				"       Style : %i\n"
				"Style String : %s\n"
				"     Shadows : %s\n"
				"     Cubemap : \"%s\"\n"
				"  CoronaSize : %.2f\n"
				"     Ambient : %.2f\n"
				"     Diffuse : %.2f\n"
				"    Specular : %.2f\n"
				"  NormalMode : %s\n"
				"RealTimeMode : %s\n"
				"    FadeDist : %.0f-%.0f\n"
				"        Spin : %.0f %.0f %.0f\n"
				"        Cone : %.0f\n"
				"    Nearclip : %.0f\n"
				//"NoStencil    : %s\n"
				//"Crepuscular  : %s\n"
				//"Ortho        : %s\n"
				,dl->origin[0],dl->origin[1],dl->origin[2]
				,dl->angles[0],dl->angles[1],dl->angles[2]
				,dl->color[0],dl->color[1],dl->color[2]
				,dl->radius, dl->corona, dl->style, dl->customstyle?dl->customstyle:"---"
				,((dl->flags&LFLAG_NOSHADOWS)?"no":"yes"), dl->cubemapname, dl->coronascale
				,dl->lightcolourscales[0], dl->lightcolourscales[1], dl->lightcolourscales[2]
				,((dl->flags&LFLAG_NORMALMODE)?"yes":"no"), ((dl->flags&LFLAG_REALTIMEMODE)?"yes":"no")
				,dl->fade[0], dl->fade[1]
				,dl->rotation[0],dl->rotation[1],dl->rotation[2], dl->fov, dl->nearclip
				//,((dl->flags&LFLAG_SHADOWMAP)?"no":"yes"),((dl->flags&LFLAG_CREPUSCULAR)?"yes":"no"),((dl->flags&LFLAG_ORTHO)?"yes":"no")
				);
	}
	else
		s = "No light selected";
	R2D_ImageColours(0,0,0,.35);
	R2D_FillBlock(x-4, y, 320+4, 16*8+4);
	R2D_ImageColours(1,1,1,1);
	R_DrawTextField(x, y, 320, 19*8, s, CON_WHITEMASK, CPRINT_LALIGN|CPRINT_TALIGN|CPRINT_NOWRAP, font_default, fontscale);
}
void R_EditLights_DrawLights(void)
{
	const float SPRITE_SIZE = 8;
	int		i;
	dlight_t	*l;
	enum
	{
//		ELS_CURSOR,
		ELS_SELECTED,
		ELS_LIGHT,
		ELS_NOSHADOW,
		ELS_MAX
	};
	char *lightshaderinfo[] =
	{
/*		"gfx/editlights/cursor",
			".59..95."
			"59....95"
			"9.9..9.9"
			"...99..."
			"...99..."
			"9.9..9.9"
			"59....95"
			".59..95.",
*/
		"gfx/editlights/selected",
			"999..999"
			"99....99"
			"9......9"
			"........"
			"........"
			"9......9"
			"99....99"
			"999..999",

		"gfx/editlights/light",
			"..1221.."
			".245542."
			"14677641"
			"25799752"
			"25799752"
			"14677641"
			".245542."
			"..1221..",

		"gfx/editlights/noshadow",
			"..1221.."
			".245542."
			"14644641"
			"274..472"	//mmm, donuts.
			"274..472"
			"14644641"
			".247742."
			"..1221..",
	};
	shader_t *shaders[ELS_MAX], *s;
	unsigned int asciipalette[256];
	asciipalette['.'] = 0;
	for (i = 0; i < 10; i++)
		asciipalette['0'+i] = 0xff000000 | ((int)(255/9.0*i)*0x010101);

	if (!r_editlights.ival)
		return;

	for (i = 0; i < ELS_MAX; i++)
	{
		shaders[i] = R_RegisterShader(lightshaderinfo[i*2+0], SUF_NONE, va(
				"{\n"
					"program defaultadditivesprite\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc gl_one gl_one\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"%s"
					"}\n"
				"}\n"
				,(i==ELS_SELECTED)?"nodepth\n":"")
			);
		if (!shaders[i]->defaulttextures->base)
			shaders[i]->defaulttextures->base = Image_GetTexture(shaders[i]->name, NULL, IF_LINEAR|IF_NOMIPMAP|IF_NOPICMIP|IF_CLAMP, lightshaderinfo[i*2+1], asciipalette, 8, 8, TF_8PAL32);
	}

	if (!r_editlights_locked)
	{
		vec3_t targ, norm;
		int ent;
		int best = -1;
		float bestscore = 0, score;

		VectorMA(r_refdef.vieworg, r_editlights_cursordistance.value, vpn, targ);	//try to aim about 1024qu infront of the camera
		CL_TraceLine(r_refdef.vieworg, targ, r_editlights_cursor, norm, &ent);		//figure out where the cursor ends up
		VectorMA(r_editlights_cursor, r_editlights_cursorpushoff.value, norm, r_editlights_cursor);	//push off from the surface by 4qu.
		VectorMA(r_editlights_cursor, -r_editlights_cursorpushback.value, vpn, r_editlights_cursor);//move it back towards the camera, for no apparent reason
		if (r_editlights_cursorgrid.value)
		{	//snap to a grid, if set
			for (i =0; i < 3; i++)
				r_editlights_cursor[i] = floor(r_editlights_cursor[i] / r_editlights_cursorgrid.value + 0.5) * r_editlights_cursorgrid.value;
		}

//		CLQ1_AddSpriteQuad(shaders[ELS_CURSOR], r_editlights_cursor, SPRITE_SIZE);

		for (i=RTL_FIRST; i<rtlights_max; i++)
		{
			l = &cl_dlights[i];
			if (!l->radius)	//dead light is dead.
				continue;

			VectorSubtract(l->origin, r_refdef.vieworg, targ);
			score = DotProduct(vpn, targ) / sqrt(DotProduct(targ,targ));
			if (score >= .95)	//there's a threshhold required for a light to be selectable.
			{
				//trace from the light to the view (so startsolid doesn't cause so many problems)
				if (score > bestscore && CL_TraceLine(l->origin, r_refdef.vieworg, r_editlights_cursor, norm, &ent) == 1.0)
				{
					bestscore = score;
					best = i;
				}
			}
		}
		r_editlights_selected = best;
	}

	for (i=RTL_FIRST; i<rtlights_max; i++)
	{
		l = &cl_dlights[i];
		if (!l->radius)	//dead light is dead.
			continue;

		//we should probably show spotlights with a special icon or something
		//dp has alternate icons for cubemaps.
		if (l->flags & LFLAG_NOSHADOWS)
			s = shaders[ELS_NOSHADOW];
		else
			s = shaders[ELS_LIGHT];
		CLQ1_AddSpriteQuad(s, l->origin, SPRITE_SIZE);
	}

	if (r_editlights_selected >= RTL_FIRST && r_editlights_selected < rtlights_max)
	{
		l = &cl_dlights[r_editlights_selected];
		CLQ1_AddSpriteQuad(shaders[ELS_SELECTED], l->origin, SPRITE_SIZE);
	}
}

static void R_EditLights_Edit_f(void)
{
	int i = r_editlights_selected;
	const char *cmd = Cmd_Argv(1);
	const char *x = Cmd_Argv(2);
	const char *y = Cmd_Argv(3);
	const char *z = Cmd_Argv(4);
	int argc = Cmd_Argc()-2;
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
	{
		Con_Printf("No light selected\n");
		return;
	}
	dl = &cl_dlights[i];
	if (!*cmd)
	{
		Con_Print("Selected light's properties:\n");
		Con_Printf("Origin       : ^[%f %f %f\\type\\r_editlights_edit origin %g %g %g^]\n", dl->origin[0],dl->origin[1],dl->origin[2], dl->origin[0],dl->origin[1],dl->origin[2]);
		Con_Printf("Angles       : ^[%f %f %f\\type\\r_editlights_edit angles %g %g %g^]\n", dl->angles[0],dl->angles[1],dl->angles[2], dl->angles[0],dl->angles[1],dl->angles[2]);
		Con_Printf("Colour       : ^[%f %f %f\\type\\r_editlights_edit avel %g %g %g^]\n", dl->color[0],dl->color[1],dl->color[2], dl->color[0],dl->color[1],dl->color[2]);
		Con_Printf("Radius       : ^[%f\\type\\r_editlights_edit radius %g^]\n", dl->radius, dl->radius);
		Con_Printf("Corona       : ^[%f\\type\\r_editlights_edit corona %g^]\n", dl->corona, dl->corona);
		Con_Printf("Style        : ^[%i\\type\\r_editlights_edit style %i^]\n", dl->style, dl->style);
		Con_Printf("Style String : ^[%s\\type\\r_editlights_edit stylestring %s^]\n", dl->customstyle?dl->customstyle:"---", dl->customstyle?dl->customstyle:"");
		Con_Printf("Shadows      : ^[%s\\type\\r_editlights_edit shadows %s^]\n", ((dl->flags&LFLAG_NOSHADOWS)?"no":"yes"), ((dl->flags&LFLAG_NOSHADOWS)?"no":"yes"));
		Con_Printf("Cubemap      : ^[\"%s\"\\type\\r_editlights_edit cubemap \"%s\"^]\n", dl->cubemapname, dl->cubemapname);
		Con_Printf("CoronaSize   : ^[%f\\type\\r_editlights_edit coronasize %g^]\n", dl->coronascale, dl->coronascale);
		Con_Printf("Ambient      : ^[%f\\type\\r_editlights_edit ambient %g^]\n", dl->lightcolourscales[0], dl->lightcolourscales[0]);
		Con_Printf("Diffuse      : ^[%f\\type\\r_editlights_edit diffuse %g^]\n", dl->lightcolourscales[1], dl->lightcolourscales[1]);
		Con_Printf("Specular     : ^[%f\\type\\r_editlights_edit specular %g^]\n", dl->lightcolourscales[2], dl->lightcolourscales[2]);
		Con_Printf("NormalMode   : ^[%s\\type\\r_editlights_edit normalmode %s^]\n", ((dl->flags&LFLAG_NORMALMODE)?"yes":"no"), ((dl->flags&LFLAG_NORMALMODE)?"yes":"no"));
		Con_Printf("RealTimeMode : ^[%s\\type\\r_editlights_edit realtimemode %s^]\n", ((dl->flags&LFLAG_REALTIMEMODE)?"yes":"no"), ((dl->flags&LFLAG_REALTIMEMODE)?"yes":"no"));
		Con_Printf("Spin         : ^[%f %f %f\\type\\r_editlights_edit avel %g %g %g^]\n", dl->rotation[0],dl->rotation[1],dl->rotation[2], dl->origin[0],dl->origin[1],dl->origin[2]);
		Con_Printf("Cone         : ^[%f\\type\\r_editlights_edit outercone %g^]\n", dl->fov, dl->fov);
		Con_Printf("NearClip     : ^[%f\\type\\r_editlights_edit nearclip %g^]\n", dl->nearclip, dl->nearclip);
//		Con_Printf("NoStencil    : ^[%s\\type\\r_editlights_edit nostencil %s^]\n", ((dl->flags&LFLAG_SHADOWMAP)?"no":"yes"), ((dl->flags&LFLAG_SHADOWMAP)?"no":"yes"));
//		Con_Printf("Crepuscular  : ^[%s\\type\\r_editlights_edit crepuscular %s^]\n", ((dl->flags&LFLAG_CREPUSCULAR)?"yes":"no"), ((dl->flags&LFLAG_CREPUSCULAR)?"yes":"no"));
//		Con_Printf("Ortho        : ^[%s\\type\\r_editlights_edit ortho %s^]\n", ((dl->flags&LFLAG_ORTHO)?"yes":"no"), ((dl->flags&LFLAG_ORTHO)?"yes":"no"));
		return;
	}
	switch(R_EditLight(dl, cmd, argc, x,y,z))
	{
	case -1:
		Con_Printf("Not enough args for %s\n", cmd);
		return;
	case -2:
		Con_Printf("Argument not known: %s\n", cmd);
		return;
	}
}
static void R_EditLights_Remove_f(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
	{
		Con_Printf("No light selected\n");
		return;
	}
	dl = &cl_dlights[i];
	dl->radius = 0;
	r_editlights_selected = -1;
}
static void R_EditLights_EditAll_f(void)
{
	int i = 0;
	const char *cmd = Cmd_Argv(1);
	const char *x = Cmd_Argv(2);
	const char *y = Cmd_Argv(3);
	const char *z = Cmd_Argv(4);
	int argc = Cmd_Argc()-2;
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("No light selected\n");
		return;
	}
	for (i = RTL_FIRST; i < rtlights_max; i++)
	{
		dl = &cl_dlights[i];
		if (dl->radius <= 0)
			continue;	//don't edit dead lights back to life
		switch(R_EditLight(dl, cmd, argc, x,y,z))
		{
		case -1:
			Con_Printf("Not enough args for %s\n", cmd);
			return;
		case -2:
			Con_Printf("Argument not known: %s\n", cmd);
			return;
		}
	}
}
static void R_EditLights_Spawn_f(void)
{
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	dl = CL_AllocSlight();
	r_editlights_selected = dl - cl_dlights;

	VectorCopy(r_editlights_cursor, dl->origin);
	dl->radius = 200;

	dl->style = 0;	//styled, but mostly static (we could use -1, but mneh).
	dl->lightcolourscales[0] = 0;
	dl->lightcolourscales[1] = 1;
	dl->lightcolourscales[2] = 1;
}
static void R_EditLights_Clone_f(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	dlight_t *src;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
	{
		Con_Printf("No light selected\n");
		return;
	}
	src = &cl_dlights[i];
	dl = CL_AllocSlight();
	r_editlights_selected = dl - cl_dlights;
	CL_CloneDlight(dl, src);

	VectorCopy(r_editlights_cursor, dl->origin);
}
static void R_EditLights_ToggleShadow_f(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
	{
		Con_Printf("No light selected\n");
		return;
	}
	dl = &cl_dlights[i];
	dl->flags ^= LFLAG_NOSHADOWS;
}
static void R_EditLights_ToggleCorona_f(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
	{
		Con_Printf("No light selected\n");
		return;
	}
	dl = &cl_dlights[i];
	dl->corona = !dl->corona;
}
static void R_EditLights_CopyInfo_f(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
		return;
	dl = &cl_dlights[i];
	CL_CloneDlight(&r_editlights_copybuffer, dl);
}
static void R_EditLights_PasteInfo_f(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	vec3_t org;
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}
	if (i < RTL_FIRST || i >= rtlights_max)
	{
		Con_Printf("No light selected\n");
		return;
	}
	dl = &cl_dlights[i];
	VectorCopy(dl->origin, org);
	CL_CloneDlight(dl, &r_editlights_copybuffer);
	VectorCopy(org, dl->origin);	//undo the origin's copy.

	//just in case its from a different map...
	if (*dl->cubemapname)
		dl->cubetexture = R_LoadReplacementTexture(dl->cubemapname, "", IF_TEXTYPE_CUBE, NULL, 0, 0, TF_INVALID);
	else
		dl->cubetexture = r_nulltex;
}

static void R_EditLights_Lock_f(void)
{
	if (!r_editlights.ival)
	{
		Con_Printf("Toggle r_editlights first\n");
		return;
	}

	if ((r_editlights_selected < RTL_FIRST || r_editlights_selected >= rtlights_max) && !r_editlights_locked)
	{
		Con_Printf("No light selected\n");
		return;
	}
	r_editlights_locked = !r_editlights_locked;
}

static char	macro_buf[256] = "";
static char *r_editlights_current_origin(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g %g %g", dl->origin[0], dl->origin[1], dl->origin[2]);
	return macro_buf;
}
static char *r_editlights_current_angles(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g %g %g", dl->angles[0], dl->angles[1], dl->angles[2]);
	return macro_buf;
}
static char *r_editlights_current_color(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g %g %g", dl->color[0], dl->color[1], dl->color[2]);
	return macro_buf;
}
static char *r_editlights_current_radius(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g", dl->radius);
	return macro_buf;
}
static char *r_editlights_current_corona(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g", dl->corona);
	return macro_buf;
}
static char *r_editlights_current_coronasize(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g", dl->coronascale);
	return macro_buf;
}
static char *r_editlights_current_style(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%i", dl->style);
	return macro_buf;
}
static char *r_editlights_current_shadows(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	if (dl->flags & LFLAG_NOSHADOWS)
		return "0";
	return "1";
}
static char *r_editlights_current_cubemap(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "\"%s\"", dl->cubemapname);
	return macro_buf;
}
static char *r_editlights_current_ambient(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g", dl->lightcolourscales[0]);
	return macro_buf;
}
static char *r_editlights_current_diffuse(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g", dl->lightcolourscales[1]);
	return macro_buf;
}
static char *r_editlights_current_specular(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	Q_snprintfz (macro_buf, sizeof(macro_buf), "%g", dl->lightcolourscales[2]);
	return macro_buf;
}
static char *r_editlights_current_normalmode(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	if (dl->flags & LFLAG_NORMALMODE)
		return "1";
	return "0";
}
static char *r_editlights_current_realtimemode(void)
{
	int i = r_editlights_selected;
	dlight_t *dl;
	if (i < RTL_FIRST || i >= rtlights_max)
		return "";
	dl = &cl_dlights[i];

	if (dl->flags & LFLAG_REALTIMEMODE)
		return "1";
	return "0";
}

void R_EditLights_RegisterCommands(void)
{
	Cmd_AddCommandD ("r_editlights_reload", R_ReloadRTLights_f, "Reload static rtlights. Argument can be rtlights|statics|bsp|none to override the source.");
	Cmd_AddCommandD ("r_editlights_save", R_SaveRTLights_f, "Saves rtlights to maps/FOO.rtlights");
	Cvar_Register (&r_editlights_import_radius,		"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_import_ambient,	"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_import_diffuse,	"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_import_specular,	"Realtime Light editing/importing");
	Cvar_Register (&r_shadow_realtime_world_importlightentitiesfrommap,	"Realtime Light editing/importing");

	Cvar_Register (&r_editlights,					"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_cursordistance,	"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_cursorpushoff,		"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_cursorpushback,	"Realtime Light editing/importing");
	Cvar_Register (&r_editlights_cursorgrid,		"Realtime Light editing/importing");

	//the rest is optional stuff that should normally be handled via csqc instead, but hurrah for dp compat...
	Cmd_AddCommandD("r_editlights_spawn", R_EditLights_Spawn_f, "Spawn a new light with default properties");
	Cmd_AddCommandD("r_editlights_clone", R_EditLights_Clone_f, "Duplicate the current light (with a new origin)");
	Cmd_AddCommandD("r_editlights_remove", R_EditLights_Remove_f, "Removes the current light.");
	Cmd_AddCommandD("r_editlights_edit", R_EditLights_Edit_f, "Changes named properties on the current light.");
	Cmd_AddCommandD("r_editlights_editall", R_EditLights_EditAll_f, "Like r_editlights_edit, but affects all lights instead of just the selected one.");
	Cmd_AddCommandD("r_editlights_toggleshadow", R_EditLights_ToggleShadow_f, "Toggles the shadow flag on the current light.");
	Cmd_AddCommandD("r_editlights_togglecorona", R_EditLights_ToggleCorona_f, "Toggles the current light's corona field.");
	Cmd_AddCommandD("r_editlights_copyinfo", R_EditLights_CopyInfo_f, "store a copy of all properties (except origin) of the selected light");
	Cmd_AddCommandD("r_editlights_pasteinfo", R_EditLights_PasteInfo_f, "apply the stored properties onto the selected light (making it exactly identical except for origin)");
	Cmd_AddCommandD("r_editlights_lock", R_EditLights_Lock_f, "Blocks changing the current light according the crosshair.");

	//DP has these as cvars. mneh.
	Cmd_AddMacroD("r_editlights_current_origin",	r_editlights_current_origin,	false, "origin of selected light");
	Cmd_AddMacroD("r_editlights_current_angles",	r_editlights_current_angles,	false, "angles of selected light");
	Cmd_AddMacroD("r_editlights_current_color",		r_editlights_current_color,		false, "color of selected light");
	Cmd_AddMacroD("r_editlights_current_radius",	r_editlights_current_radius,	false, "radius of selected light");
	Cmd_AddMacroD("r_editlights_current_corona",	r_editlights_current_corona,	false, "corona intensity of selected light");
	Cmd_AddMacroD("r_editlights_current_coronasize",r_editlights_current_coronasize,false, "corona size of selected light");
	Cmd_AddMacroD("r_editlights_current_style",		r_editlights_current_style,		false, "style of selected light");
	Cmd_AddMacroD("r_editlights_current_shadows",	r_editlights_current_shadows,	false, "shadows flag of selected light");
	Cmd_AddMacroD("r_editlights_current_cubemap",	r_editlights_current_cubemap,	false, "cubemap of selected light");
	Cmd_AddMacroD("r_editlights_current_ambient",	r_editlights_current_ambient,	false, "ambient intensity of selected light");
	Cmd_AddMacroD("r_editlights_current_diffuse",	r_editlights_current_diffuse,	false, "diffuse intensity of selected light");
	Cmd_AddMacroD("r_editlights_current_specular",	r_editlights_current_specular,	false, "specular intensity of selected light");
	Cmd_AddMacroD("r_editlights_current_normalmode",r_editlights_current_normalmode,false, "normalmode flag of selected light");
	Cmd_AddMacroD("r_editlights_current_realtimemode",	r_editlights_current_realtimemode,	false, "realtimemode flag of selected light");
}
#endif

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mplane_t		*lightplane;
vec3_t			lightspot;

static void GLQ3_AddLatLong(const qbyte latlong[2], vec3_t dir, float mag)
{
	float lat = (float)latlong[0] * (2 * M_PI)*(1.0 / 255.0);
	float lng = (float)latlong[1] * (2 * M_PI)*(1.0 / 255.0);
	dir[0] += mag * cos ( lng ) * sin ( lat );
	dir[1] += mag * sin ( lng ) * sin ( lat );
	dir[2] += mag * cos ( lat );
}

void GLQ3_LightGrid(model_t *mod, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	q3lightgridinfo_t *lg = (q3lightgridinfo_t *)mod->lightgrid;
	int index[8];
	int vi[3];
	int i, j;
	float t[8];
	vec3_t vf, vf2;
	vec3_t ambient, diffuse, direction;

	if (!lg || (!lg->lightgrid && !lg->rbspelements) || lg->numlightgridelems < 1)
	{
		if(res_ambient)
		{
			res_ambient[0] = 64;
			res_ambient[1] = 64;
			res_ambient[2] = 64;
		}

		if (res_diffuse)
		{
			res_diffuse[0] = 192;
			res_diffuse[1] = 192;
			res_diffuse[2] = 192;
		}

		if (res_dir)
		{
			res_dir[0] = 1;
			res_dir[1] = 1;
			res_dir[2] = 0.1;
		}
		return;
	}

	//If in doubt, steal someone else's code...
	//Thanks QFusion.

	for ( i = 0; i < 3; i++ )
	{
		vf[i] = (point[i] - lg->gridMins[i]) / lg->gridSize[i];
		vi[i] = (int)(vf[i]);
		vf[i] = vf[i] - floor(vf[i]);
		vf2[i] = 1.0f - vf[i];
	}

	for ( i = 0; i < 8; i++ )
	{
		//bound it properly
		index[i] =	bound(0, vi[0]+((i&1)?1:0), lg->gridBounds[0]-1) * 1                 +
					bound(0, vi[1]+((i&2)?1:0), lg->gridBounds[1]-1) * lg->gridBounds[0] +
					bound(0, vi[2]+((i&4)?1:0), lg->gridBounds[2]-1) * lg->gridBounds[3] ;
		t[i] =	((i&1)?vf[0]:vf2[0]) *
				((i&2)?vf[1]:vf2[1]) *
				((i&4)?vf[2]:vf2[2]) ;
	}

	//rbsp has a separate grid->index lookup for compression.
	if (lg->rbspindexes)
	{
		for (i = 0; i < 8; i++)
			index[i] = lg->rbspindexes[index[i]];
	}

	VectorClear(ambient);
	VectorClear(diffuse);
	VectorClear(direction);
	if (lg->rbspelements)
	{
		for (i = 0; i < 8; i++)
		{	//rbsp has up to 4 styles per grid element, which needs to be scaled by that style's current value
			float tot = 0;
			for (j = 0; j < countof(lg->rbspelements[index[i]].styles); j++)
			{
				qbyte st = lg->rbspelements[index[i]].styles[j];
				if (st != 255)
				{
					float mag = d_lightstylevalue[st] * 1.0/255 * t[i];
					//FIXME: cl_lightstyle[st].colours[rgb]
					VectorMA (ambient,      mag, lg->rbspelements[index[i]].ambient[j],   ambient);
					VectorMA (diffuse,      mag, lg->rbspelements[index[i]].diffuse[j],   diffuse);
					tot += mag;
				}
			}
			GLQ3_AddLatLong(lg->rbspelements[index[i]].direction, direction, tot);
		}
	}
	else
	{
		for (i = 0; i < 8; i++)
		{
			VectorMA (ambient,      t[i], lg->lightgrid[index[i]].ambient,   ambient);
			VectorMA (diffuse,      t[i], lg->lightgrid[index[i]].diffuse,   diffuse);
			GLQ3_AddLatLong(lg->lightgrid[index[i]].direction, direction, t[i]);
		}

		VectorScale(ambient, d_lightstylevalue[0]/255.0, ambient);
		VectorScale(diffuse, d_lightstylevalue[0]/255.0, diffuse);
		//FIXME: cl_lightstyle[0].colours[rgb]
	}

	//q3bsp has *4 overbrighting.
//	VectorScale(ambient, 4, ambient);
//	VectorScale(diffuse, 4, diffuse);

	/*ambient is the min level*/
	/*diffuse is the max level*/
	VectorCopy(ambient, res_ambient);
	if (res_diffuse)
		VectorAdd(diffuse, ambient, res_diffuse);
	if (res_dir)
		VectorCopy(direction, res_dir);
}

static int GLRecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	int			r;
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	qbyte		*lightmap;
	unsigned	scale;
	int			maps;

	if (cl.worldmodel->fromgame == fg_quake2)
	{
		if (node->contents != -1)
			return -1;		// solid
	}
	else if (node->contents < 0)
		return -1;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return GLRecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	r = GLRecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing
		
// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;
		
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= surf->lmshift;
		dt >>= surf->lmshift;

		lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{
			switch(cl.worldmodel->lightmaps.fmt)
			{
			case LM_E5BGR9:
				lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)<<2;
				for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
				{
					unsigned int l = *(unsigned int*)lightmap;
					scale = d_lightstylevalue[surf->styles[maps]];
					scale *= pow(2, (int)(l>>27)-15-9+7);

					r += max3(((l>> 0)&0x1ff), ((l>> 9)&0x1ff), ((l>>18)&0x1ff)) * scale;

					lightmap += ((surf->extents[0]>>surf->lmshift)+1) * ((surf->extents[1]>>surf->lmshift)+1)<<2;
				}
				break;
			case LM_RGB8:
				lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)*3;
				for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
				{
					scale = d_lightstylevalue[surf->styles[maps]];
					r += max3(lightmap[0],lightmap[1],lightmap[2]) * scale;
					lightmap += ((surf->extents[0]>>surf->lmshift)+1) * ((surf->extents[1]>>surf->lmshift)+1)*3;
				}
				break;
			case LM_L8:
				lightmap += dt * ((surf->extents[0]>>surf->lmshift)+1) + ds;
				for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
				{
					scale = d_lightstylevalue[surf->styles[maps]];
					r += *lightmap * scale;
					lightmap += ((surf->extents[0]>>surf->lmshift)+1) * ((surf->extents[1]>>surf->lmshift)+1);
				}
				break;
			}
			
			r >>= 8;
		}
		
		return r;
	}

// go down back side
	return GLRecursiveLightPoint (node->children[!side], mid, end);
}



int R_LightPoint (vec3_t p)
{
	vec3_t		end;
	int			r;

	if (r_refdef.flags & 1)
		return 255;

	if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED || !cl.worldmodel->lightdata)
		return 255;

	if (cl.worldmodel->fromgame == fg_quake3)
	{
		GLQ3_LightGrid(cl.worldmodel, p, NULL, end, NULL);
		return (end[0] + end[1] + end[2])/3;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - mod_lightpoint_distance.value;

	r = GLRecursiveLightPoint (cl.worldmodel->rootnode, p, end);
	
	if (r == -1)
		r = 0;

	return r;
}



#ifdef PEXT_LIGHTSTYLECOL

static float *GLRecursiveLightPoint3C (model_t *mod, mnode_t *node, const vec3_t start, const vec3_t end)
{
	static float l[6];
	float *r;
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	vec4_t	*lmvecs;
	qbyte		*lightmap, *deluxmap;
	float	scale, overbright;
	int			maps;

	if (mod->fromgame == fg_quake2)
	{
		if (node->contents != -1)
			return NULL;		// solid
	}
	else if (node->contents < 0)
		return NULL;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;

	if ( (back < 0) == side)
		return GLRecursiveLightPoint3C (mod, node->children[side], start, end);

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	r = GLRecursiveLightPoint3C (mod, node->children[side], start, mid);
	if (r && r[0]+r[1]+r[2] >= 0)
		return r;		// hit something

	if ( (back < 0) == side )
		return NULL;		// didn't hit anuthing

// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = mod->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		if (mod->facelmvecs)
			lmvecs = mod->facelmvecs[surf-mod->surfaces].lmvecs;
		else
			lmvecs = surf->texinfo->vecs;
		
		s = DotProduct (mid, lmvecs[0]) + lmvecs[0][3];
		t = DotProduct (mid, lmvecs[1]) + lmvecs[1][3];

		if (s < surf->texturemins[0] ||
			t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
		{
			l[0]=0;l[1]=0;l[2]=0;
			l[3]=0;l[4]=1;l[5]=1;
			return l;
		}

		ds >>= surf->lmshift;
		dt >>= surf->lmshift;

		lightmap = surf->samples;
		l[0]=0;l[1]=0;l[2]=0;
		l[3]=0;l[4]=0;l[5]=0;
		if (lightmap)
		{
			overbright = 1/255.0f;
			if (mod->deluxdata)
			{
				switch(mod->lightmaps.fmt)
				{
				case LM_E5BGR9:
					deluxmap = ((surf->samples - mod->lightdata)>>2)*3 + mod->deluxdata;

					lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)<<2;
					deluxmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)*3;
					for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
					{
						unsigned int lm = *(unsigned int*)lightmap;
						scale = d_lightstylevalue[surf->styles[maps]]*overbright;
						scale *= pow(2, (int)(lm>>27)-15-9+8);

						l[0] += ((lm>> 0)&0x1ff) * scale * cl_lightstyle[surf->styles[maps]].colours[0];
						l[1] += ((lm>> 9)&0x1ff) * scale * cl_lightstyle[surf->styles[maps]].colours[1];
						l[2] += ((lm>>18)&0x1ff) * scale * cl_lightstyle[surf->styles[maps]].colours[2];

						l[3] += (deluxmap[0]-127)*scale;
						l[4] += (deluxmap[1]-127)*scale;
						l[5] += (deluxmap[2]-127)*scale;

						lightmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1)<<2;
						deluxmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1) * 3;
					}
					break;
				case LM_RGB8:
					deluxmap = surf->samples - mod->lightdata + mod->deluxdata;

					lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)*3;
					deluxmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)*3;
					for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
					{
						scale = d_lightstylevalue[surf->styles[maps]]*overbright;

						l[0] += lightmap[0] * scale * cl_lightstyle[surf->styles[maps]].colours[0];
						l[1] += lightmap[1] * scale * cl_lightstyle[surf->styles[maps]].colours[1];
						l[2] += lightmap[2] * scale * cl_lightstyle[surf->styles[maps]].colours[2];

						l[3] += (deluxmap[0]-127)*scale;
						l[4] += (deluxmap[1]-127)*scale;
						l[5] += (deluxmap[2]-127)*scale;

						lightmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1) * 3;
						deluxmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1) * 3;
					}
					break;
				case LM_L8:
					deluxmap = (surf->samples - mod->lightdata)*3 + mod->deluxdata;

					lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds);
					deluxmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)*3;
					for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
					{
						scale = d_lightstylevalue[surf->styles[maps]]*overbright;

						l[0] += *lightmap * scale * cl_lightstyle[surf->styles[maps]].colours[0];
						l[1] += *lightmap * scale * cl_lightstyle[surf->styles[maps]].colours[1];
						l[2] += *lightmap * scale * cl_lightstyle[surf->styles[maps]].colours[2];

						l[3] += deluxmap[0]*scale;
						l[4] += deluxmap[1]*scale;
						l[5] += deluxmap[2]*scale;

						lightmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1);
						deluxmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1) * 3;
					}
					break;
				}

			}
			else
			{
				switch(mod->lightmaps.fmt)
				{
				case LM_E5BGR9:
					lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)<<2;
					for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
					{
						unsigned int lm = *(unsigned int*)lightmap;
						scale = d_lightstylevalue[surf->styles[maps]]*overbright;
						scale *= pow(2, (int)(lm>>27)-15-9+8);

						l[0] += ((lm>> 0)&0x1ff) * scale * cl_lightstyle[surf->styles[maps]].colours[0];
						l[1] += ((lm>> 9)&0x1ff) * scale * cl_lightstyle[surf->styles[maps]].colours[1];
						l[2] += ((lm>>18)&0x1ff) * scale * cl_lightstyle[surf->styles[maps]].colours[2];

						lightmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1)<<2;
					}
					break;
				case LM_RGB8:
					lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds)*3;
					for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
					{
						scale = d_lightstylevalue[surf->styles[maps]]*overbright;

						l[0] += lightmap[0] * scale * cl_lightstyle[surf->styles[maps]].colours[0];
						l[1] += lightmap[1] * scale * cl_lightstyle[surf->styles[maps]].colours[1];
						l[2] += lightmap[2] * scale * cl_lightstyle[surf->styles[maps]].colours[2];

						lightmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1) * 3;
					}
					break;
				case LM_L8:
					lightmap += (dt * ((surf->extents[0]>>surf->lmshift)+1) + ds);
					for (maps = 0 ; maps < MAXCPULIGHTMAPS && surf->styles[maps] != INVALID_LIGHTSTYLE ; maps++)
					{
						scale = d_lightstylevalue[surf->styles[maps]]*overbright;

						l[0] += *lightmap * scale * cl_lightstyle[surf->styles[maps]].colours[0];
						l[1] += *lightmap * scale * cl_lightstyle[surf->styles[maps]].colours[1];
						l[2] += *lightmap * scale * cl_lightstyle[surf->styles[maps]].colours[2];

						lightmap += ((surf->extents[0]>>surf->lmshift)+1) *
								((surf->extents[1]>>surf->lmshift)+1);
					}
					break;
				}
			}
		}
		
		return l;
	}

// go down back side
	return GLRecursiveLightPoint3C (mod, node->children[!side], mid, end);
}

#endif

typedef struct
{
	vec3_t gridscale;
	unsigned int count[3];
	vec3_t mins;
	unsigned int styles;

	unsigned int rootnode;

	unsigned int numnodes;
	struct bspxlgnode_s
	{	//this uses an octtree to trim samples.
		int mid[3];
		unsigned int child[8];
#define LGNODE_LEAF		(1u<<31)
#define LGNODE_MISSING	(1u<<30)
	} *nodes;
	unsigned int numleafs;
	struct bspxlgleaf_s
	{
		int mins[3];
		int size[3];
		unsigned char numstyles;
		struct bspxlgsamp_s
		{
			qbyte style;
			qbyte rgb[3];
		} *rgbvalues;	//size[x*y*z]*numstyles
	} *leafs;
} bspxlightgrid_t;
struct rctx_s {qbyte *data; size_t ofs, size;};
static qbyte ReadByte(struct rctx_s *ctx)
{
	if (ctx->ofs >= ctx->size)
	{
		ctx->ofs++;
		return 0;
	}
	return ctx->data[ctx->ofs++];
}
static int ReadInt(struct rctx_s *ctx)
{
	int r = (int)ReadByte(ctx)<<0;
		r|= (int)ReadByte(ctx)<<8;
		r|= (int)ReadByte(ctx)<<16;
		r|= (int)ReadByte(ctx)<<24;
	return r;
}
static float ReadFloat(struct rctx_s *ctx)
{
	union {float f; int i;} u;
	u.i = ReadInt(ctx);
	return u.f;
}
void BSPX_LightGridLoad(model_t *model, bspx_header_t *bspx, qbyte *mod_base)
{
	vec3_t step, mins;
	int size[3];
	bspxlightgrid_t *grid;
	unsigned int numstyles, numnodes, numleafs, rootnode;
	unsigned int nodestart, leafsamps = 0, i, j, k, s;
	struct bspxlgsamp_s *samp;
	struct rctx_s ctx = {0};
	ctx.data = BSPX_FindLump(bspx, mod_base, "LIGHTGRID_OCTREE", &ctx.size);
	model->lightgrid = NULL;
	if (!ctx.data)
		return;

	for (j = 0; j < 3; j++)
		step[j] = ReadFloat(&ctx);
	for (j = 0; j < 3; j++)
		size[j] = ReadInt(&ctx);
	for (j = 0; j < 3; j++)
		mins[j] = ReadFloat(&ctx);

	numstyles = ReadByte(&ctx);	//urgh, misaligned the entire thing
	rootnode = ReadInt(&ctx);
	numnodes = ReadInt(&ctx);
	nodestart = ctx.ofs;
	ctx.ofs += (3+8)*4*numnodes;
	numleafs = ReadInt(&ctx);
	for (i = 0; i < numleafs; i++)
	{
		unsigned int ms = 1;
		unsigned int lsz[3];
		ctx.ofs += 3*4;
		for (j = 0; j < 3; j++)
			lsz[j] = ReadInt(&ctx);
		j = lsz[0]*lsz[1]*lsz[2];
		while (j --> 0)
		{	//this loop is annonying, memcpy dreams...
			s = ReadByte(&ctx);
			if (s == 255)
				continue;
			if (ms < s)
				ms = s;
			ctx.ofs += s*4;
		}
		j = lsz[0]*lsz[1]*lsz[2];
		leafsamps += j*ms;
	}

	grid = ZG_Malloc(&model->memgroup, sizeof(*grid) + sizeof(*grid->leafs)*numleafs + sizeof(*grid->nodes)*numnodes + sizeof(struct bspxlgsamp_s)*leafsamps);
//	memset(grid, 0xcc, sizeof(*grid) + sizeof(*grid->leafs)*numleafs + sizeof(*grid->nodes)*numnodes + sizeof(struct bspxlgsamp_s)*leafsamps);
	grid->leafs = (void*)(grid+1);
	grid->nodes = (void*)(grid->leafs + numleafs);
	samp = (void*)(grid->nodes+numnodes);

	for (j = 0; j < 3; j++)
		grid->gridscale[j] = 1/step[j];	//prefer it as a multiply
	VectorCopy(mins, grid->mins);
	VectorCopy(size, grid->count);
	grid->numnodes = numnodes;
	grid->numleafs = numleafs;
	grid->rootnode = rootnode;
	(void)numstyles;

	//rewind to the nodes. *sigh*
	ctx.ofs = nodestart;
	for (i = 0; i < numnodes; i++)
	{
		for (j = 0; j < 3; j++)
			grid->nodes[i].mid[j] = ReadInt(&ctx);
		for (j = 0; j < 8; j++)
			grid->nodes[i].child[j] = ReadInt(&ctx);
	}
	ctx.ofs += 4;
	for (i = 0; i < numleafs; i++)
	{
		unsigned int leafdataofs;
		unsigned int ms = 1;
		for (j = 0; j < 3; j++)
			grid->leafs[i].mins[j] = ReadInt(&ctx);
		for (j = 0; j < 3; j++)
			grid->leafs[i].size[j] = ReadInt(&ctx);

		grid->leafs[i].rgbvalues = samp;

		j = grid->leafs[i].size[0]*grid->leafs[i].size[1]*grid->leafs[i].size[2];

		//Count the maximum styles needed for this leaf.
		leafdataofs = ctx.ofs;
		for (k = 0; k < j; k++)
		{
			s = ReadByte(&ctx);
			if (s == 255)
				continue;
			if (ms < s)
				ms = s;
			ctx.ofs += s*4;
		}
		ctx.ofs = leafdataofs;
		grid->leafs[i].numstyles = ms;

		while (j --> 0)
		{
			s = ReadByte(&ctx);
			if (s == 0xff)
				memset(samp, 0xff, sizeof(*samp));
			else
			{
				for (k = 0; k < s; k++)
				{
					if (k >= ms)
						ReadInt(&ctx);	//shouldn't be possible...
					else
					{
						samp[k].style = ReadByte(&ctx);
						samp[k].rgb[0] = ReadByte(&ctx);
						samp[k].rgb[1] = ReadByte(&ctx);
						samp[k].rgb[2] = ReadByte(&ctx);
					}
				}
				for (; k < ms; k++)
				{
					samp[k].style = k?(qbyte)~0u:0;
					samp[k].rgb[0] =
					samp[k].rgb[1] =
					samp[k].rgb[2] = 0;
				}
			}
			samp+=ms;
		}
	}

	if (ctx.ofs != ctx.size)
		grid = NULL;

	model->lightgrid = (void*)grid;
}
static float BSPX_LightGridSingleValue(bspxlightgrid_t *grid, int x, int y, int z, float w, vec3_t res_diffuse)
{
	int i;
	unsigned int node;
	struct bspxlgsamp_s *samp;
	float lev;

	node = grid->rootnode;
	while (!(node & LGNODE_LEAF))
	{
		struct bspxlgnode_s *n;
		if (node >= grid->numnodes) // also node&LGNODE_MISSING
			return 0;	//failure
		n = grid->nodes + node;
		node = n->child[
				((x>=n->mid[0])<<2)|
				((y>=n->mid[1])<<1)|
				((z>=n->mid[2])<<0)];
	}

	{
		struct bspxlgleaf_s *leaf = &grid->leafs[node & ~LGNODE_LEAF];
		x -= leaf->mins[0];
		y -= leaf->mins[1];
		z -= leaf->mins[2];
		if (x >= leaf->size[0] ||
			y >= leaf->size[1] ||
			z >= leaf->size[2])
			return 0;	//sample we're after is out of bounds...

		i = x + leaf->size[0]*(y + leaf->size[1]*z);
		samp = leaf->rgbvalues + i*leaf->numstyles;

		//no hdr support
		for (i = 0; i < leaf->numstyles; i++)
		{
			if (samp[i].style == ((qbyte)(~0u)))
				break;	//no more
			if (samp[i].style < cl_max_lightstyles)
			{
				lev = d_lightstylevalue[samp[i].style]*w;
				res_diffuse[0] += samp[i].rgb[0] * lev * cl_lightstyle[samp[i].style].colours[0];
				res_diffuse[1] += samp[i].rgb[1] * lev * cl_lightstyle[samp[i].style].colours[1];
				res_diffuse[2] += samp[i].rgb[2] * lev * cl_lightstyle[samp[i].style].colours[2];
			}
		}
		if (i == 0)
			w = 0;
	}
	return w;
}
static void BSPX_LightGridValue(void *lightgridinfo, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	bspxlightgrid_t *grid = lightgridinfo;
	int i, tile[3];
	float w, s, frac[3];

	VectorSet(res_diffuse, 0, 0, 0);	//assume worst
	VectorSet(res_ambient, 0, 0, 0);	//assume worst
	VectorSet(res_dir, 1, 0, 1);		//super lame

	for (i = 0; i < 3; i++)
	{
		tile[i] = floor((point[i] - grid->mins[i]) * grid->gridscale[i]);
		frac[i] = (point[i] - grid->mins[i]) * grid->gridscale[i] - tile[i];
	}

	for (i = 0, s = 0; i < 8; i++)
	{
		w =	((i&1)?frac[0]:1-frac[0])	//keep the lerping vaugely smooth.
		  * ((i&2)?frac[1]:1-frac[1])
		  * ((i&4)?frac[2]:1-frac[2]);
		s += w*BSPX_LightGridSingleValue(grid,	tile[0]+!!(i&1),
												tile[1]+!!(i&2),
												tile[2]+!!(i&4), w, res_diffuse);
	}

	if (s)
		VectorScale(res_diffuse, 1.0/(s*255), res_diffuse);	//average the successful ones
	VectorScale(res_diffuse, 0.5, res_ambient);	//and fix up ambients.
}

void GLQ1BSP_LightPointValues(model_t *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	vec3_t		end;
	float *r;
#ifdef RTLIGHTS
	extern cvar_t r_shadow_realtime_world, r_shadow_realtime_world_lightmaps;
#endif

	if (!model->lightdata || r_fullbright.ival || model->loadstate != MLS_LOADED)
	{
		if (model->loadstate != MLS_LOADED)
			Sys_Error("GLQ1BSP_LightPointValues: model not loaded...\n");
		res_diffuse[0] = 0;
		res_diffuse[1] = 0;
		res_diffuse[2] = 0;
	
		res_ambient[0] = 255;
		res_ambient[1] = 255;
		res_ambient[2] = 255;

		res_dir[0] = 1;
		res_dir[1] = 1;
		res_dir[2] = 0.1;
		VectorNormalize(res_dir);
		return;
	}

	if (model->lightgrid)
		return BSPX_LightGridValue(model->lightgrid, point, res_diffuse, res_ambient, res_dir);

	end[0] = point[0];
	end[1] = point[1];
	end[2] = point[2] - mod_lightpoint_distance.value;

	r = GLRecursiveLightPoint3C(model, model->rootnode, point, end);
	if (r == NULL)
	{
		res_diffuse[0] = 0;
		res_diffuse[1] = 0;
		res_diffuse[2] = 0;
	
		res_ambient[0] = 0;
		res_ambient[1] = 0;
		res_ambient[2] = 0;

		res_dir[0] = 0;
		res_dir[1] = 1;
		res_dir[2] = 1;
	}
	else
	{
		res_diffuse[0] = r[0];
		res_diffuse[1] = r[1];
		res_diffuse[2] = r[2];

		/*bright on one side, dark on the other, but not too dark*/
		res_ambient[0] = r[0];
		res_ambient[1] = r[1];
		res_ambient[2] = r[2];

		res_dir[0] = r[3];
		res_dir[1] = r[4];
		res_dir[2] = -r[5];
		if (!res_dir[0] && !res_dir[1] && !res_dir[2])
			res_dir[0] = res_dir[2] = 1;
		VectorNormalize(res_dir);
	}

#ifdef RTLIGHTS
	if (r_shadow_realtime_world.ival)
	{
		float lm = r_shadow_realtime_world_lightmaps.value;
		if (lm < 0) lm = 0;
		if (lm > 1) lm = 1;
		VectorScale(res_diffuse, lm, res_diffuse);
		VectorScale(res_ambient, lm, res_ambient);
	}
#endif
}

#endif
