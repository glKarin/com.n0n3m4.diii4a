
#include "quakedef.h"
#include "image.h"

// FIXME: fix skybox after vid_restart
cvar_t r_sky = {CF_CLIENT | CF_ARCHIVE, "r_sky", "1", "enables sky rendering (black otherwise)"};
cvar_t r_skyscroll1 = {CF_CLIENT | CF_ARCHIVE, "r_skyscroll1", "1", "speed at which upper clouds layer scrolls in quake sky"};
cvar_t r_skyscroll2 = {CF_CLIENT | CF_ARCHIVE, "r_skyscroll2", "2", "speed at which lower clouds layer scrolls in quake sky"};
cvar_t r_sky_scissor = {CF_CLIENT, "r_sky_scissor", "1", "limit rendering of sky to approximately the area of the sky surfaces"};
int skyrenderlater;
int skyrendermasked;
int skyscissor[4];

static int skyrendersphere;
static int skyrenderbox;
static rtexturepool_t *skytexturepool;
static char skyname[MAX_QPATH];
static matrix4x4_t skymatrix;
static matrix4x4_t skyinversematrix;

typedef struct suffixinfo_s
{
	const char *suffix;
	qbool flipx, flipy, flipdiagonal;
}
suffixinfo_t;
static const suffixinfo_t suffix[3][6] =
{
	{
		{"px",   false, false, false},
		{"nx",   false, false, false},
		{"py",   false, false, false},
		{"ny",   false, false, false},
		{"pz",   false, false, false},
		{"nz",   false, false, false}
	},
	{
		{"posx", false, false, false},
		{"negx", false, false, false},
		{"posy", false, false, false},
		{"negy", false, false, false},
		{"posz", false, false, false},
		{"negz", false, false, false}
	},
	{
		{"rt",   false, false,  true},
		{"lf",    true,  true,  true},
		{"bk",   false,  true, false},
		{"ft",    true, false, false},
		{"up",   false, false,  true},
		{"dn",   false, false,  true}
	}
};

static skinframe_t *skyboxskinframe[6];

void R_SkyStartFrame(void)
{
	skyrendersphere = false;
	skyrenderbox = false;
	skyrendermasked = false;
	// for depth-masked sky, we need to know whether any sky was rendered
	skyrenderlater = false;
	// we can scissor the sky to just the relevant area
	Vector4Clear(skyscissor);
	if (r_sky.integer)
	{
		if (skyboxskinframe[0] || skyboxskinframe[1] || skyboxskinframe[2] || skyboxskinframe[3] || skyboxskinframe[4] || skyboxskinframe[5])
			skyrenderbox = true;
		else if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.solidskyskinframe)
			skyrendersphere = true;
		skyrendermasked = true;
	}
}

/*
==================
R_SetSkyBox
==================
*/
static void R_UnloadSkyBox(void)
{
	int i;
	int c = 0;
	for (i = 0;i < 6;i++)
	{
		if (skyboxskinframe[i])
		{
			R_SkinFrame_PurgeSkinFrame(skyboxskinframe[i]);
			c++;
		}
		skyboxskinframe[i] = NULL;
	}
	if (c && developer_loading.integer)
		Con_Printf("unloading skybox\n");
}

static int R_LoadSkyBox(void)
{
	int i, j, success;
	int indices[4] = {0,1,2,3};
	char name[MAX_INPUTLINE];
	unsigned char *image_buffer;
	unsigned char *temp;
	char vabuf[1024];

	R_UnloadSkyBox();

	if (!skyname[0])
		return true;

	for (j=0; j<3; j++)
	{
		success = 0;
		for (i=0; i<6; i++)
		{
			if (dpsnprintf(name, sizeof(name), "%s_%s", skyname, suffix[j][i].suffix) < 0 || !(image_buffer = loadimagepixelsbgra(name, false, false, false, NULL)))
			{
				if (dpsnprintf(name, sizeof(name), "%s%s", skyname, suffix[j][i].suffix) < 0 || !(image_buffer = loadimagepixelsbgra(name, false, false, false, NULL)))
				{
					if (dpsnprintf(name, sizeof(name), "env/%s%s", skyname, suffix[j][i].suffix) < 0 || !(image_buffer = loadimagepixelsbgra(name, false, false, false, NULL)))
					{
						if (dpsnprintf(name, sizeof(name), "gfx/env/%s%s", skyname, suffix[j][i].suffix) < 0 || !(image_buffer = loadimagepixelsbgra(name, false, false, false, NULL)))
							continue;
					}
				}
			}
			temp = (unsigned char *)Mem_Alloc(tempmempool, image_width*image_height*4);
			Image_CopyMux (temp, image_buffer, image_width, image_height, suffix[j][i].flipx, suffix[j][i].flipy, suffix[j][i].flipdiagonal, 4, 4, indices);
			skyboxskinframe[i] = R_SkinFrame_LoadInternalBGRA(va(vabuf, sizeof(vabuf), "skyboxside%d", i), TEXF_CLAMP | (gl_texturecompression_sky.integer ? TEXF_COMPRESS : 0), temp, image_width, image_height, 0, 0, 0, vid.sRGB3D);
			Mem_Free(image_buffer);
			Mem_Free(temp);
			success++;
		}

		if (success)
			break;
	}

	if (j == 3)
		return false;

	if (developer_loading.integer)
		Con_Printf("loading skybox \"%s\"\n", name);

	return true;
}

int R_SetSkyBox(const char *sky)
{
	if (strcmp(sky, skyname) == 0) // no change
		return true;

	if (strlen(sky) > 1000)
	{
		Con_Printf("sky name too long (%i, max is 1000)\n", (int)strlen(sky));
		return false;
	}

	dp_strlcpy(skyname, sky, sizeof(skyname));

	return R_LoadSkyBox();
}

// LadyHavoc: added LoadSky console command
static void LoadSky_f(cmd_state_t *cmd)
{
	switch (Cmd_Argc(cmd))
	{
	case 1:
		if (skyname[0])
			Con_Printf("current sky: %s\n", skyname);
		else
			Con_Print("no skybox has been set\n");
		break;
	case 2:
		if (R_SetSkyBox(Cmd_Argv(cmd, 1)))
		{
			if (skyname[0])
				Con_Printf("skybox set to %s\n", skyname);
			else
				Con_Print("skybox disabled\n");
		}
		else
			Con_Printf(CON_ERROR "failed to load skybox %s\n", Cmd_Argv(cmd, 1));
		break;
	default:
		Con_Print("usage: loadsky skyname\n");
		break;
	}
}

static const float skyboxvertex3f[6*4*3] =
{
	// skyside[0]
	 16, -16,  16,
	 16, -16, -16,
	 16,  16, -16,
	 16,  16,  16,
	// skyside[1]
	-16,  16,  16,
	-16,  16, -16,
	-16, -16, -16,
	-16, -16,  16,
	// skyside[2]
	 16,  16,  16,
	 16,  16, -16,
	-16,  16, -16,
	-16,  16,  16,
	// skyside[3]
	-16, -16,  16,
	-16, -16, -16,
	 16, -16, -16,
	 16, -16,  16,
	// skyside[4]
	-16, -16,  16,
	 16, -16,  16,
	 16,  16,  16,
	-16,  16,  16,
	// skyside[5]
	 16, -16, -16,
	-16, -16, -16,
	-16,  16, -16,
	 16,  16, -16
};

static const float skyboxtexcoord2f[6*4*2] =
{
	// skyside[0]
	0, 1,
	1, 1,
	1, 0,
	0, 0,
	// skyside[1]
	1, 0,
	0, 0,
	0, 1,
	1, 1,
	// skyside[2]
	1, 1,
	1, 0,
	0, 0,
	0, 1,
	// skyside[3]
	0, 0,
	0, 1,
	1, 1,
	1, 0,
	// skyside[4]
	0, 1,
	1, 1,
	1, 0,
	0, 0,
	// skyside[5]
	0, 1,
	1, 1,
	1, 0,
	0, 0
};

static const int skyboxelement3i[6*2*3] =
{
	// skyside[3]
	 0,  1,  2,
	 0,  2,  3,
	// skyside[1]
	 4,  5,  6,
	 4,  6,  7,
	// skyside[0]
	 8,  9, 10,
	 8, 10, 11,
	// skyside[2]
	12, 13, 14,
	12, 14, 15,
	// skyside[4]
	16, 17, 18,
	16, 18, 19,
	// skyside[5]
	20, 21, 22,
	20, 22, 23
};

static const unsigned short skyboxelement3s[6*2*3] =
{
	// skyside[3]
	 0,  1,  2,
	 0,  2,  3,
	// skyside[1]
	 4,  5,  6,
	 4,  6,  7,
	// skyside[0]
	 8,  9, 10,
	 8, 10, 11,
	// skyside[2]
	12, 13, 14,
	12, 14, 15,
	// skyside[4]
	16, 17, 18,
	16, 18, 19,
	// skyside[5]
	20, 21, 22,
	20, 22, 23
};

static void R_SkyBox(void)
{
	int i;
	RSurf_ActiveCustomEntity(&skymatrix, &skyinversematrix, 0, 0, 1, 1, 1, 1, 6*4, skyboxvertex3f, skyboxtexcoord2f, NULL, NULL, NULL, NULL, 6*2, skyboxelement3i, skyboxelement3s, false, false);
	for (i = 0;i < 6;i++)
		if(skyboxskinframe[i])
			R_DrawCustomSurface(skyboxskinframe[i], &identitymatrix, MATERIALFLAG_SKY | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE | MATERIALFLAG_NODEPTHTEST, i*4, 4, i*2, 2, false, false, false);
}

#define skygridx 32
#define skygridx1 (skygridx + 1)
#define skygridxrecip (1.0f / (skygridx))
#define skygridy 32
#define skygridy1 (skygridy + 1)
#define skygridyrecip (1.0f / (skygridy))
#define skysphere_numverts (skygridx1 * skygridy1)
#define skysphere_numtriangles (skygridx * skygridy * 2)
static float skysphere_vertex3f[skysphere_numverts * 3];
static float skysphere_texcoord2f[skysphere_numverts * 2];
static int skysphere_element3i[skysphere_numtriangles * 3];
static unsigned short skysphere_element3s[skysphere_numtriangles * 3];

static void skyspherecalc(void)
{
	int i, j;
	unsigned short *e;
	float a, b, x, ax, ay, v[3], length, *vertex3f, *texcoord2f;
	float dx, dy, dz;
	dx = 16.0f;
	dy = 16.0f;
	dz = 16.0f / 3.0f;
	vertex3f = skysphere_vertex3f;
	texcoord2f = skysphere_texcoord2f;
	for (j = 0;j <= skygridy;j++)
	{
		a = j * skygridyrecip;
		ax = cos(a * M_PI * 2);
		ay = -sin(a * M_PI * 2);
		for (i = 0;i <= skygridx;i++)
		{
			b = i * skygridxrecip;
			x = cos((b + 0.5) * M_PI);
			v[0] = ax*x * dx;
			v[1] = ay*x * dy;
			v[2] = -sin((b + 0.5) * M_PI) * dz;
			length = 3.0f / sqrt(v[0]*v[0]+v[1]*v[1]+(v[2]*v[2]*9));
			*texcoord2f++ = v[0] * length;
			*texcoord2f++ = v[1] * length;
			*vertex3f++ = v[0];
			*vertex3f++ = v[1];
			*vertex3f++ = v[2];
		}
	}
	e = skysphere_element3s;
	for (j = 0;j < skygridy;j++)
	{
		for (i = 0;i < skygridx;i++)
		{
			*e++ =  j      * skygridx1 + i;
			*e++ =  j      * skygridx1 + i + 1;
			*e++ = (j + 1) * skygridx1 + i;

			*e++ =  j      * skygridx1 + i + 1;
			*e++ = (j + 1) * skygridx1 + i + 1;
			*e++ = (j + 1) * skygridx1 + i;
		}
	}
	for (i = 0;i < skysphere_numtriangles*3;i++)
		skysphere_element3i[i] = skysphere_element3s[i];
}

static void R_SkySphere(void)
{
	double speedscale;
	static qbool skysphereinitialized = false;
	matrix4x4_t scroll1matrix, scroll2matrix;
	if (!skysphereinitialized)
	{
		skysphereinitialized = true;
		skyspherecalc();
	}

	// wrap the scroll values just to be extra kind to float accuracy

	// scroll speed for upper layer
	speedscale = r_refdef.scene.time*r_skyscroll1.value*8.0/128.0;
	speedscale -= floor(speedscale);
	Matrix4x4_CreateTranslate(&scroll1matrix, speedscale, speedscale, 0);
	// scroll speed for lower layer (transparent layer)
	speedscale = r_refdef.scene.time*r_skyscroll2.value*8.0/128.0;
	speedscale -= floor(speedscale);
	Matrix4x4_CreateTranslate(&scroll2matrix, speedscale, speedscale, 0);

	RSurf_ActiveCustomEntity(&skymatrix, &skyinversematrix, 0, 0, 1, 1, 1, 1, skysphere_numverts, skysphere_vertex3f, skysphere_texcoord2f, NULL, NULL, NULL, NULL, skysphere_numtriangles, skysphere_element3i, skysphere_element3s, false, false);
	R_DrawCustomSurface(r_refdef.scene.worldmodel->brush.solidskyskinframe, &scroll1matrix, MATERIALFLAG_SKY | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE | MATERIALFLAG_NODEPTHTEST                                            , 0, skysphere_numverts, 0, skysphere_numtriangles, false, false, false);
	R_DrawCustomSurface(r_refdef.scene.worldmodel->brush.alphaskyskinframe, &scroll2matrix, MATERIALFLAG_SKY | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE | MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED, 0, skysphere_numverts, 0, skysphere_numtriangles, false, false, false);
}

void R_Sky(void)
{
	Matrix4x4_CreateFromQuakeEntity(&skymatrix, r_refdef.view.origin[0], r_refdef.view.origin[1], r_refdef.view.origin[2], 0, 0, 0, r_refdef.farclip * (0.5f / 16.0f));
	Matrix4x4_Invert_Simple(&skyinversematrix, &skymatrix);

	if (r_sky_scissor.integer)
	{
		// if the scissor is empty just return
		if (skyscissor[2] == 0 || skyscissor[3] == 0)
			return;
		GL_Scissor(skyscissor[0], skyscissor[1], skyscissor[2], skyscissor[3]);
		GL_ScissorTest(true);
	}
	if (skyrendersphere)
	{
		// this does not modify depth buffer
		R_SkySphere();
	}
	else if (skyrenderbox)
	{
		// this does not modify depth buffer
		R_SkyBox();
	}
	/* this will be skyroom someday
	else
	{
		// this modifies the depth buffer so we have to clear it afterward
		//R_SkyRoom();
		// clear the depthbuffer that was used while rendering the skyroom
		//GL_Clear(GL_DEPTH_BUFFER_BIT);
	}
	*/
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
}

//===============================================================

void R_ResetSkyBox(void)
{
	R_UnloadSkyBox();
	memset(skyname,0,MAX_QPATH);
	R_LoadSkyBox();
}

static void r_sky_start(void)
{
	skytexturepool = R_AllocTexturePool();
	R_LoadSkyBox();
}

static void r_sky_shutdown(void)
{
	R_UnloadSkyBox();
	R_FreeTexturePool(&skytexturepool);
}

static void r_sky_newmap(void)
{
}


void R_Sky_Init(void)
{
	Cmd_AddCommand(CF_CLIENT, "loadsky", &LoadSky_f, "load a skybox by basename (for example loadsky mtnsun_ loads mtnsun_ft.tga and so on)");
	Cvar_RegisterVariable (&r_sky);
	Cvar_RegisterVariable (&r_skyscroll1);
	Cvar_RegisterVariable (&r_skyscroll2);
	Cvar_RegisterVariable (&r_sky_scissor);
	memset(&skyboxskinframe, 0, sizeof(skyboxskinframe));
	skyname[0] = 0;
	R_RegisterModule("R_Sky", r_sky_start, r_sky_shutdown, r_sky_newmap, NULL, NULL);
}

