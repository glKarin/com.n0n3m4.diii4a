/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Refresher setup and main part of the frame generation
 *
 * =======================================================================
 */

#include "header/local.h"

#define	REF_VERSION	"Yamagi Quake II Vulkan Refresher " VKVERSION " (based on vkQuake2 v1.4.3)"

#ifdef __ANDROID__ //karin: Android Vulkan system functions
extern int Android_Vulkan_CreateSurface(ANativeWindow *window, VkInstance instance, VkSurfaceKHR *surface);
extern int Android_Vulkan_LoadLibrary(const char *path);
extern void * Android_Vulkan_GetVkGetInstanceProcAddr(void);
extern void Android_Vulkan_UnloadLibrary(void);
#endif

// world rendered and ready to render 2d elements
static qboolean world_rendered;
static qboolean RE_IsHighDPIaware = false;

viddef_t	vid;

refimport_t	ri;

model_t		*r_worldmodel;

vkconfig_t vk_config;
vkstate_t  vk_state;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles
image_t		*r_squaretexture;	// rectangle for particles

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

static float		v_blend[4];	// final blending color

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_projection_matrix[16];
float	r_view_matrix[16];
float	r_viewproj_matrix[16];
// correction matrix for perspective in Vulkan
static float r_vulkan_correction[16] = { 1.f,  0.f, 0.f, 0.f,
										 0.f, -1.f, 0.f, 0.f,
										 0.f,  0.f, .5f, 0.f,
										 0.f,  0.f, .5f, 1.f
										};
//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

static cvar_t	*r_norefresh;
static cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
static cvar_t	*r_speeds;
static cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_cull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;
cvar_t	*r_vsync;
static cvar_t	*r_mode;
cvar_t	*r_gunfov;
cvar_t	*r_farsee;
static cvar_t	*r_customwidth;
static cvar_t	*r_customheight;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*vk_overbrightbits;
cvar_t	*r_validation;
cvar_t	*vk_picmip;
cvar_t	*r_palettedtexture;
cvar_t	*vk_flashblend;
cvar_t	*vk_finish;
#if defined(__APPLE__)
cvar_t  *vk_molten_fastmath;
cvar_t  *vk_molten_metalbuffers;
#endif
cvar_t	*r_clear;
cvar_t	*r_lockpvs;
static cvar_t	*r_polyblend;
cvar_t	*r_modulate;
cvar_t	*vk_shadows;
cvar_t	*vk_pixel_size;
static cvar_t	*vk_particle_size;
static cvar_t	*vk_particle_att_a;
static cvar_t	*vk_particle_att_b;
static cvar_t	*vk_particle_att_c;
static cvar_t	*vk_particle_min_size;
static cvar_t	*vk_particle_max_size;
static cvar_t	*vk_custom_particles;
cvar_t	*vk_postprocess;
cvar_t	*vk_dynamic;
cvar_t	*vk_msaa;
cvar_t	*vk_showtris;
cvar_t	*r_lightmap;
cvar_t	*vk_texturemode;
cvar_t	*vk_lmaptexturemode;
cvar_t	*vk_aniso;
cvar_t	*vk_mip_nearfilter;
cvar_t	*vk_sampleshading;
cvar_t	*vk_device_idx;
cvar_t	*r_retexturing;
cvar_t	*r_scale8bittextures;
static cvar_t	*vk_underwater;
cvar_t	*r_nolerp_list;
cvar_t	*r_lerp_list;
cvar_t	*r_2D_unfiltered;
cvar_t	*r_videos_unfiltered;
cvar_t	*r_fixsurfsky;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
static cvar_t	*viewsize;

#if defined(__APPLE__)
PFN_vkGetPhysicalDeviceMetalFeaturesMVK qvkGetPhysicalDeviceMetalFeaturesMVK;
PFN_vkGetMoltenVKConfigurationMVK qvkGetMoltenVKConfigurationMVK;
PFN_vkSetMoltenVKConfigurationMVK qvkSetMoltenVKConfigurationMVK;
#endif


void R_RotateForEntity (entity_t *e, float *mvMatrix)
{
	Mat_Rotate(mvMatrix, -e->angles[2], 1.f, 0.f, 0.f);
	Mat_Rotate(mvMatrix, -e->angles[0], 0.f, 1.f, 0.f);
	Mat_Rotate(mvMatrix,  e->angles[1], 0.f, 0.f, 1.f);
	Mat_Translate(mvMatrix, e->origin[0], e->origin[1], e->origin[2]);
}

static void
R_DrawSpriteModel(entity_t *currententity, const model_t *currentmodel)
{
	float alpha = 1.0F;
	vec3_t point;
	dsprframe_t *frame;
	float *up, *right;
	dsprite_t *psprite;
	image_t *skin = NULL;

	/* don't even bother culling, because it's just
	   a single polygon without a surface cache */
	psprite = (dsprite_t *)currentmodel->extradata;

	currententity->frame %= psprite->numframes;
	frame = &psprite->frames[currententity->frame];

	/* normal sprite */
	up = vup;
	right = vright;

	if (currententity->flags & RF_TRANSLUCENT)
	{
		alpha = currententity->alpha;
	}

	vec3_t spriteQuad[4];

	VectorMA(currententity->origin, -frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, spriteQuad[0]);
	VectorMA(currententity->origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, spriteQuad[1]);
	VectorMA(currententity->origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, spriteQuad[2]);
	VectorMA(currententity->origin, -frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, spriteQuad[3]);

	float quadVerts[] = { spriteQuad[0][0], spriteQuad[0][1], spriteQuad[0][2], 0.f, 1.f,
						  spriteQuad[1][0], spriteQuad[1][1], spriteQuad[1][2], 0.f, 0.f,
						  spriteQuad[2][0], spriteQuad[2][1], spriteQuad[2][2], 1.f, 0.f,
						  spriteQuad[0][0], spriteQuad[0][1], spriteQuad[0][2], 0.f, 1.f,
						  spriteQuad[2][0], spriteQuad[2][1], spriteQuad[2][2], 1.f, 0.f,
						  spriteQuad[3][0], spriteQuad[3][1], spriteQuad[3][2], 1.f, 1.f };

	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawSpritePipeline.layout,
		VK_SHADER_STAGE_VERTEX_BIT, sizeof(r_viewproj_matrix), sizeof(float), &alpha);
	QVk_BindPipeline(&vk_drawSpritePipeline);

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint8_t *vertData = QVk_GetVertexBuffer(sizeof(quadVerts), &vbo, &vboOffset);
	memcpy(vertData, quadVerts, sizeof(quadVerts));

	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);

	float gamma = 2.1F - vid_gamma->value;

	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout,
		VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(gamma), &gamma);

	skin = currentmodel->skins[currententity->frame];
	if (!skin)
	{
		skin = r_notexture;
	}

	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk_drawSpritePipeline.layout, 0, 1,
		&skin->vk_texture.descriptorSet, 0, NULL);
	vkCmdDraw(vk_activeCmdbuffer, 6, 1, 0, 0);
}

static void
R_DrawNullModel(entity_t *currententity)
{
	vec3_t shadelight;
	int i, j;

	if (currententity->flags & RF_FULLBRIGHT || !r_worldmodel || !r_worldmodel->lightdata)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	}
	else
	{
		R_LightPoint(r_worldmodel->grid, currententity->origin, shadelight, currententity);
	}

	float model[16];
	Mat_Identity(model);
	R_RotateForEntity(currententity, model);

	vec3_t verts[24];
	verts[0][0] = 0.f;
	verts[0][1] = 0.f;
	verts[0][2] = -16.f;
	verts[1][0] = shadelight[0];
	verts[1][1] = shadelight[1];
	verts[1][2] = shadelight[2];

	for (i = 2, j = 0; i < 12; i+=2, j++)
	{
		verts[i][0] = 16 * cos(j*M_PI / 2);
		verts[i][1] = 16 * sin(j*M_PI / 2);
		verts[i][2] = 0.f;
		verts[i+1][0] = shadelight[0];
		verts[i+1][1] = shadelight[1];
		verts[i+1][2] = shadelight[2];
	}

	verts[12][0] = 0.f;
	verts[12][1] = 0.f;
	verts[12][2] = 16.f;
	verts[13][0] = shadelight[0];
	verts[13][1] = shadelight[1];
	verts[13][2] = shadelight[2];

	for (i = 23, j = 4; i > 13; i-=2, j--)
	{
		verts[i-1][0] = 16 * cos(j*M_PI / 2);
		verts[i-1][1] = 16 * sin(j*M_PI / 2);
		verts[i-1][2] = 0.f;
		verts[i][0] = shadelight[0];
		verts[i][1] = shadelight[1];
		verts[i][2] = shadelight[2];
	}

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *vertData = QVk_GetVertexBuffer(sizeof(verts), &vbo, &vboOffset);
	uint8_t *uboData  = QVk_GetUniformBuffer(sizeof(model), &uboOffset, &uboDescriptorSet);
	memcpy(vertData, verts, sizeof(verts));
	memcpy(uboData,  model, sizeof(model));

	QVk_BindPipeline(&vk_drawNullModelPipeline);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawNullModelPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, QVk_GetTriangleFanIbo(12), 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 12, 1, 0, 0, 0);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 12, 1, 0, 6, 0);
}

static void
R_DrawEntitiesOnList(void)
{
	int i;

	if (!r_drawentities->value)
	{
		return;
	}

	/* draw non-transparent first */
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_TRANSLUCENT)
		{
			continue; /* solid */
		}

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			const model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					R_DrawAliasModel(currententity, currentmodel);
					break;
				case mod_brush:
					R_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					R_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					R_Printf(PRINT_ALL, "%s: Bad modeltype %d\n",
						__func__, currentmodel->type);
					return;
			}
		}
	}

	/* draw transparent entities
	   we could sort these if it ever
	   becomes a problem... */

	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &r_newrefdef.entities[i];

		if (!(currententity->flags & RF_TRANSLUCENT))
		{
			continue; /* solid */
		}

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			const model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					R_DrawAliasModel(currententity, currentmodel);
					break;
				case mod_brush:
					R_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					R_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					R_Printf(PRINT_ALL, "%s: Bad modeltype %d\n",
						__func__, currentmodel->type);
					return;
			}
		}
	}
}

static void
Vk_DrawParticles(int num_particles, const particle_t particles[], const unsigned *colortable)
{
	typedef struct {
		float x,y,z,r,g,b,a,u,v;
	} pvertex;

	const particle_t *p;
	int				i;
	vec3_t			up, right;
	pvertex*	currentvertex;

	if (!num_particles)
	{
		return;
	}

	VectorScale(vup, 1.5, up);
	VectorScale(vright, 1.5, right);

	static pvertex visibleParticles[MAX_PARTICLES * 3];

	if (num_particles > MAX_PARTICLES)
	{
		num_particles = MAX_PARTICLES;
	}

	currentvertex = visibleParticles;
	for (p = particles, i = 0; i < num_particles; i++, p++)
	{
		YQ2_ALIGNAS_TYPE(unsigned) byte	color[4];
		float	scale;

		// hack a scale up to keep particles from disapearing
		scale = (p->origin[0] - r_origin[0]) * vpn[0] +
				(p->origin[1] - r_origin[1]) * vpn[1] +
				(p->origin[2] - r_origin[2]) * vpn[2];

		if (scale < 20)
		{
			scale = 1;
		}
		else
		{
			scale = 1 + scale * 0.004;
		}

		*(int *)color = colortable[p->color];

		float r = color[0] / 255.f;
		float g = color[1] / 255.f;
		float b = color[2] / 255.f;

		currentvertex->x = p->origin[0];
		currentvertex->y = p->origin[1];
		currentvertex->z = p->origin[2];
		currentvertex->r = r;
		currentvertex->g = g;
		currentvertex->b = b;
		currentvertex->a = p->alpha;
		currentvertex->u = 0.0625;
		currentvertex->v = 0.0625;
		currentvertex++;

		currentvertex->x = p->origin[0] + up[0] * scale;
		currentvertex->y = p->origin[1] + up[1] * scale;
		currentvertex->z = p->origin[2] + up[2] * scale;
		currentvertex->r = r;
		currentvertex->g = g;
		currentvertex->b = b;
		currentvertex->a = p->alpha;
		currentvertex->u = 1.0625;
		currentvertex->v = 0.0625;
		currentvertex++;

		currentvertex->x = p->origin[0] + right[0] * scale;
		currentvertex->y = p->origin[1] + right[1] * scale;
		currentvertex->z = p->origin[2] + right[2] * scale;
		currentvertex->r = r;
		currentvertex->g = g;
		currentvertex->b = b;
		currentvertex->a = p->alpha;
		currentvertex->u = 0.0625;
		currentvertex->v = 1.0625;
		currentvertex++;
	}

	QVk_BindPipeline(&vk_drawParticlesPipeline);

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint8_t *vertData = QVk_GetVertexBuffer((currentvertex - visibleParticles) * sizeof(pvertex), &vbo, &vboOffset);
	memcpy(vertData, &visibleParticles, (currentvertex - visibleParticles) * sizeof(pvertex));

	float gamma = 2.1F - vid_gamma->value;

	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout,
		VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(gamma), &gamma);

	if (vk_custom_particles->value == 2)
	{
		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								vk_drawParticlesPipeline.layout, 0, 1,
								&r_squaretexture->vk_texture.descriptorSet, 0, NULL);
	}
	else
	{
		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								vk_drawParticlesPipeline.layout, 0, 1,
								&r_particletexture->vk_texture.descriptorSet, 0, NULL);
	}

	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdDraw(vk_activeCmdbuffer, (currentvertex - visibleParticles), 1, 0, 0);
}

static void
R_DrawParticles(void)
{
	if (vk_custom_particles->value == 1)
	{
		int i;
		unsigned char color[4];
		const particle_t *p;

		if (!r_newrefdef.num_particles)
			return;

		typedef struct {
			float x, y, z, r, g, b, a;
		} ppoint;

		struct {
			float particleSize;
			float particleScale;
			float minPointSize;
			float maxPointSize;
			float att_a;
			float att_b;
			float att_c;
		} particleUbo;

		// Particle size needs to be scaled down proportionally to vk_pixel_size.
		const float divisor = (vk_pixel_size->value < 1.0f ? 1.0f : vk_pixel_size->value);
		particleUbo.particleSize = vk_particle_size->value / divisor;
		particleUbo.particleScale = vid.width * ri.Cvar_Get("viewsize", "100", CVAR_ARCHIVE)->value / 102400;
		particleUbo.minPointSize = vk_particle_min_size->value;
		particleUbo.maxPointSize = vk_particle_max_size->value;
		particleUbo.att_a = vk_particle_att_a->value;
		particleUbo.att_b = vk_particle_att_b->value;
		particleUbo.att_c = vk_particle_att_c->value;

		static ppoint visibleParticles[MAX_PARTICLES];

		for (i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++)
		{
			*(int *)color = d_8to24table[p->color];

			float r = color[0] / 255.f;
			float g = color[1] / 255.f;
			float b = color[2] / 255.f;

			visibleParticles[i].x = p->origin[0];
			visibleParticles[i].y = p->origin[1];
			visibleParticles[i].z = p->origin[2];
			visibleParticles[i].r = r;
			visibleParticles[i].g = g;
			visibleParticles[i].b = b;
			visibleParticles[i].a = p->alpha;
		}

		QVk_BindPipeline(&vk_drawPointParticlesPipeline);

		VkBuffer vbo;
		VkDeviceSize vboOffset;
		uint32_t uboOffset;
		VkDescriptorSet uboDescriptorSet;
		uint8_t *vertData = QVk_GetVertexBuffer(sizeof(ppoint) * r_newrefdef.num_particles, &vbo, &vboOffset);
		uint8_t *uboData  = QVk_GetUniformBuffer(sizeof(particleUbo), &uboOffset, &uboDescriptorSet);
		memcpy(vertData, &visibleParticles, sizeof(ppoint) * r_newrefdef.num_particles);
		memcpy(uboData,  &particleUbo, sizeof(particleUbo));
		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPointParticlesPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
		vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
		vkCmdDraw(vk_activeCmdbuffer, r_newrefdef.num_particles, 1, 0, 0);
	}
	else
	{
		Vk_DrawParticles(r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table);
	}
}

static void
R_PolyBlend(void)
{
	if (!r_polyblend->value)
	{
		return;
	}

	if (!v_blend[3])
	{
		return;
	}

	float polyTransform[] = { 0.f, 0.f, vid.width, vid.height, v_blend[0], v_blend[1], v_blend[2], v_blend[3] };
	QVk_DrawColorRect(polyTransform, sizeof(polyTransform), RP_WORLD);
}

static void
R_SetupFrame(void)
{
	int i;
	mleaf_t *leaf;

	r_framecount++;

	/* build the transformation matrix for the given view angles */
	VectorCopy(r_newrefdef.vieworg, r_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	/* current viewcluster */
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		if (!r_worldmodel)
		{
			Com_Error(ERR_DROP, "%s: bad world model", __func__);
			return;
		}

		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf(r_origin, r_worldmodel->nodes);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
		{
			/* look down a bit */
			vec3_t temp;

			VectorCopy(r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			/* look up a bit */
			vec3_t temp;

			VectorCopy(r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
	}

	for (i = 0; i < 4; i++)
	{
		v_blend[i] = r_newrefdef.blend[i];
	}

	c_brush_polys = 0;
	c_alias_polys = 0;

	/* clear out the portion of the screen that the NOWORLDMODEL defines
	   unlike OpenGL, draw a rectangle in proper location - it's easier to do in Vulkan */
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		float clearArea[] = { (float)r_newrefdef.x / vid.width, (float)r_newrefdef.y / vid.height,
							  (float)r_newrefdef.width / vid.width, (float)r_newrefdef.height / vid.height,
							  .3f, .3f, .3f, 1.f };
		QVk_DrawColorRect(clearArea, sizeof(clearArea), RP_UI);
	}
}

void Mat_Identity(float *matrix)
{
	matrix[0] = 1.f;
	matrix[1] = 0.f;
	matrix[2] = 0.f;
	matrix[3] = 0.f;
	matrix[4] = 0.f;
	matrix[5] = 1.f;
	matrix[6] = 0.f;
	matrix[7] = 0.f;
	matrix[8] = 0.f;
	matrix[9] = 0.f;
	matrix[10] = 1.f;
	matrix[11] = 0.f;
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
	matrix[15] = 1.f;
}

void Mat_Mul(float *m1, float *m2, float *res)
{
	float mul[16] = { m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12],
					  m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13],
					  m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14],
					  m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15],
					  m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12],
					  m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13],
					  m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14],
					  m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15],
					  m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12],
					  m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13],
					  m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14],
					  m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15],
					  m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12],
					  m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13],
					  m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14],
					  m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15]
	};

	memcpy(res, mul, sizeof(float) * 16);
}

void Mat_Translate(float *matrix, float x, float y, float z)
{
	float t[16] = { 1.f, 0.f, 0.f, 0.f,
					0.f, 1.f, 0.f, 0.f,
					0.f, 0.f, 1.f, 0.f,
					  x,   y,   z, 1.f };

	Mat_Mul(matrix, t, matrix);
}

void Mat_Rotate(float *matrix, float deg, float x, float y, float z)
{
	double c = cos(deg * M_PI / 180.0);
	double s = sin(deg * M_PI / 180.0);
	double cd = 1.0 - c;
	vec3_t r = { x, y, z };
	VectorNormalize(r);

	float rot[16] = { r[0]*r[0]*cd + c,		 r[1]*r[0]*cd + r[2]*s, r[0]*r[2]*cd - r[1]*s,	0.f,
					  r[0]*r[1]*cd - r[2]*s, r[1]*r[1]*cd + c,		r[1]*r[2]*cd + r[0]*s,	0.f,
					  r[0]*r[2]*cd + r[1]*s, r[1]*r[2]*cd - r[0]*s, r[2]*r[2]*cd + c,		0.f,
					  0.f,					 0.f,					0.f,					1.f
	};

	Mat_Mul(matrix, rot, matrix);
}

void Mat_Scale(float *matrix, float x, float y, float z)
{
	float s[16] = {   x, 0.f, 0.f, 0.f,
					0.f,   y, 0.f, 0.f,
					0.f, 0.f,   z, 0.f,
					0.f, 0.f, 0.f, 1.f
	};

	Mat_Mul(matrix, s, matrix);
}

void Mat_Perspective(float *matrix, float *correction_matrix, float fovy, float aspect,
	float zNear, float zFar)
{
	float xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += -(2 * vk_state.camera_separation) / zNear;
	xmax += -(2 * vk_state.camera_separation) / zNear;

	float proj[16];
	memset(proj, 0, sizeof(float) * 16);
	proj[0] = 2.f * zNear / (xmax - xmin);
	proj[2] = (xmax + xmin) / (xmax - xmin);
	proj[5] = 2.f * zNear / (ymax - ymin);
	proj[6] = (ymax + ymin) / (ymax - ymin);
	proj[10] = -(zFar + zNear) / (zFar - zNear);
	proj[11] = -1.f;
	proj[14] = -2.f * zFar * zNear / (zFar - zNear);

	// Convert projection matrix to Vulkan coordinate system (https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/)
	Mat_Mul(proj, correction_matrix, matrix);
}

void Mat_Ortho(float *matrix, float left, float right, float bottom, float top,
			   float zNear, float zFar)
{
	float proj[16];
	memset(proj, 0, sizeof(float) * 16);
	proj[0] = 2.f / (right - left);
	proj[3] = (right + left) / (right - left);
	proj[5] = 2.f / (top - bottom);
	proj[7] = (top + bottom) / (top - bottom);
	proj[10] = -2.f / (zFar - zNear);
	proj[11] = -(zFar + zNear) / (zFar - zNear);
	proj[15] = 1.f;

	// Convert projection matrix to Vulkan coordinate system (https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/)
	Mat_Mul(proj, r_vulkan_correction, matrix);
}

/*
=============
R_SetupVulkan
=============
*/
static void
R_SetupVulkan (void)
{
	float	r_proj_aspect;
	float	r_proj_fovx;
	float	r_proj_fovy;
	int		x, x2, y2, y, w, h;
	float dist = (r_farsee->value == 0) ? 4096.0f : 8192.0f;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	VkViewport viewport = {
		.x = x,
		.y = vid.height - h - y2,
		.width = w,
		.height = h,
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};

	// When rendering the world, reduce viewport size proportionally to vk_pixel_size.
	if (vk_state.current_renderpass == RP_WORLD)
	{
		const float divisor = (vk_pixel_size->value < 1.0f ? 1.0f : vk_pixel_size->value);
		viewport.x /= divisor;
		viewport.y /= divisor;
		viewport.width /= divisor;
		viewport.height /= divisor;
	}

	vkCmdSetViewport(vk_activeCmdbuffer, 0, 1, &viewport);

	// set up projection matrix
	r_proj_fovx = r_newrefdef.fov_x;
	r_proj_fovy = r_newrefdef.fov_y;
	r_proj_aspect = (float)r_newrefdef.width / r_newrefdef.height;
	Mat_Perspective(r_projection_matrix, r_vulkan_correction, r_proj_fovy, r_proj_aspect, 4, dist);

	R_SetFrustum(vup, vpn, vright, r_origin, r_proj_fovx, r_proj_fovy,
		frustum);

	// set up view matrix
	Mat_Identity(r_view_matrix);
	// put Z going up
	Mat_Translate(r_view_matrix, -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);
	Mat_Rotate(r_view_matrix, -r_newrefdef.viewangles[1], 0.f, 0.f, 1.f);
	Mat_Rotate(r_view_matrix, -r_newrefdef.viewangles[0], 0.f, 1.f, 0.f);
	Mat_Rotate(r_view_matrix, -r_newrefdef.viewangles[2], 1.f, 0.f, 0.f);
	Mat_Rotate(r_view_matrix, 90.f, 0.f, 0.f, 1.f);
	Mat_Rotate(r_view_matrix, -90.f, 1.f, 0.f, 0.f);

	// precalculate view-projection matrix
	Mat_Mul(r_view_matrix, r_projection_matrix, r_viewproj_matrix);
	// view-projection matrix will always be stored as the first push constant item, so set no offset
	vkCmdPushConstants(vk_activeCmdbuffer, vk_drawTexQuadPipeline[vk_state.current_renderpass].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(r_viewproj_matrix), r_viewproj_matrix);
}

static void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
RE_RenderView

r_newrefdef must be set before the first call
================
*/
static void
RE_RenderView(refdef_t *fd)
{
	if (r_norefresh->value)
	{
		return;
	}

	r_newrefdef = *fd;

	if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		Com_Error(ERR_DROP, "%s: NULL worldmodel", __func__);
	}

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	VkRect2D scissor = {
		.offset = { r_newrefdef.x, r_newrefdef.y },
		.extent = { r_newrefdef.width, r_newrefdef.height }
	};

	// When rendering the world, scale down scissor proportionally to vk_pixel_size.
	if (vk_state.current_renderpass == RP_WORLD)
	{
		const float divisor = (vk_pixel_size->value < 1.0f ? 1.0f : vk_pixel_size->value);
		scissor.offset.x = (int32_t)floorf(scissor.offset.x / divisor);
		scissor.offset.y = (int32_t)floorf(scissor.offset.y / divisor);
		scissor.extent.width = (uint32_t)ceilf(scissor.extent.width / divisor);
		scissor.extent.height = (uint32_t)ceilf(scissor.extent.height / divisor);
	}

	vkCmdSetScissor(vk_activeCmdbuffer, 0, 1, &scissor);

	R_PushDlights();

	// added for compatibility sake with OpenGL implementation - don't use it!
	if (vk_finish->value)
	{
		vkDeviceWaitIdle(vk_device.logical);
	}

	R_SetupFrame();

	R_SetupVulkan();

	R_MarkLeaves(); /* done here so we know if we're in water */

	R_DrawWorld();

	R_DrawEntitiesOnList();

	R_RenderDlights();

	R_DrawParticles();

	R_DrawAlphaSurfaces();

	R_Flash();

	if (r_speeds->value)
	{
		R_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys,
			c_alias_polys,
			c_visible_textures,
			c_visible_lightmaps);
	}
}

qboolean RE_EndWorldRenderpass(void)
{
	// still some issues?
	if (!vk_frameStarted)
	{
		// we can't start 2d rendering
		return false;
	}

	// 3d world has alredy rendered and 2d already initialized
	if (world_rendered)
	{
		return true;
	}

	world_rendered = true;

	// finish rendering world view to offsceen buffer
	vkCmdEndRenderPass(vk_activeCmdbuffer);

	// apply postprocessing effects to offscreen buffer:
	//	* underwater view warp if the player is submerged in liquid
	//	* restore world view to the full screen size when vk_pixel_size is >1.0
	QVk_BeginRenderpass(RP_WORLD_WARP);
	float underwaterTime;
	if (vk_underwater->value)
	{
		underwaterTime = (r_newrefdef.rdflags & RDF_UNDERWATER) ? r_newrefdef.time : 0.f;
	}
	else
	{
		underwaterTime = 0.f;
	};
	float pushConsts[] =
	{
		underwaterTime,
		viewsize->value / 100.0f,
		vid.width,
		vid.height,
		vk_viewport.x,
		vk_viewport.y,
		(vk_pixel_size->value < 1.0f ? 1.0f : vk_pixel_size->value),
		r_newrefdef.x,
		r_newrefdef.y,
		r_newrefdef.width,
		r_newrefdef.height,
	};
	vkCmdPushConstants(vk_activeCmdbuffer, vk_worldWarpPipeline.layout,
		VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(pushConsts), pushConsts);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_worldWarpPipeline.layout, 0, 1, &vk_colorbuffer.descriptorSet, 0, NULL);
	QVk_BindPipeline(&vk_worldWarpPipeline);
	// Restore full viewport for future steps.
	vkCmdSetViewport(vk_activeCmdbuffer, 0u, 1u, &vk_viewport);
	vkCmdSetScissor(vk_activeCmdbuffer, 0u, 1u, &vk_scissor);
	vkCmdDraw(vk_activeCmdbuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(vk_activeCmdbuffer);

	// start drawing UI
	QVk_BeginRenderpass(RP_UI);

	return true;
}

static void
R_SetVulkan2D(const VkViewport* viewport, const VkRect2D* scissor)
{
	// player configuration screen renders a model using the UI renderpass, so skip finishing RP_WORLD twice
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		if(!RE_EndWorldRenderpass())
			// buffers is not initialized
			return;

	vkCmdSetViewport(vk_activeCmdbuffer, 0, 1, viewport);
	vkCmdSetScissor(vk_activeCmdbuffer, 0, 1, scissor);

	// first, blit offscreen color buffer with warped/postprocessed world view
	// skip this step if we're in player config screen since it uses RP_UI and draws directly to swapchain
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		float pushConsts[] = { vk_postprocess->value, (2.1 - vid_gamma->value)};
		vkCmdPushConstants(vk_activeCmdbuffer, vk_postprocessPipeline.layout,
			VK_SHADER_STAGE_FRAGMENT_BIT, 17 * sizeof(float), sizeof(pushConsts), pushConsts);
		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_postprocessPipeline.layout, 0, 1, &vk_colorbufferWarp.descriptorSet, 0, NULL);
		QVk_BindPipeline(&vk_postprocessPipeline);
		vkCmdDraw(vk_activeCmdbuffer, 3, 1, 0, 0);
	}
}

static void
R_SetLightLevel (void)
{
	vec3_t shadelight = {0};

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	/* save off light value for server to look at */
	R_LightPoint(r_worldmodel->grid, r_newrefdef.vieworg, shadelight, NULL);

	/* pick the greatest component, which should be the
	 * same as the mono value returned by software */
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[0];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
	else
	{
		if (shadelight[1] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[1];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
}

static void
RE_RenderFrame(refdef_t *fd)
{
	if (!vk_frameStarted)
	{
		return;
	}

	RE_RenderView(fd);
	R_SetLightLevel();
	R_SetVulkan2D (&vk_viewport, &vk_scissor);
}


static void
R_Register(void)
{
	/* Init default value */
	s_blocklights = NULL;
	s_blocklights_max = NULL;

	r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);
	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	r_mode = ri.Cvar_Get("r_mode", "11", CVAR_ARCHIVE);
	r_vsync = ri.Cvar_Get("r_vsync", "0", CVAR_ARCHIVE);
	r_gunfov = ri.Cvar_Get("r_gunfov", "80", CVAR_ARCHIVE);
	r_farsee = ri.Cvar_Get("r_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);
	r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);

	vk_overbrightbits = ri.Cvar_Get("vk_overbrightbits", "1.0", CVAR_ARCHIVE);
	r_validation = ri.Cvar_Get("r_validation", "0", CVAR_ARCHIVE);
	vk_picmip = ri.Cvar_Get("vk_picmip", "0", 0);
	r_palettedtexture = ri.Cvar_Get("r_palettedtexture", "0", 0);
	vk_flashblend = ri.Cvar_Get("vk_flashblend", "0", 0);
	vk_finish = ri.Cvar_Get("vk_finish", "0", CVAR_ARCHIVE);
	r_clear = ri.Cvar_Get("r_clear", "0", CVAR_ARCHIVE);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);
	r_polyblend = ri.Cvar_Get("r_polyblend", "1", 0);
	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	vk_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	vk_pixel_size = ri.Cvar_Get("vk_pixel_size", "1", CVAR_ARCHIVE);
	vk_particle_size = ri.Cvar_Get("vk_particle_size", "40", CVAR_ARCHIVE);
	vk_particle_att_a = ri.Cvar_Get("vk_particle_att_a", "0.01", CVAR_ARCHIVE);
	vk_particle_att_b = ri.Cvar_Get("vk_particle_att_b", "0.0", CVAR_ARCHIVE);
	vk_particle_att_c = ri.Cvar_Get("vk_particle_att_c", "0.01", CVAR_ARCHIVE);
	vk_particle_min_size = ri.Cvar_Get("vk_particle_min_size", "2", CVAR_ARCHIVE);
	vk_particle_max_size = ri.Cvar_Get("vk_particle_max_size", "40", CVAR_ARCHIVE);
	vk_custom_particles = ri.Cvar_Get("vk_custom_particles", "1", CVAR_ARCHIVE);
	vk_postprocess = ri.Cvar_Get("vk_postprocess", "1", CVAR_ARCHIVE);
	vk_dynamic = ri.Cvar_Get("vk_dynamic", "1", 0);
	vk_msaa = ri.Cvar_Get("r_msaa_samples", "0", CVAR_ARCHIVE);
	vk_showtris = ri.Cvar_Get("vk_showtris", "0", 0);
	r_lightmap = ri.Cvar_Get("r_lightmap", "0", 0);
	vk_texturemode = ri.Cvar_Get("vk_texturemode", "VK_MIPMAP_LINEAR", CVAR_ARCHIVE);
	vk_lmaptexturemode = ri.Cvar_Get("vk_lmaptexturemode", "VK_MIPMAP_LINEAR", CVAR_ARCHIVE);
	vk_aniso = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
	vk_mip_nearfilter = ri.Cvar_Get("vk_mip_nearfilter", "0", CVAR_ARCHIVE);
	vk_sampleshading = ri.Cvar_Get("vk_sampleshading", "1", CVAR_ARCHIVE);
	vk_device_idx = ri.Cvar_Get("vk_device", "-1", CVAR_ARCHIVE);
#if defined(__APPLE__)
	vk_molten_fastmath = ri.Cvar_Get("vk_molten_fastmath", "0", CVAR_ARCHIVE);
	vk_molten_metalbuffers = ri.Cvar_Get("vk_molten_metalbuffer", "0", CVAR_ARCHIVE);
#endif
	r_retexturing = ri.Cvar_Get("r_retexturing", "1", CVAR_ARCHIVE);
	r_scale8bittextures = ri.Cvar_Get("r_scale8bittextures", "0", CVAR_ARCHIVE);
	vk_underwater = ri.Cvar_Get("vk_underwater", "1", CVAR_ARCHIVE);
	/* don't bilerp characters and crosshairs */
	r_nolerp_list = ri.Cvar_Get("r_nolerp_list", DEFAULT_NOLERP_LIST, CVAR_ARCHIVE);
	/* textures that should always be filtered, even if r_2D_unfiltered or an unfiltered gl mode is used */
	r_lerp_list = ri.Cvar_Get("r_lerp_list", "", CVAR_ARCHIVE);
	/* don't bilerp any 2D elements */
	r_2D_unfiltered = ri.Cvar_Get("r_2D_unfiltered", "0", CVAR_ARCHIVE);
	/* don't bilerp videos */
	r_videos_unfiltered = ri.Cvar_Get("r_videos_unfiltered", "0", CVAR_ARCHIVE);
	r_fixsurfsky = ri.Cvar_Get("r_fixsurfsky", "0", CVAR_ARCHIVE);

	// clamp vk_msaa to accepted range so that video menu doesn't crash on us
	if (vk_msaa->value < 0)
	{
		ri.Cvar_Set("r_msaa_samples", "0");
	}

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
	viewsize = ri.Cvar_Get("viewsize", "100", CVAR_ARCHIVE);

	ri.Cmd_AddCommand("vk_strings", Vk_Strings_f);
	ri.Cmd_AddCommand("vk_mem", Vk_Mem_f);
	ri.Cmd_AddCommand("imagelist", Vk_ImageList_f);
	ri.Cmd_AddCommand("screenshot", Vk_ScreenShot_f);
	ri.Cmd_AddCommand("modellist", Mod_Modellist_f);
}

/*
 * Changes the video mode
 */
static int
Vkimp_SetMode(int *pwidth, int *pheight, int mode, int fullscreen)
{
	R_Printf(PRINT_ALL, "Setting mode %d:", mode);

	/* mode -1 is not in the vid mode table - so we keep the values in pwidth
	   and pheight and don't even try to look up the mode info */
	if ((mode >= 0) && !ri.Vid_GetModeInfo(pwidth, pheight, mode))
	{
		R_Printf(PRINT_ALL, " invalid mode\n");
		return rserr_invalid_mode;
	}

	/* We trying to get resolution from desktop */
	if (mode == -2)
	{
		if(!ri.GLimp_GetDesktopMode(pwidth, pheight))
		{
			R_Printf( PRINT_ALL, " can't detect mode\n" );
			return rserr_invalid_mode;
		}
	}

	R_Printf(PRINT_ALL, " %dx%d (vid_fullscreen %i)\n", *pwidth, *pheight, fullscreen);

	if (!ri.GLimp_InitGraphics(fullscreen, pwidth, pheight))
	{
		return rserr_invalid_mode;
	}

	return rserr_ok;
}

static qboolean
R_SetMode(void)
{
	rserr_t err;
	int fullscreen;

#ifdef __ANDROID__ //karin: force fullscreen and size on Android
	ri.Cvar_Set("vid_fullscreen", "1");
	vid_fullscreen->modified = false;
	ri.Cvar_Set("r_mode", "-2");
	int width, height;
	QVk_GetDrawableSize(&width, &height);
	ri.Cvar_SetValue("r_customwidth", width);
	ri.Cvar_SetValue("r_customheight", height);
	r_customwidth->modified = false;
	r_customheight->modified = false;
#endif
	r_mode->modified = false;
	r_vsync->modified = false;

	fullscreen = (int)vid_fullscreen->value;
	vid_gamma->modified = false;

	vk_msaa->modified = false;
	r_clear->modified = false;
	r_validation->modified = false;
	vk_mip_nearfilter->modified = false;
	vk_sampleshading->modified = false;
	vk_device_idx->modified = false;
	vk_picmip->modified = false;
	vk_overbrightbits->modified = false;
	// refresh texture samplers
	vk_texturemode->modified = true;
	vk_lmaptexturemode->modified = true;

	/* a bit hackish approach to enable custom resolutions:
	   Glimp_SetMode needs these values set for mode -1 */
	vid.width = r_customwidth->value;
	vid.height = r_customheight->value;

	if ((err = Vkimp_SetMode((int*)&vid.width, (int*)&vid.height, r_mode->value, fullscreen)) == rserr_ok)
	{
		vk_state.prev_mode = r_mode->value;
	}
	else
	{
		if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue("r_mode", vk_state.prev_mode);
			r_mode->modified = false;
			R_Printf(PRINT_ALL, "%s() - invalid mode\n", __func__);
		}

		// try setting it back to something safe
		if (Vkimp_SetMode((int*)&vid.width, (int*)&vid.height, vk_state.prev_mode, false) != rserr_ok)
		{
			R_Printf(PRINT_ALL, "%s() - could not revert to safe mode\n", __func__);
			return false;
		}
	}
	return true;
}

/*
===============
RE_Init
===============
*/
static qboolean RE_Init( void )
{
	R_Printf(PRINT_ALL, "Refresh: " REF_VERSION "\n");
	R_Printf(PRINT_ALL, "Platform: " YQ2OSTYPE "\n");
	R_Printf(PRINT_ALL, "Architecture: " YQ2ARCH "\n");
	R_Printf(PRINT_ALL, "Build date: " BUILD_DATE "\n\n");

	R_Register();

	// set our "safe" modes
	vk_state.prev_mode = 4;
	// set video mode/screen resolution
	if (!R_SetMode())
	{
		R_Printf(PRINT_ALL, "%s() - could not R_SetMode()\n", __func__);
		return false;
	}

	ri.Vid_MenuInit();

	// print device information during startup
	Vk_Strings_f();

	R_Printf(PRINT_ALL, "Successfully initialized Vulkan!\n");

	return true;
}

/*
** RE_ShutdownContext
**
** This routine does all OS specific shutdown procedures for the Vulkan
** subsystem.
**
*/
static void
RE_ShutdownContext(void)
{
	// Shutdown Vulkan subsystem
	QVk_WaitAndShutdownAll();
}

void
RE_Shutdown(void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("vk_strings");
	ri.Cmd_RemoveCommand("vk_mem");

	QVk_WaitAndShutdownAll();

	/* Cleanup buffers */
	if (s_blocklights)
	{
		free(s_blocklights);
	}

	s_blocklights = NULL;
	s_blocklights_max = NULL;
#ifdef __ANDROID__ //karin: unload Vulkan library
	Android_Vulkan_UnloadLibrary();
#endif
}

static void
RE_BeginFrame(float camera_separation)
{
	// world has not rendered yet
	world_rendered = false;

	/* VK hasn't been initialized yet. I'm pretty sure that
	   we can't get here without having called QVk_Init(),
	   but better save than sorry. */
	if (!vk_initialized)
	{
		vk_frameStarted = false;
		return;
	}

	if (vk_recreateSwapchainNeeded)
	{
		if (QVk_RecreateSwapchain() != true)
		{
			vk_frameStarted = false;
			return;
		}
	}

	// if ri.Sys_Error() had been issued mid-frame, we might end up here without properly submitting the image, so call QVk_EndFrame to be safe
	QVk_EndFrame(true);

	/*
	** change modes if necessary
	*/
	if (vk_texturemode->modified || vk_lmaptexturemode->modified ||
		r_nolerp_list->modified || r_2D_unfiltered->modified ||
		r_lerp_list->modified || r_videos_unfiltered->modified ||
		vk_aniso->modified)
	{
		if (vk_texturemode->modified || vk_aniso->modified ||
			r_nolerp_list->modified || r_2D_unfiltered->modified)
		{
			Vk_TextureMode(vk_texturemode->string);
			vk_texturemode->modified = false;
		}

		if (vk_lmaptexturemode->modified || vk_aniso->modified)
		{
			Vk_LmapTextureMode(vk_lmaptexturemode->string);
			vk_lmaptexturemode->modified = false;
		}

		vk_aniso->modified = false;
		r_nolerp_list->modified = false;
		r_lerp_list->modified = false;
		r_2D_unfiltered->modified = false;
		r_videos_unfiltered->modified = false;
	}

	if (QVk_BeginFrame(&vk_viewport, &vk_scissor) == VK_SUCCESS)
		QVk_BeginRenderpass(RP_WORLD);
}

/*
=====================
RE_EndFrame
=====================
*/
static void
RE_EndFrame(void)
{
	QVk_EndFrame(false);

	// world has not rendered yet
	world_rendered = false;
}

unsigned r_rawpalette[256];

static void
RE_SetPalette(const unsigned char *palette)
{
	int i;

	byte *rp = (byte *)r_rawpalette;

	if (palette)
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = palette[i * 3 + 0];
			rp[i * 4 + 1] = palette[i * 3 + 1];
			rp[i * 4 + 2] = palette[i * 3 + 2];
			rp[i * 4 + 3] = 0xff;
		}
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = d_8to24table[i] & 0xff;
			rp[i * 4 + 1] = (d_8to24table[i] >> 8) & 0xff;
			rp[i * 4 + 2] = (d_8to24table[i] >> 16) & 0xff;
			rp[i * 4 + 3] = 0xff;
		}
	}
}

void
R_DrawBeam(entity_t *currententity )
{
#define NUM_BEAM_SEGS 6

	int i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = currententity->oldorigin[0];
	oldorigin[1] = currententity->oldorigin[1];
	oldorigin[2] = currententity->oldorigin[2];

	origin[0] = currententity->origin[0];
	origin[1] = currententity->origin[1];
	origin[2] = currententity->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
	{
		return;
	}

	PerpendicularVector(perpvec, normalized_direction);
	VectorScale(perpvec, currententity->frame / 2, perpvec);

	for (i = 0; i < 6; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0 / NUM_BEAM_SEGS)*i);
		VectorAdd(start_points[i], origin, start_points[i]);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	r = (d_8to24table[currententity->skinnum & 0xFF]) & 0xFF;
	g = (d_8to24table[currententity->skinnum & 0xFF] >> 8) & 0xFF;
	b = (d_8to24table[currententity->skinnum & 0xFF] >> 16) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	float color[4] = { r, g, b, currententity->alpha };

	struct {
		float v[3];
	} beamvertex[NUM_BEAM_SEGS*4];

	for (i = 0; i < NUM_BEAM_SEGS; i++)
	{
		int idx = i * 4;
		beamvertex[idx].v[0] = start_points[i][0];
		beamvertex[idx].v[1] = start_points[i][1];
		beamvertex[idx].v[2] = start_points[i][2];

		beamvertex[idx + 1].v[0] = end_points[i][0];
		beamvertex[idx + 1].v[1] = end_points[i][1];
		beamvertex[idx + 1].v[2] = end_points[i][2];

		beamvertex[idx + 2].v[0] = start_points[(i + 1) % NUM_BEAM_SEGS][0];
		beamvertex[idx + 2].v[1] = start_points[(i + 1) % NUM_BEAM_SEGS][1];
		beamvertex[idx + 2].v[2] = start_points[(i + 1) % NUM_BEAM_SEGS][2];

		beamvertex[idx + 3].v[0] = end_points[(i + 1) % NUM_BEAM_SEGS][0];
		beamvertex[idx + 3].v[1] = end_points[(i + 1) % NUM_BEAM_SEGS][1];
		beamvertex[idx + 3].v[2] = end_points[(i + 1) % NUM_BEAM_SEGS][2];
	}

	QVk_BindPipeline(&vk_drawBeamPipeline);

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *vertData = QVk_GetVertexBuffer(sizeof(beamvertex), &vbo, &vboOffset);
	uint8_t *uboData  = QVk_GetUniformBuffer(sizeof(color), &uboOffset, &uboDescriptorSet);
	memcpy(vertData, beamvertex, sizeof(beamvertex));
	memcpy(uboData,  color, sizeof(color));

	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawBeamPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdDraw(vk_activeCmdbuffer, NUM_BEAM_SEGS * 4, 1, 0, 0);
}

//===================================================================

static int
RE_InitContext(void *win)
{
	char title[40] = {0};

#ifdef __ANDROID__ //karin: ANativeWindow
	ANativeWindow *window = (ANativeWindow *)win;
#else
	SDL_Window *window = (SDL_Window *)win;
#endif

	if(window == NULL)
	{
		R_Printf(PRINT_ALL, "%s() must not be called with NULL argument!", __func__);
		return false;
	}

	/* Window title - set here so we can display renderer name in it */
	snprintf(title, sizeof(title), "Yamagi Quake II - Vulkan Render");
#if !defined(__ANDROID__) //karin: unuse SDL
	SDL_SetWindowTitle(window, title);
#endif
	// window is ready, initialize Vulkan now
	QVk_SetWindow(window);

#if !defined(__ANDROID__) //karin: unuse SDL
#if SDL_VERSION_ATLEAST(2, 26, 0)
	// Figure out if we are high dpi aware.
	int flags = SDL_GetWindowFlags(window);
#ifdef USE_SDL3
	RE_IsHighDPIaware = (flags & SDL_WINDOW_HIGH_PIXEL_DENSITY) ? true : false;
#else
	RE_IsHighDPIaware = (flags & SDL_WINDOW_ALLOW_HIGHDPI) ? true : false;
#endif
#else
	RE_IsHighDPIaware = true;
#endif
	if (RE_IsHighDPIaware)
	{
		R_Printf(PRINT_ALL, "%s() - HighDPI is enabled\n", __func__);
	}
#endif

	if (!QVk_Init())
	{
		R_Printf(PRINT_ALL, "%s() - could not initialize Vulkan!\n", __func__);
		return false;
	}

	QVk_PostInit();

	return true;
}

#ifdef __ANDROID__ //karin: ANativeWindow
qboolean Vkimp_CreateSurface(ANativeWindow *window)
#else
qboolean Vkimp_CreateSurface(SDL_Window *window)
#endif
{
#ifdef __ANDROID__ //karin: create surface on Android
	if (!Android_Vulkan_CreateSurface(window, vk_instance, &vk_surface))
	{
		R_Printf(PRINT_ALL, "%s() Android_Vulkan_CreateSurface failed",
				__func__);
		return false;
	}
#else
#ifdef USE_SDL3
	if (!SDL_Vulkan_CreateSurface(window, vk_instance, NULL, &vk_surface))
#else
	if (!SDL_Vulkan_CreateSurface(window, vk_instance, &vk_surface))
#endif
	{
		R_Printf(PRINT_ALL, "%s() SDL_Vulkan_CreateSurface failed: %s",
				__func__, SDL_GetError());
		return false;
	}
#endif

	/* This is totaly obscure: For some strange reasons the renderer
	   maintains two(!) repesentations of the resolution. One comes
	   from the client and is saved in r_newrefdef. The other one
	   is determined here and saved in vid. Several calculations take
	   both representations into account.

	   The values will always be the same. The GLimp_InitGraphics()
	   call above communicates the requested resolution to the client
	   where it ends up in the vid subsystem and the vid system writes
	   it into r_newrefdef.

	   We can't avoid the client roundtrip, because we can get the
	   real size of the drawable (which can differ from the resolution
	   due to high dpi awareness) only after the render context was
	   created by GLimp_InitGraphics() and need to communicate it
	   somehow to the client. So we just overwrite the values saved
	   in vid with a call to RI_GetDrawableSize(), just like the
	   client does. This makes sure that both values are the same
	   and everything is okay.

	   We also need to take the special case fullscreen window into
	   account. With the fullscreen windows we cannot use the
	   drawable size, it would scale all cases to the size of the
	   window. Instead use the drawable size when the user wants
	   native resolution (the fullscreen window fills the screen)
	   and use the requested resolution in all other cases. */
	if (RE_IsHighDPIaware)
	{
		if (vid_fullscreen->value != 2)
		{
			QVk_GetDrawableSize(&vid.width, &vid.height);
		}
		else
		{
			if (r_mode->value == -2)
			{
				/* User requested native resolution. */
				QVk_GetDrawableSize(&vid.width, &vid.height);
			}
		}
	}

	return true;
}

static qboolean
RE_IsVsyncActive(void)
{
	if (r_vsync->value)
	{
		return true;
	}
	else
	{
		return false;
	}
}

static int RE_PrepareForWindow(void)
{
#ifdef __ANDROID__ //karin: load Vulkan library
	if (Android_Vulkan_LoadLibrary(NULL))
	{
		R_Printf(PRINT_ALL, "%s() Loader import failed", __func__);
	}
#else
	if (SDL_Vulkan_LoadLibrary(NULL))
	{
		R_Printf(PRINT_ALL, "%s() Loader import failed: %s", __func__, SDL_GetError());
	}
#endif

#ifdef __ANDROID__ //karin: get vkGetInstanceProcAddr
	volkInitializeCustom(Android_Vulkan_GetVkGetInstanceProcAddr());
#else
#ifdef USE_SDL3
	volkInitializeCustom((void *)SDL_Vulkan_GetVkGetInstanceProcAddr());
#else
	volkInitializeCustom(SDL_Vulkan_GetVkGetInstanceProcAddr());
#endif
#endif

#if defined(__APPLE__)
	void *molten = dlopen("libMoltenVK.dylib", RTLD_LOCAL | RTLD_NOW);
	if (!molten)
	{
		return -1;
	}

	qvkGetPhysicalDeviceMetalFeaturesMVK =
		(PFN_vkGetPhysicalDeviceMetalFeaturesMVK)dlsym(molten, "vkGetPhysicalDeviceMetalFeaturesMVK");
	qvkGetMoltenVKConfigurationMVK =
		(PFN_vkGetMoltenVKConfigurationMVK)dlsym(molten, "vkGetMoltenVKConfigurationMVK");
	qvkSetMoltenVKConfigurationMVK =
		(PFN_vkSetMoltenVKConfigurationMVK)dlsym(molten, "vkSetMoltenVKConfigurationMVK");
#endif

#ifdef __ANDROID__ //karin: Vulkan initialize success
    return 1;
#else
	return SDL_WINDOW_VULKAN;
#endif
}

/*
===============
GetRefAPI
===============
*/
Q2_DLL_EXPORTED refexport_t
GetRefAPI(refimport_t imp)
{
	refexport_t refexport = {0};

	// Need to communicate the SDL major version to the client.
#if !defined(__ANDROID__) //karin: unuse SDL
#ifdef USE_SDL3
	SDL_Version ver;
#else
	SDL_version ver;
#endif
	SDL_VERSION(&ver);
#endif

	ri = imp;

	refexport.api_version = API_VERSION;
#ifdef __ANDROID__ //karin: fixed 2 as SDL version
	refexport.framework_version = 2;
#else
	refexport.framework_version = ver.major;
#endif

	refexport.BeginRegistration = RE_BeginRegistration;
	refexport.RegisterModel = RE_RegisterModel;
	refexport.RegisterSkin = RE_RegisterSkin;
	refexport.DrawFindPic = RE_Draw_FindPic;
	refexport.SetSky = RE_SetSky;
	refexport.EndRegistration = RE_EndRegistration;

	refexport.RenderFrame = RE_RenderFrame;

	refexport.DrawGetPicSize = RE_Draw_GetPicSize;
	refexport.DrawPicScaled = RE_Draw_PicScaled;
	refexport.DrawStretchPic = RE_Draw_StretchPic;
	refexport.DrawCharScaled = RE_Draw_CharScaled;
	refexport.DrawTileClear = RE_Draw_TileClear;
	refexport.DrawFill = RE_Draw_Fill;
	refexport.DrawFadeScreen= RE_Draw_FadeScreen;

	refexport.DrawStretchRaw = RE_Draw_StretchRaw;

	refexport.Init = RE_Init;
	refexport.IsVSyncActive = RE_IsVsyncActive;
	refexport.Shutdown = RE_Shutdown;
	refexport.InitContext = RE_InitContext;
	refexport.GetDrawableSize = QVk_GetDrawableSize;
	refexport.ShutdownContext = RE_ShutdownContext;
	refexport.PrepareForWindow = RE_PrepareForWindow;

	refexport.SetPalette = RE_SetPalette;
	refexport.BeginFrame = RE_BeginFrame;
	refexport.EndWorldRenderpass = RE_EndWorldRenderpass;
	refexport.EndFrame = RE_EndFrame;

	// Tell the client that we're unsing the
	// new renderer restart API.
	ri.Vid_RequestRestart(RESTART_NO);

	Swap_Init ();

	return refexport;
}

void R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(level, msg, argptr);
	va_end(argptr);
}

/*
 * this is only here so the functions in shared source files
 * (shared.c, rand.c, flash.c, mem.c/hunk.c) can link
 */
void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[4096]; // MAXPRINTMSG == 4096

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Sys_Error(ERR_FATAL, "%s", text);
}

void
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(PRINT_ALL, msg, argptr);
	va_end(argptr);
}

void
Com_Error(int code, const char *fmt, ...)
{
	va_list argptr;
	char text[4096]; // MAXPRINTMSG == 4096

	va_start(argptr, fmt);
	vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	ri.Sys_Error(code, "%s", text);
}
