#include "quakedef.h"
#ifdef VKQUAKE
#include "vkrenderer.h"
#include "glquake.h"
#include "gl_draw.h"
#include "shader.h"
#include "renderque.h"

//FIXME: instead of switching rendertargets and back, we should be using an alternative queue.

#define PERMUTATION_BEM_VR (1u<<11)
#define PERMUTATION_BEM_FP16 (1u<<12)
#define PERMUTATION_BEM_MULTISAMPLE (1u<<13)
#define PERMUTATION_BEM_DEPTHONLY (1u<<14)
#define PERMUTATION_BEM_WIREFRAME (1u<<15)

#undef BE_Init
#undef BE_SelectMode
#undef BE_GenBrushModelVBO
#undef BE_ClearVBO
#undef BE_UploadAllLightmaps
#undef BE_LightCullModel
#undef BE_SelectEntity
#undef BE_SelectDLight
#undef BE_GetTempBatch
#undef BE_SubmitBatch
#undef BE_DrawMesh_List
#undef BE_DrawMesh_Single
#undef BE_SubmitMeshes
#undef BE_DrawWorld
#undef BE_VBO_Begin
#undef BE_VBO_Data
#undef BE_VBO_Finish
#undef BE_VBO_Destroy
#undef BE_Scissor

#undef BE_RenderToTextureUpdate2d

extern cvar_t r_shadow_realtime_world_lightmaps;
extern cvar_t gl_overbright;
extern cvar_t r_portalrecursion;

extern cvar_t r_polygonoffset_shadowmap_offset, r_polygonoffset_shadowmap_factor;
extern cvar_t r_wireframe;
extern cvar_t vk_stagingbuffers;

static unsigned int vk_usedynamicstaging;

#ifdef RTLIGHTS
static void VK_TerminateShadowMap(void);
#endif
void VKBE_BeginShadowmapFace(void);

static void R_DrawPortal(batch_t *batch, batch_t **blist, batch_t *depthmasklist[2], int portaltype);

#define MAX_TMUS 32

extern texid_t r_whiteimage, missing_texture_gloss, missing_texture_normal;
extern texid_t r_blackimage, r_blackcubeimage, r_whitecubeimage;

static void BE_RotateForEntity (const entity_t *fte_restrict e, const model_t *fte_restrict mod);
static void VKBE_SetupLightCBuffer(dlight_t *l, vec3_t colour, vec3_t axis[3]);

#ifdef VK_EXT_debug_utils
static void DebugSetName(VkObjectType objtype, uint64_t handle, const char *name)
{
	if (vkSetDebugUtilsObjectNameEXT)
	{
		VkDebugUtilsObjectNameInfoEXT info =
		{
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			NULL,
			objtype,
			handle,
			name?name:"UNNAMED"
		};
		vkSetDebugUtilsObjectNameEXT(vk.device, &info);
	}
}
#else
#define DebugSetName(t,h,n)
#endif

/*========================================== tables for deforms =====================================*/
#define frand() (rand()*(1.0/RAND_MAX))
#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand()*((x)-floor(x)))
#define R_FastSin(x) r_sintable[FTABLE_CLAMP(x)]

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

static float *FTableForFunc ( unsigned int func )
{
	switch (func)
	{
		case SHADER_FUNC_SIN:
			return r_sintable;

		case SHADER_FUNC_TRIANGLE:
			return r_triangletable;

		case SHADER_FUNC_SQUARE:
			return r_squaretable;

		case SHADER_FUNC_SAWTOOTH:
			return r_sawtoothtable;

		case SHADER_FUNC_INVERSESAWTOOTH:
			return r_inversesawtoothtable;
	}

	//bad values allow us to crash (so I can debug em)
	return NULL;
}

static void FTable_Init(void)
{
	unsigned int i;
	double t;
	for (i = 0; i < FTABLE_SIZE; i++)
	{
		t = (double)i / (double)FTABLE_SIZE;

		r_sintable[i] = sin(t * 2*M_PI);

		if (t < 0.25)
			r_triangletable[i] = t * 4.0;
		else if (t < 0.75)
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = (t - 0.75) * 4.0 - 1.0;

		if (t < 0.5)
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}
}

typedef vec3_t mat3_t[3];
static mat3_t axisDefault={{1, 0, 0},
					{0, 1, 0},
					{0, 0, 1}};

static void Matrix3_Transpose (mat3_t in, mat3_t out)
{
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}
static void Matrix3_Multiply_Vec3 (const mat3_t a, const vec3_t b, vec3_t product)
{
	product[0] = a[0][0]*b[0] + a[0][1]*b[1] + a[0][2]*b[2];
	product[1] = a[1][0]*b[0] + a[1][1]*b[1] + a[1][2]*b[2];
	product[2] = a[2][0]*b[0] + a[2][1]*b[1] + a[2][2]*b[2];
}

static int Matrix3_Compare(const mat3_t in, const mat3_t out)
{
	return !memcmp(in, out, sizeof(mat3_t));
}

/*================================================*/

//dlight-specific constant-buffer
typedef struct
{
	float l_cubematrix[16];
	vec3_t l_lightposition; float padl1;
	vec3_t l_colour; float pad2;
	vec3_t l_lightcolourscale; float l_lightradius;
	vec4_t l_shadowmapproj;
	vec2_t l_shadowmapscale; vec2_t pad3;
} vkcbuf_light_t;

//entity-specific constant-buffer
typedef struct
{
	float m_modelviewproj[16];
	float m_model[16];
	float m_modelinv[16];
	vec3_t e_eyepos;
	float e_time;
	vec3_t e_light_ambient; float pad1;
	vec3_t e_light_dir;		float pad2;
	vec3_t e_light_mul;		float pad3;
	vec4_t e_lmscale[4];
	vec3_t e_uppercolour;	float pad4;
	vec3_t e_lowercolour;	float pad5;
	vec3_t e_glowmod;		float pad6;
	vec4_t e_colourident;
	vec4_t w_fogcolours;
	float w_fogdensity;		float w_fogdepthbias;	vec2_t pad7;
} vkcbuf_entity_t;

enum 
{
	VK_BUFF_POS,
	VK_BUFF_TC,
	VK_BUFF_COL,
	VK_BUFF_LMTC,
	VK_BUFF_NORM,
	VK_BUFF_SDIR,
	VK_BUFF_TDIR,
	VK_BUFF_MAX
};

typedef struct
{	//there should be only one copy of this struct for each thread that renders anything in vulkan.

	//descriptor sets are: 0) entity+light 1) batch textures + pass textures
	VkDescriptorSet descriptorsets[1];

	//commandbuffer state, to avoid redundant state changes.
	VkPipeline activepipeline;

	float depthrange;

} vkrendercontext_t;

typedef struct
{
	unsigned int inited;

	backendmode_t mode;
	unsigned int modepermutation;
	unsigned int flags;
	unsigned int forcebeflags;

	float	identitylighting;
	float	identitylightmap;
	float		curtime;
	const entity_t	*curentity;
	const dlight_t	*curdlight;
	shader_t	*curshader;
	shader_t	*depthonly;
	texnums_t	*curtexnums;
	vbo_t *batchvbo;
	batch_t *curbatch;
	batch_t dummybatch;
	vec4_t lightshadowmapproj;
	vec2_t lightshadowmapscale;

	unsigned int curlmode;
	shader_t	*shader_rtlight[LSHADER_MODES];

	program_t			*programfixedemu[2];

	mesh_t		**meshlist;
	unsigned int nummeshes;

	unsigned int wbatch;
	unsigned int maxwbatches;
	batch_t *wbatches;

	VkDescriptorBufferInfo ubo_entity;
	VkDescriptorBufferInfo ubo_light;
	vec4_t lightinfo;	//org+radius

	VkBuffer staticbuf;	//holds fallback vertex info so we don't crash from it
	vk_poolmem_t staticbufmem;

	texid_t tex_currentrender;

	struct vk_rendertarg rt_reflection;
	struct vk_rendertarg rt_refraction;
	texid_t tex_refraction;	//separate from rt_reflection, because $reasons
	texid_t tex_ripplemap;

	vkrendercontext_t rc;

	struct shadowmaps_s
	{
		uint32_t width;
		uint32_t height;
		VkImage image;	//array. multiple allows for things to happen out of order, which should help to avoid barrier stalls.
		VkDeviceMemory memory;

		uint32_t seq;
		struct
		{
			VkFramebuffer framebuffer;
			image_t qimage;		//this is silly, but whatever.
			vk_image_t vimage;
		} buf[8];
	} shadow[2]; //omni, spot
	texid_t currentshadowmap;

	VkDescriptorSetLayout textureLayout;

#ifdef VK_KHR_acceleration_structure
	qboolean needtlas;	//frame delay, urgh...
	VkAccelerationStructureKHR tlas;
#endif
} vkbackend_t;

#define VERTEXSTREAMSIZE (1024*1024*2)	//2mb = 1 PAE jumbo page

#define DYNVBUFFSIZE 65536
#define DYNIBUFFSIZE 65536

static vecV_t tmpbuf[65536];	//max verts per mesh

static vkbackend_t shaderstate;

extern int be_maxpasses;

struct blobheader
{
	unsigned char blobmagic[4];
	unsigned int blobversion;
	unsigned int defaulttextures;	//s_diffuse etc flags
	unsigned int numtextures;		//s_t0 count
	unsigned int permutations;		//

	unsigned int cvarsoffset;
	unsigned int cvarslength;

	unsigned int vertoffset;
	unsigned int vertlength;
	unsigned int fragoffset;
	unsigned int fraglength;
};

static float VK_ShaderReadArgument(const char *arglist, const char *arg, char type, qbyte size, void *out)
{
	qbyte i;
	const char *var;
	int arglen = strlen(arg);

	//grab an argument instead, otherwise 0
	var = arglist;
	while((var = strchr(var, '#')))
	{
		if (!Q_strncasecmp(var+1, arg, arglen))
		{
			if (var[1+arglen] == '=')
			{
				var = var+arglen+2;
				for (i = 0; i < size; i++)
				{
					while (*var == ' ' || *var == '\t' || *var == ',')
						var++;

					if (type == 'F')
						((float*)out)[i] = BigFloat(strtod(var, (char**)&var));
					else
						((int*)out)[i] = BigLong(strtol(var, (char**)&var, 0));
					if (!var)
						break;
				}
				return 1;
			}
			if (var[1+arglen] == '#' || !var[1+arglen])
			{
				for (i = 0; i < size; i++)
				{
					if (type == 'F')
						((float*)out)[i] = BigFloat(1);
					else
						((int*)out)[i] = BigLong(1);
				}
				return 1;	//present, but no value
			}
		}
		var++;
	}
	return 0;	//not present.
}

#if 0
//this should use shader pass flags, but those are specific to the shader, not the program, which makes this awkward.
static VkSampler VK_GetSampler(unsigned int flags)
{
	static VkSampler ret;
	qboolean clamptoedge = flags & IF_CLAMP;
	VkSamplerCreateInfo lmsampinfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	if (ret)
		return ret;

	if (flags & IF_LINEAR)
	{
		lmsampinfo.minFilter = lmsampinfo.magFilter = VK_FILTER_LINEAR;
		lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	else if (flags & IF_NEAREST)
	{
		lmsampinfo.minFilter = lmsampinfo.magFilter = VK_FILTER_NEAREST;
		lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
	else
	{
		int *filter = (flags & IF_UIPIC)?vk.filterpic:vk.filtermip;
		if (filter[0])
			lmsampinfo.minFilter = VK_FILTER_LINEAR;
		else
			lmsampinfo.minFilter = VK_FILTER_NEAREST;
		if (filter[1])
			lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		else
			lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		if (filter[2])
			lmsampinfo.magFilter = VK_FILTER_LINEAR;
		else
			lmsampinfo.magFilter = VK_FILTER_NEAREST;
	}

	lmsampinfo.addressModeU = clamptoedge?VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:VK_SAMPLER_ADDRESS_MODE_REPEAT;
	lmsampinfo.addressModeV = clamptoedge?VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:VK_SAMPLER_ADDRESS_MODE_REPEAT;
	lmsampinfo.addressModeW = clamptoedge?VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:VK_SAMPLER_ADDRESS_MODE_REPEAT;
	lmsampinfo.mipLodBias = 0.0;
	lmsampinfo.anisotropyEnable = (flags & IF_NEAREST)?false:(vk.max_anistophy > 1);
	lmsampinfo.maxAnisotropy = vk.max_anistophy;
	lmsampinfo.compareEnable = VK_FALSE;
	lmsampinfo.compareOp = VK_COMPARE_OP_NEVER;
	lmsampinfo.minLod = vk.mipcap[0];	//this isn't quite right
	lmsampinfo.maxLod = vk.mipcap[1];
	lmsampinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	lmsampinfo.unnormalizedCoordinates = VK_FALSE;
	VkAssert(vkCreateSampler(vk.device, &lmsampinfo, NULL, &ret));

	return ret;
}
#endif

//creates the layout stuff for the prog.
static void VK_FinishProg(program_t *prog, const char *name)
{
	{
		VkDescriptorSetLayout desclayout;
		VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		VkDescriptorSetLayoutBinding dbs[3+MAX_TMUS], *db = dbs;
		uint32_t i;
		//VkSampler samp = VK_GetSampler(0);

		db->binding = db-dbs;
		db->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		db->descriptorCount = 1;
		db->stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
		db->pImmutableSamplers = NULL;
		db++;

		db->binding = db-dbs;
		db->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		db->descriptorCount = 1;
		db->stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
		db->pImmutableSamplers = NULL;
		db++;

#ifdef VK_KHR_acceleration_structure
		if (prog->rayquery)
		{
			db->binding = db-dbs;
			db->descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			db->descriptorCount = 1;
			db->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			db->pImmutableSamplers = NULL;
			db++;
		}
#endif

		for (i = 0; i < 32; i++)
		{
			if (!(prog->defaulttextures & (1u<<i)))
				continue;
			db->binding = db-dbs;
			db->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			db->descriptorCount = 1;
			db->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			db->pImmutableSamplers = NULL;//&samp;
			db++;
		}

		for (i = 0; i < prog->numsamplers; i++)
		{
			db->binding = db-dbs;
			db->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			db->descriptorCount = 1;
			db->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			db->pImmutableSamplers = NULL;//&samp;
			db++;
		}

		descSetLayoutCreateInfo.bindingCount = db-dbs;
		descSetLayoutCreateInfo.pBindings = dbs;
		if (vk.khr_push_descriptor)
			descSetLayoutCreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		VkAssert(vkCreateDescriptorSetLayout(vk.device, &descSetLayoutCreateInfo, NULL, &desclayout));
		prog->desclayout = desclayout;
	}

	{
		VkDescriptorSetLayout sets[1] = {prog->desclayout};
		VkPipelineLayout layout;
		VkPushConstantRange push[1];
		VkPipelineLayoutCreateInfo pipeLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		push[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		push[0].offset = 0;
		push[0].size = sizeof(vec4_t);

		pipeLayoutCreateInfo.flags = 0;
		pipeLayoutCreateInfo.setLayoutCount = countof(sets);
		pipeLayoutCreateInfo.pSetLayouts = sets;
		pipeLayoutCreateInfo.pushConstantRangeCount = !strcmp(name, "fixedemu_flat");
		pipeLayoutCreateInfo.pPushConstantRanges = push;
		VkAssert(vkCreatePipelineLayout(vk.device, &pipeLayoutCreateInfo, vkallocationcb, &layout));
		prog->layout = layout;
	}
}

qboolean VK_LoadBlob(program_t *prog, void *blobdata, const char *name)
{
	//fixme: should validate that the offset+lengths are within the blobdata.
	struct blobheader *blob = blobdata;
	VkShaderModuleCreateInfo info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	VkShaderModule vert, frag;
	unsigned char *cvardata;

	if (blob->blobmagic[0] != 0xff || blob->blobmagic[1] != 'S' || blob->blobmagic[2] != 'P' || blob->blobmagic[3] != 'V')
	{
		Con_Printf(CON_ERROR"Blob %s is outdated\n", name);
		return false;	//bad magic.
	}
	if (blob->blobversion != 1)
	{
		Con_Printf(CON_ERROR"Blob %s is outdated\n", name);
		return false;
	}

	prog->supportedpermutations = blob->permutations;
#define VKPERMUTATION_RAYQUERY			(1u<<31)
	if (blob->permutations&~((PERMUTATIONS-1)|VKPERMUTATION_RAYQUERY))
	{
		Con_Printf("Blob %s has unknown permutations\n", name);
		return false;
	}
	if (prog->supportedpermutations&VKPERMUTATION_RAYQUERY)
	{	//not really a permutation.
#ifndef VK_KHR_ray_query
		Con_Printf(CON_ERROR"Blob %s requires vk_khr_ray_query\n", name);
		return false;
#else
		if (!vk.khr_ray_query)
		{	//the actual spv extension. let compiling catch it?
			Con_Printf(CON_ERROR"Blob %s requires vk_khr_ray_query\n", name);
			return false;
		}
		if (!vk.khr_acceleration_structure)
		{	//what we're meant to be using to feed it... *sigh*
			Con_Printf(CON_ERROR"Blob %s requires vk_khr_acceleration_structure\n", name);
			return false;
		}
		prog->supportedpermutations&=~VKPERMUTATION_RAYQUERY;
		prog->rayquery = true;
#endif
	}
	else
		prog->rayquery = false;

	info.flags = 0;
	info.codeSize = blob->vertlength;
	info.pCode = (uint32_t*)((char*)blob+blob->vertoffset);
	VkAssert(vkCreateShaderModule(vk.device, &info, vkallocationcb, &vert));
	DebugSetName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vert, name);

	info.flags = 0;
	info.codeSize = blob->fraglength;
	info.pCode = (uint32_t*)((char*)blob+blob->fragoffset);
	VkAssert(vkCreateShaderModule(vk.device, &info, vkallocationcb, &frag));
	DebugSetName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)frag, name);

	prog->vert = vert;
	prog->frag = frag;
	prog->numsamplers = blob->numtextures;
	prog->defaulttextures = blob->defaulttextures;

	if (blob->cvarslength)
	{
		prog->cvardata = BZ_Malloc(blob->cvarslength);
		prog->cvardatasize = blob->cvarslength;
		memcpy(prog->cvardata, (char*)blob+blob->cvarsoffset, blob->cvarslength);
	}
	else
	{
		prog->cvardata = NULL;
		prog->cvardatasize = 0;
	}

	//go through the cvars and a) validate them. b) create them with the right defaults.
	//FIXME: validate
	for (cvardata = prog->cvardata; cvardata < prog->cvardata + prog->cvardatasize; )
	{
		unsigned char type = cvardata[2], size = cvardata[3]-'0';
		const char *cvarname;
		cvar_t *var;

		cvardata += 4;
		cvarname = cvardata;
		cvardata += strlen(cvarname)+1;

		if (type >= 'A' && type <= 'Z')
		{	//args will be handled by the blob loader.
			//the blob contains default values, overwrite them with the user's preferences...
			VK_ShaderReadArgument(name, cvarname, type, size, cvardata);
		}
		else
		{
			var = Cvar_FindVar(cvarname);
			if (var)
				var->flags |= CVAR_SHADERSYSTEM;	//just in case
			else
			{
				union
				{
					int i;
					float f;
				} u;
				char value[128];
				uint32_t i;
				*value = 0;
				for (i = 0; i < size; i++)
				{
					u.i = (cvardata[i*4+0]<<24)|(cvardata[i*4+1]<<16)|(cvardata[i*4+2]<<8)|(cvardata[i*4+3]<<0);
					if (i)
						Q_strncatz(value, " ", sizeof(value));
					if (type == 'i' || type == 'b')
						Q_strncatz(value, va("%i", u.i), sizeof(value));
					else
						Q_strncatz(value, va("%f", u.f), sizeof(value));
				}
				Cvar_Get(cvarname, value, CVAR_SHADERSYSTEM, "GLSL Settings");
			}
		}
		cvardata += 4*size;
	}

	VK_FinishProg(prog, name);

	prog->pipelines = NULL;	//generated as needed, depending on blend states etc.
	return true;
}
static void VKBE_ReallyDeleteProg(void *vprog)
{	//nothing else is refering to this data any more, its safe to obliterate it.
	program_t *prog = vprog;
	struct pipeline_s *pipe;
	while(prog->pipelines)
	{
		pipe = prog->pipelines;
		prog->pipelines = pipe->next;

		if (pipe->pipeline)
			vkDestroyPipeline(vk.device, pipe->pipeline, vkallocationcb);
		Z_Free(pipe);
	}
	if (prog->layout)
		vkDestroyPipelineLayout(vk.device, prog->layout, vkallocationcb);
	if (prog->desclayout)
		vkDestroyDescriptorSetLayout(vk.device, prog->desclayout, vkallocationcb);
	if (prog->vert)
		vkDestroyShaderModule(vk.device, prog->vert, vkallocationcb);
	if (prog->frag)
		vkDestroyShaderModule(vk.device, prog->frag, vkallocationcb);
}

void VKBE_DeleteProg(program_t *prog)
{
	//schedule the deletes when its safe to do so.
	VK_AtFrameEnd(VKBE_ReallyDeleteProg, prog, sizeof(*prog));

	//clear stuff out so that the caller doesn't get confused.
	Z_Free(prog->cvardata);
	prog->cvardata = NULL;
	prog->pipelines = NULL;
	prog->layout = VK_NULL_HANDLE;
	prog->desclayout = VK_NULL_HANDLE;
	prog->vert = VK_NULL_HANDLE;
	prog->frag = VK_NULL_HANDLE;
}

static unsigned int VKBE_ApplyShaderBits(unsigned int bits)
{
	if (shaderstate.flags & (BEF_FORCEADDITIVE|BEF_FORCETRANSPARENT|BEF_FORCENODEPTH|BEF_FORCEDEPTHTEST|BEF_FORCEDEPTHWRITE|BEF_LINES))
	{
		if (shaderstate.flags & BEF_FORCEADDITIVE)
			bits = (bits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
						| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE);
		else if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			if ((bits & SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO) || !(bits & SBITS_BLEND_BITS) || (bits&SBITS_ATEST_GE128)) 	/*if transparency is forced, clear alpha test bits*/
				bits = (bits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
							| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}

		if (shaderstate.flags & BEF_FORCENODEPTH) 	/*EF_NODEPTHTEST dp extension*/
			bits |= SBITS_MISC_NODEPTHTEST;
		else
		{
			if (shaderstate.flags & BEF_FORCEDEPTHTEST)
				bits &= ~SBITS_MISC_NODEPTHTEST;
			if (shaderstate.flags & BEF_FORCEDEPTHWRITE)
				bits |= SBITS_MISC_DEPTHWRITE;
		}

		if (shaderstate.flags & BEF_LINES)
			bits |= SBITS_LINES;
	}
	return bits;
}

static const char LIGHTPASS_SHADER[] = "\
{\n\
	program rtlight\n\
	{\n\
		nodepth\n\
		blendfunc add\n\
	}\n\
}";
static const char LIGHTPASS_SHADER_RQ[] = "\
{\n\
	program rq_rtlight\n\
	{\n\
		nodepth\n\
		blendfunc add\n\
	}\n\
}";

void VKBE_Init(void)
{
	int i;
	char *c;

	sh_config.pDeleteProg = VKBE_DeleteProg;

	be_maxpasses = 1;
	memset(&shaderstate, 0, sizeof(shaderstate));
	shaderstate.inited = true;
	for (i = 0; i < MAXRLIGHTMAPS; i++)
		shaderstate.dummybatch.lightmap[i] = -1;

	shaderstate.identitylighting = 1;
	shaderstate.identitylightmap = 1;

	//make sure the world draws correctly
	r_worldentity.shaderRGBAf[0] = 1;
	r_worldentity.shaderRGBAf[1] = 1;
	r_worldentity.shaderRGBAf[2] = 1;
	r_worldentity.shaderRGBAf[3] = 1;
	r_worldentity.axis[0][0] = 1;
	r_worldentity.axis[1][1] = 1;
	r_worldentity.axis[2][2] = 1;
	r_worldentity.light_avg[0] = 1;
	r_worldentity.light_avg[1] = 1;
	r_worldentity.light_avg[2] = 1;

	FTable_Init();

	shaderstate.depthonly = R_RegisterShader("depthonly", SUF_NONE, 
				"{\n"
					"program depthonly\n"
					"{\n"
						"depthwrite\n"
						"maskcolor\n"
					"}\n"
				"}\n");


	shaderstate.programfixedemu[0] = Shader_FindGeneric("fixedemu", QR_VULKAN);
	shaderstate.programfixedemu[1] = Shader_FindGeneric("fixedemu_flat", QR_VULKAN);

	R_InitFlashblends();

/*
	{
		VkDescriptorPoolCreateInfo dpi = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		VkDescriptorPoolSize dpisz[2];
		dpi.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		dpi.maxSets = 512;
		dpi.poolSizeCount = countof(dpisz);
		dpi.pPoolSizes = dpisz;

		dpisz[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		dpisz[0].descriptorCount = 2;

		dpisz[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		dpisz[1].descriptorCount = MAX_TMUS;

		VkAssert(vkCreateDescriptorPool(vk.device, &dpi, NULL, &shaderstate.texturedescpool));
	}
*/
	{
		struct stagingbuf lazybuf;
		void *buffer = VKBE_CreateStagingBuffer(&lazybuf, sizeof(vec4_t)*65536+sizeof(vec3_t)*3*65536, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		vec4_t *col = buffer;
		vec3_t *norm = (vec3_t*)(col+65536);
		vec3_t *sdir = norm+65536;
		vec3_t *tdir = sdir+65536;
		for (i = 0; i < 65536; i++)
		{
			Vector4Set(col[i], 1, 1, 1, 1);
			VectorSet(norm[i], 1, 0, 0);
			VectorSet(sdir[i], 0, 1, 0);
			VectorSet(tdir[i], 0, 0, 1);
		}
		shaderstate.staticbuf = VKBE_FinishStaging(&lazybuf, &shaderstate.staticbufmem);
	}


	c = vk_stagingbuffers.string;
	if (*c)
	{
		vk_usedynamicstaging = 0;
		while (*c)
		{
			if (*c == 'u')
				vk_usedynamicstaging |= 1u<<DB_UBO;
			else if (*c == 'e' || *c == 'i')
				vk_usedynamicstaging |= 1u<<DB_EBO;
			else if (*c == 'v')
				vk_usedynamicstaging |= 1u<<DB_VBO;
			else if (*c == '0')
				vk_usedynamicstaging |= 0;	//for explicly none.
			else
				Con_Printf("%s: unknown char %c\n", vk_stagingbuffers.string, *c);
			c++;
		}
	}
	else
		vk_usedynamicstaging = 0u;
}

static struct descpool *VKBE_CreateDescriptorPool(void)
{
	struct descpool *np = Z_Malloc(sizeof(*np));
	
	VkDescriptorPoolCreateInfo dpi = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	VkDescriptorPoolSize dpisz[3];
	dpi.flags = 0;
	dpi.maxSets = np->totalsets = 512;
	dpi.poolSizeCount = 0;
	dpi.pPoolSizes = dpisz;

	dpisz[dpi.poolSizeCount].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	dpisz[dpi.poolSizeCount].descriptorCount = 2*dpi.maxSets;
	dpi.poolSizeCount++;

	dpisz[dpi.poolSizeCount].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	dpisz[dpi.poolSizeCount].descriptorCount = MAX_TMUS*dpi.maxSets;
	dpi.poolSizeCount++;

#ifdef VK_KHR_acceleration_structure
	if (vk.khr_acceleration_structure)
	{
		dpisz[dpi.poolSizeCount].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		dpisz[dpi.poolSizeCount].descriptorCount = dpi.maxSets;
		dpi.poolSizeCount++;
	}
#endif

	VkAssert(vkCreateDescriptorPool(vk.device, &dpi, NULL, &np->pool));

	return np;
}
static VkDescriptorSet VKBE_TempDescriptorSet(VkDescriptorSetLayout layout)
{
	VkDescriptorSet ret;
	VkDescriptorSetAllocateInfo setinfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

	if (vk.descpool->availsets == 0)
	{
		if (vk.descpool->next)
			vk.descpool = vk.descpool->next;
		else
			vk.descpool = vk.descpool->next = VKBE_CreateDescriptorPool();
		vkResetDescriptorPool(vk.device, vk.descpool->pool, 0);
		vk.descpool->availsets = vk.descpool->totalsets;
	}
	vk.descpool->availsets--;

	setinfo.descriptorPool = vk.descpool->pool;
	setinfo.descriptorSetCount = 1;
	setinfo.pSetLayouts = &layout;
	vkAllocateDescriptorSets(vk.device, &setinfo, &ret);

	return ret;
}

static const struct
{
	const char *name;
	VkBufferUsageFlags usage;
	qboolean nomap;
	VkDeviceSize align;
} dynbuf_info[DB_MAX] =
{	//FIXME: set alignment properly.
	{"DB_VBO",	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
	{"DB_EBO",	VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
	{"DB_UBO",	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
	{"DB_STAGING",	VK_BUFFER_USAGE_TRANSFER_SRC_BIT},
#ifdef VK_KHR_acceleration_structure
	{"DB_ACCELERATIONSTRUCT",	VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, true, 256},
	{"DB_ACCELERATIONSCRATCH",	VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true},
	{"DB_ACCELERATIONMESHDATA",	VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT},
	{"DB_ACCELERATIONINSTANCE",	VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT},
#endif
};
//creates a new dynamic buffer for us to use while streaming. because spoons.
static struct dynbuffer *VKBE_AllocNewStreamingBuffer(struct dynbuffer **link, enum dynbuf_e type, VkDeviceSize minsize)
{
	VkBufferCreateInfo bufinf = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	VkMemoryAllocateFlagsInfo memAllocFlagsInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
	struct dynbuffer *n = Z_Malloc(sizeof(*n));
	qboolean usestaging = (vk_usedynamicstaging & (1u<<type))!=0;

	while(1)
	{
		bufinf.flags = 0;
		bufinf.size = n->size = (1u<<20);
		bufinf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufinf.queueFamilyIndexCount = 0;
		bufinf.pQueueFamilyIndices = NULL;

		while (bufinf.size < minsize)
			bufinf.size *= 2;

		n->size = bufinf.size;

		bufinf.usage = dynbuf_info[type].usage;
		if (type != DB_STAGING && usestaging)
		{
			//create two buffers, one staging/host buffer and one device buffer
			bufinf.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			vkCreateBuffer(vk.device, &bufinf, vkallocationcb, &n->devicebuf);
			DebugSetName(VK_OBJECT_TYPE_BUFFER, (uint64_t)n->devicebuf, dynbuf_info[type].name);
			bufinf.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			vkCreateBuffer(vk.device, &bufinf, vkallocationcb, &n->stagingbuf);
			DebugSetName(VK_OBJECT_TYPE_BUFFER, (uint64_t)n->devicebuf, "DB_AUTOSTAGING");

			vkGetBufferMemoryRequirements(vk.device, n->devicebuf, &mem_reqs);
			n->align = mem_reqs.alignment-1;
			memAllocInfo.allocationSize = mem_reqs.size;
			memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &n->devicememory));
			DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)n->devicememory, "DB_AUTOSTAGING");
			VkAssert(vkBindBufferMemory(vk.device, n->devicebuf, n->devicememory, 0));

			n->renderbuf = n->devicebuf;
		}
		else
		{	//single buffer. we'll write directly to the buffer.
			vkCreateBuffer(vk.device, &bufinf, vkallocationcb, &n->stagingbuf);
			DebugSetName(VK_OBJECT_TYPE_BUFFER, (uint64_t)n->stagingbuf, dynbuf_info[type].name);

			n->renderbuf = n->stagingbuf;
		}

		//now allocate some host-visible memory for the buffer that we're going to map.
		vkGetBufferMemoryRequirements(vk.device, n->stagingbuf, &mem_reqs);
		n->align = mem_reqs.alignment-1;
		memAllocInfo.allocationSize = mem_reqs.size;
		memAllocInfo.memoryTypeIndex = ~0;
		if (memAllocInfo.memoryTypeIndex == ~0 && dynbuf_info[type].nomap)
			memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//	if (memAllocInfo.memoryTypeIndex == ~0)
	//		memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (memAllocInfo.memoryTypeIndex == ~0 && n->renderbuf == n->stagingbuf)	//probably won't get anything, but whatever.
			memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (memAllocInfo.memoryTypeIndex == ~0)
			memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		if (memAllocInfo.memoryTypeIndex == ~0)
		{	//if we can't find any usable memory, force staging instead.
			vkDestroyBuffer(vk.device, n->stagingbuf, vkallocationcb);
			if (usestaging)
				Sys_Error("Unable to allocate buffer memory");
			usestaging = true;
			continue;
		}
		memAllocFlagsInfo.flags = 0;
		if (bufinf.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
			memAllocFlagsInfo.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		if (memAllocFlagsInfo.flags)
			memAllocInfo.pNext = &memAllocFlagsInfo;
		VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &n->stagingmemory));
		DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)n->stagingmemory, dynbuf_info[type].name);
		VkAssert(vkBindBufferMemory(vk.device, n->stagingbuf, n->stagingmemory, 0));

		if (dynbuf_info[type].nomap)
		{
			n->ptr = NULL;	//don't want to map this.
			n->stagingcoherent = true;
		}
		else
		{
			VkAssert(vkMapMemory(vk.device, n->stagingmemory, 0, n->size, 0, &n->ptr));	//persistent-mapped.

			n->stagingcoherent = !!(vk.memory_properties.memoryTypes[memAllocInfo.memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}
		n->next = *link;
		*link = n;
		return n;
	}
}
static void *fte_restrict VKBE_AllocateStreamingSpace(enum dynbuf_e type, size_t datasize, VkBuffer *buf, VkDeviceSize *offset)
{	//FIXME: ubos need alignment
	struct dynbuffer *b = vk.dynbuf[type];
	void *ret;
	if (!b)
	{
		if (!vk.frame->dynbufs[type])
			VKBE_AllocNewStreamingBuffer(&vk.frame->dynbufs[type], type, datasize);
		b = vk.dynbuf[type] = vk.frame->dynbufs[type];
		b->offset = b->flushed = 0;
	}

	if (offset?	//urgh...
		b->offset + datasize > b->size:		//regular offsetable buffer...
		(b->offset || datasize > b->size))	//stoopid buffer space that must have the whole buffer to itself for some reason.
	{
		//flush the old one, just in case.
		if (!b->stagingcoherent)
		{
			VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			range.offset = b->flushed;
			range.size = b->offset-b->flushed;
			range.memory = b->stagingmemory;
			vkFlushMappedMemoryRanges(vk.device, 1, &range);
		}

		if (b->devicebuf != VK_NULL_HANDLE)
		{
			struct vk_fencework *fence = VK_FencedBegin(NULL, 0);
			VkBufferCopy bcr = {0};
			bcr.srcOffset = b->flushed;
			bcr.dstOffset = b->flushed;
			bcr.size = b->offset-b->flushed;
			vkCmdCopyBuffer(fence->cbuf, b->stagingbuf, b->devicebuf, 1, &bcr);
			VK_FencedSubmit(fence);
		}

		if (!b->next)
			VKBE_AllocNewStreamingBuffer(&b->next, type, datasize);
		b = vk.dynbuf[type] = b->next;
		b->offset = 0;
		b->flushed = 0;
	}

	*buf = b->renderbuf;
	if (offset)
		*offset = b->offset;

	ret = (qbyte*)b->ptr + b->offset;
	b->offset += datasize;	//FIXME: alignment
	if (dynbuf_info[type].align)
	{
		b->offset += dynbuf_info[type].align;
		b->offset &= ~(dynbuf_info[type].align-1);
	}
	return ret;
}

//called when a new swapchain has been created.
//makes sure there's no nulls or anything.
void VKBE_InitFramePools(struct vkframe *frame)
{
	uint32_t i;
	for (i = 0; i < DB_MAX; i++)
		frame->dynbufs[i] = NULL;
	frame->descpools = vk.khr_push_descriptor?NULL:VKBE_CreateDescriptorPool();


	frame->numcbufs = 0;
	frame->maxcbufs = 0;
	frame->cbufs = NULL;
	/*{
		VkCommandBufferAllocateInfo cbai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		cbai.commandPool = vk.cmdpool;
		cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbai.commandBufferCount = frame->maxcbufs;
		VkAssert(vkAllocateCommandBuffers(vk.device, &cbai, frame->cbufs));
	}*/

	{
		VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VkAssert(vkCreateFence(vk.device,&fci,vkallocationcb,&frame->finishedfence));
	}
}

//called just before submits
//makes sure that our persistent-mapped memory writes can actually be seen by the hardware.
void VKBE_FlushDynamicBuffers(void)
{
	struct vk_fencework *fence = NULL;
	uint32_t i;
	struct dynbuffer *d;
	VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};

	for (i = 0; i < DB_MAX; i++)
	{
		d = vk.dynbuf[i];
		if (!d || d->flushed == d->offset)
			continue;

		if (!d->stagingcoherent)
		{
			range.offset = d->flushed;
			range.size = d->offset - d->flushed;
			range.memory = d->stagingmemory;
			vkFlushMappedMemoryRanges(vk.device, 1, &range);
		}

		if (d->devicebuf != VK_NULL_HANDLE)
		{	
			VkBufferCopy bcr = {0};
			bcr.srcOffset = d->flushed;
			bcr.dstOffset = d->flushed;
			bcr.size = d->offset - d->flushed;
			if (!fence)
				fence = VK_FencedBegin(NULL, 0);
			vkCmdCopyBuffer(fence->cbuf, d->stagingbuf, d->devicebuf, 1, &bcr);
		}
		d->flushed = d->offset;
	}

	if (fence)
		VK_FencedSubmit(fence);
}

void VKBE_Set2D(qboolean twodee)
{
	if (twodee)
		shaderstate.forcebeflags = BEF_FORCENODEPTH;
	else
		shaderstate.forcebeflags = 0;
	shaderstate.curtime = realtime;
}

#ifdef VK_KHR_acceleration_structure
static void VKBE_DestroyTLAS(void *ctx)
{
	VkAccelerationStructureKHR *tlas = ctx;
	vkDestroyAccelerationStructureKHR(vk.device, *tlas, vkallocationcb);
}
static VkDeviceAddress VKBE_GetBufferDeviceAddress(VkBuffer buf)
{
	VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, NULL, buf};
	return vkGetBufferDeviceAddress(vk.device, &info);
}

struct blasgeom_s
{
	VkBuffer vertbuf;
	VkBuffer idxbuf;
	VkDeviceSize vertoffset;
	VkDeviceSize idxoffset;
	size_t numverts;
	size_t numtris;
};
static qboolean VKBE_GenerateAccelerationMesh_BSP(model_t *mod, struct blasgeom_s *geom)
{
	unsigned int sno;
	msurface_t *surf;
	mesh_t *mesh;
	unsigned int numverts;
	unsigned int numindexes,i;
	unsigned int *ptr_elements;
	vec3_t *ptr_verts;

	numverts = 0;
	numindexes = 0;
	for (sno = 0; sno < mod->nummodelsurfaces; sno++)
	{
		surf = &mod->surfaces[sno+mod->firstmodelsurface];
		if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
			continue;

		if (surf->mesh)
		{
			mesh = surf->mesh;
			numverts += mesh->numvertexes;
			numindexes += mesh->numindexes;
		}
		else if (surf->numedges > 2)
		{
			numverts += surf->numedges;
			numindexes += (surf->numedges-2) * 3;
		}
	}
	if (!numindexes)
		return false;

geom->idxoffset = geom->vertoffset = 0;
	ptr_elements = VKBE_AllocateStreamingSpace(DB_ACCELERATIONMESHDATA, sizeof(*ptr_elements)*numindexes, &geom->idxbuf, &geom->idxoffset);
	ptr_verts = VKBE_AllocateStreamingSpace(DB_ACCELERATIONMESHDATA, sizeof(*ptr_verts)*numverts, &geom->vertbuf, &geom->vertoffset);

	numverts = 0;
	numindexes = 0;

	for (sno = 0; sno < mod->nummodelsurfaces; sno++)
	{
		surf = &mod->surfaces[sno+mod->firstmodelsurface];
		if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
			continue;

		if (surf->mesh)
		{
			mesh = surf->mesh;
			for (i = 0; i < mesh->numvertexes; i++)
				VectorCopy(mesh->xyz_array[i], ptr_verts[numverts+i]);
			for (i = 0; i < mesh->numindexes; i+=3)
			{
				//flip the triangles as we go
				ptr_elements[numindexes+i+0] = numverts+mesh->indexes[i+2];
				ptr_elements[numindexes+i+1] = numverts+mesh->indexes[i+1];
				ptr_elements[numindexes+i+2] = numverts+mesh->indexes[i+0];
			}
			numverts += mesh->numvertexes;
			numindexes += i;
		}
		else if (surf->numedges > 2)
		{
			float *vec;
			medge_t *edge;
			int lindex;
			for (i = 0; i < surf->numedges; i++)
			{
				lindex = mod->surfedges[surf->firstedge + i];

				if (lindex > 0)
				{
					edge = &mod->edges[lindex];
					vec = mod->vertexes[edge->v[0]].position;
				}
				else
				{
					edge = &mod->edges[-lindex];
					vec = mod->vertexes[edge->v[1]].position;
				}

				VectorCopy(vec, ptr_verts[numverts+i]);
			}
			for (i = 2; i < surf->numedges; i++)
			{
				//quake is backwards, not ode
				ptr_elements[numindexes++] = numverts+i;
				ptr_elements[numindexes++] = numverts+i-1;
				ptr_elements[numindexes++] = numverts;
			}
			numverts += surf->numedges;
		}
	}

	geom->numverts = numverts;
	geom->numtris = numindexes/3;
	return true;
}
static VkAccelerationStructureKHR VKBE_GenerateBLAS(model_t *mod)
{
	struct blasgeom_s geom = {VK_NULL_HANDLE,VK_NULL_HANDLE,0,0};
	VkAccelerationStructureCreateInfoKHR asci = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
	VkAccelerationStructureBuildGeometryInfoKHR asbgi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
	uint32_t maxPrimitiveCounts[1] = {0};
	VkAccelerationStructureGeometryKHR asg[1];
	VkAccelerationStructureBuildRangeInfoKHR asbri[1];
	VkAccelerationStructureBuildRangeInfoKHR const *const asbrip = {asbri};

	VkAccelerationStructureBuildSizesInfoKHR asbsi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

	VkBuffer transformbuf, scratchbuf;
	VkDeviceSize transformoffset = 0;

	//this is stupid. oh well.
	VkTransformMatrixKHR *transform;
	transform = VKBE_AllocateStreamingSpace(DB_ACCELERATIONINSTANCE, sizeof(*transform), &transformbuf, &transformoffset);
	Vector4Set(transform->matrix[0], 1,0,0,0);
	Vector4Set(transform->matrix[1], 0,1,0,0);
	Vector4Set(transform->matrix[2], 0,0,1,0);

	//FIXME: use of VKBE_AllocateStreamingSpace on the geomdata, transform, and blas storage itself mean we can only use this for a single frame, regenerating each time. which is wasteful for a blas that contains the entire worldmodel.
	VKBE_GenerateAccelerationMesh_BSP(mod, &geom);

	asg[0].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	asg[0].pNext = NULL;
	asg[0].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	asg[0].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	asg[0].geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	asg[0].geometry.triangles.pNext = NULL;
	asg[0].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	asg[0].geometry.triangles.vertexData.deviceAddress = VKBE_GetBufferDeviceAddress(geom.vertbuf);
	asg[0].geometry.triangles.vertexStride = sizeof(vec3_t);
	asg[0].geometry.triangles.maxVertex = geom.numverts;
	asg[0].geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	asg[0].geometry.triangles.indexData.deviceAddress = VKBE_GetBufferDeviceAddress(geom.idxbuf);
	asg[0].geometry.triangles.transformData.deviceAddress = VKBE_GetBufferDeviceAddress(transformbuf);

	asbri[0].firstVertex = geom.vertoffset/sizeof(vec3_t);
	asbri[0].primitiveCount = maxPrimitiveCounts[0] = geom.numtris;
	asbri[0].primitiveOffset = geom.idxoffset;
	asbri[0].transformOffset = transformoffset;

	asci.createFlags = 0;
	asci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	asci.deviceAddress = 0;	//no overriding here.

	asbgi.type = asci.type;
	asbgi.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR /* | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR*/;
	asbgi.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	asbgi.srcAccelerationStructure = VK_NULL_HANDLE; //ignored here
	asbgi.dstAccelerationStructure = VK_NULL_HANDLE; //filled in later
	asbgi.geometryCount = countof(asg);
	asbgi.pGeometries = asg;
	asbgi.ppGeometries = NULL;	//too much indirection! oh noes!

	VKBE_FlushDynamicBuffers();

	vkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &asbgi, maxPrimitiveCounts, &asbsi);
	VKBE_AllocateStreamingSpace(DB_ACCELERATIONSTRUCT, asci.size = asbsi.accelerationStructureSize, &asci.buffer, &asci.offset);
	VKBE_AllocateStreamingSpace(DB_ACCELERATIONSCRATCH, asbsi.buildScratchSize, &scratchbuf, NULL);
	asbgi.scratchData.deviceAddress = VKBE_GetBufferDeviceAddress(scratchbuf);

	vkCreateAccelerationStructureKHR(vk.device, &asci, vkallocationcb, &asbgi.dstAccelerationStructure);
	DebugSetName(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)asbgi.dstAccelerationStructure, "ShadowBLAS");
	vkCmdBuildAccelerationStructuresKHR(vk.rendertarg->cbuf, 1, &asbgi, &asbrip);

	{
		VkMemoryBarrier membarrier	= {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
		membarrier.srcAccessMask	= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		membarrier.dstAccessMask	= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		vkCmdPipelineBarrier(vk.rendertarg->cbuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &membarrier, 0, NULL, 0, NULL);
	}
	return asbgi.dstAccelerationStructure;
}
static VkAccelerationStructureKHR VKBE_GenerateTLAS(void)
{
	VkAccelerationStructureKHR blas = VKBE_GenerateBLAS(r_worldentity.model);
	VkAccelerationStructureDeviceAddressInfoKHR blasinfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, NULL, blas};
	VkAccelerationStructureCreateInfoKHR asci = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
	VkAccelerationStructureBuildGeometryInfoKHR asbgi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
	uint32_t maxPrimitiveCounts[1] = {0};
	VkAccelerationStructureGeometryKHR asg[1];
	VkAccelerationStructureBuildRangeInfoKHR asbri[1];
	VkAccelerationStructureBuildRangeInfoKHR const *const asbrip = {asbri};

	VkAccelerationStructureBuildSizesInfoKHR asbsi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

	VkBuffer instancesbuf, scratchbuf;
	VkDeviceSize instancesofs = 0;
	size_t numinstances = 1;
	VkAccelerationStructureInstanceKHR *instances = VKBE_AllocateStreamingSpace(DB_ACCELERATIONINSTANCE, sizeof(*instances)*numinstances, &instancesbuf, &instancesofs);

#if 0
	batch_t **worldbatches = r_worldentity.model->batches; //FIXME
	batch_t *batch;
	int i, id = 0;
	for (i = 0; i < SHADER_SORT_COUNT; i++)
	{
		if (worldbatches)
		{
			for (batch = worldbatches[i]; batch; batch = batch->next)
			{
				if (batch->meshes == batch->firstmesh)
					continue;	//nothing to do...

				if (batch->buildmeshes)
					batch->buildmeshes(batch);

				{
					shader_t *shader = batch->shader;
					unsigned int bf;
					unsigned int nummeshes = batch->meshes - batch->firstmesh;
					if (!nummeshes)
						continue;

					//ubo[id].stuff = ...;

					VectorCopy(batch->ent->axis[0], instances->transform.matrix[0]);	instances->transform.matrix[0][3] = batch->ent->origin[0];
					VectorCopy(batch->ent->axis[1], instances->transform.matrix[1]);	instances->transform.matrix[1][3] = batch->ent->origin[1];
					VectorCopy(batch->ent->axis[2], instances->transform.matrix[2]);	instances->transform.matrix[2][3] = batch->ent->origin[2];
					instances->instanceCustomIndex = id++;	//extra info
					if (batch->shader->flags & SHADER_SKY)
						instances->mask = 0x2;
					else if (batch->shader->sort > SHADER_SORT_OPAQUE)
						instances->mask = 0x4;
					else
						instances->mask = 0x1;
					instances->instanceShaderBindingTableRecordOffset = shader->id;	//material id
					if (shader->flags & SHADER_CULL_FRONT)
						instances->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
					else if (shader->flags & SHADER_CULL_BACK)
						instances->flags = 0;
					else
						instances->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
					instances->accelerationStructureReference = batch->blas;	//no half measures
				}
			}
		}
		//and non-world too... may require temporary blas for lerping/skeletal models.
	}
#else
	Vector4Set(instances->transform.matrix[0], 1,0,0,0);
	Vector4Set(instances->transform.matrix[1], 0,1,0,0);
	Vector4Set(instances->transform.matrix[2], 0,0,1,0);
	instances->instanceCustomIndex = 0; //index into our ssbo... if we had one...
	instances->mask = 0x01;
	instances->instanceShaderBindingTableRecordOffset = 0;	//FIXME: alphamasked stuff needs a texture somehow
	instances->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; //FIXME: optimise
	instances->accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(vk.device, &blasinfo);
#endif
	asg[0].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	asg[0].pNext = NULL;
	asg[0].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	asg[0].geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	asg[0].geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	asg[0].geometry.instances.pNext = NULL;
	asg[0].geometry.instances.arrayOfPointers = false;
	asg[0].geometry.instances.data.deviceAddress = VKBE_GetBufferDeviceAddress(instancesbuf);

	asbri[0].firstVertex = 0;
	asbri[0].primitiveCount = maxPrimitiveCounts[0] = numinstances;
	asbri[0].primitiveOffset = instancesofs;
	asbri[0].transformOffset = 0;

	asci.createFlags = 0;
	asci.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	asci.deviceAddress = 0;	//no overriding here.

	asbgi.type = asci.type;
	asbgi.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR /* | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR*/;
	asbgi.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	asbgi.srcAccelerationStructure = VK_NULL_HANDLE; //ignored here
	asbgi.dstAccelerationStructure = VK_NULL_HANDLE; //filled in later
	asbgi.geometryCount = countof(asg);
	asbgi.pGeometries = asg;
	asbgi.ppGeometries = NULL;	//too much indirection! oh noes!

	VKBE_FlushDynamicBuffers();
	vkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &asbgi, maxPrimitiveCounts, &asbsi);
	VKBE_AllocateStreamingSpace(DB_ACCELERATIONSTRUCT, asci.size = asbsi.accelerationStructureSize, &asci.buffer, &asci.offset);
	VKBE_AllocateStreamingSpace(DB_ACCELERATIONSCRATCH, asbsi.buildScratchSize, &scratchbuf, NULL);
	asbgi.scratchData.deviceAddress = VKBE_GetBufferDeviceAddress(scratchbuf);

	vkCreateAccelerationStructureKHR(vk.device, &asci, vkallocationcb, &asbgi.dstAccelerationStructure);
	DebugSetName(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)asbgi.dstAccelerationStructure, "ShadowTLAS");
	vkCmdBuildAccelerationStructuresKHR(vk.rendertarg->cbuf, 1, &asbgi, &asbrip);
	VK_AtFrameEnd(VKBE_DestroyTLAS, &asbgi.dstAccelerationStructure, sizeof(asbgi.dstAccelerationStructure));	//clean up the tlas, each frame gets a new one.
	VK_AtFrameEnd(VKBE_DestroyTLAS, &blas, sizeof(blas));	//clean up the tlas, each frame gets a new one.

	{
		VkMemoryBarrier membarrier	= {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
		membarrier.srcAccessMask	= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		membarrier.dstAccessMask	= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		vkCmdPipelineBarrier(vk.rendertarg->cbuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &membarrier, 0, NULL, 0, NULL);
	}
	//FIXME: use a compute cbuf, add a fence to block the rtlight queries until this BVH is done.
	//vkResetEvent
	//vkCmdSetEvent
	//vkCmdPipelineBarrier
	return asbgi.dstAccelerationStructure;
}
#endif

//called at the start of each frame
//resets the working dynamic buffers to this frame's storage, to avoid stepping on frames owned by the gpu
void VKBE_RestartFrame(void)
{
	uint32_t i;
	for (i = 0; i < DB_MAX; i++)
		vk.dynbuf[i] = NULL;

	shaderstate.rc.activepipeline = VK_NULL_HANDLE;
	vk.descpool = vk.frame->descpools;
	if (vk.descpool)
	{
		vkResetDescriptorPool(vk.device, vk.descpool->pool, 0);
		vk.descpool->availsets = vk.descpool->totalsets;
	}

#ifdef VK_KHR_acceleration_structure
	if (vk.khr_ray_query && r_worldentity.model && shaderstate.needtlas)
		shaderstate.tlas = VKBE_GenerateTLAS();
	else
		shaderstate.tlas = VK_NULL_HANDLE;
	shaderstate.needtlas = false;
#endif
}

void VKBE_ShutdownFramePools(struct vkframe *frame)
{
	struct dynbuffer *db;
	struct descpool *dp;
	uint32_t i;

	for (i = 0; i < DB_MAX; i++)
	{
		while(frame->dynbufs[i])
		{
			db = frame->dynbufs[i];
			vkDestroyBuffer(vk.device, db->stagingbuf, vkallocationcb);
			vkFreeMemory(vk.device, db->stagingmemory, vkallocationcb);
			if (db->devicebuf != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(vk.device, db->devicebuf, vkallocationcb);
				vkFreeMemory(vk.device, db->devicememory, vkallocationcb);
			}
			frame->dynbufs[i] = db->next;
			Z_Free(db);
		}
	}

	while(frame->descpools)
	{
		dp = frame->descpools;
		vkDestroyDescriptorPool(vk.device, dp->pool, vkallocationcb);
		frame->descpools = dp->next;
		Z_Free(dp);
	}
}

void VKBE_Shutdown(void)
{
	if (!shaderstate.inited)
		return;

#ifdef RTLIGHTS
	Sh_Shutdown();
#endif

	Shader_ReleaseGeneric(shaderstate.programfixedemu[0]);
	Shader_ReleaseGeneric(shaderstate.programfixedemu[1]);

	shaderstate.inited = false;
#ifdef RTLIGHTS
	VK_TerminateShadowMap();
#endif
	Z_Free(shaderstate.wbatches);
	shaderstate.wbatches = NULL;

	vkDestroyBuffer(vk.device, shaderstate.staticbuf, vkallocationcb);
	VK_ReleasePoolMemory(&shaderstate.staticbufmem);
}

static texid_t SelectPassTexture(const shaderpass_t *pass)
{
	safeswitch(pass->texgen)
	{
	case T_GEN_DIFFUSE:
		return shaderstate.curtexnums->base;
	case T_GEN_NORMALMAP:
		if (TEXLOADED(shaderstate.curtexnums->bump))
			return shaderstate.curtexnums->bump;
		else
			return missing_texture_normal;
	case T_GEN_SPECULAR:
		if (TEXLOADED(shaderstate.curtexnums->specular))
			return shaderstate.curtexnums->specular;
		else
			return missing_texture_gloss;
	case T_GEN_UPPEROVERLAY:
		return shaderstate.curtexnums->upperoverlay;
	case T_GEN_LOWEROVERLAY:
		return shaderstate.curtexnums->loweroverlay;
	case T_GEN_FULLBRIGHT:
		return shaderstate.curtexnums->fullbright;
	case T_GEN_PALETTED:
		return shaderstate.curtexnums->paletted;
	case T_GEN_REFLECTCUBE:
		if (TEXLOADED(shaderstate.curtexnums->reflectcube))
			return shaderstate.curtexnums->reflectcube;
		else if (shaderstate.curbatch->envmap)
			return shaderstate.curbatch->envmap;
		else
			return r_blackcubeimage;	//FIXME
	case T_GEN_REFLECTMASK:
		return shaderstate.curtexnums->reflectmask;
	case T_GEN_DISPLACEMENT:
		return shaderstate.curtexnums->displacement;
	case T_GEN_OCCLUSION:
		return shaderstate.curtexnums->occlusion;
	case T_GEN_TRANSMISSION:
		return shaderstate.curtexnums->transmission;
	case T_GEN_THICKNESS:
		return shaderstate.curtexnums->thickness;

	case T_GEN_ANIMMAP:
		return pass->anim_frames[(int)(pass->anim_fps * shaderstate.curtime) % pass->anim_numframes];
	case T_GEN_SINGLEMAP:
		return pass->anim_frames[0];
	case T_GEN_DELUXMAP:
		{
			int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi < 0 || !lightmap[lmi]->hasdeluxe)
				return r_nulltex;
			else
			{
				lmi+=1;
				return lightmap[lmi]->lightmap_texture;
			}
		}
	case T_GEN_LIGHTMAP:
		{
			int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi < 0)
				return r_whiteimage;
			else
				return lightmap[lmi]->lightmap_texture;
		}

	case T_GEN_CURRENTRENDER:
		return shaderstate.tex_currentrender;
#ifdef HAVE_MEDIA_DECODER
	case T_GEN_VIDEOMAP:
		if (pass->cin)
			return Media_UpdateForShader(pass->cin);
		return r_nulltex;
#endif

	case T_GEN_LIGHTCUBEMAP:	//light's projected cubemap
		if (shaderstate.curdlight)
			return shaderstate.curdlight->cubetexture;
		else
			return r_blackcubeimage;

	case T_GEN_SHADOWMAP:	//light's depth values.
		return shaderstate.currentshadowmap;

	case T_GEN_REFLECTION:	//reflection image (mirror-as-fbo)
		return &shaderstate.rt_reflection.q_colour;
	case T_GEN_REFRACTION:	//refraction image (portal-as-fbo)
		return shaderstate.tex_refraction;
	case T_GEN_REFRACTIONDEPTH:	//refraction image (portal-as-fbo)
		return &shaderstate.rt_refraction.q_depth;
	case T_GEN_RIPPLEMAP:	//ripplemap image (water surface distortions-as-fbo)
		return shaderstate.tex_ripplemap;

	case T_GEN_SOURCECOLOUR: //used for render-to-texture targets
		return vk.sourcecolour;
	case T_GEN_SOURCEDEPTH:	//used for render-to-texture targets
		return vk.sourcedepth;

	case T_GEN_SOURCECUBE:	//used for render-to-texture targets
		return r_blackcubeimage;

	case T_GEN_GBUFFER0:
	case T_GEN_GBUFFER1:
	case T_GEN_GBUFFER2:
	case T_GEN_GBUFFER3:
	case T_GEN_GBUFFER4:
	case T_GEN_GBUFFER5:
	case T_GEN_GBUFFER6:
	case T_GEN_GBUFFER7:
	safedefault:
		return r_nulltex;
	}
	return r_nulltex;
}

static void T_Gen_CurrentRender(void)
{
	vk_image_t *img;
	/*gah... I pitty the gl drivers*/
	if (!shaderstate.tex_currentrender)
	{
		shaderstate.tex_currentrender = Image_CreateTexture("***$currentrender***", NULL, 0);
		shaderstate.tex_currentrender->vkimage = Z_Malloc(sizeof(*shaderstate.tex_currentrender->vkimage));
	}
	img = shaderstate.tex_currentrender->vkimage;
	if (img->width != vid.fbpwidth || img->height != vid.fbpheight)
	{
		//FIXME: free the old image when its safe to do so.
		*img = VK_CreateTexture2DArray(vid.fbpwidth, vid.fbpheight, 1, 1, -vk.backbufformat, PTI_2D, false, shaderstate.tex_currentrender->ident);

		if (!img->sampler)
			VK_CreateSampler(shaderstate.tex_currentrender->flags, img);
	}


	vkCmdEndRenderPass(vk.rendertarg->cbuf);
	
	//submit now?

	//copy the backbuffer to our image
	{
		VkImageCopy region;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;
		region.dstOffset.x = 0;
		region.dstOffset.y = 0;
		region.dstOffset.z = 0;
		region.extent.width = vid.fbpwidth;
		region.extent.height = vid.fbpheight;
		region.extent.depth = 1;

		set_image_layout(vk.rendertarg->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		VK_ACCESS_TRANSFER_READ_BIT,		VK_PIPELINE_STAGE_TRANSFER_BIT);
		set_image_layout(vk.rendertarg->cbuf, img->image, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,			0,					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		VK_ACCESS_TRANSFER_WRITE_BIT,		VK_PIPELINE_STAGE_TRANSFER_BIT);
		vkCmdCopyImage(vk.rendertarg->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		set_image_layout(vk.rendertarg->cbuf, img->image, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		VK_ACCESS_TRANSFER_WRITE_BIT,		VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	VK_ACCESS_SHADER_READ_BIT,		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		set_image_layout(vk.rendertarg->cbuf, vk.frame->backbuf->colour.image, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		VK_ACCESS_TRANSFER_READ_BIT,		VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		img->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}


	//submit now?
	//barrier?
	vkCmdBeginRenderPass(vk.rendertarg->cbuf, &vk.rendertarg->restartinfo, VK_SUBPASS_CONTENTS_INLINE);
	//fixme: viewport+scissor?
}

static void R_FetchPlayerColour(unsigned int cv, vec3_t rgb)
{
	int i;

	if (cv >= 16)
	{
		rgb[0] = (((cv&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[15]+0)) / (256.0*256);
		rgb[1] = (((cv&0x00ff00)>>8)**((unsigned char*)&d_8to24rgbtable[15]+1)) / (256.0*256);
		rgb[2] = (((cv&0x0000ff)>>0)**((unsigned char*)&d_8to24rgbtable[15]+2)) / (256.0*256);
		return;
	}
	i = cv;
	if (i >= 8)
	{
		i<<=4;
	}
	else
	{
		i<<=4;
		i+=15;
	}
	i*=3;
	rgb[0] = host_basepal[i+0] / 255.0;
	rgb[1] = host_basepal[i+1] / 255.0;
	rgb[2] = host_basepal[i+2] / 255.0;
/*	if (!gammaworks)
	{
		*retred = gammatable[*retred];
		*retgreen = gammatable[*retgreen];
		*retblue = gammatable[*retblue];
	}*/
}

//source is always packed
//dest is packed too
static void colourgen(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, avec4_t *srcf, vec4_t *dst, const mesh_t *mesh)
{
	switch (pass->rgbgen)
	{
	case RGB_GEN_ENTITY:
		while((cnt)--)
		{
			dst[cnt][0] = shaderstate.curentity->shaderRGBAf[0];
			dst[cnt][1] = shaderstate.curentity->shaderRGBAf[1];
			dst[cnt][2] = shaderstate.curentity->shaderRGBAf[2];
		}
		break;
	case RGB_GEN_ONE_MINUS_ENTITY:
		while((cnt)--)
		{
			dst[cnt][0] = 1-shaderstate.curentity->shaderRGBAf[0];
			dst[cnt][1] = 1-shaderstate.curentity->shaderRGBAf[1];
			dst[cnt][2] = 1-shaderstate.curentity->shaderRGBAf[2];
		}
		break;
	case RGB_GEN_VERTEX_LIGHTING:
#if MAXRLIGHTMAPS > 1
		if (mesh->colors4f_array[1])
		{
			float lm[MAXRLIGHTMAPS];
			lm[0] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]]/256.0f*shaderstate.identitylighting;
			lm[1] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[1]]/256.0f*shaderstate.identitylighting;
			lm[2] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[2]]/256.0f*shaderstate.identitylighting;
			lm[3] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[3]]/256.0f*shaderstate.identitylighting;
			while((cnt)--)
			{
				VectorScale(		mesh->colors4f_array[0][cnt], lm[0], dst[cnt]);
				VectorMA(dst[cnt],	lm[1], mesh->colors4f_array[1][cnt], dst[cnt]);
				VectorMA(dst[cnt],	lm[2], mesh->colors4f_array[2][cnt], dst[cnt]);
				VectorMA(dst[cnt],	lm[3], mesh->colors4f_array[3][cnt], dst[cnt]);
			}
			break;
		}
#endif

		if (shaderstate.identitylighting != 1)
		{
			if (srcf)
			{
				while((cnt)--)
				{
					dst[cnt][0] = srcf[cnt][0]*shaderstate.identitylighting;
					dst[cnt][1] = srcf[cnt][1]*shaderstate.identitylighting;
					dst[cnt][2] = srcf[cnt][2]*shaderstate.identitylighting;
				}
			}
			else if (srcb)
			{
				float t = shaderstate.identitylighting * (1/255.0);
				while((cnt)--)
				{
					dst[cnt][0] = srcb[cnt][0]*t;
					dst[cnt][1] = srcb[cnt][1]*t;
					dst[cnt][2] = srcb[cnt][2]*t;
				}
			}
			else
			{
				while((cnt)--)
				{
					dst[cnt][0] = shaderstate.identitylighting;
					dst[cnt][1] = shaderstate.identitylighting;
					dst[cnt][2] = shaderstate.identitylighting;
				}
			}
			break;
		}
	case RGB_GEN_VERTEX_EXACT:
		if (srcf)
		{
			while((cnt)--)
			{
				dst[cnt][0] = srcf[cnt][0];
				dst[cnt][1] = srcf[cnt][1];
				dst[cnt][2] = srcf[cnt][2];
			}
		}
		else if (srcb)
		{
			float t = 1/255.0;
			while((cnt)--)
			{
				dst[cnt][0] = srcb[cnt][0]*t;
				dst[cnt][1] = srcb[cnt][1]*t;
				dst[cnt][2] = srcb[cnt][2]*t;
			}
		}
		else
		{
			while((cnt)--)
			{
				dst[cnt][0] = 1;
				dst[cnt][1] = 1;
				dst[cnt][2] = 1;
			}
			break;
		}
		break;
	case RGB_GEN_ONE_MINUS_VERTEX:
		if (srcf)
		{
			while((cnt)--)
			{
				dst[cnt][0] = 1-srcf[cnt][0];
				dst[cnt][1] = 1-srcf[cnt][1];
				dst[cnt][2] = 1-srcf[cnt][2];
			}
		}
		break;
	case RGB_GEN_IDENTITY_LIGHTING:
		if (shaderstate.curbatch->vtlightstyle[0] != 255 && d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]] != 256)
		{
			vec_t val = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]]/256.0f;
			while((cnt)--)
			{
				dst[cnt][0] = val;
				dst[cnt][1] = val;
				dst[cnt][2] = val;
			}
		}
		else
		{
			//compensate for overbrights
			while((cnt)--)
			{
				dst[cnt][0] = shaderstate.identitylighting;
				dst[cnt][1] = shaderstate.identitylighting;
				dst[cnt][2] = shaderstate.identitylighting;
			}
		}
		break;
	case RGB_GEN_IDENTITY_OVERBRIGHT:
		while((cnt)--)
		{
			dst[cnt][0] = shaderstate.identitylightmap;
			dst[cnt][1] = shaderstate.identitylightmap;
			dst[cnt][2] = shaderstate.identitylightmap;
		}
		break;
	default:
	case RGB_GEN_IDENTITY:
		while((cnt)--)
		{
			dst[cnt][0] = shaderstate.identitylighting;
			dst[cnt][1] = shaderstate.identitylighting;
			dst[cnt][2] = shaderstate.identitylighting;
		}
		break;
	case RGB_GEN_CONST:
		while((cnt)--)
		{
			dst[cnt][0] = pass->rgbgen_func.args[0];
			dst[cnt][1] = pass->rgbgen_func.args[1];
			dst[cnt][2] = pass->rgbgen_func.args[2];
		}
		break;
	case RGB_GEN_ENTITY_LIGHTING_DIFFUSE:
		R_LightArrays(shaderstate.curentity, mesh->xyz_array, dst, cnt, mesh->normals_array, shaderstate.identitylighting, true);
		break;
	case RGB_GEN_LIGHTING_DIFFUSE:
		R_LightArrays(shaderstate.curentity, mesh->xyz_array, dst, cnt, mesh->normals_array, shaderstate.identitylighting, false);
		break;
	case RGB_GEN_WAVE:
		{
			float *table;
			float c;

			table = FTableForFunc(pass->rgbgen_func.type);
			c = pass->rgbgen_func.args[2] + shaderstate.curtime * pass->rgbgen_func.args[3];
			c = FTABLE_EVALUATE(table, c) * pass->rgbgen_func.args[1] + pass->rgbgen_func.args[0];
			c = bound(0.0f, c, 1.0f);

			while((cnt)--)
			{
				dst[cnt][0] = c;
				dst[cnt][1] = c;
				dst[cnt][2] = c;
			}
		}
		break;

	case RGB_GEN_TOPCOLOR:
		if (cnt)
		{
			vec3_t rgb;
			R_FetchPlayerColour(shaderstate.curentity->topcolour, rgb);
			while((cnt)--)
			{
				dst[cnt][0] = rgb[0];
				dst[cnt][1] = rgb[1];
				dst[cnt][2] = rgb[2];
			}
		}
		break;
	case RGB_GEN_BOTTOMCOLOR:
		if (cnt)
		{
			vec3_t rgb;
			R_FetchPlayerColour(shaderstate.curentity->bottomcolour, rgb);
			while((cnt)--)
			{
				dst[cnt][0] = rgb[0];
				dst[cnt][1] = rgb[1];
				dst[cnt][2] = rgb[2];
			}
		}
		break;
	}
}
static void alphagen(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, avec4_t *srcf, avec4_t *dst, const mesh_t *mesh)
{
	float *table;
	float t;
	float f;
	vec3_t v1, v2;
	int i;

	switch (pass->alphagen)
	{
	default:
	case ALPHA_GEN_IDENTITY:
		if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			while(cnt--)
				dst[cnt][3] = shaderstate.curentity->shaderRGBAf[3];
		}
		else
		{
			while(cnt--)
				dst[cnt][3] = 1;
		}
		break;

	case ALPHA_GEN_CONST:
		t = pass->alphagen_func.args[0];
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_WAVE:
		table = FTableForFunc(pass->alphagen_func.type);
		f = pass->alphagen_func.args[2] + shaderstate.curtime * pass->alphagen_func.args[3];
		f = FTABLE_EVALUATE(table, f) * pass->alphagen_func.args[1] + pass->alphagen_func.args[0];
		t = bound(0.0f, f, 1.0f);
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_PORTAL:
		//FIXME: should this be per-vert?
		if (r_refdef.recurse)
			f = 1;
		else
		{
			VectorAdd(mesh->xyz_array[0], shaderstate.curentity->origin, v1);
			VectorSubtract(r_origin, v1, v2);
			f = VectorLength(v2) * (1.0 / shaderstate.curshader->portaldist);
			f = bound(0.0f, f, 1.0f);
		}

		while(cnt--)
			dst[cnt][3] = f;
		break;

	case ALPHA_GEN_VERTEX:
		if (srcf)
		{
			while(cnt--)
			{
				dst[cnt][3] = srcf[cnt][3];
			}
		}
		else if (srcb)
		{
			float t = 1/255.0;
			while(cnt--)
			{
				dst[cnt][3] = srcb[cnt][3]*t;
			}
		}
		else
		{
			while(cnt--)
			{
				dst[cnt][3] = 1;
			}
			break;
		}
		break;

	case ALPHA_GEN_ENTITY:
		f = bound(0, shaderstate.curentity->shaderRGBAf[3], 1);
		while(cnt--)
		{
			dst[cnt][3] = f;
		}
		break;


	case ALPHA_GEN_SPECULAR:
		{
			VectorSubtract(r_origin, shaderstate.curentity->origin, v1);

			if (!Matrix3_Compare((const vec3_t*)shaderstate.curentity->axis, (const vec3_t*)axisDefault))
			{
				Matrix3_Multiply_Vec3(shaderstate.curentity->axis, v1, v2);
			}
			else
			{
				VectorCopy(v1, v2);
			}

			for (i = 0; i < cnt; i++)
			{
				VectorSubtract(v2, mesh->xyz_array[i], v1);
				f = DotProduct(v1, mesh->normals_array[i] ) * Q_rsqrt(DotProduct(v1,v1));
				f = f * f * f * f * f;
				dst[i][3] = bound (0.0f, f, 1.0f);
			}
		}
		break;
	}
}

//true if we used an array (flag to use uniforms for it instead if false)
static void BE_GenerateColourMods(unsigned int vertcount, const shaderpass_t *pass, VkBuffer *buffer, VkDeviceSize *offset)
{
	const mesh_t *m = shaderstate.meshlist[0];
//	if (pass->flags & SHADER_PASS_NOCOLORARRAY)
//		error
	if (					   ((pass->rgbgen == RGB_GEN_VERTEX_LIGHTING) ||
								(pass->rgbgen == RGB_GEN_VERTEX_EXACT) ||
								(pass->rgbgen == RGB_GEN_ONE_MINUS_VERTEX)) &&
								(pass->alphagen == ALPHA_GEN_VERTEX))
	{
		if (shaderstate.batchvbo)
		{	//just use the colour vbo provided
			*buffer = shaderstate.batchvbo->colours[0].vk.buff;
			*offset = shaderstate.batchvbo->colours[0].vk.offs;
		}
		else
		{	//we can't use the vbo due to gaps that we don't want to have to deal with
			//we can at least ensure that the data is written in one go to aid cpu cache.
			vec4_t *fte_restrict map;
			unsigned int mno;
			map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec4_t), buffer, offset);
			if (m->colors4f_array[0])
			{
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->colors4f_array[0], m->numvertexes * sizeof(vec4_t));
					map += m->numvertexes;
				}
			}
			else if (m->colors4b_array)
			{
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					uint32_t v;
					m = shaderstate.meshlist[mno];
					for (v = 0; v < m->numvertexes; v++)
						Vector4Scale(m->colors4b_array[v], 1.0/255, map[v]);
					map += m->numvertexes;
				}
			}
			else
			{
				for (mno = 0; mno < vertcount; mno++)
					Vector4Set(map[mno], 1, 1, 1, 1);
			}
		}
	}
	else
	{
		vec4_t *fte_restrict map;
		unsigned int mno;
		map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec4_t), buffer, offset);
		for (mno = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			colourgen(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], map, m);
			alphagen(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], map, m);
			map += m->numvertexes;
		}
	}
}

/*********************************************************************************************************/
/*========================================== texture coord generation =====================================*/
static void tcgen_environment(float *st, unsigned int numverts, float *xyz, float *normal)
{
	int			i;
	vec3_t		viewer, reflected;
	float		d;

	vec3_t		rorg;

	RotateLightVector(shaderstate.curentity->axis, shaderstate.curentity->origin, r_origin, rorg);

	for (i = 0 ; i < numverts ; i++, xyz += sizeof(vecV_t)/sizeof(vec_t), normal += 3, st += 2 )
	{
		VectorSubtract (rorg, xyz, viewer);
		VectorNormalizeFast (viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

static float *tcgen(const shaderpass_t *pass, int cnt, float *dst, const mesh_t *mesh)
{
	int i;
	vecV_t *src;
	switch (pass->tcgen)
	{
	default:
	case TC_GEN_BASE:
		return (float*)mesh->st_array;
	case TC_GEN_LIGHTMAP:
		return (float*)mesh->lmst_array[0];
	case TC_GEN_NORMAL:
		return (float*)mesh->normals_array;
	case TC_GEN_SVECTOR:
		return (float*)mesh->snormals_array;
	case TC_GEN_TVECTOR:
		return (float*)mesh->tnormals_array;
	case TC_GEN_ENVIRONMENT:
		if (!mesh->normals_array)
			return (float*)mesh->st_array;
		tcgen_environment(dst, cnt, (float*)mesh->xyz_array, (float*)mesh->normals_array);
		return dst;

	case TC_GEN_DOTPRODUCT:
		return dst;//mesh->st_array[0];
	case TC_GEN_VECTOR:
		src = mesh->xyz_array;
		for (i = 0; i < cnt; i++, dst += 2)
		{
			dst[0] = DotProduct(pass->tcgenvec[0], src[i]);
			dst[1] = DotProduct(pass->tcgenvec[1], src[i]);
		}
		return dst;
	}
}

/*src and dst can be the same address when tcmods are chained*/
static void tcmod(const tcmod_t *tcmod, int cnt, const float *src, float *dst, const mesh_t *mesh)
{
	float *table;
	float t1, t2;
	float cost, sint;
	int j;

	switch (tcmod->type)
	{
		case SHADER_TCMOD_ROTATE:
			cost = tcmod->args[0] * shaderstate.curtime;
			sint = R_FastSin(cost);
			cost = R_FastSin(cost + 0.25);

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				t1 = cost * (src[0] - 0.5f) - sint * (src[1] - 0.5f) + 0.5f;
				t2 = cost * (src[1] - 0.5f) + sint * (src[0] - 0.5f) + 0.5f;
				dst[0] = t1;
				dst[1] = t2;
			}
			break;

		case SHADER_TCMOD_SCALE:
			t1 = tcmod->args[0];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1;
				dst[1] = src[1] * t2;
			}
			break;

		case SHADER_TCMOD_TURB:
			t1 = tcmod->args[2] + shaderstate.curtime * tcmod->args[3];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] + R_FastSin (src[0]*t2+t1) * t2;
				dst[1] = src[1] + R_FastSin (src[1]*t2+t1) * t2;
			}
			break;

		case SHADER_TCMOD_STRETCH:
			table = FTableForFunc(tcmod->args[0]);
			t2 = tcmod->args[3] + shaderstate.curtime * tcmod->args[4];
			t1 = FTABLE_EVALUATE(table, t2) * tcmod->args[2] + tcmod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1 + t2;
				dst[1] = src[1] * t1 + t2;
			}
			break;

		case SHADER_TCMOD_SCROLL:
			t1 = tcmod->args[0] * shaderstate.curtime;
			t2 = tcmod->args[1] * shaderstate.curtime;

			for (j = 0; j < cnt; j++, dst += 2, src+=2)
			{
				dst[0] = src[0] + t1;
				dst[1] = src[1] + t2;
			}
			break;

		case SHADER_TCMOD_TRANSFORM:
			for (j = 0; j < cnt; j++, dst+=2, src+=2)
			{
				t1 = src[0];
				t2 = src[1];
				dst[0] = t1 * tcmod->args[0] + t2 * tcmod->args[2] + tcmod->args[4];
				dst[1] = t1 * tcmod->args[1] + t1 * tcmod->args[3] + tcmod->args[5];
			}
			break;

		case SHADER_TCMOD_PAGE:
		default:
			for (j = 0; j < cnt; j++, dst += 2, src+=2)
			{
				dst[0] = src[0];
				dst[1] = src[1];
			}
			break;
	}
}

static void BE_GenerateTCMods(const shaderpass_t *pass, float *dest)
{
	mesh_t *mesh;
	unsigned int mno;
	int i;
	float *src;
	for (mno = 0; mno < shaderstate.nummeshes; mno++)
	{
		mesh = shaderstate.meshlist[mno];
		src = tcgen(pass, mesh->numvertexes, dest, mesh);
		//tcgen might return unmodified info
		if (pass->numtcmods)
		{
			tcmod(&pass->tcmods[0], mesh->numvertexes, src, dest, mesh);
			for (i = 1; i < pass->numtcmods; i++)
			{
				tcmod(&pass->tcmods[i], mesh->numvertexes, dest, dest, mesh);
			}
		}
		else if (src != dest)
		{
			memcpy(dest, src, sizeof(vec2_t)*mesh->numvertexes);
		}
		dest += mesh->numvertexes*2;
	}
}

//end texture coords
/*******************************************************************************************************************/
static void deformgen(const deformv_t *deformv, int cnt, vecV_t *src, vecV_t *dst, const mesh_t *mesh)
{
	float *table;
	int j, k;
	float args[4];
	float deflect;
	switch (deformv->type)
	{
	default:
	case DEFORMV_NONE:
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
		break;

	case DEFORMV_WAVE:
		if (!mesh->normals_array)
		{
			if (src != dst)
				memcpy(dst, src, sizeof(*src)*cnt);
			return;
		}
		args[0] = deformv->func.args[0];
		args[1] = deformv->func.args[1];
		args[3] = deformv->func.args[2] + deformv->func.args[3] * shaderstate.curtime;
		table = FTableForFunc(deformv->func.type);

		for ( j = 0; j < cnt; j++ )
		{
			deflect = deformv->args[0] * (src[j][0]+src[j][1]+src[j][2]) + args[3];
			deflect = FTABLE_EVALUATE(table, deflect) * args[1] + args[0];

			// Deflect vertex along its normal by wave amount
			VectorMA(src[j], deflect, mesh->normals_array[j], dst[j]);
		}
		break;

	case DEFORMV_NORMAL:
		//normal does not actually move the verts, but it does change the normals array
		//we don't currently support that.
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
/*
		args[0] = deformv->args[1] * shaderstate.curtime;

		for ( j = 0; j < cnt; j++ )
		{
			args[1] = normalsArray[j][2] * args[0];

			deflect = deformv->args[0] * R_FastSin(args[1]);
			normalsArray[j][0] *= deflect;
			deflect = deformv->args[0] * R_FastSin(args[1] + 0.25);
			normalsArray[j][1] *= deflect;
			VectorNormalizeFast(normalsArray[j]);
		}
*/		break;

	case DEFORMV_MOVE:
		table = FTableForFunc(deformv->func.type);
		deflect = deformv->func.args[2] + shaderstate.curtime * deformv->func.args[3];
		deflect = FTABLE_EVALUATE(table, deflect) * deformv->func.args[1] + deformv->func.args[0];

		for ( j = 0; j < cnt; j++ )
			VectorMA(src[j], deflect, deformv->args, dst[j]);
		break;

	case DEFORMV_BULGE:
		args[0] = deformv->args[0]/(2*M_PI);
		args[1] = deformv->args[1];
		args[2] = shaderstate.curtime * deformv->args[2]/(2*M_PI);

		for (j = 0; j < cnt; j++)
		{
			deflect = R_FastSin(mesh->st_array[j][0]*args[0] + args[2])*args[1];
			dst[j][0] = src[j][0]+deflect*mesh->normals_array[j][0];
			dst[j][1] = src[j][1]+deflect*mesh->normals_array[j][1];
			dst[j][2] = src[j][2]+deflect*mesh->normals_array[j][2];
		}
		break;

	case DEFORMV_AUTOSPRITE:
		if (mesh->numindexes < 6)
			break;

		for (j = 0; j < cnt-3; j+=4, src+=4, dst+=4)
		{
			vec3_t mid, d;
			float radius;
			mid[0] = 0.25*(src[0][0] + src[1][0] + src[2][0] + src[3][0]);
			mid[1] = 0.25*(src[0][1] + src[1][1] + src[2][1] + src[3][1]);
			mid[2] = 0.25*(src[0][2] + src[1][2] + src[2][2] + src[3][2]);
			VectorSubtract(src[0], mid, d);
			radius = 2*VectorLength(d);

			for (k = 0; k < 4; k++)
			{
				dst[k][0] = mid[0] + radius*((mesh->st_array[j+k][0]-0.5)*r_refdef.m_view[0+0]-(mesh->st_array[j+k][1]-0.5)*r_refdef.m_view[0+1]);
				dst[k][1] = mid[1] + radius*((mesh->st_array[j+k][0]-0.5)*r_refdef.m_view[4+0]-(mesh->st_array[j+k][1]-0.5)*r_refdef.m_view[4+1]);
				dst[k][2] = mid[2] + radius*((mesh->st_array[j+k][0]-0.5)*r_refdef.m_view[8+0]-(mesh->st_array[j+k][1]-0.5)*r_refdef.m_view[8+1]);
			}
		}
		break;

	case DEFORMV_AUTOSPRITE2:
		if (mesh->numindexes < 6)
			break;

		for (k = 0; k < mesh->numindexes; k += 6)
		{
			int long_axis, short_axis;
			vec3_t axis;
			float len[3];
			mat3_t m0, m1, m2, result;
			float *quad[4];
			vec3_t rot_centre, tv, tv2;

			quad[0] = (float *)(src + mesh->indexes[k+0]);
			quad[1] = (float *)(src + mesh->indexes[k+1]);
			quad[2] = (float *)(src + mesh->indexes[k+2]);

			for (j = 2; j >= 0; j--)
			{
				quad[3] = (float *)(src + mesh->indexes[k+3+j]);
				if (!VectorEquals (quad[3], quad[0]) &&
					!VectorEquals (quad[3], quad[1]) &&
					!VectorEquals (quad[3], quad[2]))
				{
					break;
				}
			}

			// build a matrix were the longest axis of the billboard is the Y-Axis
			VectorSubtract(quad[1], quad[0], m0[0]);
			VectorSubtract(quad[2], quad[0], m0[1]);
			VectorSubtract(quad[2], quad[1], m0[2]);
			len[0] = DotProduct(m0[0], m0[0]);
			len[1] = DotProduct(m0[1], m0[1]);
			len[2] = DotProduct(m0[2], m0[2]);

			if ((len[2] > len[1]) && (len[2] > len[0]))
			{
				if (len[1] > len[0])
				{
					long_axis = 1;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 1;
				}
			}
			else if ((len[1] > len[2]) && (len[1] > len[0]))
			{
				if (len[2] > len[0])
				{
					long_axis = 2;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 2;
				}
			}
			else //if ( (len[0] > len[1]) && (len[0] > len[2]) )
			{
				if (len[2] > len[1])
				{
					long_axis = 2;
					short_axis = 1;
				}
				else
				{
					long_axis = 1;
					short_axis = 2;
				}
			}

			if (DotProduct(m0[long_axis], m0[short_axis]))
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorCopy(axis, m0[1]);

				if (axis[0] || axis[1])
				{
					VectorVectors(m0[1], m0[2], m0[0]);
				}
				else
				{
					VectorVectors(m0[1], m0[0], m0[2]);
				}
			}
			else
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorNormalize2(m0[short_axis], m0[0]);
				VectorCopy(axis, m0[1]);
				CrossProduct(m0[0], m0[1], m0[2]);
			}

			for (j = 0; j < 3; j++)
				rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

			if (shaderstate.curentity)
			{
				VectorAdd(shaderstate.curentity->origin, rot_centre, tv);
			}
			else
			{
				VectorCopy(rot_centre, tv);
			}
			VectorSubtract(r_origin, tv, tv);

			// filter any longest-axis-parts off the camera-direction
			deflect = -DotProduct(tv, axis);

			VectorMA(tv, deflect, axis, m1[2]);
			VectorNormalizeFast(m1[2]);
			VectorCopy(axis, m1[1]);
			CrossProduct(m1[1], m1[2], m1[0]);

			Matrix3_Transpose(m1, m2);
			Matrix3_Multiply(m2, m0, result);

			for (j = 0; j < 4; j++)
			{
				int v = ((vecV_t*)quad[j]-src);
				VectorSubtract(quad[j], rot_centre, tv);
				Matrix3_Multiply_Vec3((void *)result, tv, tv2);
				VectorAdd(rot_centre, tv2, dst[v]);
			}
		}
		break;

//	case DEFORMV_PROJECTION_SHADOW:
//		break;
	}
}

static void BE_CreatePipeline(program_t *p, unsigned int shaderflags, unsigned int blendflags, unsigned int permu)
{
	struct pipeline_s *pipe;
	VkDynamicState dynamicStateEnables[2]={0};
	VkPipelineDynamicStateCreateInfo dyn = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	VkVertexInputBindingDescription vbinds[VK_BUFF_MAX] = {{0}};
	VkVertexInputAttributeDescription vattrs[VK_BUFF_MAX] = {{0}};
	VkPipelineVertexInputStateCreateInfo vi = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	VkPipelineInputAssemblyStateCreateInfo ia = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	VkPipelineViewportStateCreateInfo vp = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	VkPipelineRasterizationStateCreateInfo rs = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	VkPipelineMultisampleStateCreateInfo ms = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	VkPipelineDepthStencilStateCreateInfo ds = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	VkPipelineColorBlendStateCreateInfo cb = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	VkPipelineColorBlendAttachmentState att_state[1];
	VkGraphicsPipelineCreateInfo pipeCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	VkPipelineShaderStageCreateInfo shaderStages[2] = {{0}};
	VkPipelineRasterizationStateRasterizationOrderAMD ro = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD};	//long enough names for you?
#ifdef VK_KHR_fragment_shading_rate
	VkPipelineFragmentShadingRateStateCreateInfoKHR shadingrate = {VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR};
#endif
	struct specdata_s
	{
		int alphamode;
		int permu[16];
		union
		{
			float f;
			int i;
		} cvars[64];
	} specdata;
	VkSpecializationMapEntry specentries[256] = {{0}};
	VkSpecializationInfo specInfo = {0}, *bugsbeware;
	VkResult err;
	uint32_t i, s;
	unsigned char *cvardata;

	if (!p->vert || !p->frag)
		Sys_Error("program missing required shader\n");	//PANIC


	pipe = Z_Malloc(sizeof(*pipe));
	if (!p->pipelines)
		p->pipelines = pipe;
	else
	{	//insert at end. if it took us a while to realise that we needed it, chances are its not that common.
		//so don't cause the other pipelines to waste cycles for it.
		struct pipeline_s *prev;
		for (prev = p->pipelines; ; prev = prev->next)
			if (!prev->next)
				break;
		prev->next = pipe;
	}

	pipe->flags = shaderflags;
	pipe->blendbits = blendflags;
	pipe->permu = permu;

	if (permu&PERMUTATION_BEM_WIREFRAME)
	{
		blendflags |= SBITS_MISC_NODEPTHTEST;
		blendflags &= ~SBITS_MISC_DEPTHWRITE;

		blendflags &= ~(SHADER_CULL_FRONT|SHADER_CULL_BACK);
	}

	dyn.flags = 0;
	dyn.dynamicStateCount = 0;
	dyn.pDynamicStates = dynamicStateEnables;

	//it wasn't supposed to be like this!
	//this stuff gets messy with tcmods and rgbgen/alphagen stuff
	vbinds[VK_BUFF_POS].binding = VK_BUFF_POS;
	vbinds[VK_BUFF_POS].stride = sizeof(vecV_t);
	vbinds[VK_BUFF_POS].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_POS].binding = vbinds[VK_BUFF_POS].binding;
	vattrs[VK_BUFF_POS].location = VK_BUFF_POS;
	vattrs[VK_BUFF_POS].format = VK_FORMAT_R32G32B32_SFLOAT;
	vattrs[VK_BUFF_POS].offset = 0;
	vbinds[VK_BUFF_TC].binding = VK_BUFF_TC;
	vbinds[VK_BUFF_TC].stride = sizeof(vec2_t);
	vbinds[VK_BUFF_TC].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_TC].binding = vbinds[VK_BUFF_TC].binding;
	vattrs[VK_BUFF_TC].location = VK_BUFF_TC;
	vattrs[VK_BUFF_TC].format = VK_FORMAT_R32G32_SFLOAT;
	vattrs[VK_BUFF_TC].offset = 0;
	vbinds[VK_BUFF_COL].binding = VK_BUFF_COL;
	vbinds[VK_BUFF_COL].stride = sizeof(vec4_t);
	vbinds[VK_BUFF_COL].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_COL].binding = vbinds[VK_BUFF_COL].binding;
	vattrs[VK_BUFF_COL].location = VK_BUFF_COL;
	vattrs[VK_BUFF_COL].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vattrs[VK_BUFF_COL].offset = 0;
	vbinds[VK_BUFF_LMTC].binding = VK_BUFF_LMTC;
	vbinds[VK_BUFF_LMTC].stride = sizeof(vec2_t);
	vbinds[VK_BUFF_LMTC].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_LMTC].binding = vbinds[VK_BUFF_LMTC].binding;
	vattrs[VK_BUFF_LMTC].location = VK_BUFF_LMTC;
	vattrs[VK_BUFF_LMTC].format = VK_FORMAT_R32G32_SFLOAT;
	vattrs[VK_BUFF_LMTC].offset = 0;

	//fixme: in all seriousness, why is this not a single buffer?
	vbinds[VK_BUFF_NORM].binding = VK_BUFF_NORM;
	vbinds[VK_BUFF_NORM].stride = sizeof(vec3_t);
	vbinds[VK_BUFF_NORM].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_NORM].binding = vbinds[VK_BUFF_NORM].binding;
	vattrs[VK_BUFF_NORM].location = VK_BUFF_NORM;
	vattrs[VK_BUFF_NORM].format = VK_FORMAT_R32G32B32_SFLOAT;
	vattrs[VK_BUFF_NORM].offset = 0;
	vbinds[VK_BUFF_SDIR].binding = VK_BUFF_SDIR;
	vbinds[VK_BUFF_SDIR].stride = sizeof(vec3_t);
	vbinds[VK_BUFF_SDIR].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_SDIR].binding = vbinds[VK_BUFF_SDIR].binding;
	vattrs[VK_BUFF_SDIR].location = VK_BUFF_SDIR;
	vattrs[VK_BUFF_SDIR].format = VK_FORMAT_R32G32B32_SFLOAT;
	vattrs[VK_BUFF_SDIR].offset = 0;
	vbinds[VK_BUFF_TDIR].binding = VK_BUFF_TDIR;
	vbinds[VK_BUFF_TDIR].stride = sizeof(vec3_t);
	vbinds[VK_BUFF_TDIR].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vattrs[VK_BUFF_TDIR].binding = vbinds[VK_BUFF_TDIR].binding;
	vattrs[VK_BUFF_TDIR].location = VK_BUFF_TDIR;
	vattrs[VK_BUFF_TDIR].format = VK_FORMAT_R32G32B32_SFLOAT;
	vattrs[VK_BUFF_TDIR].offset = 0;

	vi.vertexBindingDescriptionCount = countof(vbinds);
	vi.pVertexBindingDescriptions = vbinds;
	vi.vertexAttributeDescriptionCount = countof(vattrs);
	vi.pVertexAttributeDescriptions = vattrs;

	ia.topology = (blendflags&SBITS_LINES)?VK_PRIMITIVE_TOPOLOGY_LINE_LIST:VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vp.viewportCount = 1;
	dynamicStateEnables[dyn.dynamicStateCount++] =	VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = 1;
	dynamicStateEnables[dyn.dynamicStateCount++] =	VK_DYNAMIC_STATE_SCISSOR;
	//FIXME: fillModeNonSolid might mean mode_line is not supported.
	rs.polygonMode = (permu&PERMUTATION_BEM_WIREFRAME)?VK_POLYGON_MODE_LINE:VK_POLYGON_MODE_FILL;
	rs.lineWidth = 1;
	rs.cullMode = ((shaderflags&SHADER_CULL_FRONT)?VK_CULL_MODE_FRONT_BIT:0) | ((shaderflags&SHADER_CULL_BACK)?VK_CULL_MODE_BACK_BIT:0);
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	if (shaderflags & SHADER_POLYGONOFFSET)
	{
		rs.depthBiasEnable = VK_TRUE;
		rs.depthBiasConstantFactor = -25;//shader->polyoffset.unit;
		rs.depthBiasClamp = 0;
		rs.depthBiasSlopeFactor = -0.05;//shader->polyoffset.factor;
	}
	else
		rs.depthBiasEnable = VK_FALSE;

	if (vk.amd_rasterization_order)
	{
		unsigned int b = blendflags & SBITS_BLEND_BITS;
		//we potentially allow a little z-fighting if they're equal. a single batch shouldn't really have such primitives.
		//must be no blending, or additive blending.
		switch(blendflags & SBITS_DEPTHFUNC_BITS)
		{
		case SBITS_DEPTHFUNC_EQUAL:
			break;
		default:
			if ((blendflags&(SBITS_MISC_NODEPTHTEST|SBITS_MISC_DEPTHWRITE)) == SBITS_MISC_DEPTHWRITE &&
				(!b || b == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO) || b == SBITS_DSTBLEND_ONE))
			{
				rs.pNext = &ro;
				ro.rasterizationOrder = VK_RASTERIZATION_ORDER_RELAXED_AMD;
			}
		}
	}

	ms.pSampleMask = NULL;
	if (permu & PERMUTATION_BEM_MULTISAMPLE)
		ms.rasterizationSamples = vk.multisamplebits;
	else
		ms.rasterizationSamples = 1;
//	ms.sampleShadingEnable = VK_TRUE;	//call the fragment shader multiple times, instead of just once per final pixel
//	ms.minSampleShading = 0.25;
	ds.depthTestEnable = (blendflags&SBITS_MISC_NODEPTHTEST)?VK_FALSE:VK_TRUE;
	ds.depthWriteEnable = (blendflags&SBITS_MISC_DEPTHWRITE)?VK_TRUE:VK_FALSE;
	switch(blendflags & SBITS_DEPTHFUNC_BITS)
	{
	default:
	case SBITS_DEPTHFUNC_CLOSEREQUAL:	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;	break;
	case SBITS_DEPTHFUNC_EQUAL:			ds.depthCompareOp = VK_COMPARE_OP_EQUAL;			break;
	case SBITS_DEPTHFUNC_CLOSER:		ds.depthCompareOp = VK_COMPARE_OP_LESS;				break;
	case SBITS_DEPTHFUNC_FURTHER:		ds.depthCompareOp = VK_COMPARE_OP_GREATER;			break;
	}
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_NEVER;//VK_COMPARE_OP_ALWAYS;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;
	memset(att_state, 0, sizeof(att_state));
	att_state[0].colorWriteMask =
		((blendflags&SBITS_MASK_RED)?0:VK_COLOR_COMPONENT_R_BIT) |
		((blendflags&SBITS_MASK_GREEN)?0:VK_COLOR_COMPONENT_G_BIT) |
		((blendflags&SBITS_MASK_BLUE)?0:VK_COLOR_COMPONENT_B_BIT) |
		((blendflags&SBITS_MASK_ALPHA)?0:VK_COLOR_COMPONENT_A_BIT);

	if ((blendflags & SBITS_BLEND_BITS) && (blendflags & SBITS_BLEND_BITS)!=(SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO))
	{
		switch(blendflags & SBITS_SRCBLEND_BITS)
		{
		case SBITS_SRCBLEND_ZERO:					att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;				att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;				break;
		case SBITS_SRCBLEND_ONE:					att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;					att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;					break;
		case SBITS_SRCBLEND_DST_COLOR:				att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;			att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;			break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_COLOR:	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;	break;
		case SBITS_SRCBLEND_SRC_ALPHA:				att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			break;
		case SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	break;
		case SBITS_SRCBLEND_DST_ALPHA:				att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;			att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;			break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA:	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;	break;
		case SBITS_SRCBLEND_ALPHA_SATURATE:			att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;	break;
		default:	Sys_Error("Bad shader blend src\n"); return;
		}
		switch(blendflags & SBITS_DSTBLEND_BITS)
		{
		case SBITS_DSTBLEND_ZERO:					att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;				att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;				break;
		case SBITS_DSTBLEND_ONE:					att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;					att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;					break;
		case SBITS_DSTBLEND_SRC_ALPHA:				att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	break;
		case SBITS_DSTBLEND_DST_ALPHA:				att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;			att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;			break;
		case SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA:	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;	break;
		case SBITS_DSTBLEND_SRC_COLOR:				att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;			att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;			break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR:	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	break;
		default:	Sys_Error("Bad shader blend dst\n"); return;
		}
		att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
		att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
		att_state[0].blendEnable = VK_TRUE;
	}
	else
	{
		att_state[0].blendEnable = VK_FALSE;
	}
	if (permu&PERMUTATION_BEM_DEPTHONLY)
		cb.attachmentCount = 0;
	else
		cb.attachmentCount = 1;
	cb.pAttachments = att_state;


	s = 0;
	specentries[s].constantID = 0;
	specentries[s].offset = offsetof(struct specdata_s, alphamode);
	specentries[s].size = sizeof(specdata.alphamode);
	s++;
	if (blendflags & SBITS_ATEST_GE128)
		specdata.alphamode = 3;
	else if (blendflags & SBITS_ATEST_GT0)
		specdata.alphamode = 2;
	else if (blendflags & SBITS_ATEST_LT128)
		specdata.alphamode = 1;
	else //if (blendflags & SBITS_ATEST_NONE)
		specdata.alphamode = 0;

	for (i = 0; i < countof(specdata.permu); i++)
	{
		specentries[s].constantID = 16+i;
		specentries[s].offset = offsetof(struct specdata_s, permu[i]);
		specentries[s].size = sizeof(specdata.permu[i]);
		s++;
		specdata.permu[i] = !!(permu & (1u<<i));
	}

	//cvars
	for (cvardata = p->cvardata, i = 0; cvardata < p->cvardata + p->cvardatasize; )
	{
		unsigned short id = (cvardata[0]<<8)|cvardata[1];
		unsigned char type = cvardata[2], size = cvardata[3]-'0';
		char *name;
		cvar_t *var;
		unsigned int u;

		cvardata += 4;
		name = cvardata;
		cvardata += strlen(name)+1;

		if (i + size > countof(specdata.cvars))
			break;	//error

		if (type >= 'A' && type <= 'Z')
		{	//args will be handled by the blob loader.
			for (u = 0; u < size && u < 4; u++)
			{
				specentries[s].constantID = id;
				specentries[s].offset = offsetof(struct specdata_s, cvars[i]);
				specentries[s].size = sizeof(specdata.cvars[i]);

				specdata.cvars[i].i = (cvardata[u*4+0]<<24)|(cvardata[u*4+1]<<16)|(cvardata[u*4+2]<<8)|(cvardata[u*4+3]<<0);
				s++;
				i++;
				id++;
			}
		}
		else
		{
			var = Cvar_FindVar(name);
			if (var)
			{
				for (u = 0; u < size && u < 4; u++)
				{
					specentries[s].constantID = id;
					specentries[s].offset = offsetof(struct specdata_s, cvars[i]);
					specentries[s].size = sizeof(specdata.cvars[i]);

					if (type == 'i')
						specdata.cvars[i].i = var->ival;
					else
						specdata.cvars[i].f = var->vec4[u];
					s++;
					i++;
					id++;
				}
			}
		}
		cvardata += 4*size;
	}

	specInfo.mapEntryCount = s;
	specInfo.pMapEntries = specentries;
	specInfo.dataSize = sizeof(specdata);
	specInfo.pData = &specdata;

#if 0//def _DEBUG
	//vk_layer_lunarg_drawstate fucks up and pokes invalid bits of stack.
	bugsbeware = Z_Malloc(sizeof(*bugsbeware) + sizeof(*specentries)*s + sizeof(specdata));
	*bugsbeware = specInfo;
	bugsbeware->pData = bugsbeware+1;
	bugsbeware->pMapEntries = (VkSpecializationMapEntry*)((char*)bugsbeware->pData + specInfo.dataSize);
	memcpy((void*)bugsbeware->pData, specInfo.pData, specInfo.dataSize);
	memcpy((void*)bugsbeware->pMapEntries, specInfo.pMapEntries, sizeof(*specInfo.pMapEntries)*specInfo.mapEntryCount);
#else
	bugsbeware = &specInfo;
#endif
	//fixme: add more specialisations for custom cvars (yes, this'll flush+reload pipelines if they're changed)
	//fixme: add specialisations for permutations I guess
	//fixme: add geometry+tesselation support. because we can.

	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = p->vert;
	shaderStages[0].pName = "main";
	shaderStages[0].pSpecializationInfo = bugsbeware;
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = p->frag;
	shaderStages[1].pName = "main";
	shaderStages[1].pSpecializationInfo = bugsbeware;

	pipeCreateInfo.flags				= 0;
	pipeCreateInfo.stageCount			= countof(shaderStages);
	pipeCreateInfo.pStages				= shaderStages;
	pipeCreateInfo.pVertexInputState	= &vi;
	pipeCreateInfo.pInputAssemblyState	= &ia;
	pipeCreateInfo.pTessellationState	= NULL;	//null is okay!
	pipeCreateInfo.pViewportState		= &vp;
	pipeCreateInfo.pRasterizationState	= &rs;
	pipeCreateInfo.pMultisampleState	= &ms;
	pipeCreateInfo.pDepthStencilState	= &ds;
	pipeCreateInfo.pColorBlendState		= &cb;
	pipeCreateInfo.pDynamicState		= &dyn;
	pipeCreateInfo.layout				= p->layout;
	i = (permu&PERMUTATION_BEM_DEPTHONLY)?RP_DEPTHONLY:RP_FULLCLEAR;
	if (permu&PERMUTATION_BEM_MULTISAMPLE)
		i |= RP_MULTISAMPLE;
	if (permu&PERMUTATION_BEM_FP16)
		i |= RP_FP16;
	if (permu&PERMUTATION_BEM_VR)
		i |= RP_VR;
	pipeCreateInfo.renderPass			= VK_GetRenderPass(i);
	pipeCreateInfo.subpass				= 0;
	pipeCreateInfo.basePipelineHandle	= VK_NULL_HANDLE;
	pipeCreateInfo.basePipelineIndex	= -1;	//used to create derivatives for pipelines created in the same call.

//	pipeCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

#ifdef VK_KHR_fragment_shading_rate
	if (vk.khr_fragment_shading_rate)
	{
		//three ways to specify rates... we need to set which one wins here. we only do pipeline rates.
		shadingrate.combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;//pipeline vs primitive
		shadingrate.combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;//previous vs attachment
		if (blendflags & SBITS_MISC_FULLRATE)
		{
			shadingrate.fragmentSize.width = 1;
			shadingrate.fragmentSize.height = 1;
		}
		else
		{	//actually this is more quater-rate. oh well.
			shadingrate.fragmentSize.width = 2;
			shadingrate.fragmentSize.height = 2;
		}

		shadingrate.pNext = pipeCreateInfo.pNext;
		pipeCreateInfo.pNext = &shadingrate;
	}
#endif

	err = vkCreateGraphicsPipelines(vk.device, vk.pipelinecache, 1, &pipeCreateInfo, vkallocationcb, &pipe->pipeline);
	DebugSetName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipe->pipeline, p->name);

	if (err)
	{	//valid err values are VK_ERROR_OUT_OF_HOST_MEMORY, VK_ERROR_OUT_OF_DEVICE_MEMORY, VK_ERROR_INVALID_SHADER_NV
		//VK_INCOMPLETE is a Qualcom bug with certain spirv-opt optimisations.
		shaderstate.rc.activepipeline = VK_NULL_HANDLE;
		Sys_Error("%s creating pipeline %s for material %s. Check spir-v modules / drivers.\n", VK_VKErrorToString(err), p->name, shaderstate.curshader->name);
		return;
	}

	vkCmdBindPipeline(vk.rendertarg->cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderstate.rc.activepipeline=pipe->pipeline);
}
static void BE_BindPipeline(program_t *p, unsigned int shaderflags, unsigned int blendflags, unsigned int permu)
{
	struct pipeline_s *pipe;
	blendflags &=	0
					| SBITS_SRCBLEND_BITS | SBITS_DSTBLEND_BITS | SBITS_MASK_BITS | SBITS_ATEST_BITS
					| SBITS_MISC_DEPTHWRITE | SBITS_MISC_NODEPTHTEST | SBITS_DEPTHFUNC_BITS
					| SBITS_LINES | SBITS_MISC_FULLRATE
					;
	shaderflags &= 0
					| SHADER_CULL_FRONT | SHADER_CULL_BACK
					| SHADER_POLYGONOFFSET
					;
	permu |= shaderstate.modepermutation;

	if (shaderflags & (SHADER_CULL_FRONT | SHADER_CULL_BACK))
		shaderflags ^= r_refdef.flipcull;

	for (pipe = p->pipelines; pipe; pipe = pipe->next)
	{
		if (pipe->flags == shaderflags)
			if (pipe->blendbits == blendflags)
				if (pipe->permu == permu)
				{
					if (pipe->pipeline != shaderstate.rc.activepipeline)
					{
						shaderstate.rc.activepipeline = pipe->pipeline;
						if (shaderstate.rc.activepipeline)
							vkCmdBindPipeline(vk.rendertarg->cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderstate.rc.activepipeline);
					}
					return;
				}
	}

	//oh look. we need to build an entirely new pipeline object. hurrah... not.
	//split into a different function because of abusive stack combined with windows stack probes.
	BE_CreatePipeline(p, shaderflags, blendflags, permu);
}

static void BE_SetupTextureDescriptor(texid_t tex, texid_t fallbacktex, VkDescriptorSet set, VkWriteDescriptorSet *firstdesc, VkWriteDescriptorSet *desc, VkDescriptorImageInfo *img)
{
	if (!tex || !tex->vkimage)
		tex = fallbacktex;

	desc->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc->pNext = NULL;
	desc->dstSet = set;
	desc->dstBinding = desc-firstdesc;
	desc->dstArrayElement = 0;
	desc->descriptorCount = 1;
	desc->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	img->imageLayout = tex->vkimage->layout;
	img->imageView = tex->vkimage->view;
	img->sampler = tex->vkimage->sampler;
	desc->pImageInfo = img;
	desc->pBufferInfo = NULL;
	desc->pTexelBufferView = NULL;
}
static void BE_SetupUBODescriptor(VkDescriptorSet set, VkWriteDescriptorSet *firstdesc, VkWriteDescriptorSet *desc, VkDescriptorBufferInfo *info)
{
	desc->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc->pNext = NULL;
	desc->dstSet = set;
	desc->dstBinding = desc-firstdesc;
	desc->dstArrayElement = 0;
	desc->descriptorCount = 1;
	desc->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desc->pImageInfo = NULL;
	desc->pBufferInfo = info;
	desc->pTexelBufferView = NULL;
}
#ifdef VK_KHR_acceleration_structure
static void BE_SetupAccelerationDescriptor(VkDescriptorSet set, VkWriteDescriptorSet *firstdesc, VkWriteDescriptorSet *desc, VkWriteDescriptorSetAccelerationStructureKHR *descas)
{
	desc->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descas->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	desc->pNext = descas;
	descas->pNext = NULL;
	desc->dstSet = set;
	desc->dstBinding = desc-firstdesc;
	desc->dstArrayElement = 0;
	desc->descriptorCount = descas->accelerationStructureCount = 1;
	desc->descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	desc->pImageInfo = NULL;
	desc->pBufferInfo = NULL;
	desc->pTexelBufferView = NULL;
	descas->pAccelerationStructures = &shaderstate.tlas;
}
#endif

static qboolean BE_SetupMeshProgram(program_t *p, shaderpass_t *pass, unsigned int shaderbits, unsigned int idxcount)
{
	int perm = 0;
	if (!p)
		return false;

	if (TEXLOADED(shaderstate.curtexnums->bump))
		perm |= PERMUTATION_BUMPMAP;
	if (TEXLOADED(shaderstate.curtexnums->fullbright))
		perm |= PERMUTATION_FULLBRIGHT;
	if (TEXLOADED(shaderstate.curtexnums->upperoverlay) || TEXLOADED(shaderstate.curtexnums->loweroverlay))
		perm |= PERMUTATION_UPPERLOWER;
	if (TEXLOADED(shaderstate.curtexnums->reflectcube) || TEXLOADED(shaderstate.curtexnums->reflectmask))
		perm |= PERMUTATION_REFLECTCUBEMASK;
	if (r_refdef.globalfog.density)
		perm |= PERMUTATION_FOG;
//	if (r_glsl_offsetmapping.ival && TEXLOADED(shaderstate.curtexnums->bump))
//		perm |= PERMUTATION_OFFSET;
	perm &= p->supportedpermutations;

	BE_BindPipeline(p, shaderbits, VKBE_ApplyShaderBits(pass->shaderbits), perm);
	if (!shaderstate.rc.activepipeline)
		return false;	//err, something bad happened.

	//most gpus will have a fairly low descriptor set limit of 4 (this is the minimum required)
	//that isn't enough for all our textures, so we need to make stuff up as required.
	{
		VkDescriptorSet set = shaderstate.rc.descriptorsets[0] = vk.khr_push_descriptor?VK_NULL_HANDLE:VKBE_TempDescriptorSet(p->desclayout);
		VkWriteDescriptorSet descs[MAX_TMUS], *desc = descs;
#ifdef VK_KHR_acceleration_structure
		VkWriteDescriptorSetAccelerationStructureKHR descas;
#endif
		VkDescriptorImageInfo imgs[MAX_TMUS], *img = imgs;
		unsigned int i;
		texid_t t;
		//why do I keep wanting to write 'desk'? its quite annoying.

		//light / scene
		BE_SetupUBODescriptor(set, descs, desc++, &shaderstate.ubo_entity);
		BE_SetupUBODescriptor(set, descs, desc++, &shaderstate.ubo_light);
#ifdef VK_KHR_acceleration_structure
		if (p->rayquery)	//an alternative to shadowmaps...
		{
			shaderstate.needtlas = true;
			if (!shaderstate.tlas)
				return false;	//nope... maybe next frame
			BE_SetupAccelerationDescriptor(set, descs, desc++, &descas);
		}
#endif
		if (p->defaulttextures & (1u<<S_SHADOWMAP))
			BE_SetupTextureDescriptor(shaderstate.currentshadowmap, r_whiteimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_PROJECTIONMAP))
			BE_SetupTextureDescriptor(shaderstate.curdlight?shaderstate.curdlight->cubetexture:r_nulltex, r_whitecubeimage, set, descs, desc++, img++);

		//material
		if (p->defaulttextures & (1u<<S_DIFFUSE))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->base, r_blackimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_NORMALMAP))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->bump, missing_texture_normal, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_SPECULAR))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->specular, missing_texture_gloss, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_UPPERMAP))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->upperoverlay, r_blackimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_LOWERMAP))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->loweroverlay, r_blackimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_FULLBRIGHT))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->fullbright, r_blackimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_PALETTED))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->paletted, r_blackimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_REFLECTCUBE))
		{
			if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->reflectcube))
				t = shaderstate.curtexnums->reflectcube;
			else if (shaderstate.curbatch->envmap)
				t = shaderstate.curbatch->envmap;
			else
				t = r_blackcubeimage;	//FIXME
			BE_SetupTextureDescriptor(t, r_blackcubeimage, set, descs, desc++, img++);
		}
		if (p->defaulttextures & (1u<<S_REFLECTMASK))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->reflectmask, r_whiteimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_DISPLACEMENT))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->displacement, r_whiteimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_OCCLUSION))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->occlusion, r_whiteimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_TRANSMISSION))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->transmission, r_whiteimage, set, descs, desc++, img++);
		if (p->defaulttextures & (1u<<S_THICKNESS))
			BE_SetupTextureDescriptor(shaderstate.curtexnums->thickness, r_whiteimage, set, descs, desc++, img++);

		//batch
		if (p->defaulttextures & (1u<<S_LIGHTMAP0))
		{
			unsigned int lmi = shaderstate.curbatch->lightmap[0];
			BE_SetupTextureDescriptor((lmi<numlightmaps)?lightmap[lmi]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
		}
		if (p->defaulttextures & (1u<<S_DELUXEMAP0))
		{
			texid_t delux = NULL;
			unsigned int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi<numlightmaps && lightmap[lmi]->hasdeluxe)
				delux = lightmap[lmi+1]->lightmap_texture;
			BE_SetupTextureDescriptor(delux, r_whiteimage, set, descs, desc++, img++);
		}
#if MAXRLIGHTMAPS > 1
		if (p->defaulttextures & ((1u<<S_LIGHTMAP1)|(1u<<S_LIGHTMAP2)|(1u<<S_LIGHTMAP3)))
		{
			int lmi = shaderstate.curbatch->lightmap[1];
			BE_SetupTextureDescriptor((lmi<numlightmaps)?lightmap[lmi]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
			lmi = shaderstate.curbatch->lightmap[2];
			BE_SetupTextureDescriptor((lmi<numlightmaps)?lightmap[lmi]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
			lmi = shaderstate.curbatch->lightmap[3];
			BE_SetupTextureDescriptor((lmi<numlightmaps)?lightmap[lmi]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
		}
		if (p->defaulttextures & ((1u<<S_DELUXEMAP1)|(1u<<S_DELUXEMAP2)|(1u<<S_DELUXEMAP3)))
		{
			int lmi = shaderstate.curbatch->lightmap[1];
			if (lmi<numlightmaps && lightmap[lmi]->hasdeluxe)
			{
				BE_SetupTextureDescriptor((lmi+1<numlightmaps)?lightmap[lmi+1]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
				lmi = shaderstate.curbatch->lightmap[2];
				BE_SetupTextureDescriptor((lmi+1<numlightmaps)?lightmap[lmi+1]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
				lmi = shaderstate.curbatch->lightmap[3];
				BE_SetupTextureDescriptor((lmi+1<numlightmaps)?lightmap[lmi+1]->lightmap_texture:NULL, r_whiteimage, set, descs, desc++, img++);
			}
			else
			{
				BE_SetupTextureDescriptor(NULL, r_whiteimage, set, descs, desc++, img++);
				BE_SetupTextureDescriptor(NULL, r_whiteimage, set, descs, desc++, img++);
				BE_SetupTextureDescriptor(NULL, r_whiteimage, set, descs, desc++, img++);
			}
		}
#endif

		//shader / pass
		for (i = 0; i < p->numsamplers; i++)
			BE_SetupTextureDescriptor(SelectPassTexture(pass+i), r_blackimage, set, descs, desc++, img++);

		if (!set)
			vkCmdPushDescriptorSetKHR(vk.rendertarg->cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, p->layout, 0, desc-descs, descs);
		else
			vkUpdateDescriptorSets(vk.device, desc-descs, descs, 0, NULL);
	}
	if (!vk.khr_push_descriptor)
		vkCmdBindDescriptorSets(vk.rendertarg->cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, p->layout, 0, countof(shaderstate.rc.descriptorsets), shaderstate.rc.descriptorsets, 0, NULL);

	RQuantAdd(RQUANT_PRIMITIVEINDICIES, idxcount);
	RQuantAdd(RQUANT_DRAWS, 1);

	return true;
}

static void BE_DrawMeshChain_Internal(void)
{
	shader_t *altshader;
	unsigned int vertcount, idxcount, idxfirst;
	mesh_t *m;
	qboolean vblends;	//software
//	void *map;
//	int i;
	unsigned int mno;
	unsigned int passno;
	//extern cvar_t r_polygonoffset_submodel_factor;
//	float pushdepth;
//	float pushfactor;

	//I wasn't going to do this... but gah.
	VkBuffer vertexbuffers[VK_BUFF_MAX];
	VkDeviceSize vertexoffsets[VK_BUFF_MAX];

	altshader = shaderstate.curshader;
	switch (shaderstate.mode)
	{
	case BEM_LIGHT:
		altshader = shaderstate.shader_rtlight[shaderstate.curlmode];
		break;
	case BEM_DEPTHONLY:
		altshader = shaderstate.curshader->bemoverrides[bemoverride_depthonly];
		if (!altshader)
			altshader = shaderstate.depthonly;
		break;
	case BEM_WIREFRAME:
		altshader = R_RegisterShader("wireframe", SUF_NONE, 
			"{\n"
				"{\n"
					"map $whiteimage\n"
				"}\n"
			"}\n"
			);
		break;
	default:
	case BEM_STANDARD:
		altshader = shaderstate.curshader;
		break;
	}
	if (!altshader)
		return;

	if (shaderstate.forcebeflags & BEF_FORCENODEPTH)
	{
		RQuantAdd(RQUANT_2DBATCHES, 1);
	}
	else if (shaderstate.curentity == &r_worldentity)
	{
		RQuantAdd(RQUANT_WORLDBATCHES, 1);
	}
	else
	{
		RQuantAdd(RQUANT_ENTBATCHES, 1);
	}

	if (altshader->flags & SHADER_HASCURRENTRENDER)
		T_Gen_CurrentRender();	//requires lots of pass-related work...

	//if this flag is set, then we have to generate our own arrays. to avoid processing extra verticies this may require that we re-pack the verts
	if (shaderstate.meshlist[0]->xyz2_array)// && !altshader->prog)
	{
		vblends = true;
		shaderstate.batchvbo = NULL;
	}
	else
	{
		vblends = false;
		if (altshader->flags & SHADER_NEEDSARRAYS)
			shaderstate.batchvbo = NULL;
		else if (shaderstate.curshader->numdeforms)
			shaderstate.batchvbo = NULL;
	}

	/*index buffers are common to all passes*/
	if (shaderstate.batchvbo)
	{
		/*however, we still want to try to avoid discontinuities, because that would otherwise be more draw calls. we can have gaps in verts though*/
		if (shaderstate.nummeshes == 1)
		{
			m = shaderstate.meshlist[0];

			vkCmdBindIndexBuffer(vk.rendertarg->cbuf, shaderstate.batchvbo->indicies.vk.buff, shaderstate.batchvbo->indicies.vk.offs, VK_INDEX_TYPE);
			idxfirst = m->vbofirstelement;

			vertcount = m->vbofirstvert + m->numvertexes;
			idxcount = m->numindexes;
		}
		else if (0)//shaderstate.nummeshes == shaderstate.curbatch->maxmeshes)
		{
			idxfirst = 0;
			vertcount = shaderstate.batchvbo->vertcount;
			idxcount = shaderstate.batchvbo->indexcount;

			vkCmdBindIndexBuffer(vk.rendertarg->cbuf, shaderstate.batchvbo->indicies.vk.buff, shaderstate.batchvbo->indicies.vk.offs, VK_INDEX_TYPE);
		}
		else
		{
			index_t *fte_restrict map;
			VkBuffer buf;
			unsigned int i;
			VkDeviceSize offset;
			vertcount = shaderstate.batchvbo->vertcount;
			for (mno = 0, idxcount = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				idxcount += m->numindexes;
			}
			map = VKBE_AllocateStreamingSpace(DB_EBO, idxcount * sizeof(*map), &buf, &offset);
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				for (i = 0; i < m->numindexes; i++)
					map[i] = m->indexes[i]+m->vbofirstvert;
				map += m->numindexes;
			}
			vkCmdBindIndexBuffer(vk.rendertarg->cbuf, buf, offset, VK_INDEX_TYPE);
			idxfirst = 0;
		}
	}
	else
	{	/*we're going to be using dynamic array stuff here, so generate an index array list that has no vertex gaps*/
		index_t *fte_restrict map;
		VkBuffer buf;
		unsigned int i;
		VkDeviceSize offset;
		for (mno = 0, vertcount = 0, idxcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			vertcount += m->numvertexes;
			idxcount += m->numindexes;
		}

		map = VKBE_AllocateStreamingSpace(DB_EBO, idxcount * sizeof(*map), &buf, &offset);
		for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			if (!vertcount)
				memcpy(map, m->indexes, sizeof(index_t)*m->numindexes);
			else
			{
				for (i = 0; i < m->numindexes; i++)
					map[i] = m->indexes[i]+vertcount;
			}
			map += m->numindexes;
			vertcount += m->numvertexes;
		}
		vkCmdBindIndexBuffer(vk.rendertarg->cbuf, buf, offset, VK_INDEX_TYPE);
		idxfirst = 0;
	}

	/*vertex buffers are common to all passes*/
	if (shaderstate.batchvbo && !vblends)
	{
		vertexbuffers[VK_BUFF_POS] = shaderstate.batchvbo->coord.vk.buff;
		vertexoffsets[VK_BUFF_POS] = shaderstate.batchvbo->coord.vk.offs;
	}
	else
	{
		vecV_t *fte_restrict map;
		const mesh_t *m;
		unsigned int mno;
		unsigned int i;

		map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vecV_t), &vertexbuffers[VK_BUFF_POS], &vertexoffsets[VK_BUFF_POS]);
		
		if (vblends)
		{
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				const mesh_t *m = shaderstate.meshlist[mno];
				vecV_t *ov = shaderstate.curshader->numdeforms?tmpbuf:map;
				vecV_t *iv1 = m->xyz_array;
				vecV_t *iv2 = m->xyz2_array;
				float w1 = m->xyz_blendw[0];
				float w2 = m->xyz_blendw[1];
				for (i = 0; i < m->numvertexes; i++)
				{
					ov[i][0] = iv1[i][0]*w1 + iv2[i][0]*w2;
					ov[i][1] = iv1[i][1]*w1 + iv2[i][1]*w2;
					ov[i][2] = iv1[i][2]*w1 + iv2[i][2]*w2;
				}
				if (shaderstate.curshader->numdeforms)
				{
					for (i = 0; i < shaderstate.curshader->numdeforms-1; i++)
						deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, tmpbuf, m);
					deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, map, m);
				}
				map += m->numvertexes;
			}
		}
		else if (shaderstate.curshader->numdeforms > 1)
		{	//horrible code, because multiple deforms would otherwise READ from the gpu memory
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				deformgen(&shaderstate.curshader->deforms[0], m->numvertexes, m->xyz_array, tmpbuf, m);
				for (i = 1; i < shaderstate.curshader->numdeforms-1; i++)
					deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, tmpbuf, m);
				deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, tmpbuf, map, m);
				map += m->numvertexes;
			}
		}
		else
		{
			for (mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				deformgen(&shaderstate.curshader->deforms[0], m->numvertexes, m->xyz_array, map, m);
				map += m->numvertexes;
			}
		}
	}

	if (altshader->prog)
	{
		if (shaderstate.batchvbo)
		{
			vertexbuffers[VK_BUFF_COL] = shaderstate.batchvbo->colours[0].vk.buff;
			vertexoffsets[VK_BUFF_COL] = shaderstate.batchvbo->colours[0].vk.offs;
			vertexbuffers[VK_BUFF_TC]  = shaderstate.batchvbo->texcoord.vk.buff;
			vertexoffsets[VK_BUFF_TC]  = shaderstate.batchvbo->texcoord.vk.offs;
			vertexbuffers[VK_BUFF_LMTC]= shaderstate.batchvbo->lmcoord[0].vk.buff;
			vertexoffsets[VK_BUFF_LMTC]= shaderstate.batchvbo->lmcoord[0].vk.offs;

			vertexbuffers[VK_BUFF_NORM]= shaderstate.batchvbo->normals.vk.buff;
			vertexoffsets[VK_BUFF_NORM]= shaderstate.batchvbo->normals.vk.offs;
			vertexbuffers[VK_BUFF_SDIR]= shaderstate.batchvbo->svector.vk.buff;
			vertexoffsets[VK_BUFF_SDIR]= shaderstate.batchvbo->svector.vk.offs;
			vertexbuffers[VK_BUFF_TDIR]= shaderstate.batchvbo->tvector.vk.buff;
			vertexoffsets[VK_BUFF_TDIR]= shaderstate.batchvbo->tvector.vk.offs;

			if (!vertexbuffers[VK_BUFF_COL])
			{
				vertexbuffers[VK_BUFF_COL] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_COL] = 0;
			}
			if (!vertexbuffers[VK_BUFF_LMTC])
			{
				vertexbuffers[VK_BUFF_LMTC] = vertexbuffers[VK_BUFF_TC];
				vertexoffsets[VK_BUFF_LMTC] = vertexoffsets[VK_BUFF_TC];
			}
		}
		else
		{
			const mesh_t *m;
			unsigned int mno;
			unsigned int i;

			if (shaderstate.meshlist[0]->normals_array[0])
			{
				vec4_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec3_t), &vertexbuffers[VK_BUFF_NORM], &vertexoffsets[VK_BUFF_NORM]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->normals_array[0], sizeof(vec3_t)*m->numvertexes);
					map += m->numvertexes;
				}
			}
			else
			{
				vertexbuffers[VK_BUFF_NORM] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_NORM] = sizeof(vec4_t)*65536;
			}

			if (shaderstate.meshlist[0]->snormals_array[0])
			{
				vec4_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec3_t), &vertexbuffers[VK_BUFF_SDIR], &vertexoffsets[VK_BUFF_SDIR]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->snormals_array[0], sizeof(vec3_t)*m->numvertexes);
					map += m->numvertexes;
				}
			}
			else
			{
				vertexbuffers[VK_BUFF_SDIR] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_SDIR] = sizeof(vec4_t)*65536 + sizeof(vec3_t)*65536;
			}

			if (shaderstate.meshlist[0]->tnormals_array[0])
			{
				vec4_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec3_t), &vertexbuffers[VK_BUFF_TDIR], &vertexoffsets[VK_BUFF_TDIR]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->tnormals_array[0], sizeof(vec3_t)*m->numvertexes);
					map += m->numvertexes;
				}
			}
			else
			{
				vertexbuffers[VK_BUFF_TDIR] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_TDIR] = sizeof(vec4_t)*65536 + sizeof(vec3_t)*65536 + sizeof(vec3_t)*65536;
			}

			if (shaderstate.meshlist[0]->colors4f_array[0])
			{
				vec4_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec4_t), &vertexbuffers[VK_BUFF_COL], &vertexoffsets[VK_BUFF_COL]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->colors4f_array[0], sizeof(vec4_t)*m->numvertexes);
					map += m->numvertexes;
				}
			}
			else if (shaderstate.meshlist[0]->colors4b_array)
			{
				vec4_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec4_t), &vertexbuffers[VK_BUFF_COL], &vertexoffsets[VK_BUFF_COL]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					for (i = 0; i < m->numvertexes; i++)
					{
						Vector4Scale(m->colors4b_array[i], (1/255.0), map[i]);
					}
					map += m->numvertexes;
				}
			}
			else
			{	//FIXME: use some predefined buffer
				vec4_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec4_t), &vertexbuffers[VK_BUFF_COL], &vertexoffsets[VK_BUFF_COL]);
				for (i = 0; i < vertcount; i++)
				{
					Vector4Set(map[i], 1, 1, 1, 1);
				}
			}

			if (shaderstate.meshlist[0]->lmst_array[0])
			{
				vec2_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec2_t), &vertexbuffers[VK_BUFF_TC], &vertexoffsets[VK_BUFF_TC]);
				vec2_t *fte_restrict lmmap = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec2_t), &vertexbuffers[VK_BUFF_LMTC], &vertexoffsets[VK_BUFF_LMTC]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->st_array, sizeof(vec2_t)*m->numvertexes);
					memcpy(lmmap, m->lmst_array[0], sizeof(vec2_t)*m->numvertexes);
					map += m->numvertexes;
					lmmap += m->numvertexes;
				}
			}
			else
			{
				vec2_t *fte_restrict map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec2_t), &vertexbuffers[VK_BUFF_TC], &vertexoffsets[VK_BUFF_TC]);
				for (mno = 0; mno < shaderstate.nummeshes; mno++)
				{
					m = shaderstate.meshlist[mno];
					memcpy(map, m->st_array, sizeof(*m->st_array)*m->numvertexes);
					map += m->numvertexes;
				}
				
				vertexbuffers[VK_BUFF_LMTC] = vertexbuffers[VK_BUFF_TC];
				vertexoffsets[VK_BUFF_LMTC] = vertexoffsets[VK_BUFF_TC];
			}
		}

		vkCmdBindVertexBuffers(vk.rendertarg->cbuf, 0, VK_BUFF_MAX, vertexbuffers, vertexoffsets);
		if (BE_SetupMeshProgram(altshader->prog, altshader->passes, altshader->flags, idxcount))
		{
//			vkCmdPushConstants(vk.rendertarg->cbuf, altshader->prog->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(shaderstate.curtexnums->factors), shaderstate.curtexnums->factors);
			vkCmdDrawIndexed(vk.rendertarg->cbuf, idxcount, 1, idxfirst, 0, 0);
		}
	}
	else if (1)
	{
		shaderpass_t *p;

		//Vulkan has no fixed function pipeline. we emulate it if we were given no spir-v to run.

		for (passno = 0; passno < altshader->numpasses; passno += p->numMergedPasses)
		{
			p = &altshader->passes[passno];

			if (p->texgen == T_GEN_UPPEROVERLAY && !TEXLOADED(shaderstate.curtexnums->upperoverlay))
				continue;
			if (p->texgen == T_GEN_LOWEROVERLAY && !TEXLOADED(shaderstate.curtexnums->loweroverlay))
				continue;
			if (p->texgen == T_GEN_FULLBRIGHT && !TEXLOADED(shaderstate.curtexnums->fullbright))
				continue;

			if (p->prog)
			{
				if (shaderstate.batchvbo)
				{
					vertexbuffers[VK_BUFF_TC] = shaderstate.batchvbo->texcoord.vk.buff;
					vertexoffsets[VK_BUFF_TC] = shaderstate.batchvbo->texcoord.vk.offs;
					vertexbuffers[VK_BUFF_LMTC] = shaderstate.batchvbo->lmcoord[0].vk.buff;
					vertexoffsets[VK_BUFF_LMTC] = shaderstate.batchvbo->lmcoord[0].vk.offs;
				}
				else
				{
					float *map;
					map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec2_t), &vertexbuffers[VK_BUFF_TC], &vertexoffsets[VK_BUFF_TC]);
					BE_GenerateTCMods(p, map);

					vertexbuffers[VK_BUFF_LMTC] = vertexbuffers[VK_BUFF_TC];
					vertexoffsets[VK_BUFF_LMTC] = vertexoffsets[VK_BUFF_TC];
				}

				BE_GenerateColourMods(vertcount, p, &vertexbuffers[VK_BUFF_COL], &vertexoffsets[VK_BUFF_COL]);

				vertexbuffers[VK_BUFF_NORM] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_NORM] = sizeof(vec4_t)*65536;
				vertexbuffers[VK_BUFF_SDIR] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_SDIR] = vertexoffsets[VK_BUFF_NORM] + sizeof(vec3_t)*65536;
				vertexbuffers[VK_BUFF_TDIR] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_TDIR] = vertexoffsets[VK_BUFF_SDIR] + sizeof(vec3_t)*65536;

				vkCmdBindVertexBuffers(vk.rendertarg->cbuf, 0, VK_BUFF_MAX, vertexbuffers, vertexoffsets);
				if (BE_SetupMeshProgram(p->prog, p, altshader->flags, idxcount))
					vkCmdDrawIndexed(vk.rendertarg->cbuf, idxcount, 1, idxfirst, 0, 0);
				continue;
			}

			if (shaderstate.batchvbo)
			{	//texcoords are all compatible with static arrays, supposedly
				if (p->tcgen == TC_GEN_LIGHTMAP)
				{
					vertexbuffers[VK_BUFF_TC] = shaderstate.batchvbo->lmcoord[0].vk.buff;
					vertexoffsets[VK_BUFF_TC] = shaderstate.batchvbo->lmcoord[0].vk.offs;
				}
				else if (p->tcgen == TC_GEN_BASE)
				{
					vertexbuffers[VK_BUFF_TC] = shaderstate.batchvbo->texcoord.vk.buff;
					vertexoffsets[VK_BUFF_TC] = shaderstate.batchvbo->texcoord.vk.offs;
				}
				else
					Sys_Error("tcgen %u not supported\n", p->tcgen);
			}
			else
			{
				float *map;
				map = VKBE_AllocateStreamingSpace(DB_VBO, vertcount * sizeof(vec2_t), &vertexbuffers[VK_BUFF_TC], &vertexoffsets[VK_BUFF_TC]);
				BE_GenerateTCMods(p, map);
			}

			vertexbuffers[VK_BUFF_LMTC] = vertexbuffers[VK_BUFF_TC];
			vertexoffsets[VK_BUFF_LMTC] = vertexoffsets[VK_BUFF_TC];

			vertexbuffers[VK_BUFF_NORM] = shaderstate.staticbuf;
			vertexoffsets[VK_BUFF_NORM] = sizeof(vec4_t)*65536;
			vertexbuffers[VK_BUFF_SDIR] = shaderstate.staticbuf;
			vertexoffsets[VK_BUFF_SDIR] = vertexoffsets[VK_BUFF_NORM] + sizeof(vec3_t)*65536;
			vertexbuffers[VK_BUFF_TDIR] = shaderstate.staticbuf;
			vertexoffsets[VK_BUFF_TDIR] = vertexoffsets[VK_BUFF_SDIR] + sizeof(vec3_t)*65536;

			if (p->flags & SHADER_PASS_NOCOLORARRAY)
			{
				avec4_t passcolour;
				static avec4_t fakesource = {1,1,1,1};
				m = shaderstate.meshlist[0];
				colourgen(p, 1, NULL, &fakesource, &passcolour, m);
				alphagen(p, 1, NULL, &fakesource, &passcolour, m);

				//make sure nothing bugs out... this should be pure white.
				vertexbuffers[VK_BUFF_COL] = shaderstate.staticbuf;
				vertexoffsets[VK_BUFF_COL] = 0;

				vkCmdBindVertexBuffers(vk.rendertarg->cbuf, 0, VK_BUFF_MAX, vertexbuffers, vertexoffsets);
				if (BE_SetupMeshProgram(shaderstate.programfixedemu[1], p, altshader->flags, idxcount))
				{
					vkCmdPushConstants(vk.rendertarg->cbuf, shaderstate.programfixedemu[1]->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(passcolour), passcolour);
					vkCmdDrawIndexed(vk.rendertarg->cbuf, idxcount, 1, idxfirst, 0, 0);
				}
			}
			else
			{
				BE_GenerateColourMods(vertcount, p, &vertexbuffers[VK_BUFF_COL], &vertexoffsets[VK_BUFF_COL]);
				vkCmdBindVertexBuffers(vk.rendertarg->cbuf, 0, VK_BUFF_MAX, vertexbuffers, vertexoffsets);
				if (BE_SetupMeshProgram(shaderstate.programfixedemu[0], p, altshader->flags, idxcount))
					vkCmdDrawIndexed(vk.rendertarg->cbuf, idxcount, 1, idxfirst, 0, 0);
			}
		}
	}
}

void VKBE_SelectMode(backendmode_t mode)
{
	shaderstate.mode = mode;
	shaderstate.modepermutation = 0;

	if (vk.rendertarg->rpassflags & RP_MULTISAMPLE)
		shaderstate.modepermutation |= PERMUTATION_BEM_MULTISAMPLE;
	if (vk.rendertarg->rpassflags & RP_FP16)
		shaderstate.modepermutation |= PERMUTATION_BEM_FP16;
	if (vk.rendertarg->rpassflags & RP_VR)
		shaderstate.modepermutation |= PERMUTATION_BEM_VR;

	switch(mode)
	{
	default:
		break;

	case BEM_DEPTHONLY:
		shaderstate.modepermutation |= PERMUTATION_BEM_DEPTHONLY;
		break;

	case BEM_WIREFRAME:
		shaderstate.modepermutation |= PERMUTATION_BEM_WIREFRAME;
		break;

	case BEM_LIGHT:
		//fixme: is this actually needed, or just a waste of time?
		VKBE_SelectEntity(&r_worldentity);
		break;
	}
}
qboolean VKBE_GenerateRTLightShader(unsigned int lmode)
{
	if (!shaderstate.shader_rtlight[lmode])
	{
#ifdef VK_KHR_acceleration_structure
		if (lmode & LSHADER_RAYQUERY)
			shaderstate.shader_rtlight[lmode] = R_RegisterShader(va("rq_rtlight%s%s",
															(lmode & LSHADER_SPOT)?"#SPOT=1":"#SPOT=0",
															(lmode & LSHADER_CUBE)?"#CUBE=1":"#CUBE=0")
														, SUF_NONE, LIGHTPASS_SHADER_RQ);
		else
#endif
			shaderstate.shader_rtlight[lmode] = R_RegisterShader(va("rtlight%s%s%s",
															(lmode & LSHADER_SMAP)?"#PCF=1":"#PCF=0",
															(lmode & LSHADER_SPOT)?"#SPOT=1":"#SPOT=0",
															(lmode & LSHADER_CUBE)?"#CUBE=1":"#CUBE=0")
														, SUF_NONE, LIGHTPASS_SHADER);
	}
	if (shaderstate.shader_rtlight[lmode]->flags & SHADER_NODRAW)
		return false;
	return true;
}
qboolean VKBE_SelectDLight(dlight_t *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode)
{
	if (dl && TEXLOADED(dl->cubetexture))
		lmode |= LSHADER_CUBE;

	if (!VKBE_GenerateRTLightShader(lmode))
	{
		lmode &= ~(LSHADER_SMAP|LSHADER_CUBE);
		if (!VKBE_GenerateRTLightShader(lmode))
		{
			VKBE_SetupLightCBuffer(NULL, colour, NULL);
			return false;
		}
	}
	shaderstate.curdlight = dl;
	shaderstate.curlmode = lmode;

	VKBE_SetupLightCBuffer(dl, colour, axis);
	return true;
}

void VKBE_SelectEntity(entity_t *ent)
{
	BE_RotateForEntity(ent, ent->model);
}

//fixme: create allocations within larger ring buffers, use separate staging.
void *VKBE_CreateStagingBuffer(struct stagingbuf *n, size_t size, VkBufferUsageFlags usage)
{
	void *ptr;
	VkBufferCreateInfo bufinf = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

	memset(&n->mem, 0, sizeof(n->mem));

	n->retbuf = VK_NULL_HANDLE;
	n->usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufinf.flags = 0;
	bufinf.size = n->size = size;
	bufinf.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufinf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufinf.queueFamilyIndexCount = 0;
	bufinf.pQueueFamilyIndices = NULL;
	vkCreateBuffer(vk.device, &bufinf, vkallocationcb, &n->buf);

	vkGetBufferMemoryRequirements(vk.device, n->buf, &mem_reqs);

	memAllocInfo.allocationSize = mem_reqs.size;
	memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	if (memAllocInfo.memoryTypeIndex == ~0)
		Sys_Error("Unable to allocate buffer memory");

	VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &n->mem.memory));
	DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)n->mem.memory, "VKBE_CreateStagingBuffer");
	VkAssert(vkBindBufferMemory(vk.device, n->buf, n->mem.memory, n->mem.offset));
	VkAssert(vkMapMemory(vk.device, n->mem.memory, 0, n->size, 0, &ptr));

	return ptr;
}

struct fencedbufferwork
{
	struct vk_fencework fw;

	VkBuffer buf;
	vk_poolmem_t mem;
};
static void VKBE_DoneBufferStaging(void *staging)
{
	struct fencedbufferwork *n = staging;
	vkDestroyBuffer(vk.device, n->buf, vkallocationcb);
	VK_ReleasePoolMemory(&n->mem);
}
VkBuffer VKBE_FinishStaging(struct stagingbuf *n, vk_poolmem_t *mem)
{
	struct fencedbufferwork *fence;
	VkBuffer retbuf;
	
	//caller filled the staging buffer, and now wants to copy stuff to the gpu.
	vkUnmapMemory(vk.device, n->mem.memory);

	//create the hardware buffer
	if (n->retbuf)
		retbuf = n->retbuf;
	else
	{
		VkBufferCreateInfo bufinf = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};

		bufinf.flags = 0;
		bufinf.size = n->size;
		bufinf.usage = n->usage;
		bufinf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufinf.queueFamilyIndexCount = 0;
		bufinf.pQueueFamilyIndices = NULL;
		vkCreateBuffer(vk.device, &bufinf, vkallocationcb, &retbuf);
	}

	//sort out its memory
	{
		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(vk.device, retbuf, &mem_reqs);
		if (!VK_AllocatePoolMemory(vk_find_memory_require(mem_reqs.memoryTypeBits, 0), mem_reqs.size, mem_reqs.alignment, mem))
		{
			VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			VkMemoryDedicatedAllocateInfoKHR khr_mdai = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR};

			//shouldn't really happen, but just in case...
			mem_reqs.size = max(1,mem_reqs.size);

			memAllocInfo.allocationSize = mem_reqs.size;
			memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, 0);
			if (vk.khr_dedicated_allocation)
			{
				khr_mdai.buffer = retbuf;
				khr_mdai.pNext = memAllocInfo.pNext;
				memAllocInfo.pNext = &khr_mdai;
			}

			mem->pool = NULL;
			mem->offset = 0;
			mem->size = mem_reqs.size;
			mem->memory = VK_NULL_HANDLE;

			VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &mem->memory));
			DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)mem->memory, "VKBE_FinishStaging");
		}
		VkAssert(vkBindBufferMemory(vk.device, retbuf, mem->memory, mem->offset));
	}
	

	fence = VK_FencedBegin(VKBE_DoneBufferStaging, sizeof(*fence));
	fence->buf = n->buf;
	fence->mem = n->mem;

	//FIXME: barrier?

	//add the copy command
	{
		VkBufferCopy bcr = {0};
		bcr.srcOffset = 0;
		bcr.dstOffset = 0;
		bcr.size = n->size;
		vkCmdCopyBuffer(fence->fw.cbuf, n->buf, retbuf, 1, &bcr);
	}

	//FIXME: barrier?

	VK_FencedSubmit(fence);

	return retbuf;
}

void VKBE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch)
{
	int maxvboelements;
	int maxvboverts;
	int vert = 0, idx = 0;
	batch_t *batch;
	vbo_t *vbo;
	int i, j;
	mesh_t *m;
	index_t *vboedata;
	qbyte *vbovdatastart, *vbovdata;
	struct stagingbuf vbuf, ebuf;
	vk_poolmem_t *poolmem;

	vbo = Z_Malloc(sizeof(*vbo));

	maxvboverts = 0;
	maxvboelements = 0;
	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		for (i=0 ; i<batch->maxmeshes ; i++)
		{
			m = batch->mesh[i];
			maxvboelements += m->numindexes;
			maxvboverts += m->numvertexes;
		}
	}

	if (!maxvboverts || !maxvboelements)
		return;

	//determine array offsets.
	vbovdatastart = vbovdata = NULL;
	vbo->coord.vk.offs = vbovdata-vbovdatastart;		vbovdata += sizeof(vecV_t)*maxvboverts;
	vbo->texcoord.vk.offs = vbovdata-vbovdatastart;		vbovdata += sizeof(vec2_t)*maxvboverts;
	vbo->lmcoord[0].vk.offs = vbovdata-vbovdatastart;	vbovdata += sizeof(vec2_t)*maxvboverts;
	vbo->normals.vk.offs = vbovdata-vbovdatastart;		vbovdata += sizeof(vec3_t)*maxvboverts;
	vbo->svector.vk.offs = vbovdata-vbovdatastart;		vbovdata += sizeof(vec3_t)*maxvboverts;
	vbo->tvector.vk.offs = vbovdata-vbovdatastart;		vbovdata += sizeof(vec3_t)*maxvboverts;
	vbo->colours[0].vk.offs = vbovdata-vbovdatastart;	vbovdata += sizeof(vec4_t)*maxvboverts;

	vbovdatastart = vbovdata = VKBE_CreateStagingBuffer(&vbuf, vbovdata-vbovdatastart, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	vboedata = VKBE_CreateStagingBuffer(&ebuf, sizeof(*vboedata) * maxvboelements, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	vbo->indicies.vk.offs = 0;

	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		batch->vbo = vbo;
		for (j=0 ; j<batch->maxmeshes ; j++)
		{
			m = batch->mesh[j];
			m->vbofirstvert = vert;

			if (m->xyz_array)
				memcpy(vbovdata + vbo->coord.vk.offs		+ vert*sizeof(vecV_t), m->xyz_array,		sizeof(vecV_t)*m->numvertexes);
			if (m->st_array)
				memcpy(vbovdata + vbo->texcoord.vk.offs		+ vert*sizeof(vec2_t), m->st_array,			sizeof(vec2_t)*m->numvertexes);
			if (m->lmst_array[0])
				memcpy(vbovdata + vbo->lmcoord[0].vk.offs	+ vert*sizeof(vec2_t), m->lmst_array[0],	sizeof(vec2_t)*m->numvertexes);
			if (m->normals_array)
				memcpy(vbovdata + vbo->normals.vk.offs		+ vert*sizeof(vec3_t), m->normals_array,	sizeof(vec3_t)*m->numvertexes);
			if (m->snormals_array)
				memcpy(vbovdata + vbo->svector.vk.offs		+ vert*sizeof(vec3_t), m->snormals_array,	sizeof(vec3_t)*m->numvertexes);
			if (m->tnormals_array)
				memcpy(vbovdata + vbo->tvector.vk.offs		+ vert*sizeof(vec3_t), m->tnormals_array,	sizeof(vec3_t)*m->numvertexes);
			if (m->colors4f_array[0])
				memcpy(vbovdata + vbo->colours[0].vk.offs	+ vert*sizeof(vec4_t), m->colors4f_array[0],sizeof(vec4_t)*m->numvertexes);

			m->vbofirstelement = idx;
			for (i = 0; i < m->numindexes; i++)
			{
				*vboedata++ = vert + m->indexes[i];
			}
			idx += m->numindexes;
			vert += m->numvertexes;
		}
	}

	vbo->vbomem = poolmem = Z_Malloc(sizeof(*poolmem));
	vbo->coord.vk.buff = 
	vbo->texcoord.vk.buff = 
	vbo->lmcoord[0].vk.buff = 
	vbo->normals.vk.buff = 
	vbo->svector.vk.buff = 
	vbo->tvector.vk.buff = 
	vbo->colours[0].vk.buff = VKBE_FinishStaging(&vbuf, poolmem);

	vbo->ebomem = poolmem = Z_Malloc(sizeof(*poolmem));
	vbo->indicies.vk.buff = VKBE_FinishStaging(&ebuf, poolmem);
	vbo->indicies.vk.offs = 0;

	vbo->indexcount = maxvboelements;
	vbo->vertcount = maxvboverts;

	vbo->next = *vbochain;
	*vbochain = vbo;
}

void VKBE_GenBrushModelVBO(model_t *mod)
{
	unsigned int vcount, cvcount;

	batch_t *batch, *fbatch;
	int sortid;
	int i;

	fbatch = NULL;
	vcount = 0;
	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	{
		if (!mod->batches[sortid])
			continue;

		for (fbatch = batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
		{
			for (i = 0, cvcount = 0; i < batch->maxmeshes; i++)
				cvcount += batch->mesh[i]->numvertexes;

			if (vcount + cvcount > MAX_INDICIES)
			{
				VKBE_GenBatchVBOs(&mod->vbos, fbatch, batch);
				fbatch = batch;
				vcount = 0;
			}

			vcount += cvcount;
		}

		VKBE_GenBatchVBOs(&mod->vbos, fbatch, batch);
	}
}

struct vkbe_clearvbo
{
	struct vk_frameend fe;
	vbo_t *vbo;
};
static void VKBE_SafeClearVBO(void *vboptr)
{
	vbo_t *vbo = *(vbo_t**)vboptr;

	if (vbo->indicies.vk.buff)
	{
		vkDestroyBuffer(vk.device, vbo->indicies.vk.buff, vkallocationcb);
		VK_ReleasePoolMemory(vbo->ebomem);
		BZ_Free(vbo->ebomem);
	}

	if (vbo->coord.vk.buff)
	{
		vkDestroyBuffer(vk.device, vbo->coord.vk.buff, vkallocationcb);
		VK_ReleasePoolMemory(vbo->vbomem);
		BZ_Free(vbo->vbomem);
	}

	BZ_Free(vbo);
}
/*Wipes a vbo*/
void VKBE_ClearVBO(vbo_t *vbo, qboolean dataonly)
{
	if (dataonly)
	{
		//create one for the safe callback to clear.
		vbo_t *nvbo = BZ_Malloc(sizeof(*vbo));
		nvbo->indicies = vbo->indicies;
		nvbo->coord = vbo->coord;

		//scrub it now
		memset(&vbo->indicies, 0, sizeof(vbo->indicies));
		memset(&vbo->coord, 0, sizeof(vbo->coord));
		vbo = nvbo;
	}
	VK_AtFrameEnd(VKBE_SafeClearVBO, &vbo, sizeof(vbo));
}

void VK_UploadLightmap(lightmapinfo_t *lm)
{
	extern cvar_t r_lightmap_nearest;
	struct pendingtextureinfo mips;
	image_t *tex;

	lm->modified = false;
	if (!TEXVALID(lm->lightmap_texture))
	{
		lm->lightmap_texture = Image_CreateTexture("***lightmap***", NULL, (r_lightmap_nearest.ival?IF_NEAREST:IF_LINEAR));
		if (!lm->lightmap_texture)
			return;
	}
	tex = lm->lightmap_texture;

	if (0)//vk.frame && tex->vkimage)
	{	//the inline streaming path.
		//the double-copy sucks but at least ensures that the dma copies stuff from THIS frame and not some of the next one too.
		int *data;
		VkBufferImageCopy bic;
		VkBuffer buf;
		//size_t x = 0, w = lm->width;
		size_t x = lm->rectchange.l, w = lm->rectchange.r - lm->rectchange.l;
		size_t y = lm->rectchange.t, h = lm->rectchange.b - lm->rectchange.t, i;

		data = VKBE_AllocateStreamingSpace(DB_STAGING, w * h * 4, &buf, &bic.bufferOffset);
		bic.bufferRowLength = w;
		bic.bufferImageHeight = h;
		bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bic.imageSubresource.mipLevel = 0;
		bic.imageSubresource.baseArrayLayer = 0;
		bic.imageSubresource.layerCount = 1;
		bic.imageOffset.x = x;
		bic.imageOffset.y = y;
		bic.imageOffset.z = 0;
		bic.imageExtent.width = w;
		bic.imageExtent.height = h;
		bic.imageExtent.depth = 1;

		if (w == lm->width)	//can just copy the lot in a single call.
			memcpy(data, lm->lightmaps + 4*(y * lm->width), w*h*4);
		else
		{	//there's unused data on each row, oh well.
			for (i = 0; i < h; i++)
				memcpy(data + i * w, lm->lightmaps + 4*((y+i) * lm->width + x), w*4);
		}
		vkCmdCopyBufferToImage(vk.rendertarg->cbuf, buf, tex->vkimage->image, tex->vkimage->layout, 1, &bic);
	}
	else
	{	//the slow out-of-frame generic path.
		mips.extrafree = NULL;
		mips.type = PTI_2D;
		mips.mip[0].data = lm->lightmaps;
		mips.mip[0].needfree = false;
		mips.mip[0].width = lm->width;
		mips.mip[0].height = lm->height;
		mips.mip[0].depth = 1;
		switch(lm->fmt)
		{
		default:
		case PTI_A2BGR10:
		case PTI_E5BGR9:
		case PTI_RGBA16F:
		case PTI_RGBA32F:
		case PTI_L8:
			mips.encoding = lm->fmt;
			break;
		case PTI_BGRA8:
			mips.encoding = PTI_BGRX8;
			break;
		case TF_BGR24:	//shouldn't happen
			mips.encoding = PTI_R8;
			break;
		}
		mips.mipcount = 1;
		VK_LoadTextureMips(tex, &mips);
		tex->status = TEX_LOADED;
		tex->width = lm->width;
		tex->height = lm->height;
	}

	//invert the size so we're not always updating the entire thing.
	lm->rectchange.l = lm->width;
	lm->rectchange.t = lm->height;
	lm->rectchange.r = 0;
	lm->rectchange.b = 0;
	lm->modified = false;
}
/*upload all lightmaps at the start to reduce lags*/
static void BE_UploadLightmaps(qboolean force)
{
	int i;

	for (i = 0; i < numlightmaps; i++)
	{
		if (!lightmap[i])
			continue;

		if (force && !lightmap[i]->external)
		{
			lightmap[i]->rectchange.l = 0;
			lightmap[i]->rectchange.t = 0;
			lightmap[i]->rectchange.r = lightmap[i]->width;
			lightmap[i]->rectchange.b = lightmap[i]->height;
			lightmap[i]->modified = true;
		}

		if (lightmap[i]->modified)
		{
			VK_UploadLightmap(lightmap[i]);
		}
	}
}

void VKBE_UploadAllLightmaps(void)
{
	BE_UploadLightmaps(true);
}

qboolean VKBE_LightCullModel(vec3_t org, model_t *model)
{
#ifdef RTLIGHTS
	if ((shaderstate.mode == BEM_LIGHT || shaderstate.mode == BEM_STENCIL || shaderstate.mode == BEM_DEPTHONLY))
	{
		float dist;
		vec3_t disp;
		if (model->type == mod_alias)
		{
			VectorSubtract(org, shaderstate.lightinfo, disp);
			dist = DotProduct(disp, disp);
			if (dist > model->radius*model->radius + shaderstate.lightinfo[3]*shaderstate.lightinfo[3])
				return true;
		}
		else
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				if (shaderstate.lightinfo[i]-shaderstate.lightinfo[3] > org[i] + model->maxs[i])
					return true;
				if (shaderstate.lightinfo[i]+shaderstate.lightinfo[3] < org[i] + model->mins[i])
					return true;
			}
		}
	}
#endif
	return false;
}

batch_t *VKBE_GetTempBatch(void)
{
	if (shaderstate.wbatch >= shaderstate.maxwbatches)
	{
		shaderstate.wbatch++;
		return NULL;
	}
	return &shaderstate.wbatches[shaderstate.wbatch++];
}

static void VKBE_SetupLightCBuffer(dlight_t *dl, vec3_t colour, vec3_t axis[3])
{
#ifdef RTLIGHTS
	extern cvar_t gl_specular;
#endif
	vkcbuf_light_t *cbl = VKBE_AllocateStreamingSpace(DB_UBO, (sizeof(*cbl) + 0x0ff) & ~0xff, &shaderstate.ubo_light.buffer, &shaderstate.ubo_light.offset);
	shaderstate.ubo_light.range = sizeof(*cbl);

	if (!dl)
	{
		memset(cbl, 0, sizeof(*cbl));

		Vector4Set(shaderstate.lightinfo, 0, 0, 0, 0);
		return;
	}


	cbl->l_lightradius = dl->radius;

#ifdef RTLIGHTS
	if (shaderstate.curlmode & LSHADER_ORTHO)
	{
		float view[16];
		float proj[16];
		float xmin = -dl->radius;
		float ymin = -dl->radius;
		float znear = -dl->radius;
		float xmax = dl->radius;
		float ymax = dl->radius;
		float zfar = dl->radius;
		Matrix4x4_CM_Orthographic(proj, xmin, xmax, ymax, ymin, znear, zfar);
		Matrix4x4_CM_ModelViewMatrixFromAxis(view, axis[0], axis[2], axis[1], dl->origin);
		Matrix4_Multiply(proj, view, cbl->l_cubematrix);
//		Matrix4x4_CM_LightMatrixFromAxis(cbl->l_cubematrix, axis[0], axis[1], axis[2], dl->origin);
	}
	else if (shaderstate.curlmode & LSHADER_SPOT)
	{
		float view[16];
		float proj[16];
		extern cvar_t r_shadow_shadowmapping_nearclip;
		Matrix4x4_CM_Projection_Far(proj, dl->fov, dl->fov, dl->nearclip?dl->nearclip:r_shadow_shadowmapping_nearclip.value, dl->radius, false);
		Matrix4x4_CM_ModelViewMatrixFromAxis(view, axis[0], axis[1], axis[2], dl->origin);
		Matrix4_Multiply(proj, view, cbl->l_cubematrix);
	}
	else
#endif
		Matrix4x4_CM_LightMatrixFromAxis(cbl->l_cubematrix, axis[0], axis[1], axis[2], dl->origin);
	VectorCopy(dl->origin, cbl->l_lightposition);
	cbl->padl1 = 0;
	VectorCopy(colour, cbl->l_colour);
#ifdef RTLIGHTS
	VectorCopy(dl->lightcolourscales, cbl->l_lightcolourscale);
	cbl->l_lightcolourscale[0] = dl->lightcolourscales[0];
	cbl->l_lightcolourscale[1] = dl->lightcolourscales[1];
	cbl->l_lightcolourscale[2] = dl->lightcolourscales[2] * gl_specular.value;
#endif
	cbl->l_lightradius = dl->radius;
	Vector4Copy(shaderstate.lightshadowmapproj, cbl->l_shadowmapproj);
	Vector2Copy(shaderstate.lightshadowmapscale, cbl->l_shadowmapscale);

	VectorCopy(dl->origin, shaderstate.lightinfo);
	shaderstate.lightinfo[3] = dl->radius;
}


//also updates the entity constant buffer
static void BE_RotateForEntity (const entity_t *fte_restrict e, const model_t *fte_restrict mod)
{
	int i;
	float modelmatrix[16];
	float *m = modelmatrix;
	float *proj;
	vkcbuf_entity_t *fte_restrict cbe = VKBE_AllocateStreamingSpace(DB_UBO, (sizeof(*cbe) + 0x0ff) & ~0xff, &shaderstate.ubo_entity.buffer, &shaderstate.ubo_entity.offset);
	shaderstate.ubo_entity.range = sizeof(*cbe);

	shaderstate.curentity = e;

	if (e->flags & RF_DEPTHHACK)
		proj = r_refdef.m_projection_view;
	else
		proj = r_refdef.m_projection_std;

	if ((e->flags & RF_WEAPONMODEL) && r_refdef.playerview->viewentity > 0)
	{
		float em[16];
		float vm[16];

		if (e->flags & RF_WEAPONMODELNOBOB)
		{
			vm[0] = r_refdef.weaponmatrix[0][0];
			vm[1] = r_refdef.weaponmatrix[0][1];
			vm[2] = r_refdef.weaponmatrix[0][2];
			vm[3] = 0;

			vm[4] = r_refdef.weaponmatrix[1][0];
			vm[5] = r_refdef.weaponmatrix[1][1];
			vm[6] = r_refdef.weaponmatrix[1][2];
			vm[7] = 0;

			vm[8]  = r_refdef.weaponmatrix[2][0];
			vm[9]  = r_refdef.weaponmatrix[2][1];
			vm[10] = r_refdef.weaponmatrix[2][2];
			vm[11] = 0;

			vm[12] = r_refdef.weaponmatrix[3][0];
			vm[13] = r_refdef.weaponmatrix[3][1];
			vm[14] = r_refdef.weaponmatrix[3][2];
			vm[15] = 1;
		}
		else
		{
			vm[0] = r_refdef.weaponmatrix_bob[0][0];
			vm[1] = r_refdef.weaponmatrix_bob[0][1];
			vm[2] = r_refdef.weaponmatrix_bob[0][2];
			vm[3] = 0;

			vm[4] = r_refdef.weaponmatrix_bob[1][0];
			vm[5] = r_refdef.weaponmatrix_bob[1][1];
			vm[6] = r_refdef.weaponmatrix_bob[1][2];
			vm[7] = 0;

			vm[8] = r_refdef.weaponmatrix_bob[2][0];
			vm[9] = r_refdef.weaponmatrix_bob[2][1];
			vm[10] = r_refdef.weaponmatrix_bob[2][2];
			vm[11] = 0;

			vm[12] = r_refdef.weaponmatrix_bob[3][0];
			vm[13] = r_refdef.weaponmatrix_bob[3][1];
			vm[14] = r_refdef.weaponmatrix_bob[3][2];
			vm[15] = 1;
		}

		em[0] = e->axis[0][0];
		em[1] = e->axis[0][1];
		em[2] = e->axis[0][2];
		em[3] = 0;

		em[4] = e->axis[1][0];
		em[5] = e->axis[1][1];
		em[6] = e->axis[1][2];
		em[7] = 0;

		em[8] = e->axis[2][0];
		em[9] = e->axis[2][1];
		em[10] = e->axis[2][2];
		em[11] = 0;

		em[12] = e->origin[0];
		em[13] = e->origin[1];
		em[14] = e->origin[2];
		em[15] = 1;

		Matrix4_Multiply(vm, em, m);
	}
	else
	{
		m[0] = e->axis[0][0];
		m[1] = e->axis[0][1];
		m[2] = e->axis[0][2];
		m[3] = 0;

		m[4] = e->axis[1][0];
		m[5] = e->axis[1][1];
		m[6] = e->axis[1][2];
		m[7] = 0;

		m[8] = e->axis[2][0];
		m[9] = e->axis[2][1];
		m[10] = e->axis[2][2];
		m[11] = 0;

		m[12] = e->origin[0];
		m[13] = e->origin[1];
		m[14] = e->origin[2];
		m[15] = 1;
	}

	if (e->scale != 1 && e->scale != 0)	//hexen 2 stuff
	{
#ifdef HEXEN2
		float z;
		float escale;
		escale = e->scale;
		switch(e->drawflags&SCALE_TYPE_MASK)
		{
		default:
		case SCALE_TYPE_UNIFORM:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			VectorScale((m+8), escale, (m+8));
			break;
		case SCALE_TYPE_XYONLY:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			break;
		case SCALE_TYPE_ZONLY:
			VectorScale((m+8), escale, (m+8));
			break;
		}
		if (mod && (e->drawflags&SCALE_TYPE_MASK) != SCALE_TYPE_XYONLY)
		{
			switch(e->drawflags&SCALE_ORIGIN_MASK)
			{
			case SCALE_ORIGIN_CENTER:
				z = ((mod->maxs[2] + mod->mins[2]) * (1-escale))/2;
				VectorMA((m+12), z, e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_BOTTOM:
				VectorMA((m+12), mod->mins[2]*(1-escale), e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_TOP:
				VectorMA((m+12), -mod->maxs[2], e->axis[2], (m+12));
				break;
			}
		}
#else
		VectorScale((m+0), e->scale, (m+0));
		VectorScale((m+4), e->scale, (m+4));
		VectorScale((m+8), e->scale, (m+8));
#endif
	}
	else if (mod && !strcmp(mod->name, "progs/eyes.mdl"))
	{
		/*resize eyes, to make them easier to see*/
		m[14] -= (22 + 8);
		VectorScale((m+0), 2, (m+0));
		VectorScale((m+4), 2, (m+4));
		VectorScale((m+8), 2, (m+8));
	}
	if (mod && !ruleset_allow_larger_models.ival && mod->clampscale != 1)
	{	//possibly this should be on a per-frame basis, but that's a real pain to do
		Con_DPrintf("Rescaling %s by %f\n", mod->name, mod->clampscale);
		VectorScale((m+0), mod->clampscale, (m+0));
		VectorScale((m+4), mod->clampscale, (m+4));
		VectorScale((m+8), mod->clampscale, (m+8));
	}

	{
		float modelview[16];
		Matrix4_Multiply(r_refdef.m_view, m, modelview);
		Matrix4_Multiply(proj, modelview, cbe->m_modelviewproj);
	}
	memcpy(cbe->m_model, m, sizeof(cbe->m_model));
	Matrix4_Invert(modelmatrix, cbe->m_modelinv);
	Matrix4x4_CM_Transform3(cbe->m_modelinv, r_origin, cbe->e_eyepos);

	cbe->e_time = shaderstate.curtime = r_refdef.time - shaderstate.curentity->shaderTime;

	VectorCopy(e->light_avg, cbe->e_light_ambient);	cbe->pad1 = 0;
	VectorCopy(e->light_dir, cbe->e_light_dir);		cbe->pad2 = 0;
	VectorCopy(e->light_range, cbe->e_light_mul);	cbe->pad3 = 0;

	for (i = 0; i < MAXRLIGHTMAPS ; i++)
	{
		//FIXME: this is fucked, the batch isn't known yet.
		#if 0
		extern cvar_t gl_overbright;
		unsigned char s = shaderstate.curbatch?shaderstate.curbatch->lmlightstyle[i]:0;
		float sc;
		if (s == 255)
		{
			if (i == 0)
			{
				if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
					sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
				else
					sc = shaderstate.identitylighting;
				cbe->e_lmscale[i][0] = sc;
				cbe->e_lmscale[i][1] = sc;
				cbe->e_lmscale[i][2] = sc;
				cbe->e_lmscale[i][3] = 1;
				i++;
			}
			for (; i < MAXRLIGHTMAPS ; i++)
			{
				cbe->e_lmscale[i][0] = 0;
				cbe->e_lmscale[i][1] = 0;
				cbe->e_lmscale[i][2] = 0;
				cbe->e_lmscale[i][3] = 1;
			}
			break;
		}
		#else
		float sc = 1;
		#endif
		if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
			sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
		else
			sc = shaderstate.identitylighting;
//		sc *= d_lightstylevalue[s]/256.0f;

		Vector4Set(cbe->e_lmscale[i], sc, sc, sc, 1);
	}

	R_FetchPlayerColour(e->topcolour, cbe->e_uppercolour);		cbe->pad4 = 0;
	R_FetchPlayerColour(e->bottomcolour, cbe->e_lowercolour);	cbe->pad5 = 0;
	VectorCopy(e->glowmod, cbe->e_glowmod);						cbe->pad6 = 0;
	if (shaderstate.flags & BEF_FORCECOLOURMOD)
		Vector4Copy(e->shaderRGBAf, cbe->e_colourident);
	else
		Vector4Set(cbe->e_colourident, 1, 1, 1, e->shaderRGBAf[3]);

	VectorCopy(r_refdef.globalfog.colour, cbe->w_fogcolours);
	cbe->w_fogcolours[3] = r_refdef.globalfog.alpha;

	cbe->w_fogdensity = r_refdef.globalfog.density;
	cbe->w_fogdepthbias = r_refdef.globalfog.depthbias;
	Vector2Set(cbe->pad7, 0, 0);

	/*ndr = (e->flags & RF_DEPTHHACK)?0.333:1;
	if (ndr != shaderstate.rc.depthrange)
	{
		VkViewport viewport;
		shaderstate.rc.depthrange = ndr;

		viewport.x = r_refdef.pxrect.x;
		viewport.y = r_refdef.pxrect.y;
		viewport.width = r_refdef.pxrect.width;
		viewport.height = r_refdef.pxrect.height;
		viewport.minDepth = 0;
		viewport.maxDepth = ndr;
		vkCmdSetViewport(vk.rendertarg->cbuf, 0, 1, &viewport);
	}*/
}

void VKBE_SubmitBatch(batch_t *batch)
{
	shader_t *shader = batch->shader;
	unsigned int bf;
	shaderstate.nummeshes = batch->meshes - batch->firstmesh;
	if (!shaderstate.nummeshes)
		return;
	shaderstate.curbatch = batch;
	shaderstate.batchvbo = batch->vbo;
	shaderstate.meshlist = batch->mesh + batch->firstmesh;
	shaderstate.curshader = shader->remapto;
	bf = batch->flags | shaderstate.forcebeflags;
	if (shaderstate.curentity != batch->ent || shaderstate.flags != bf)
	{
		shaderstate.flags = bf;
		BE_RotateForEntity(batch->ent, batch->ent->model);
		shaderstate.curtime = r_refdef.time - shaderstate.curentity->shaderTime;
	}
	if (batch->skin)
		shaderstate.curtexnums = batch->skin;
	else if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;

	BE_DrawMeshChain_Internal();
}

void VKBE_DrawMesh_List(shader_t *shader, int nummeshes, mesh_t **meshlist, vbo_t *vbo, texnums_t *texnums, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.batchvbo = vbo;
	shaderstate.curshader = shader->remapto;
	if (texnums)
		shaderstate.curtexnums = texnums;
	else if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.meshlist = meshlist;
	shaderstate.nummeshes = nummeshes;
	shaderstate.flags = beflags | shaderstate.forcebeflags;

	BE_DrawMeshChain_Internal();
}

void VKBE_DrawMesh_Single(shader_t *shader, mesh_t *meshchain, vbo_t *vbo, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.batchvbo = vbo;
	shaderstate.curtime = realtime;
	shaderstate.curshader = shader->remapto;
	if (shader->numdefaulttextures)
		shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
	else
		shaderstate.curtexnums = shader->defaulttextures;
	shaderstate.meshlist = &meshchain;
	shaderstate.nummeshes = 1;
	shaderstate.flags = beflags | shaderstate.forcebeflags;

	BE_DrawMeshChain_Internal();
}

void VKBE_RT_Destroy(struct vk_rendertarg *targ)
{
	if (targ->framebuffer)
	{
		vkDestroyFramebuffer(vk.device, targ->framebuffer, vkallocationcb);
		VK_DestroyVkTexture(&targ->depth);
		VK_DestroyVkTexture(&targ->colour);
	}
	memset(targ, 0, sizeof(*targ));
}


struct vkbe_rtpurge
{
	VkFramebuffer framebuffer;
	vk_image_t colour;
	vk_image_t mscolour;
	vk_image_t depth;
};
static void VKBE_RT_Purge(void *ptr)
{
	struct vkbe_rtpurge *ctx = ptr;
	vkDestroyFramebuffer(vk.device, ctx->framebuffer, vkallocationcb);
	VK_DestroyVkTexture(&ctx->depth);
	VK_DestroyVkTexture(&ctx->mscolour);
	VK_DestroyVkTexture(&ctx->colour);
}
void VKBE_RT_Gen(struct vk_rendertarg *targ, vk_image_t *colour, uint32_t width, uint32_t height, qboolean clear, unsigned int flags)
{
	//sooooo much work...
	VkImageCreateInfo colour_imginfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	VkImageCreateInfo mscolour_imginfo;
	VkImageCreateInfo depth_imginfo;
	struct vkbe_rtpurge *purge;
	static VkClearValue clearvalues[3];

	if (colour)
	{	//override the width+height if we already have an image to draw to.
		width = colour->width;
		height = colour->height;
	}

	targ->restartinfo.clearValueCount = 3;
	targ->depthcleared = true;	//will be once its activated.

	if (targ->width == width && targ->height == height && targ->q_colour.flags == flags && (!(targ->rpassflags&RP_MULTISAMPLE))==(targ->mscolour.image==VK_NULL_HANDLE))
	{
		if ((colour && colour->image == targ->colour.image) || (!colour && !targ->externalimage))
		{
			if (width == 0 || height == 0)
				targ->restartinfo.renderPass = VK_NULL_HANDLE;	//illegal combination used for destruction.
			else if (clear || targ->firstuse)
				targ->restartinfo.renderPass = VK_GetRenderPass(RP_FULLCLEAR|targ->rpassflags);
			else
				targ->restartinfo.renderPass = VK_GetRenderPass(RP_DEPTHCLEAR|targ->rpassflags);	//don't care
			return;	//no work to do.
		}
	}

	if (targ->framebuffer || targ->externalimage)
	{	//schedule the old one to be destroyed at the end of the current frame. DIE OLD ONE, DIE!
		purge = VK_AtFrameEnd(VKBE_RT_Purge, NULL, sizeof(*purge));
		purge->framebuffer = targ->framebuffer; 
		purge->colour = targ->colour;
		if (targ->externalimage)
			purge->colour.image = VK_NULL_HANDLE;
		purge->mscolour = targ->mscolour;
		purge->depth = targ->depth;
		memset(&targ->colour, 0, sizeof(targ->colour));
		memset(&targ->mscolour, 0, sizeof(targ->mscolour));
		memset(&targ->depth, 0, sizeof(targ->depth));
		targ->framebuffer = VK_NULL_HANDLE;
	}

	targ->externalimage = !!colour;
	targ->q_colour.vkimage = &targ->colour;
	targ->q_depth.vkimage = &targ->depth;
	targ->q_colour.status = TEX_LOADED;
	targ->q_colour.width = width;
	targ->q_colour.height = height;
	targ->q_colour.flags = flags;

	targ->width = width;
	targ->height = height;

	if (width == 0 && height == 0)
	{
		targ->restartinfo.renderPass = VK_NULL_HANDLE;
		return;	//destroyed
	}
	if (targ->externalimage)
	{
		VkFormat old = vk.backbufformat;
		colour_imginfo.format = vk.backbufformat = colour->vkformat;
		targ->rpassflags |= RP_VR;
		targ->restartinfo.renderPass = VK_GetRenderPass(RP_FULLCLEAR|targ->rpassflags);

		vk.backbufformat = old;
	}
	else
	{
		targ->rpassflags &= ~RP_VR;
		targ->restartinfo.renderPass = VK_GetRenderPass(RP_FULLCLEAR|targ->rpassflags);

		colour_imginfo.format = (targ->rpassflags&RP_FP16)?VK_FORMAT_R16G16B16A16_SFLOAT:vk.backbufformat;
	}
	colour_imginfo.flags = 0;
	colour_imginfo.imageType = VK_IMAGE_TYPE_2D;
	colour_imginfo.extent.width = width;
	colour_imginfo.extent.height = height;
	colour_imginfo.extent.depth = 1;
	colour_imginfo.mipLevels = 1;
	colour_imginfo.arrayLayers = 1;
	//colour buffer is always 1 sample. if multisampling then we have a hidden 'mscolour' image that is paired with the depth, resolving to the 'colour' image.
	colour_imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
	colour_imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	colour_imginfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	colour_imginfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	colour_imginfo.queueFamilyIndexCount = 0;
	colour_imginfo.pQueueFamilyIndices = NULL;
	colour_imginfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (targ->externalimage)
	{
		targ->colour.image = colour->image;
	}
	else
	{
		VkAssert(vkCreateImage(vk.device, &colour_imginfo, vkallocationcb, &targ->colour.image));
		DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)targ->colour.image, "RT Colour");
	}

	depth_imginfo = colour_imginfo;
	depth_imginfo.format = vk.depthformat;
	depth_imginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	if (targ->rpassflags&RP_MULTISAMPLE)
	{
		mscolour_imginfo = colour_imginfo;
		depth_imginfo.samples = mscolour_imginfo.samples = vk.multisamplebits;
		VkAssert(vkCreateImage(vk.device, &mscolour_imginfo, vkallocationcb, &targ->mscolour.image));
		DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)targ->mscolour.image, "RT MS Colour");
		VK_AllocateBindImageMemory(&targ->mscolour, true);
	}
	VkAssert(vkCreateImage(vk.device, &depth_imginfo, vkallocationcb, &targ->depth.image));
	DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)targ->depth.image, "RT Depth");

	if (targ->externalimage)	//an external image is assumed to already have memory bound. don't allocate it elsewhere.
		memset(&targ->colour.mem, 0, sizeof(targ->colour.mem));
	else
		VK_AllocateBindImageMemory(&targ->colour, true);
	VK_AllocateBindImageMemory(&targ->depth, true);

	{
		VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ivci.components.r = VK_COMPONENT_SWIZZLE_R;
		ivci.components.g = VK_COMPONENT_SWIZZLE_G;
		ivci.components.b = VK_COMPONENT_SWIZZLE_B;
		ivci.components.a = VK_COMPONENT_SWIZZLE_A;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.flags = 0;

		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.format = colour_imginfo.format;
		ivci.image = targ->colour.image;
		VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->colour.view));

		if (targ->rpassflags&RP_MULTISAMPLE)
		{
			ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ivci.format = mscolour_imginfo.format;
			ivci.image = targ->mscolour.image;
			VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->mscolour.view));
		}

		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		ivci.format = depth_imginfo.format;
		ivci.image = targ->depth.image;
		VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->depth.view));
	}

	{
		VkSamplerCreateInfo lmsampinfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		lmsampinfo.minFilter = lmsampinfo.magFilter = (flags&IF_NEAREST)?VK_FILTER_NEAREST:VK_FILTER_LINEAR;
		lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		lmsampinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		lmsampinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		lmsampinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		lmsampinfo.mipLodBias = 0.0;
		lmsampinfo.anisotropyEnable = VK_FALSE;
		lmsampinfo.maxAnisotropy = 1.0;
		lmsampinfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		lmsampinfo.minLod = 0;
		lmsampinfo.maxLod = 0;
		lmsampinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		lmsampinfo.unnormalizedCoordinates = VK_FALSE;

		lmsampinfo.compareEnable = VK_FALSE;
		VK_CreateSamplerInfo(&lmsampinfo, &targ->colour);

		lmsampinfo.compareEnable = VK_TRUE;
		VK_CreateSamplerInfo(&lmsampinfo, &targ->depth);
	}

	targ->colour.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	targ->mscolour.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	targ->depth.layout = VK_IMAGE_LAYOUT_UNDEFINED;

	{
		VkFramebufferCreateInfo fbinfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		VkImageView attachments[3] = {targ->colour.view, targ->depth.view, targ->mscolour.view};
		fbinfo.flags = 0;
		fbinfo.renderPass = targ->restartinfo.renderPass;
		fbinfo.attachmentCount = (targ->rpassflags&RP_MULTISAMPLE)?3:2;
		fbinfo.pAttachments = attachments;
		fbinfo.width = width;
		fbinfo.height = height;
		fbinfo.layers = 1;
		VkAssert(vkCreateFramebuffer(vk.device, &fbinfo, vkallocationcb, &targ->framebuffer));
	}

	targ->restartinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	targ->restartinfo.pNext = NULL;
	targ->restartinfo.framebuffer = targ->framebuffer;
	targ->restartinfo.renderArea.offset.x = 0;
	targ->restartinfo.renderArea.offset.y = 0;
	targ->restartinfo.renderArea.extent.width = width;
	targ->restartinfo.renderArea.extent.height = height;
	targ->restartinfo.pClearValues = clearvalues;
	clearvalues[1].depthStencil.depth = 1;
}

struct vkbe_rtpurge_cube
{
	vk_image_t colour;
	vk_image_t depth;
	struct
	{
		VkFramebuffer framebuffer;
		VkImageView iv[2];
	} face[6];
};
static void VKBE_RT_Purge_Cube(void *ptr)
{
	uint32_t f;
	struct vkbe_rtpurge_cube *ctx = ptr;
	for (f = 0; f < 6; f++)
	{
		vkDestroyFramebuffer(vk.device, ctx->face[f].framebuffer, vkallocationcb);
		vkDestroyImageView(vk.device, ctx->face[f].iv[0], vkallocationcb);
		vkDestroyImageView(vk.device, ctx->face[f].iv[1], vkallocationcb);
	}
	VK_DestroyVkTexture(&ctx->depth);
	VK_DestroyVkTexture(&ctx->colour);
}
//generate a cubemap-compatible 2d array, set up 6 render targets that render to their own views
void VKBE_RT_Gen_Cube(struct vk_rendertarg_cube *targ, uint32_t size, qboolean clear)
{
	VkImageCreateInfo colour_imginfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	VkImageCreateInfo depth_imginfo;
	struct vkbe_rtpurge_cube *purge;
	uint32_t f;
	static VkClearValue clearvalues[2];

	if (targ->size == size && !clear)
		return;	//no work to do.

	for (f = 0; f < 6; f++)
	{
		if (clear)
			targ->face[f].restartinfo.renderPass = VK_GetRenderPass(RP_FULLCLEAR|targ->face[f].rpassflags);
		else
			targ->face[f].restartinfo.renderPass = VK_GetRenderPass(RP_DEPTHCLEAR|targ->face[f].rpassflags);	//don't care
		targ->face[f].restartinfo.clearValueCount = 2;
	}

	if (targ->size == size)
		return;	//no work to do.

	if (targ->size)
	{	//schedule the old one to be destroyed at the end of the current frame. DIE OLD ONE, DIE!
		purge = VK_AtFrameEnd(VKBE_RT_Purge_Cube, NULL, sizeof(*purge));
		for (f = 0; f < 6; f++)
		{
			purge->face[f].framebuffer = targ->face[f].framebuffer;
			targ->face[f].framebuffer = VK_NULL_HANDLE;
			purge->face[f].iv[0] = targ->face[f].colour.view;
			purge->face[f].iv[1] = targ->face[f].depth.view;
			targ->face[f].colour.view = VK_NULL_HANDLE;
			targ->face[f].depth.view = VK_NULL_HANDLE;
		}
		purge->colour = targ->colour;
		purge->depth = targ->depth;
		memset(&targ->colour, 0, sizeof(targ->colour));
		memset(&targ->depth, 0, sizeof(targ->depth));
	}

	targ->size = size;
	if (!size)
		return;

	targ->q_colour.vkimage = &targ->colour;
	targ->q_depth.vkimage = &targ->depth;

	colour_imginfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	colour_imginfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	colour_imginfo.imageType = VK_IMAGE_TYPE_2D;
	colour_imginfo.extent.width = size;
	colour_imginfo.extent.height = size;
	colour_imginfo.mipLevels = 1;
	colour_imginfo.arrayLayers = 6;
	colour_imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
	colour_imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	colour_imginfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	colour_imginfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	colour_imginfo.queueFamilyIndexCount = 0;
	colour_imginfo.pQueueFamilyIndices = NULL;
	colour_imginfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAssert(vkCreateImage(vk.device, &colour_imginfo, vkallocationcb, &targ->colour.image));
	DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)targ->colour.image, "RT Cube Colour");

	depth_imginfo = colour_imginfo;
	depth_imginfo.format = vk.depthformat;
	depth_imginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
	VkAssert(vkCreateImage(vk.device, &depth_imginfo, vkallocationcb, &targ->depth.image));
	DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)targ->depth.image, "RT Cube Depth");

	VK_AllocateBindImageMemory(&targ->colour, true);
	VK_AllocateBindImageMemory(&targ->depth, true);

//		set_image_layout(vk.frame->cbuf, targ->colour.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//		set_image_layout(vk.frame->cbuf, targ->depth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	//public sampler
	{
		VkSamplerCreateInfo lmsampinfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		lmsampinfo.minFilter = lmsampinfo.magFilter = VK_FILTER_LINEAR;
		lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		lmsampinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		lmsampinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		lmsampinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		lmsampinfo.mipLodBias = 0.0;
		lmsampinfo.anisotropyEnable = VK_FALSE;
		lmsampinfo.maxAnisotropy = 1.0;
		lmsampinfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		lmsampinfo.minLod = 0;
		lmsampinfo.maxLod = 0;
		lmsampinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		lmsampinfo.unnormalizedCoordinates = VK_FALSE;

		lmsampinfo.compareEnable = VK_FALSE;
		VkAssert(vkCreateSampler(vk.device, &lmsampinfo, NULL, &targ->colour.sampler));

		lmsampinfo.compareEnable = VK_TRUE;
		VkAssert(vkCreateSampler(vk.device, &lmsampinfo, NULL, &targ->depth.sampler));
	}

	//public cubemap views
	{
		VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ivci.components.r = VK_COMPONENT_SWIZZLE_R;
		ivci.components.g = VK_COMPONENT_SWIZZLE_G;
		ivci.components.b = VK_COMPONENT_SWIZZLE_B;
		ivci.components.a = VK_COMPONENT_SWIZZLE_A;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 6;
		ivci.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		ivci.flags = 0;

		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.format = colour_imginfo.format;
		ivci.image = targ->colour.image;
		VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->colour.view));

		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		ivci.format = depth_imginfo.format;
		ivci.image = targ->depth.image;
		VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->depth.view));
	}

	for (f = 0; f < 6; f++)
	{
		targ->face[f].width = targ->face[f].height = size;

		//per-face view for the framebuffer
		{
			VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			ivci.components.r = VK_COMPONENT_SWIZZLE_R;
			ivci.components.g = VK_COMPONENT_SWIZZLE_G;
			ivci.components.b = VK_COMPONENT_SWIZZLE_B;
			ivci.components.a = VK_COMPONENT_SWIZZLE_A;
			ivci.subresourceRange.baseMipLevel = 0;
			ivci.subresourceRange.levelCount = 1;
			ivci.subresourceRange.baseArrayLayer = f;
			ivci.subresourceRange.layerCount = 1;
			ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ivci.flags = 0;

			ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ivci.format = colour_imginfo.format;
			ivci.image = targ->colour.image;
			VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->face[f].colour.view));

			ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ivci.format = depth_imginfo.format;
			ivci.image = targ->depth.image;
			VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &targ->face[f].depth.view));
		}

		targ->colour.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		targ->depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		{
			VkFramebufferCreateInfo fbinfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
			VkImageView attachments[2] = {targ->face[f].colour.view, targ->face[f].depth.view};
			fbinfo.flags = 0;
			fbinfo.renderPass = VK_GetRenderPass(RP_FULLCLEAR|targ->face[f].rpassflags);
			fbinfo.attachmentCount = countof(attachments);
			fbinfo.pAttachments = attachments;
			fbinfo.width = size;
			fbinfo.height = size;
			fbinfo.layers = 1;
			VkAssert(vkCreateFramebuffer(vk.device, &fbinfo, vkallocationcb, &targ->face[f].framebuffer));
		}

		targ->face[f].restartinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		targ->face[f].restartinfo.pNext = NULL;
		targ->face[f].restartinfo.framebuffer = targ->face[f].framebuffer;
		targ->face[f].restartinfo.renderArea.offset.x = 0;
		targ->face[f].restartinfo.renderArea.offset.y = 0;
		targ->face[f].restartinfo.renderArea.extent.width = size;
		targ->face[f].restartinfo.renderArea.extent.height = size;
		targ->face[f].restartinfo.pClearValues = clearvalues;
	}
	clearvalues[1].depthStencil.depth = 1;
}

void VKBE_RT_Begin(struct vk_rendertarg *targ)
{
	if (vk.rendertarg == targ)
		return;

	r_refdef.pxrect.x = 0;
	r_refdef.pxrect.y = 0;
	r_refdef.pxrect.width = targ->width;
	r_refdef.pxrect.height = targ->height;
	r_refdef.pxrect.maxheight = targ->height;

	vid.fbpwidth = targ->width;
	vid.fbpheight = targ->height;

#if 0
	targ->cbuf = vk.rendertarg->cbuf;
	if (vk.rendertarg)
		vkCmdEndRenderPass(vk.rendertarg->cbuf);
#else
	shaderstate.rc.activepipeline = VK_NULL_HANDLE;
	targ->cbuf = VK_AllocFrameCBuf();

	{
		VkCommandBufferBeginInfo begininf = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		VkCommandBufferInheritanceInfo inh = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
		begininf.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begininf.pInheritanceInfo = &inh;
		inh.renderPass = VK_NULL_HANDLE;	//unused
		inh.subpass = 0;					//unused
		inh.framebuffer = VK_NULL_HANDLE;	//unused
		inh.occlusionQueryEnable = VK_FALSE;
		inh.queryFlags = 0;
		inh.pipelineStatistics = 0;
		vkBeginCommandBuffer(targ->cbuf, &begininf);
	}
#endif

	targ->prevtarg = vk.rendertarg;
	vk.rendertarg = targ;

	VKBE_SelectMode(shaderstate.mode);

	vkCmdBeginRenderPass(vk.rendertarg->cbuf, &targ->restartinfo, VK_SUBPASS_CONTENTS_INLINE);
	//future reuse shouldn't clear stuff
	if (targ->restartinfo.clearValueCount)
	{
		targ->depthcleared = true;
		targ->restartinfo.renderPass = VK_GetRenderPass(RP_RESUME|targ->rpassflags);
		targ->restartinfo.clearValueCount = 0;
	}

	{
		VkRect2D wrekt;
		VkViewport viewport;
		viewport.x = r_refdef.pxrect.x;
		viewport.y = r_refdef.pxrect.y;
		viewport.width = r_refdef.pxrect.width;
		viewport.height = r_refdef.pxrect.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		vkCmdSetViewport(vk.rendertarg->cbuf, 0, 1, &viewport);
		wrekt.offset.x = viewport.x;
		wrekt.offset.y = viewport.y;
		wrekt.extent.width = viewport.width;
		wrekt.extent.height = viewport.height;
		vkCmdSetScissor(vk.rendertarg->cbuf, 0, 1, &wrekt);
	}
}

void VKBE_RT_End(struct vk_rendertarg *targ)
{
	if (R2D_Flush)
		R2D_Flush();

	vk.rendertarg = vk.rendertarg->prevtarg;

	vid.fbpwidth = vk.rendertarg->width;
	vid.fbpheight = vk.rendertarg->height;

#if 0
#else
	shaderstate.rc.activepipeline = VK_NULL_HANDLE;
	vkCmdEndRenderPass(targ->cbuf);
	vkEndCommandBuffer(targ->cbuf);

	VK_Submit_Work(targ->cbuf, VK_NULL_HANDLE, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_NULL_HANDLE, VK_NULL_HANDLE, NULL, NULL);
	
//		VK_Submit_Work(VkCommandBuffer cmdbuf, VkSemaphore semwait, VkPipelineStageFlags semwaitstagemask, VkSemaphore semsignal, VkFence fencesignal, struct vkframe *presentframe, struct vk_fencework *fencedwork)
#endif

	VKBE_SelectMode(shaderstate.mode);
}

static qboolean BE_GenerateRefraction(batch_t *batch, shader_t *bs)
{
	float oldil;
	int oldbem;
//	struct vk_rendertarg *targ;
	//these flags require rendering some view as an fbo
	if (r_refdef.recurse)
		return false;
	if (r_refdef.recurse == r_portalrecursion.ival || r_refdef.recurse == R_MAX_RECURSE)
		return false;
	if (shaderstate.mode != BEM_STANDARD && shaderstate.mode != BEM_DEPTHDARK)
		return false;
	oldbem = shaderstate.mode;
	oldil = shaderstate.identitylighting;
//	targ = vk.rendertarg;

	if (bs->flags & SHADER_HASREFLECT)
	{
		vrect_t orect = r_refdef.vrect;
		pxrect_t oprect = r_refdef.pxrect;

		r_refdef.vrect.x = 0;
		r_refdef.vrect.y = 0;
		r_refdef.vrect.width = max(1, vid.fbvwidth*bs->portalfboscale);
		r_refdef.vrect.height = max(1, vid.fbvheight*bs->portalfboscale);
		VKBE_RT_Gen(&shaderstate.rt_reflection, NULL, r_refdef.vrect.width, r_refdef.vrect.height, false, RT_IMAGEFLAGS);
		VKBE_RT_Begin(&shaderstate.rt_reflection);
		R_DrawPortal(batch, cl.worldmodel->batches, NULL, 1);
		VKBE_RT_End(&shaderstate.rt_reflection);
		r_refdef.vrect = orect;
		r_refdef.pxrect = oprect;
	}
	if (bs->flags & (SHADER_HASREFRACT|SHADER_HASREFRACTDEPTH))
	{
		extern cvar_t r_refract_fbo;
		if (r_refract_fbo.ival || (bs->flags & SHADER_HASREFRACTDEPTH))
		{
			vrect_t ovrect = r_refdef.vrect;
			pxrect_t oprect = r_refdef.pxrect;

			r_refdef.vrect.x = 0;
			r_refdef.vrect.y = 0;
			r_refdef.vrect.width = vid.fbvwidth/2;
			r_refdef.vrect.height = vid.fbvheight/2;
			VKBE_RT_Gen(&shaderstate.rt_refraction, NULL, r_refdef.vrect.width, r_refdef.vrect.height, false, RT_IMAGEFLAGS);
			VKBE_RT_Begin(&shaderstate.rt_refraction);
			R_DrawPortal(batch, cl.worldmodel->batches, NULL, ((bs->flags & SHADER_HASREFRACTDEPTH)?3:2));	//fixme
			VKBE_RT_End(&shaderstate.rt_refraction);
			r_refdef.vrect = ovrect;
			r_refdef.pxrect = oprect;

			shaderstate.tex_refraction = &shaderstate.rt_refraction.q_colour;
		}
		else
		{
			R_DrawPortal(batch, cl.worldmodel->batches, NULL, 3);
			T_Gen_CurrentRender();
			shaderstate.tex_refraction = shaderstate.tex_currentrender;
		}
	}
	/*
	if (bs->flags & SHADER_HASRIPPLEMAP)
	{
		vrect_t orect = r_refdef.vrect;
		pxrect_t oprect = r_refdef.pxrect;
		r_refdef.vrect.x = 0;
		r_refdef.vrect.y = 0;
		r_refdef.vrect.width = vid.fbvwidth/2;
		r_refdef.vrect.height = vid.fbvheight/2;
		r_refdef.pxrect.x = 0;
		r_refdef.pxrect.y = 0;
		r_refdef.pxrect.width = vid.fbpwidth/2;
		r_refdef.pxrect.height = vid.fbpheight/2;

		if (!shaderstate.tex_ripplemap)
		{
			//FIXME: can we use RGB8 instead?
			shaderstate.tex_ripplemap = Image_CreateTexture("***tex_ripplemap***", NULL, 0);
			if (!shaderstate.tex_ripplemap->num)
				qglGenTextures(1, &shaderstate.tex_ripplemap->num);
		}
		if (shaderstate.tex_ripplemap->width != r_refdef.pxrect.width || shaderstate.tex_ripplemap->height != r_refdef.pxrect.height)
		{
			shaderstate.tex_ripplemap->width = r_refdef.pxrect.width;
			shaderstate.tex_ripplemap->height = r_refdef.pxrect.height;
			GL_MTBind(0, GL_TEXTURE_2D, shaderstate.tex_ripplemap);
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, r_refdef.pxrect.width, r_refdef.pxrect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		oldfbo = GLBE_FBO_Update(&shaderstate.fbo_reflectrefrac, 0, &shaderstate.tex_ripplemap, 1, r_nulltex, r_refdef.pxrect.width, r_refdef.pxrect.height, 0);
		r_refdef.pxrect.maxheight = shaderstate.fbo_reflectrefrac.rb_size[1];
		GL_ViewportUpdate();

		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT);

		r_refdef.vrect.x = 0;
		r_refdef.vrect.y = 0;
		r_refdef.vrect.width = vid.fbvwidth;
		r_refdef.vrect.height = vid.fbvheight;
		BE_RT_Begin(&shaderstate.rt_refraction, vid.fbpwidth, vid.fbpheight);

		r_refdef.recurse+=1; //paranoid, should stop potential infinite loops
		GLBE_SubmitMeshes(cl.worldmodel->batches, SHADER_SORT_RIPPLE, SHADER_SORT_RIPPLE);
		r_refdef.recurse-=1;

		r_refdef.vrect = orect;
		r_refdef.pxrect = oprect;
		BE_RT_End();
	}
	*/
	VKBE_SelectMode(oldbem);
	shaderstate.identitylighting = oldil;

	return true;
}

static void BE_SubmitMeshesSortList(batch_t *sortlist)
{
	batch_t *batch;
	for (batch = sortlist; batch; batch = batch->next)
	{
		if (batch->meshes == batch->firstmesh)
			continue;

		if (batch->buildmeshes)
			batch->buildmeshes(batch);

		if (batch->shader->flags & SHADER_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT)
				continue;

		if (batch->shader->flags & SHADER_SKY)
		{
			if (shaderstate.mode == BEM_STANDARD || shaderstate.mode == BEM_DEPTHDARK)
			{
				if (R_DrawSkyChain (batch))
					continue;
			}
			else if (shaderstate.mode != BEM_FOG && shaderstate.mode != BEM_CREPUSCULAR && shaderstate.mode != BEM_WIREFRAME)
				continue;
		}

		if ((batch->shader->flags & (SHADER_HASREFLECT | SHADER_HASREFRACT | SHADER_HASRIPPLEMAP)) && shaderstate.mode != BEM_WIREFRAME)
			if (!BE_GenerateRefraction(batch, batch->shader))
				continue;

		VKBE_SubmitBatch(batch);
	}
}


/*generates a new modelview matrix, as well as vpn vectors*/
static void R_MirrorMatrix(plane_t *plane)
{
	float mirror[16];
	float view[16];
	float result[16];

	vec3_t pnorm;
	VectorNegate(plane->normal, pnorm);

	mirror[0] = 1-2*pnorm[0]*pnorm[0];
	mirror[1] = -2*pnorm[0]*pnorm[1];
	mirror[2] = -2*pnorm[0]*pnorm[2];
	mirror[3] = 0;

	mirror[4] = -2*pnorm[1]*pnorm[0];
	mirror[5] = 1-2*pnorm[1]*pnorm[1];
	mirror[6] = -2*pnorm[1]*pnorm[2] ;
	mirror[7] = 0;

	mirror[8]  = -2*pnorm[2]*pnorm[0];
	mirror[9]  = -2*pnorm[2]*pnorm[1];
	mirror[10] = 1-2*pnorm[2]*pnorm[2];
	mirror[11] = 0;

	mirror[12] = -2*pnorm[0]*plane->dist;
	mirror[13] = -2*pnorm[1]*plane->dist;
	mirror[14] = -2*pnorm[2]*plane->dist;
	mirror[15] = 1;

	view[0] = vpn[0];
	view[1] = vpn[1];
	view[2] = vpn[2];
	view[3] = 0;

	view[4] = -vright[0];
	view[5] = -vright[1];
	view[6] = -vright[2];
	view[7] = 0;

	view[8]  = vup[0];
	view[9]  = vup[1];
	view[10] = vup[2];
	view[11] = 0;

	view[12] = r_refdef.vieworg[0];
	view[13] = r_refdef.vieworg[1];
	view[14] = r_refdef.vieworg[2];
	view[15] = 1;

	VectorMA(r_refdef.vieworg, 0.25, plane->normal, r_refdef.pvsorigin);

	Matrix4_Multiply(mirror, view, result);

	vpn[0] = result[0];
	vpn[1] = result[1];
	vpn[2] = result[2];

	vright[0] = -result[4];
	vright[1] = -result[5];
	vright[2] = -result[6];

	vup[0] = result[8];
	vup[1] = result[9];
	vup[2] = result[10];

	r_refdef.vieworg[0] = result[12];
	r_refdef.vieworg[1] = result[13];
	r_refdef.vieworg[2] = result[14];
}
static entity_t *R_NearestPortal(plane_t *plane)
{
	int i;
	entity_t *best = NULL;
	float dist, bestd = 0;
	//for q3-compat, portals on world scan for a visedict to use for their view.
	for (i = 0; i < cl_numvisedicts; i++)
	{
		if (cl_visedicts[i].rtype == RT_PORTALSURFACE)
		{
			dist = DotProduct(cl_visedicts[i].origin, plane->normal)-plane->dist;
			dist = fabs(dist);
			if (dist < 64 && (!best || dist < bestd))
				best = &cl_visedicts[i];
		}
	}
	return best;
}

static void TransformCoord(vec3_t in, vec3_t planea[3], vec3_t planeo, vec3_t viewa[3], vec3_t viewo, vec3_t result)
{
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	local[0] = in[0] - planeo[0];
	local[1] = in[1] - planeo[1];
	local[2] = in[2] - planeo[2];

	VectorClear(transformed);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(local, planea[i]);
		VectorMA(transformed, d, viewa[i], transformed);
	}

	result[0] = transformed[0] + viewo[0];
	result[1] = transformed[1] + viewo[1];
	result[2] = transformed[2] + viewo[2];
}
static void TransformDir(vec3_t in, vec3_t planea[3], vec3_t viewa[3], vec3_t result)
{
	int		i;
	float	d;
	vec3_t tmp;

	VectorCopy(in, tmp);

	VectorClear(result);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(tmp, planea[i]);
		VectorMA(result, d, viewa[i], result);
	}
}

void R_ObliqueNearClip(float *viewmat, mplane_t *wplane);
void CL_DrawDebugPlane(float *normal, float dist, float r, float g, float b, qboolean enqueue);
static void R_DrawPortal(batch_t *batch, batch_t **blist, batch_t *depthmasklist[2], int portaltype)
{
	entity_t *view;
	plane_t plane, oplane;
	float vmat[16];
	refdef_t oldrefdef;
	vec3_t r;
	int i;
	mesh_t *mesh = batch->mesh[batch->firstmesh];
	pvsbuffer_t newvis;
	float ivmat[16], trmat[16];

	if (r_refdef.recurse >= R_MAX_RECURSE-1)
		return;

	if (!mesh->xyz_array)
		return;

	if (!mesh->normals_array)
	{
		VectorSet(plane.normal, 0, 0, 1);
	}
	else
	{
		VectorCopy(mesh->normals_array[0], plane.normal);
	}

	if (batch->ent == &r_worldentity)
	{
		plane.dist = DotProduct(mesh->xyz_array[0], plane.normal);
	}
	else
	{
		vec3_t point;
		VectorCopy(plane.normal, oplane.normal);
		//rotate the surface normal around its entity's matrix
		plane.normal[0] = oplane.normal[0]*batch->ent->axis[0][0] + oplane.normal[1]*batch->ent->axis[1][0] + oplane.normal[2]*batch->ent->axis[2][0];
		plane.normal[1] = oplane.normal[0]*batch->ent->axis[0][1] + oplane.normal[1]*batch->ent->axis[1][1] + oplane.normal[2]*batch->ent->axis[2][1];
		plane.normal[2] = oplane.normal[0]*batch->ent->axis[0][2] + oplane.normal[1]*batch->ent->axis[1][2] + oplane.normal[2]*batch->ent->axis[2][2];

		//rotate some point on the mesh around its entity's matrix
		point[0] = mesh->xyz_array[0][0]*batch->ent->axis[0][0] + mesh->xyz_array[0][1]*batch->ent->axis[1][0] + mesh->xyz_array[0][2]*batch->ent->axis[2][0] + batch->ent->origin[0];
		point[1] = mesh->xyz_array[0][0]*batch->ent->axis[0][1] + mesh->xyz_array[0][1]*batch->ent->axis[1][1] + mesh->xyz_array[0][2]*batch->ent->axis[2][1] + batch->ent->origin[1];
		point[2] = mesh->xyz_array[0][0]*batch->ent->axis[0][2] + mesh->xyz_array[0][1]*batch->ent->axis[1][2] + mesh->xyz_array[0][2]*batch->ent->axis[2][2] + batch->ent->origin[2];

		//now we can figure out the plane dist
		plane.dist = DotProduct(point, plane.normal);
	}

	//if we're too far away from the surface, don't draw anything
	if (batch->shader->flags & SHADER_AGEN_PORTAL)
	{
		/*there's a portal alpha blend on that surface, that fades out after this distance*/
		if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist > batch->shader->portaldist)
			return;
	}
	//if we're behind it, then also don't draw anything. for our purposes, behind is when the entire near clipplane is behind.
	if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist < -r_refdef.mindist)
		return;

	TRACE(("R_DrawPortal: portal type %i\n", portaltype));

	oldrefdef = r_refdef;
	r_refdef.recurse+=1;

	r_refdef.externalview = true;

	switch(portaltype)
	{
	case 1: /*fbo explicit mirror (fucked depth, working clip plane)*/
		//fixme: pvs is surely wrong?
//		r_refdef.flipcull ^= SHADER_CULL_FLIP;
		R_MirrorMatrix(&plane);
		Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, vpn, vright, vup, r_refdef.vieworg);

		VectorCopy(mesh->xyz_array[0], r_refdef.pvsorigin);
		for (i = 1; i < mesh->numvertexes; i++)
			VectorAdd(r_refdef.pvsorigin, mesh->xyz_array[i], r_refdef.pvsorigin);
		VectorScale(r_refdef.pvsorigin, 1.0/mesh->numvertexes, r_refdef.pvsorigin);
		break;
	
	case 2:	/*fbo refraction (fucked depth, working clip plane)*/
	case 3:	/*screen copy refraction (screen depth, fucked clip planes)*/
		/*refraction image (same view, just with things culled*/
		r_refdef.externalview = oldrefdef.externalview;
		VectorNegate(plane.normal, plane.normal);
		plane.dist = -plane.dist;

		//use the player's origin for r_viewleaf, because there's not much we can do anyway*/
		VectorCopy(r_origin, r_refdef.pvsorigin);

		if (cl.worldmodel && cl.worldmodel->funcs.ClusterPVS && !r_novis.ival)
		{
			int clust, i, j;
			float d;
			vec3_t point;
			r_refdef.forcevis = true;
			r_refdef.forcedvis = NULL;
			newvis.buffer = alloca(newvis.buffersize=cl.worldmodel->pvsbytes);
			for (i = batch->firstmesh; i < batch->meshes; i++)
			{
				mesh = batch->mesh[i];
				VectorClear(point);
				for (j = 0; j < mesh->numvertexes; j++)
					VectorAdd(point, mesh->xyz_array[j], point);
				VectorScale(point, 1.0f/mesh->numvertexes, point);
				d = DotProduct(point, plane.normal) - plane.dist;
				d += 0.1;	//an epsilon on the far side
				VectorMA(point, d, plane.normal, point);

				clust = cl.worldmodel->funcs.ClusterForPoint(cl.worldmodel, point, NULL);
				if (i == batch->firstmesh)
					r_refdef.forcedvis = cl.worldmodel->funcs.ClusterPVS(cl.worldmodel, clust, &newvis, PVM_REPLACE);
				else
					r_refdef.forcedvis = cl.worldmodel->funcs.ClusterPVS(cl.worldmodel, clust, &newvis, PVM_MERGE);
			}
//			memset(newvis, 0xff, pvsbytes);
		}
		Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, vpn, vright, vup, r_refdef.vieworg);
		break;

	case 0:		/*q3 portal*/
	default:
#ifdef CSQC_DAT
		if (CSQC_SetupToRenderPortal(batch->ent->keynum))
		{
			oplane = plane;

			//transform the old surface plane into the new view matrix
			Matrix4_Invert(r_refdef.m_view, ivmat);
			Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, vpn, vright, vup, r_refdef.vieworg);
			Matrix4_Multiply(ivmat, vmat, trmat);
			plane.normal[0] = -(oplane.normal[0] * trmat[0] + oplane.normal[1] * trmat[1] + oplane.normal[2] * trmat[2]);
			plane.normal[1] = -(oplane.normal[0] * trmat[4] + oplane.normal[1] * trmat[5] + oplane.normal[2] * trmat[6]);
			plane.normal[2] = -(oplane.normal[0] * trmat[8] + oplane.normal[1] * trmat[9] + oplane.normal[2] * trmat[10]);
			plane.dist = -oplane.dist + trmat[12]*oplane.normal[0] + trmat[13]*oplane.normal[1] + trmat[14]*oplane.normal[2];

			if (Cvar_Get("temp_useplaneclip", "1", 0, "temp")->ival)
				portaltype = 1;	//make sure the near clipplane is used.
		}
		else
#endif
			if (!(view = R_NearestPortal(&plane)) || VectorCompare(view->origin, view->oldorigin))
		{
			//a portal with no portal entity, or a portal rentity with an origin equal to its oldorigin, is a mirror.
//			r_refdef.flipcull ^= SHADER_CULL_FLIP;
			R_MirrorMatrix(&plane);
			Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, vpn, vright, vup, r_refdef.vieworg);

			VectorCopy(mesh->xyz_array[0], r_refdef.pvsorigin);
			for (i = 1; i < mesh->numvertexes; i++)
				VectorAdd(r_refdef.pvsorigin, mesh->xyz_array[i], r_refdef.pvsorigin);
			VectorScale(r_refdef.pvsorigin, 1.0/mesh->numvertexes, r_refdef.pvsorigin);

			portaltype = 1;
		}
		else
		{
			float d;
			vec3_t paxis[3], porigin, vaxis[3], vorg;
			void PerpendicularVector( vec3_t dst, const vec3_t src );

			oplane = plane;

			/*calculate where the surface is meant to be*/
			VectorCopy(mesh->normals_array[0], paxis[0]);
			PerpendicularVector(paxis[1], paxis[0]);
			CrossProduct(paxis[0], paxis[1], paxis[2]);
			d = DotProduct(view->origin, plane.normal) - plane.dist;
			VectorMA(view->origin, -d, paxis[0], porigin);

			/*grab the camera origin*/
			VectorNegate(view->axis[0], vaxis[0]);
			VectorNegate(view->axis[1], vaxis[1]);
			VectorCopy(view->axis[2], vaxis[2]);
			VectorCopy(view->oldorigin, vorg);

			VectorCopy(vorg, r_refdef.pvsorigin);

			/*rotate it a bit*/
			if (view->framestate.g[FS_REG].frame[1])	//oldframe
			{
				if (view->framestate.g[FS_REG].frame[0])	//newframe
					d = realtime * view->framestate.g[FS_REG].frame[0];	//newframe
				else
					d = view->skinnum + sin(realtime)*4;
			}
			else
				d = view->skinnum;

			if (d)
			{
				vec3_t rdir;
				VectorCopy(vaxis[1], rdir);
				RotatePointAroundVector(vaxis[1], vaxis[0], rdir, d);
				CrossProduct(vaxis[0], vaxis[1], vaxis[2]);
			}

			TransformCoord(oldrefdef.vieworg, paxis, porigin, vaxis, vorg, r_refdef.vieworg);
			TransformDir(vpn, paxis, vaxis, vpn);
			TransformDir(vright, paxis, vaxis, vright);
			TransformDir(vup, paxis, vaxis, vup);
			Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, vpn, vright, vup, r_refdef.vieworg);

			//transform the old surface plane into the new view matrix
			if (Matrix4_Invert(r_refdef.m_view, ivmat))
			{
				Matrix4_Multiply(ivmat, vmat, trmat);
				plane.normal[0] = -(oplane.normal[0] * trmat[0] + oplane.normal[1] * trmat[1] + oplane.normal[2] * trmat[2]);
				plane.normal[1] = -(oplane.normal[0] * trmat[4] + oplane.normal[1] * trmat[5] + oplane.normal[2] * trmat[6]);
				plane.normal[2] = -(oplane.normal[0] * trmat[8] + oplane.normal[1] * trmat[9] + oplane.normal[2] * trmat[10]);
				plane.dist = -oplane.dist + trmat[12]*oplane.normal[0] + trmat[13]*oplane.normal[1] + trmat[14]*oplane.normal[2];
				portaltype = 1;
			}
		}
		break;
	}

	/*FIXME: can we get away with stenciling the screen?*/
	/*Add to frustum culling instead of clip planes?*/
/*	if (qglClipPlane && portaltype)
	{
		GLdouble glplane[4];
		glplane[0] = plane.normal[0];
		glplane[1] = plane.normal[1];
		glplane[2] = plane.normal[2];
		glplane[3] = plane.dist;
		qglClipPlane(GL_CLIP_PLANE0, glplane);
		qglEnable(GL_CLIP_PLANE0);
	}
*/	//fixme: we can probably scissor a smaller frusum
	R_SetFrustum (r_refdef.m_projection_std, vmat);
	if (r_refdef.frustum_numplanes < MAXFRUSTUMPLANES)
	{
		extern int SignbitsForPlane (mplane_t *out);
		mplane_t fp;
		VectorCopy(plane.normal, fp.normal);
		fp.dist = plane.dist;

//		if (DotProduct(fp.normal, vpn) < 0)
//		{
//			VectorNegate(fp.normal, fp.normal);
//			fp.dist *= -1;
//		}

		fp.type = PLANE_ANYZ;
		fp.signbits = SignbitsForPlane (&fp);

		if (portaltype == 1 || portaltype == 2)
			R_ObliqueNearClip(vmat, &fp);

		//our own culling should be an epsilon forwards so we don't still draw things behind the line due to precision issues.
		fp.dist += 0.01;
		r_refdef.frustum[r_refdef.frustum_numplanes++] = fp;
	}

	//force culling to update to match the new front face.
//	memcpy(r_refdef.m_view, vmat, sizeof(float)*16);
#if 0
	if (depthmasklist)
	{
		/*draw already-drawn portals as depth-only, to ensure that their contents are not harmed*/
		/*we can only do this AFTER the oblique perspective matrix is calculated, to avoid depth inconsistancies, while we still have the old view matrix*/
		int i;
		batch_t *dmask = NULL;
		//portals to mask are relative to the old view still.
		GLBE_SelectEntity(&r_worldentity);
		currententity = NULL;
		if (gl_config.arb_depth_clamp)
			qglEnable(GL_DEPTH_CLAMP_ARB);	//ignore the near clip plane(ish), this means nearer portals can still mask further ones.
		GL_ForceDepthWritable();
		GLBE_SelectMode(BEM_DEPTHONLY);
		for (i = 0; i < 2; i++)
		{
			for (dmask = depthmasklist[i]; dmask; dmask = dmask->next)
			{
				if (dmask == batch)
					continue;
				if (dmask->meshes == dmask->firstmesh)
					continue;
				GLBE_SubmitBatch(dmask);
			}
		}
		GLBE_SelectMode(BEM_STANDARD);
		if (gl_config.arb_depth_clamp)
			qglDisable(GL_DEPTH_CLAMP_ARB);

		currententity = NULL;
	}
#endif

	currententity = NULL;

	//now determine the stuff the backend will use.
	memcpy(r_refdef.m_view, vmat, sizeof(float)*16);
	VectorAngles(vpn, vup, r_refdef.viewangles, false);
	VectorCopy(r_refdef.vieworg, r_origin);

	//determine r_refdef.flipcull & SHADER_CULL_FLIP based upon whether right is right or not.
	CrossProduct(vpn, vup, r);
	if (DotProduct(r, vright) < 0)
		r_refdef.flipcull |= SHADER_CULL_FLIP;
	else
		r_refdef.flipcull &= ~SHADER_CULL_FLIP;
	if (r_refdef.m_projection_std[5]<0)
		r_refdef.flipcull ^= SHADER_CULL_FLIP;

	VKBE_SelectEntity(&r_worldentity);

	Surf_SetupFrame();
	Surf_DrawWorld();
	//FIXME: just call Surf_DrawWorld instead?
//	R_RenderScene();

#if 0
	if (r_portaldrawplanes.ival)
	{
		//the front of the plane should generally point away from the camera, and will be drawn in bright green. woo
		CL_DrawDebugPlane(plane.normal, plane.dist+0.01, 0.0, 0.5, 0.0, false);
		CL_DrawDebugPlane(plane.normal, plane.dist-0.01, 0.0, 0.5, 0.0, false);
		//the back of the plane points towards the camera, and will be drawn in blue, for the luls
		VectorNegate(plane.normal, plane.normal);
		plane.dist *= -1;
		CL_DrawDebugPlane(plane.normal, plane.dist+0.01, 0.0, 0.0, 0.2, false);
		CL_DrawDebugPlane(plane.normal, plane.dist-0.01, 0.0, 0.0, 0.2, false);
	}
#endif


	r_refdef = oldrefdef;

	/*broken stuff*/
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

	VKBE_SelectEntity(&r_worldentity);

	TRACE(("GLR_DrawPortal: portal drawn\n"));

	currententity = NULL;
}

static void BE_SubmitMeshesPortals(batch_t **worldlist, batch_t *dynamiclist)
{
	batch_t *batch, *old;
	int i;
	/*attempt to draw portal shaders*/
	if (shaderstate.mode == BEM_STANDARD)
	{
		for (i = 0; i < 2; i++)
		{
			for (batch = i?dynamiclist:worldlist[SHADER_SORT_PORTAL]; batch; batch = batch->next)
			{
				if (batch->meshes == batch->firstmesh)
					continue;

				if (batch->buildmeshes)
					batch->buildmeshes(batch);

				/*draw already-drawn portals as depth-only, to ensure that their contents are not harmed*/
				VKBE_SelectMode(BEM_DEPTHONLY);
				for (old = worldlist[SHADER_SORT_PORTAL]; old && old != batch; old = old->next)
				{
					if (old->meshes == old->firstmesh)
						continue;
					VKBE_SubmitBatch(old);
				}
				if (!old)
				{
					for (old = dynamiclist; old != batch; old = old->next)
					{
						if (old->meshes == old->firstmesh)
							continue;
						VKBE_SubmitBatch(old);
					}
				}
				VKBE_SelectMode(BEM_STANDARD);

				R_DrawPortal(batch, worldlist, NULL, 0);

				{
					VkClearAttachment clr;
					VkClearRect rect;
					clr.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					clr.clearValue.depthStencil.depth = 1;
					clr.clearValue.depthStencil.stencil = 0;
					clr.colorAttachment = 1;
					rect.rect.offset.x = r_refdef.pxrect.x;
					rect.rect.offset.y = r_refdef.pxrect.y;
					rect.rect.extent.width = r_refdef.pxrect.width;
					rect.rect.extent.height = r_refdef.pxrect.height;
					rect.layerCount = 1;
					rect.baseArrayLayer = 0;
					vkCmdClearAttachments(vk.rendertarg->cbuf, 1, &clr, 1, &rect);
				}
				VKBE_SelectMode(BEM_DEPTHONLY);
				VKBE_SubmitBatch(batch);
				VKBE_SelectMode(BEM_STANDARD);
			}
		}
	}
}

void VKBE_SubmitMeshes (batch_t **worldbatches, batch_t **blist, int first, int stop)
{
	int i;

	for (i = first; i < stop; i++)
	{
		if (worldbatches)
		{
			if (i == SHADER_SORT_PORTAL  && !r_refdef.recurse)
				BE_SubmitMeshesPortals(worldbatches, blist[i]);

			BE_SubmitMeshesSortList(worldbatches[i]);
		}
		BE_SubmitMeshesSortList(blist[i]);
	}
}

#ifdef RTLIGHTS
//FIXME: needs context for threading
void VKBE_BaseEntTextures(const qbyte *scenepvs, const int *sceneareas)
{
	batch_t *batches[SHADER_SORT_COUNT];
	BE_GenModelBatches(batches, shaderstate.curdlight, shaderstate.mode, scenepvs, sceneareas);
	VKBE_SubmitMeshes(NULL, batches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
	VKBE_SelectEntity(&r_worldentity);
}

struct vk_shadowbuffer
{
	qboolean isstatic;

	VkBuffer vbuffer;
	VkDeviceSize voffset;
	vk_poolmem_t vmemory;
	unsigned int numverts;

	VkBuffer ibuffer;
	VkDeviceSize ioffset;
	vk_poolmem_t imemory;
	unsigned int numindicies;
};
//FIXME: needs context for threading
struct vk_shadowbuffer *VKBE_GenerateShadowBuffer(vecV_t *verts, int numverts, index_t *indicies, int numindicies, qboolean istemp)
{
	static struct vk_shadowbuffer tempbuf;
	if (!numverts || !numindicies)
		return NULL;
	if (istemp)
	{
		struct vk_shadowbuffer *buf = &tempbuf;
		void *fte_restrict map;

		map = VKBE_AllocateStreamingSpace(DB_VBO, sizeof(*verts)*numverts, &buf->vbuffer, &buf->voffset);
		memcpy(map, verts, sizeof(*verts)*numverts);
		buf->numverts = numverts;

		map = VKBE_AllocateStreamingSpace(DB_EBO, sizeof(*indicies)*numindicies, &buf->ibuffer, &buf->ioffset);
		memcpy(map, indicies, sizeof(*indicies)*numindicies);
		buf->numindicies = numindicies;
		return buf;
	}
	else
	{
		//FIXME: these buffers should really be some subsection of a larger buffer
		struct vk_shadowbuffer *buf = BZ_Malloc(sizeof(*buf));
		struct stagingbuf vbuf;
		void *fte_restrict map;
		buf->isstatic = true;

		map = VKBE_CreateStagingBuffer(&vbuf, sizeof(*verts) * numverts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		memcpy(map, verts, sizeof(*verts) * numverts);
		buf->vbuffer = VKBE_FinishStaging(&vbuf, &buf->vmemory);
		buf->voffset = 0;
		buf->numverts = numverts;

		map = VKBE_CreateStagingBuffer(&vbuf, sizeof(*indicies) * numindicies, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		memcpy(map, indicies, sizeof(*indicies) * numindicies);
		buf->ibuffer = VKBE_FinishStaging(&vbuf, &buf->imemory);
		buf->ioffset = 0;
		buf->numindicies = numindicies;
		return buf;
	}
}
static void VKBE_DestroyShadowBuffer_Delayed(void *ctx)
{
	struct vk_shadowbuffer *buf = ctx;
	vkDestroyBuffer(vk.device, buf->vbuffer, vkallocationcb);
	vkDestroyBuffer(vk.device, buf->ibuffer, vkallocationcb);
	VK_ReleasePoolMemory(&buf->vmemory);
	VK_ReleasePoolMemory(&buf->imemory);
}
void VKBE_DestroyShadowBuffer(struct vk_shadowbuffer *buf)
{
	if (buf && buf->isstatic)
	{
		VK_AtFrameEnd(VKBE_DestroyShadowBuffer_Delayed, buf, sizeof(*buf));
		Z_Free(buf);
	}
}

//draws all depth-only surfaces from the perspective of the light.
//FIXME: needs context for threading
void VKBE_RenderShadowBuffer(struct vk_shadowbuffer *buf)
{
	shader_t *depthonlyshader;
	if (!buf)
		return;

	depthonlyshader = R_RegisterShader("depthonly", SUF_NONE,
				"{\n"
					"program depthonly\n"
					"{\n"
						"depthwrite\n"
						"maskcolor\n"
					"}\n"
				"}\n"
			);

	vkCmdBindVertexBuffers(vk.rendertarg->cbuf, 0, 1, &buf->vbuffer, &buf->voffset);
	vkCmdBindIndexBuffer(vk.rendertarg->cbuf, buf->ibuffer, buf->ioffset, VK_INDEX_TYPE);
	if (BE_SetupMeshProgram(depthonlyshader->prog, depthonlyshader->passes, 0, buf->numindicies))
		vkCmdDrawIndexed(vk.rendertarg->cbuf, buf->numindicies, 1, 0, 0, 0);
}


static void VK_TerminateShadowMap(void)
{
	struct shadowmaps_s *shad;
	unsigned int sbuf, i;

	for (sbuf = 0; sbuf < countof(shaderstate.shadow); sbuf++)
	{
		shad = &shaderstate.shadow[sbuf];
		if (!shad->image)
			continue;

		for (i = 0; i < countof(shad->buf); i++)
		{
			vkDestroyImageView(vk.device, shad->buf[i].vimage.view, vkallocationcb);
			vkDestroySampler(vk.device, shad->buf[i].vimage.sampler, vkallocationcb);
			vkDestroyFramebuffer(vk.device, shad->buf[i].framebuffer, vkallocationcb);
		}
		vkDestroyImage(vk.device, shad->image, vkallocationcb);
		vkFreeMemory(vk.device, shad->memory, vkallocationcb);

		shad->width = 0;
		shad->height = 0;
	}
}

qboolean VKBE_BeginShadowmap(qboolean isspot, uint32_t width, uint32_t height)
{
	struct shadowmaps_s *shad = &shaderstate.shadow[isspot];
	unsigned int sbuf;

//	const qboolean altqueue = false;

//	if (!altqueue)
		vkCmdEndRenderPass(vk.rendertarg->cbuf);

	if (shad->width != width || shad->height != height)
	{
		//actually, this will really only happen once per.
		//so we can be lazy and not free here... check out validation/leak warnings if this changes...

		unsigned int i;
		VkFramebufferCreateInfo fbinfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		VkImageCreateInfo imginfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		imginfo.format = vk.depthformat;
		imginfo.flags = 0;
		imginfo.imageType = VK_IMAGE_TYPE_2D;
		imginfo.extent.width = width;
		imginfo.extent.height = height;
		imginfo.extent.depth = 1;
		imginfo.mipLevels = 1;
		imginfo.arrayLayers = countof(shad->buf);
		imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
		imginfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imginfo.queueFamilyIndexCount = 0;
		imginfo.pQueueFamilyIndices = NULL;
		imginfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkAssert(vkCreateImage(vk.device, &imginfo, vkallocationcb, &shad->image));
		DebugSetName(VK_OBJECT_TYPE_IMAGE, (uint64_t)shad->image, "Shadowmap");

		{
			VkMemoryRequirements mem_reqs;
			VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			vkGetImageMemoryRequirements(vk.device, shad->image, &mem_reqs);
			memAllocInfo.allocationSize = mem_reqs.size;
			memAllocInfo.memoryTypeIndex = vk_find_memory_try(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (memAllocInfo.memoryTypeIndex == ~0)
				memAllocInfo.memoryTypeIndex = vk_find_memory_require(mem_reqs.memoryTypeBits, 0);
			VkAssert(vkAllocateMemory(vk.device, &memAllocInfo, vkallocationcb, &shad->memory));
			DebugSetName(VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)shad->memory, "VKBE_BeginShadowmap");
			VkAssert(vkBindImageMemory(vk.device, shad->image, shad->memory, 0));
		}

		fbinfo.flags = 0;
		fbinfo.renderPass = VK_GetRenderPass(RP_DEPTHONLY);
		fbinfo.attachmentCount = 1;
		fbinfo.width = width;
		fbinfo.height = height;
		fbinfo.layers = 1;
		for (i = 0; i < countof(shad->buf); i++)
		{
			VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			ivci.format = imginfo.format;
			ivci.components.r = VK_COMPONENT_SWIZZLE_R;
			ivci.components.g = VK_COMPONENT_SWIZZLE_G;
			ivci.components.b = VK_COMPONENT_SWIZZLE_B;
			ivci.components.a = VK_COMPONENT_SWIZZLE_A;
			ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ivci.subresourceRange.baseMipLevel = 0;
			ivci.subresourceRange.levelCount = 1;
			ivci.subresourceRange.baseArrayLayer = i;
			ivci.subresourceRange.layerCount = 1;
			ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ivci.flags = 0;
			ivci.image = shad->image;
			shad->buf[i].vimage.image = shad->image;
			VkAssert(vkCreateImageView(vk.device, &ivci, vkallocationcb, &shad->buf[i].vimage.view));

			{
				VkSamplerCreateInfo lmsampinfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
				lmsampinfo.minFilter = lmsampinfo.magFilter = VK_FILTER_LINEAR;
				lmsampinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				lmsampinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				lmsampinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				lmsampinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				lmsampinfo.mipLodBias = 0.0;
				lmsampinfo.anisotropyEnable = VK_FALSE;
				lmsampinfo.maxAnisotropy = 1.0;
				lmsampinfo.compareEnable = VK_TRUE;
				lmsampinfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				lmsampinfo.minLod = 0;
				lmsampinfo.maxLod = 0;
				lmsampinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
				lmsampinfo.unnormalizedCoordinates = VK_FALSE;
				VkAssert(vkCreateSampler(vk.device, &lmsampinfo, NULL, &shad->buf[i].vimage.sampler));
			}

			shad->buf[i].qimage.vkimage = &shad->buf[i].vimage;
			shad->buf[i].vimage.layout = VK_IMAGE_LAYOUT_UNDEFINED;

			fbinfo.pAttachments = &shad->buf[i].vimage.view;
			VkAssert(vkCreateFramebuffer(vk.device, &fbinfo, vkallocationcb, &shad->buf[i].framebuffer));
		}

		shad->width = width;
		shad->height = height;
	}

	sbuf = shad->seq++%countof(shad->buf);
	shaderstate.currentshadowmap = &shad->buf[sbuf].qimage;

/*	set_image_layout(vk.rendertarg->cbuf, shaderstate.currentshadowmap->vkimage->image, VK_IMAGE_ASPECT_DEPTH_BIT,
		shaderstate.currentshadowmap->vkimage->layout,		VK_ACCESS_SHADER_READ_BIT,						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	shaderstate.currentshadowmap->vkimage->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
*/
	{
		VkClearValue clearval;
		VkRenderPassBeginInfo rpass = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		clearval.depthStencil.depth = 1;
		clearval.depthStencil.stencil = 0;
		rpass.renderPass = VK_GetRenderPass(RP_DEPTHONLY);
		rpass.framebuffer = shad->buf[sbuf].framebuffer;
		rpass.renderArea.offset.x = 0;
		rpass.renderArea.offset.y = 0;
		rpass.renderArea.extent.width = width;
		rpass.renderArea.extent.height = height;
		rpass.clearValueCount = 1;
		rpass.pClearValues = &clearval;
		vkCmdBeginRenderPass(vk.rendertarg->cbuf, &rpass, VK_SUBPASS_CONTENTS_INLINE);
	}
	shaderstate.currentshadowmap->vkimage->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//renderpass should transition it for us.

	//viewport+scissor will be done elsewhere
	//that wasn't too painful, was it?...
	return true;
}

void VKBE_DoneShadows(void)
{
//	struct shadowmaps_s *shad = &shaderstate.shadow[isspot];
	VkViewport viewport;

//	const qboolean altqueue = false;

	//we've rendered the shadowmap, but now we need to blit it to the screen
	//so set stuff back to the main view. FIXME: do these in batches to ease the load on tilers.
	vkCmdEndRenderPass(vk.rendertarg->cbuf);

	/*if (altqueue)
	{
		vkCmdSetEvent(alt, shadowcompleteevent);
		VKBE_FlushDynamicBuffers();
		VK_Submit_Work();
		vkCmdWaitEvents(main, 1, &shadowcompleteevent, barrierstuff);
		vkCmdResetEvent(main, shadowcompleteevent);
	}
	else*/
	{
		set_image_layout(vk.rendertarg->cbuf, shaderstate.currentshadowmap->vkimage->image, VK_IMAGE_ASPECT_DEPTH_BIT,
			shaderstate.currentshadowmap->vkimage->layout, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		shaderstate.currentshadowmap->vkimage->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkCmdBeginRenderPass(vk.rendertarg->cbuf, &vk.rendertarg->restartinfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport.x = r_refdef.pxrect.x;
		viewport.y = r_refdef.pxrect.y;//r_refdef.pxrect.maxheight - (r_refdef.pxrect.y+r_refdef.pxrect.height);	//silly GL...
		viewport.width = r_refdef.pxrect.width;
		viewport.height = r_refdef.pxrect.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		vkCmdSetViewport(vk.rendertarg->cbuf, 0, 1, &viewport);
	}

	VKBE_SelectEntity(&r_worldentity);
}

void VKBE_SetupForShadowMap(dlight_t *dl, int texwidth, int texheight, float shadowscale)
{
#define SHADOWMAP_SIZE 512
	extern cvar_t r_shadow_shadowmapping_nearclip, r_shadow_shadowmapping_bias;
	float nc = dl->nearclip?dl->nearclip:r_shadow_shadowmapping_nearclip.value;
	float bias = r_shadow_shadowmapping_bias.value;

	//much of the projection matrix cancels out due to symmetry and stuff
	//we need to scale between -0.5,0.5 within the sub-image. the fragment shader will center on the subimage based upon the major axis.
	//in d3d, the depth value is scaled between 0 and 1 (gl is -1 to 1).
	//d3d's framebuffer is upside down or something annoying like that.
	shaderstate.lightshadowmapproj[0] = shadowscale * (1.0-(1.0/texwidth)) * 0.5/3.0;	//pinch x inwards
	shaderstate.lightshadowmapproj[1] = -shadowscale * (1.0-(1.0/texheight)) * 0.5/2.0;	//pinch y inwards
	shaderstate.lightshadowmapproj[2] = 0.5*(dl->radius+nc)/(nc-dl->radius);	//proj matrix 10
	shaderstate.lightshadowmapproj[3] = (dl->radius*nc)/(nc-dl->radius) - bias*nc*(1024/texheight);	//proj matrix 14	

	shaderstate.lightshadowmapscale[0] = 1.0/(SHADOWMAP_SIZE*3);
	shaderstate.lightshadowmapscale[1] = -1.0/(SHADOWMAP_SIZE*2);
}

//FIXME: needs context for threading
void VKBE_BeginShadowmapFace(void)
{
	VkRect2D wrekt;
	VkViewport viewport;

	viewport.x = r_refdef.pxrect.x;
	viewport.y = r_refdef.pxrect.maxheight - (r_refdef.pxrect.y+r_refdef.pxrect.height);	//silly GL...
	viewport.width = r_refdef.pxrect.width;
	viewport.height = r_refdef.pxrect.height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	vkCmdSetViewport(vk.rendertarg->cbuf, 0, 1, &viewport);

	wrekt.offset.x = viewport.x;
	wrekt.offset.y = viewport.y;
	wrekt.extent.width = viewport.width;
	wrekt.extent.height = viewport.height;
	vkCmdSetScissor(vk.rendertarg->cbuf, 0, 1, &wrekt);

	//shadowmaps never multisample...
	shaderstate.modepermutation &= ~(PERMUTATION_BEM_MULTISAMPLE|PERMUTATION_BEM_FP16|PERMUTATION_BEM_VR);
}
#endif

void VKBE_DrawWorld (batch_t **worldbatches)
{
	batch_t *batches[SHADER_SORT_COUNT];
	RSpeedLocals();

	shaderstate.curentity = NULL;

	{
		VkViewport viewport;

		viewport.x = r_refdef.pxrect.x;
		viewport.y = r_refdef.pxrect.y;
		viewport.width = r_refdef.pxrect.width;
		viewport.height = r_refdef.pxrect.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		vkCmdSetViewport(vk.rendertarg->cbuf, 0, 1, &viewport);
	}

	if (!r_refdef.recurse)
	{
		if (shaderstate.wbatch > shaderstate.maxwbatches)
		{
			int newm = shaderstate.wbatch;
			Z_Free(shaderstate.wbatches);
			shaderstate.wbatches = Z_Malloc(newm * sizeof(*shaderstate.wbatches));
			memset(shaderstate.wbatches + shaderstate.maxwbatches, 0, (newm - shaderstate.maxwbatches) * sizeof(*shaderstate.wbatches));
			shaderstate.maxwbatches = newm;
		}
		shaderstate.wbatch = 0;
	}

	RSpeedRemark();

	shaderstate.curdlight = NULL;
	//fixme: figure out some way to safely orphan this data so that we can throw the rest to a worker.
	BE_GenModelBatches(batches, shaderstate.curdlight, BEM_STANDARD, r_refdef.scenevis, r_refdef.sceneareas);

	BE_UploadLightmaps(false);
	if (r_refdef.scenevis)
	{
		//make sure the world draws correctly
		r_worldentity.shaderRGBAf[0] = 1;
		r_worldentity.shaderRGBAf[1] = 1;
		r_worldentity.shaderRGBAf[2] = 1;
		r_worldentity.shaderRGBAf[3] = 1;
		r_worldentity.axis[0][0] = 1;
		r_worldentity.axis[1][1] = 1;
		r_worldentity.axis[2][2] = 1;

#ifdef RTLIGHTS
		if (r_refdef.scenevis && r_shadow_realtime_world.ival)
			shaderstate.identitylighting = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			shaderstate.identitylighting = r_lightmap_scale.value;
		shaderstate.identitylighting *= r_refdef.hdr_value;
		shaderstate.identitylightmap = shaderstate.identitylighting / (1<<gl_overbright.ival);

		if (r_lightprepass)
		{
			//set up render target for gbuffer
			//draw opaque gbuffers
			//switch render targets to lighting (renderpasses?)
			//draw lpp lights
			//revert to screen
			//draw opaques again.
		}
		else
		{
			VKBE_SelectMode(BEM_STANDARD);

			
			VKBE_SubmitMeshes(worldbatches, batches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
			RSpeedEnd(RSPEED_OPAQUE);

#ifdef RTLIGHTS
			RSpeedRemark();
			VKBE_SelectEntity(&r_worldentity);
			Sh_DrawLights(r_refdef.scenevis);
			RSpeedEnd(RSPEED_RTLIGHTS);
#endif
		}

		RSpeedRemark();
		VKBE_SubmitMeshes(worldbatches, batches, SHADER_SORT_SEETHROUGH+1, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_TRANSPARENTS);

		if (r_wireframe.ival)
		{
			VKBE_SelectMode(BEM_WIREFRAME);
			VKBE_SubmitMeshes(worldbatches, batches, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);
			VKBE_SelectMode(BEM_STANDARD);
		}
	}
	else
	{
		shaderstate.identitylighting = 1;
		shaderstate.identitylightmap = 1;
		VKBE_SubmitMeshes(NULL, batches, SHADER_SORT_PORTAL, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_TRANSPARENTS);
	}

	R_RenderDlights ();

	shaderstate.identitylighting = 1;
	BE_RotateForEntity(&r_worldentity, NULL);

	if (r_refdef.recurse)
		RQ_RenderBatch();
	else
		RQ_RenderBatchClear();
}

void VKBE_VBO_Begin(vbobctx_t *ctx, size_t maxsize)
{
	struct stagingbuf *n = Z_Malloc(sizeof(*n));
	ctx->vboptr[0] = n;
	ctx->maxsize = maxsize;
	ctx->pos = 0;

	ctx->fallback = VKBE_CreateStagingBuffer(n, maxsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	//preallocate the target buffer, so we can prematurely refer to it.
	{
		VkBufferCreateInfo bufinf = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};

		bufinf.flags = 0;
		bufinf.size = n->size;
		bufinf.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufinf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufinf.queueFamilyIndexCount = 0;
		bufinf.pQueueFamilyIndices = NULL;
		vkCreateBuffer(vk.device, &bufinf, vkallocationcb, &n->retbuf);
	}
}
void VKBE_VBO_Data(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray)
{
	struct stagingbuf *n = ctx->vboptr[0];
	varray->vk.offs = ctx->pos;
	varray->vk.buff = n->retbuf;
	ctx->pos += size;

	memcpy((char*)ctx->fallback + varray->vk.offs, data, size);
}
void VKBE_VBO_Finish(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem)
{
	struct stagingbuf *n;
	struct stagingbuf ebo;
	vk_poolmem_t *poolmem;
	index_t *map = VKBE_CreateStagingBuffer(&ebo, esize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	memcpy(map, edata, esize);
	*ebomem = poolmem = Z_Malloc(sizeof(*poolmem));
	earray->vk.buff = VKBE_FinishStaging(&ebo, poolmem);
	earray->vk.offs = 0;

	if (ctx)
	{
		n = ctx->vboptr[0];
		*vbomem = poolmem = Z_Malloc(sizeof(*poolmem));
		/*buffer was pre-created*/VKBE_FinishStaging(n, poolmem);
		Z_Free(n);
	}
}
void VKBE_VBO_Destroy(vboarray_t *vearray, void *mem)
{
	vk_poolmem_t *poolmem = mem;
	struct fencedbufferwork *fence;
	if (!vearray->vk.buff)
		return;	//not actually allocated...

	fence = VK_AtFrameEnd(VKBE_DoneBufferStaging, NULL, sizeof(*fence));
	fence->buf = vearray->vk.buff;
	fence->mem = *poolmem;

	Z_Free(poolmem);
}

void VKBE_Scissor(srect_t *rect)
{
	VkRect2D wrekt;
	if (rect)
	{
		wrekt.offset.x = rect->x * vid.fbpwidth;
		wrekt.offset.y = (1 - (rect->height + rect->y))*vid.fbpheight;  //our api was made for gl. :(
		wrekt.extent.width = rect->width * vid.fbpwidth;
		wrekt.extent.height = rect->height * vid.fbpheight;

		if (wrekt.offset.x+wrekt.extent.width > vid.fbpwidth)
			wrekt.extent.width = vid.fbpwidth - wrekt.offset.x;
		if (wrekt.offset.y+wrekt.extent.height > vid.fbpheight)
			wrekt.extent.height = vid.fbpheight - wrekt.offset.y;
		if (wrekt.offset.x < 0)
		{
			wrekt.extent.width += wrekt.offset.x;
			wrekt.offset.x = 0;
		}
		if (wrekt.offset.y < 0)
		{
			wrekt.extent.height += wrekt.offset.x;
			wrekt.offset.y = 0;
		}
	}
	else
	{
		wrekt.offset.x = 0;
		wrekt.offset.y = 0;
		wrekt.extent.width = vid.fbpwidth;
		wrekt.extent.height = vid.fbpheight;
	}

	vkCmdSetScissor(vk.rendertarg->cbuf, 0, 1, &wrekt);
}

#endif
