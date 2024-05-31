
#include "quakedef.h"
#include "image.h"

cvar_t r_lightningbeam_thickness = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_thickness", "8", "thickness of the lightning beam effect"};
cvar_t r_lightningbeam_scroll = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_scroll", "5", "speed of texture scrolling on the lightning beam effect"};
cvar_t r_lightningbeam_repeatdistance = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_repeatdistance", "128", "how far to stretch the texture along the lightning beam effect"};
cvar_t r_lightningbeam_color_red = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_color_red", "1", "color of the lightning beam effect"};
cvar_t r_lightningbeam_color_green = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_color_green", "1", "color of the lightning beam effect"};
cvar_t r_lightningbeam_color_blue = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_color_blue", "1", "color of the lightning beam effect"};
cvar_t r_lightningbeam_qmbtexture = {CF_CLIENT | CF_ARCHIVE, "r_lightningbeam_qmbtexture", "0", "load the qmb textures/particles/lightning.pcx texture instead of generating one, can look better"};

static texture_t cl_beams_externaltexture;
static texture_t cl_beams_builtintexture;

static void r_lightningbeams_start(void)
{
	memset(&cl_beams_externaltexture, 0, sizeof(cl_beams_externaltexture));
	memset(&cl_beams_builtintexture, 0, sizeof(cl_beams_builtintexture));
}

static void CL_Beams_SetupExternalTexture(void)
{
	if (Mod_LoadTextureFromQ3Shader(r_main_mempool, "r_lightning.c", &cl_beams_externaltexture, "textures/particles/lightning", false, false, TEXF_ALPHA | TEXF_FORCELINEAR, MATERIALFLAG_WALL | MATERIALFLAG_NOCULLFACE | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW))
		Cvar_SetValueQuick(&r_lightningbeam_qmbtexture, false);
}

static void CL_Beams_SetupBuiltinTexture(void)
{
	// beam direction is horizontal in the lightning texture
	int texwidth = 128;
	int texheight = 64;
	float r, g, b, intensity, thickness = texheight * 0.25f, border = thickness + 2.0f, ithickness = 1.0f / thickness, center, n;
	int x, y;
	unsigned char *data;
	skinframe_t *skinframe;
	float centersamples[17][2];

	// make a repeating noise pattern for the beam path
	for (x = 0; x < 16; x++)
	{
		centersamples[x][0] = lhrandom(border, texheight - border);
		centersamples[x][1] = lhrandom(0.2f, 1.00f);
	}
	centersamples[16][0] = centersamples[0][0];
	centersamples[16][1] = centersamples[0][1];

	data = (unsigned char *)Mem_Alloc(tempmempool, texwidth * texheight * 4);

	// iterate by columns and draw the entire column of pixels
	for (x = 0; x < texwidth; x++)
	{
		r = x * 16.0f / texwidth;
		y = (int)r;
		g = r - y;
		center = centersamples[y][0] * (1.0f - g) + centersamples[y+1][0] * g;
		n = centersamples[y][1] * (1.0f - g) + centersamples[y + 1][1] * g;
		for (y = 0; y < texheight; y++)
		{
			intensity = 1.0f - fabs((y - center) * ithickness);
			if (intensity > 0)
			{
				intensity = pow(intensity * n, 2);
				r = intensity * 1.000f * 255.0f;
				g = intensity * 2.000f * 255.0f;
				b = intensity * 4.000f * 255.0f;
				data[(y * texwidth + x) * 4 + 2] = (unsigned char)(bound(0, r, 255));
				data[(y * texwidth + x) * 4 + 1] = (unsigned char)(bound(0, g, 255));
				data[(y * texwidth + x) * 4 + 0] = (unsigned char)(bound(0, b, 255));
			}
			else
				intensity = 0.0f;
			data[(y * texwidth + x) * 4 + 3] = (unsigned char)255;
		}
	}

	skinframe = R_SkinFrame_LoadInternalBGRA("lightningbeam", TEXF_FORCELINEAR, data, texwidth, texheight, 0, 0, 0, false);
	Mod_LoadCustomMaterial(r_main_mempool, &cl_beams_builtintexture, "cl_beams_builtintexture", 0, MATERIALFLAG_WALL | MATERIALFLAG_NOCULLFACE | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW, skinframe);
	Mem_Free(data);
}

static void r_lightningbeams_shutdown(void)
{
	memset(&cl_beams_externaltexture, 0, sizeof(cl_beams_externaltexture));
	memset(&cl_beams_builtintexture, 0, sizeof(cl_beams_builtintexture));
}

static void r_lightningbeams_newmap(void)
{
	if (cl_beams_externaltexture.currentskinframe)
		R_SkinFrame_MarkUsed(cl_beams_externaltexture.currentskinframe);
	if (cl_beams_builtintexture.currentskinframe)
		R_SkinFrame_MarkUsed(cl_beams_builtintexture.currentskinframe);
}

void R_LightningBeams_Init(void)
{
	Cvar_RegisterVariable(&r_lightningbeam_thickness);
	Cvar_RegisterVariable(&r_lightningbeam_scroll);
	Cvar_RegisterVariable(&r_lightningbeam_repeatdistance);
	Cvar_RegisterVariable(&r_lightningbeam_color_red);
	Cvar_RegisterVariable(&r_lightningbeam_color_green);
	Cvar_RegisterVariable(&r_lightningbeam_color_blue);
	Cvar_RegisterVariable(&r_lightningbeam_qmbtexture);
	R_RegisterModule("R_LightningBeams", r_lightningbeams_start, r_lightningbeams_shutdown, r_lightningbeams_newmap, NULL, NULL);
}

static void CL_Beam_AddQuad(model_t *mod, msurface_t *surf, const vec3_t start, const vec3_t end, const vec3_t offset, float t1, float t2)
{
	int e0, e1, e2, e3;
	vec3_t n;
	vec3_t dir;
	float c[4];

	Vector4Set(c, r_lightningbeam_color_red.value, r_lightningbeam_color_green.value, r_lightningbeam_color_blue.value, 1.0f);

	VectorSubtract(end, start, dir);
	CrossProduct(dir, offset, n);
	VectorNormalize(n);

	e0 = Mod_Mesh_IndexForVertex(mod, surf, start[0] + offset[0], start[1] + offset[1], start[2] + offset[2], n[0], n[1], n[2], t1, 0, 0, 0, c[0], c[1], c[2], c[3]);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, start[0] - offset[0], start[1] - offset[1], start[2] - offset[2], n[0], n[1], n[2], t1, 1, 0, 0, c[0], c[1], c[2], c[3]);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, end[0] - offset[0], end[1] - offset[1], end[2] - offset[2], n[0], n[1], n[2], t2, 1, 0, 0, c[0], c[1], c[2], c[3]);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, end[0] + offset[0], end[1] + offset[1], end[2] + offset[2], n[0], n[1], n[2], t2, 0, 0, 0, c[0], c[1], c[2], c[3]);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void CL_Beam_AddPolygons(const beam_t *b)
{
	vec3_t beamdir, right, up, offset, start, end;
	vec_t beamscroll = r_refdef.scene.time * -r_lightningbeam_scroll.value;
	vec_t beamrepeatscale = 1.0f / r_lightningbeam_repeatdistance.value;
	float length, t1, t2;
	model_t *mod;
	msurface_t *surf;

	if (r_lightningbeam_qmbtexture.integer && cl_beams_externaltexture.currentskinframe == NULL)
		CL_Beams_SetupExternalTexture();
	if (!r_lightningbeam_qmbtexture.integer && cl_beams_builtintexture.currentskinframe == NULL)
		CL_Beams_SetupBuiltinTexture();

	// calculate beam direction (beamdir) vector and beam length
	// get difference vector
	CL_Beam_CalculatePositions(b, start, end);
	VectorSubtract(end, start, beamdir);
	// find length of difference vector
	length = sqrt(DotProduct(beamdir, beamdir));
	// calculate scale to make beamdir a unit vector (normalized)
	t1 = 1.0f / length;
	// scale beamdir so it is now normalized
	VectorScale(beamdir, t1, beamdir);

	// calculate up vector such that it points toward viewer, and rotates around the beamdir
	// get direction from start of beam to viewer
	VectorSubtract(r_refdef.view.origin, start, up);
	// remove the portion of the vector that moves along the beam
	// (this leaves only a vector pointing directly away from the beam)
	t1 = -DotProduct(up, beamdir);
	VectorMA(up, t1, beamdir, up);
	// generate right vector from forward and up, the result is unnormalized
	CrossProduct(beamdir, up, right);
	// now normalize the right vector and up vector
	VectorNormalize(right);
	VectorNormalize(up);

	// calculate T coordinate scrolling (start and end texcoord along the beam)
	t1 = beamscroll;
	t1 = t1 - (int)t1;
	t2 = t1 + beamrepeatscale * length;

	// the beam is 3 polygons in this configuration:
	//  *   2
	//   * *
	// 1*****
	//   * *
	//  *   3
	// they are showing different portions of the beam texture, creating an
	// illusion of a beam that appears to curl around in 3D space
	// (and realize that the whole polygon assembly orients itself to face
	//  the viewer)

	mod = CL_Mesh_Scene();
	surf = Mod_Mesh_AddSurface(mod, r_lightningbeam_qmbtexture.integer ? &cl_beams_externaltexture : &cl_beams_builtintexture, false);
	// polygon 1
	VectorM(r_lightningbeam_thickness.value, right, offset);
	CL_Beam_AddQuad(mod, surf, start, end, offset, t1, t2);
	// polygon 2
	VectorMAM(r_lightningbeam_thickness.value * 0.70710681f, right, r_lightningbeam_thickness.value * 0.70710681f, up, offset);
	CL_Beam_AddQuad(mod, surf, start, end, offset, t1 + 0.33f, t2 + 0.33f);
	// polygon 3
	VectorMAM(r_lightningbeam_thickness.value * 0.70710681f, right, r_lightningbeam_thickness.value * -0.70710681f, up, offset);
	CL_Beam_AddQuad(mod, surf, start, end, offset, t1 + 0.66f, t2 + 0.66f);
}
