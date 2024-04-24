
#include "quakedef.h"
#include "cl_collision.h"

#define MAX_RENDERTARGETS 4

cvar_t gl_debug = {CF_CLIENT, "gl_debug", "0", "enables OpenGL debug output, 0 = off, 1 = HIGH severity only, 2 = also MEDIUM severity, 3 = also LOW severity messages.  (note: enabling may not take effect until vid_restart on some drivers)"};
cvar_t gl_paranoid = {CF_CLIENT, "gl_paranoid", "0", "enables OpenGL error checking and other tests"};
cvar_t gl_printcheckerror = {CF_CLIENT, "gl_printcheckerror", "0", "prints all OpenGL error checks, useful to identify location of driver crashes"};

cvar_t r_render = {CF_CLIENT, "r_render", "1", "enables rendering 3D views (you want this on!)"};
cvar_t r_renderview = {CF_CLIENT, "r_renderview", "1", "enables rendering 3D views (you want this on!)"};
cvar_t r_waterwarp = {CF_CLIENT | CF_ARCHIVE, "r_waterwarp", "1", "warp view while underwater"};
cvar_t gl_polyblend = {CF_CLIENT | CF_ARCHIVE, "gl_polyblend", "1", "tints view while underwater, hurt, etc"};

cvar_t v_flipped = {CF_CLIENT, "v_flipped", "0", "mirror the screen (poor man's left handed mode)"};
qbool v_flipped_state = false;

r_viewport_t gl_viewport;
matrix4x4_t gl_modelmatrix;
matrix4x4_t gl_viewmatrix;
matrix4x4_t gl_modelviewmatrix;
matrix4x4_t gl_projectionmatrix;
matrix4x4_t gl_modelviewprojectionmatrix;
float gl_modelview16f[16];
float gl_modelviewprojection16f[16];
qbool gl_modelmatrixchanged;

#ifdef DEBUGGL
void GL_PrintError(GLenum errornumber, const char *filename, unsigned int linenumber)
{
	switch(errornumber)
	{
#ifdef GL_INVALID_ENUM
	case GL_INVALID_ENUM:
		Con_Printf("GL_INVALID_ENUM at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_INVALID_VALUE
	case GL_INVALID_VALUE:
		Con_Printf("GL_INVALID_VALUE at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_INVALID_OPERATION
	case GL_INVALID_OPERATION:
		Con_Printf("GL_INVALID_OPERATION at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_STACK_OVERFLOW
	case GL_STACK_OVERFLOW:
		Con_Printf("GL_STACK_OVERFLOW at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_STACK_UNDERFLOW
	case GL_STACK_UNDERFLOW:
		Con_Printf("GL_STACK_UNDERFLOW at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_OUT_OF_MEMORY
	case GL_OUT_OF_MEMORY:
		Con_Printf("GL_OUT_OF_MEMORY at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_TABLE_TOO_LARGE
	case GL_TABLE_TOO_LARGE:
		Con_Printf("GL_TABLE_TOO_LARGE at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		Con_Printf("GL_INVALID_FRAMEBUFFER_OPERATION at %s:%i\n", filename, linenumber);
		break;
#endif
	default:
		Con_Printf("GL UNKNOWN (%i) at %s:%i\n", errornumber, filename, linenumber);
		break;
	}
}

static void GLAPIENTRY GL_DebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam)
{
	const char *sev = "ENUM?", *typ = "ENUM?", *src = "ENUM?";
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW_ARB: sev = "LOW"; break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB: sev = "MED"; break;
	case GL_DEBUG_SEVERITY_HIGH_ARB: sev = "HIGH"; break;
	}
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR_ARB: typ = "ERROR"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: typ = "DEPRECATED"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: typ = "UNDEFINED"; break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB: typ = "PORTABILITY"; break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB: typ = "PERFORMANCE"; break;
	case GL_DEBUG_TYPE_OTHER_ARB: typ = "OTHER"; break;
	}
	switch (source)
	{
	case GL_DEBUG_SOURCE_API_ARB: src = "API"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: src = "SHADER"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: src = "WIN"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: src = "THIRDPARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB: src = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER_ARB: src = "OTHER"; break;
	}
	Con_Printf("GLDEBUG: %s %s %s: %u: %s\n", sev, typ, src, (unsigned int)id, message);
}
#endif

#define BACKENDACTIVECHECK if (!gl_state.active) Sys_Error("GL backend function called when backend is not active");

void SCR_ScreenShot_f(cmd_state_t *cmd);

typedef struct gltextureunit_s
{
	int pointer_texcoord_components;
	int pointer_texcoord_gltype;
	size_t pointer_texcoord_stride;
	const void *pointer_texcoord_pointer;
	const r_meshbuffer_t *pointer_texcoord_vertexbuffer;
	size_t pointer_texcoord_offset;

	rtexture_t *texture;
	int t2d, t3d, tcubemap;
	int arrayenabled;
}
gltextureunit_t;

typedef struct gl_state_s
{
	int cullface;
	int cullfaceenable;
	int blendfunc1;
	int blendfunc2;
	qbool blend;
	GLboolean depthmask;
	int colormask; // stored as bottom 4 bits: r g b a (3 2 1 0 order)
	int depthtest;
	int depthfunc;
	float depthrange[2];
	float polygonoffset[2];
	qbool alphatocoverage;
	int scissortest;
	unsigned int unit;
	gltextureunit_t units[MAX_TEXTUREUNITS];
	float color4f[4];
	int lockrange_first;
	int lockrange_count;
	int vertexbufferobject;
	int elementbufferobject;
	int uniformbufferobject;
	int framebufferobject;
	int defaultframebufferobject; // deal with platforms that use a non-zero default fbo
	qbool pointer_color_enabled;

	// GL3.2 Core requires that we have a GL_VERTEX_ARRAY_OBJECT, but... just one.
	unsigned int defaultvao;

	int pointer_vertex_components;
	int pointer_vertex_gltype;
	size_t pointer_vertex_stride;
	const void *pointer_vertex_pointer;
	const r_meshbuffer_t *pointer_vertex_vertexbuffer;
	size_t pointer_vertex_offset;

	int pointer_color_components;
	int pointer_color_gltype;
	size_t pointer_color_stride;
	const void *pointer_color_pointer;
	const r_meshbuffer_t *pointer_color_vertexbuffer;
	size_t pointer_color_offset;

	void *preparevertices_tempdata;
	size_t preparevertices_tempdatamaxsize;
	int preparevertices_numvertices;

	memexpandablearray_t meshbufferarray;

	qbool active;
}
gl_state_t;

static gl_state_t gl_state;


/*
note: here's strip order for a terrain row:
0--1--2--3--4
|\ |\ |\ |\ |
| \| \| \| \|
A--B--C--D--E
clockwise

A0B, 01B, B1C, 12C, C2D, 23D, D3E, 34E

*elements++ = i + row;
*elements++ = i;
*elements++ = i + row + 1;
*elements++ = i;
*elements++ = i + 1;
*elements++ = i + row + 1;


for (y = 0;y < rows - 1;y++)
{
	for (x = 0;x < columns - 1;x++)
	{
		i = y * rows + x;
		*elements++ = i + columns;
		*elements++ = i;
		*elements++ = i + columns + 1;
		*elements++ = i;
		*elements++ = i + 1;
		*elements++ = i + columns + 1;
	}
}

alternative:
0--1--2--3--4
| /| /|\ | /|
|/ |/ | \|/ |
A--B--C--D--E
counterclockwise

for (y = 0;y < rows - 1;y++)
{
	for (x = 0;x < columns - 1;x++)
	{
		i = y * rows + x;
		*elements++ = i;
		*elements++ = i + columns;
		*elements++ = i + columns + 1;
		*elements++ = i + columns;
		*elements++ = i + columns + 1;
		*elements++ = i + 1;
	}
}
*/

int polygonelement3i[(POLYGONELEMENTS_MAXPOINTS-2)*3];
unsigned short polygonelement3s[(POLYGONELEMENTS_MAXPOINTS-2)*3];
int quadelement3i[QUADELEMENTS_MAXQUADS*6];
unsigned short quadelement3s[QUADELEMENTS_MAXQUADS*6];

static void GL_VBOStats_f(cmd_state_t *cmd)
{
	GL_Mesh_ListVBOs(true);
}

static void GL_Backend_ResetState(void);

static void gl_backend_start(void)
{
	memset(&gl_state, 0, sizeof(gl_state));

	Mem_ExpandableArray_NewArray(&gl_state.meshbufferarray, r_main_mempool, sizeof(r_meshbuffer_t), 128);

	Con_Printf("OpenGL backend started\n");

	CHECKGLERROR

	switch(vid.renderpath)
	{
#ifndef USE_GLES2
	case RENDERPATH_GL32:
		// GL3.2 Core requires that we have a VAO bound - but using more than one has no performance benefit so this is just placeholder
		qglGenVertexArrays(1, &gl_state.defaultvao);
		qglBindVertexArray(gl_state.defaultvao);
		// fall through
#endif //USE_GLES2
	case RENDERPATH_GLES2:
		// fetch current fbo here (default fbo is not 0 on some GLES devices)
		CHECKGLERROR
		qglGetIntegerv(GL_FRAMEBUFFER_BINDING, &gl_state.defaultframebufferobject);CHECKGLERROR
		break;
	}

	GL_Backend_ResetState();
}

static void gl_backend_shutdown(void)
{
	Con_Print("OpenGL backend shutting down\n");

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		break;
	}

	if (gl_state.preparevertices_tempdata)
		Mem_Free(gl_state.preparevertices_tempdata);

	Mem_ExpandableArray_FreeArray(&gl_state.meshbufferarray);

	memset(&gl_state, 0, sizeof(gl_state));
}

static void gl_backend_newmap(void)
{
}

static void gl_backend_devicelost(void)
{
	int i, endindex;
	r_meshbuffer_t *buffer;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		break;
	}
	endindex = (int)Mem_ExpandableArray_IndexRange(&gl_state.meshbufferarray);
	for (i = 0;i < endindex;i++)
	{
		buffer = (r_meshbuffer_t *) Mem_ExpandableArray_RecordAtIndex(&gl_state.meshbufferarray, i);
		if (!buffer || !buffer->isdynamic)
			continue;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			break;
		}
	}
}

static void gl_backend_devicerestored(void)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		break;
	}
}

void gl_backend_init(void)
{
	int i;

	for (i = 0;i < POLYGONELEMENTS_MAXPOINTS - 2;i++)
	{
		polygonelement3s[i * 3 + 0] = 0;
		polygonelement3s[i * 3 + 1] = i + 1;
		polygonelement3s[i * 3 + 2] = i + 2;
	}
	// elements for rendering a series of quads as triangles
	for (i = 0;i < QUADELEMENTS_MAXQUADS;i++)
	{
		quadelement3s[i * 6 + 0] = i * 4;
		quadelement3s[i * 6 + 1] = i * 4 + 1;
		quadelement3s[i * 6 + 2] = i * 4 + 2;
		quadelement3s[i * 6 + 3] = i * 4;
		quadelement3s[i * 6 + 4] = i * 4 + 2;
		quadelement3s[i * 6 + 5] = i * 4 + 3;
	}

	for (i = 0;i < (POLYGONELEMENTS_MAXPOINTS - 2)*3;i++)
		polygonelement3i[i] = polygonelement3s[i];
	for (i = 0;i < QUADELEMENTS_MAXQUADS*6;i++)
		quadelement3i[i] = quadelement3s[i];

	Cvar_RegisterVariable(&r_render);
	Cvar_RegisterVariable(&r_renderview);
	Cvar_RegisterVariable(&r_waterwarp);
	Cvar_RegisterVariable(&gl_polyblend);
	Cvar_RegisterVariable(&v_flipped);
	Cvar_RegisterVariable(&gl_debug);
	Cvar_RegisterVariable(&gl_paranoid);
	Cvar_RegisterVariable(&gl_printcheckerror);

	Cmd_AddCommand(CF_CLIENT, "gl_vbostats", GL_VBOStats_f, "prints a list of all buffer objects (vertex data and triangle elements) and total video memory used by them");

	R_RegisterModule("GL_Backend", gl_backend_start, gl_backend_shutdown, gl_backend_newmap, gl_backend_devicelost, gl_backend_devicerestored);
}

void GL_SetMirrorState(qbool state);

void R_Viewport_TransformToScreen(const r_viewport_t *v, const vec4_t in, vec4_t out)
{
	vec4_t temp;
	float iw;
	Matrix4x4_Transform4 (&v->viewmatrix, in, temp);
	Matrix4x4_Transform4 (&v->projectmatrix, temp, out);
	iw = 1.0f / out[3];
	out[0] = v->x + (out[0] * iw + 1.0f) * v->width * 0.5f;

	// for an odd reason, inverting this is wrong for R_Shadow_ScissorForBBox (we then get badly scissored lights)
	//out[1] = v->y + v->height - (out[1] * iw + 1.0f) * v->height * 0.5f;
	out[1] = v->y + (out[1] * iw + 1.0f) * v->height * 0.5f;

	out[2] = v->z + (out[2] * iw + 1.0f) * v->depth * 0.5f;
}

void GL_Finish(void)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		qglFinish();CHECKGLERROR
		break;
	}
}

static int bboxedges[12][2] =
{
	// top
	{0, 1}, // +X
	{0, 2}, // +Y
	{1, 3}, // Y, +X
	{2, 3}, // X, +Y
	// bottom
	{4, 5}, // +X
	{4, 6}, // +Y
	{5, 7}, // Y, +X
	{6, 7}, // X, +Y
	// verticals
	{0, 4}, // +Z
	{1, 5}, // X, +Z
	{2, 6}, // Y, +Z
	{3, 7}, // XY, +Z
};

qbool R_ScissorForBBox(const float *mins, const float *maxs, int *scissor)
{
	int i, ix1, iy1, ix2, iy2;
	float x1, y1, x2, y2;
	vec4_t v, v2;
	float vertex[20][3];
	int j, k;
	vec4_t plane4f;
	int numvertices;
	float corner[8][4];
	float dist[8];
	int sign[8];
	float f;

	scissor[0] = r_refdef.view.viewport.x;
	scissor[1] = r_refdef.view.viewport.y;
	scissor[2] = r_refdef.view.viewport.width;
	scissor[3] = r_refdef.view.viewport.height;

	// if view is inside the box, just say yes it's visible
	if (BoxesOverlap(r_refdef.view.origin, r_refdef.view.origin, mins, maxs))
		return false;

	// transform all corners that are infront of the nearclip plane
	VectorNegate(r_refdef.view.frustum[4].normal, plane4f);
	plane4f[3] = r_refdef.view.frustum[4].dist;
	numvertices = 0;
	for (i = 0;i < 8;i++)
	{
		Vector4Set(corner[i], (i & 1) ? maxs[0] : mins[0], (i & 2) ? maxs[1] : mins[1], (i & 4) ? maxs[2] : mins[2], 1);
		dist[i] = DotProduct4(corner[i], plane4f);
		sign[i] = dist[i] > 0;
		if (!sign[i])
		{
			VectorCopy(corner[i], vertex[numvertices]);
			numvertices++;
		}
	}
	// if some points are behind the nearclip, add clipped edge points to make
	// sure that the scissor boundary is complete
	if (numvertices > 0 && numvertices < 8)
	{
		// add clipped edge points
		for (i = 0;i < 12;i++)
		{
			j = bboxedges[i][0];
			k = bboxedges[i][1];
			if (sign[j] != sign[k])
			{
				f = dist[j] / (dist[j] - dist[k]);
				VectorLerp(corner[j], f, corner[k], vertex[numvertices]);
				numvertices++;
			}
		}
	}

	// if we have no points to check, it is behind the view plane
	if (!numvertices)
		return true;

	// if we have some points to transform, check what screen area is covered
	x1 = y1 = x2 = y2 = 0;
	v[3] = 1.0f;
	//Con_Printf("%i vertices to transform...\n", numvertices);
	for (i = 0;i < numvertices;i++)
	{
		VectorCopy(vertex[i], v);
		R_Viewport_TransformToScreen(&r_refdef.view.viewport, v, v2);
		//Con_Printf("%.3f %.3f %.3f %.3f transformed to %.3f %.3f %.3f %.3f\n", v[0], v[1], v[2], v[3], v2[0], v2[1], v2[2], v2[3]);
		if (i)
		{
			if (x1 > v2[0]) x1 = v2[0];
			if (x2 < v2[0]) x2 = v2[0];
			if (y1 > v2[1]) y1 = v2[1];
			if (y2 < v2[1]) y2 = v2[1];
		}
		else
		{
			x1 = x2 = v2[0];
			y1 = y2 = v2[1];
		}
	}

	// now convert the scissor rectangle to integer screen coordinates
	ix1 = (int)(x1 - 1.0f);
	//iy1 = vid.height - (int)(y2 - 1.0f);
	//iy1 = r_refdef.view.viewport.width + 2 * r_refdef.view.viewport.x - (int)(y2 - 1.0f);
	iy1 = (int)(y1 - 1.0f);
	ix2 = (int)(x2 + 1.0f);
	//iy2 = vid.height - (int)(y1 + 1.0f);
	//iy2 = r_refdef.view.viewport.height + 2 * r_refdef.view.viewport.y - (int)(y1 + 1.0f);
	iy2 = (int)(y2 + 1.0f);
	//Con_Printf("%f %f %f %f\n", x1, y1, x2, y2);

	// clamp it to the screen
	if (ix1 < r_refdef.view.viewport.x) ix1 = r_refdef.view.viewport.x;
	if (iy1 < r_refdef.view.viewport.y) iy1 = r_refdef.view.viewport.y;
	if (ix2 > r_refdef.view.viewport.x + r_refdef.view.viewport.width) ix2 = r_refdef.view.viewport.x + r_refdef.view.viewport.width;
	if (iy2 > r_refdef.view.viewport.y + r_refdef.view.viewport.height) iy2 = r_refdef.view.viewport.y + r_refdef.view.viewport.height;

	// if it is inside out, it's not visible
	if (ix2 <= ix1 || iy2 <= iy1)
		return true;

	// the light area is visible, set up the scissor rectangle
	scissor[0] = ix1;
	scissor[1] = iy1;
	scissor[2] = ix2 - ix1;
	scissor[3] = iy2 - iy1;

	// D3D Y coordinate is top to bottom, OpenGL is bottom to top, fix the D3D one
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		break;
	}

	return false;
}


static void R_Viewport_ApplyNearClipPlaneFloatGL(const r_viewport_t *v, float *m, float normalx, float normaly, float normalz, float dist)
{
	float q[4];
	float d;
	float clipPlane[4], v3[3], v4[3];
	float normal[3];

	// This is inspired by Oblique Depth Projection from http://www.terathon.com/code/oblique.php

	VectorSet(normal, normalx, normaly, normalz);
	Matrix4x4_Transform3x3(&v->viewmatrix, normal, clipPlane);
	VectorScale(normal, -dist, v3);
	Matrix4x4_Transform(&v->viewmatrix, v3, v4);
	// FIXME: LadyHavoc: I think this can be done more efficiently somehow but I can't remember the technique
	clipPlane[3] = -DotProduct(v4, clipPlane);

	// Calculate the clip-space corner point opposite the clipping plane
	// as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
	// transform it into camera space by multiplying it
	// by the inverse of the projection matrix
	q[0] = ((clipPlane[0] < 0.0f ? -1.0f : clipPlane[0] > 0.0f ? 1.0f : 0.0f) + m[8]) / m[0];
	q[1] = ((clipPlane[1] < 0.0f ? -1.0f : clipPlane[1] > 0.0f ? 1.0f : 0.0f) + m[9]) / m[5];
	q[2] = -1.0f;
	q[3] = (1.0f + m[10]) / m[14];

	// Calculate the scaled plane vector
	d = 2.0f / DotProduct4(clipPlane, q);

	// Replace the third row of the projection matrix
	m[2] = clipPlane[0] * d;
	m[6] = clipPlane[1] * d;
	m[10] = clipPlane[2] * d + 1.0f;
	m[14] = clipPlane[3] * d;
}

void R_Viewport_InitOrtho(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float x1, float y1, float x2, float y2, float nearclip, float farclip, const float *nearplane)
{
	float left = x1, right = x2, bottom = y2, top = y1, zNear = nearclip, zFar = farclip;
	float m[16];
	memset(v, 0, sizeof(*v));
	v->type = R_VIEWPORTTYPE_ORTHO;
	v->cameramatrix = *cameramatrix;
	v->x = x;
	v->y = y;
	v->z = 0;
	v->width = width;
	v->height = height;
	v->depth = 1;
	memset(m, 0, sizeof(m));
	m[0]  = 2/(right - left);
	m[5]  = 2/(top - bottom);
	m[10] = -2/(zFar - zNear);
	m[12] = - (right + left)/(right - left);
	m[13] = - (top + bottom)/(top - bottom);
	m[14] = - (zFar + zNear)/(zFar - zNear);
	m[15] = 1;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		break;
	}
	v->screentodepth[0] = -farclip / (farclip - nearclip);
	v->screentodepth[1] = farclip * nearclip / (farclip - nearclip);

	Matrix4x4_Invert_Full(&v->viewmatrix, &v->cameramatrix);

	if (nearplane)
		R_Viewport_ApplyNearClipPlaneFloatGL(v, m, nearplane[0], nearplane[1], nearplane[2], nearplane[3]);

	Matrix4x4_FromArrayFloatGL(&v->projectmatrix, m);

#if 0
	{
		vec4_t test1;
		vec4_t test2;
		Vector4Set(test1, (x1+x2)*0.5f, (y1+y2)*0.5f, 0.0f, 1.0f);
		R_Viewport_TransformToScreen(v, test1, test2);
		Con_Printf("%f %f %f -> %f %f %f\n", test1[0], test1[1], test1[2], test2[0], test2[1], test2[2]);
	}
#endif
}

void R_Viewport_InitOrtho3D(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float frustumx, float frustumy, float nearclip, float farclip, const float *nearplane)
{
	matrix4x4_t tempmatrix, basematrix;
	float m[16];
	memset(v, 0, sizeof(*v));

	v->type = R_VIEWPORTTYPE_PERSPECTIVE;
	v->cameramatrix = *cameramatrix;
	v->x = x;
	v->y = y;
	v->z = 0;
	v->width = width;
	v->height = height;
	v->depth = 1;
	memset(m, 0, sizeof(m));
	m[0]  = 1.0 / frustumx;
	m[5]  = 1.0 / frustumy;
	m[10] = -2 / (farclip - nearclip);
	m[14] = -(farclip + nearclip) / (farclip - nearclip);
	m[15] = 1;
	v->screentodepth[0] = -farclip / (farclip - nearclip);
	v->screentodepth[1] = farclip * nearclip / (farclip - nearclip);

	Matrix4x4_Invert_Full(&tempmatrix, &v->cameramatrix);
	Matrix4x4_CreateRotate(&basematrix, -90, 1, 0, 0);
	Matrix4x4_ConcatRotate(&basematrix, 90, 0, 0, 1);
	Matrix4x4_Concat(&v->viewmatrix, &basematrix, &tempmatrix);

	if (nearplane)
		R_Viewport_ApplyNearClipPlaneFloatGL(v, m, nearplane[0], nearplane[1], nearplane[2], nearplane[3]);

	if(v_flipped.integer)
	{
		m[0] = -m[0];
		m[4] = -m[4];
		m[8] = -m[8];
		m[12] = -m[12];
	}

	Matrix4x4_FromArrayFloatGL(&v->projectmatrix, m);
}

void R_Viewport_InitPerspective(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float frustumx, float frustumy, float nearclip, float farclip, const float *nearplane)
{
	matrix4x4_t tempmatrix, basematrix;
	float m[16];
	memset(v, 0, sizeof(*v));

	v->type = R_VIEWPORTTYPE_PERSPECTIVE;
	v->cameramatrix = *cameramatrix;
	v->x = x;
	v->y = y;
	v->z = 0;
	v->width = width;
	v->height = height;
	v->depth = 1;
	memset(m, 0, sizeof(m));
	m[0]  = 1.0 / frustumx;
	m[5]  = 1.0 / frustumy;
	m[10] = -(farclip + nearclip) / (farclip - nearclip);
	m[11] = -1;
	m[14] = -2 * nearclip * farclip / (farclip - nearclip);
	v->screentodepth[0] = -farclip / (farclip - nearclip);
	v->screentodepth[1] = farclip * nearclip / (farclip - nearclip);

	Matrix4x4_Invert_Full(&tempmatrix, &v->cameramatrix);
	Matrix4x4_CreateRotate(&basematrix, -90, 1, 0, 0);
	Matrix4x4_ConcatRotate(&basematrix, 90, 0, 0, 1);
	Matrix4x4_Concat(&v->viewmatrix, &basematrix, &tempmatrix);

	if (nearplane)
		R_Viewport_ApplyNearClipPlaneFloatGL(v, m, nearplane[0], nearplane[1], nearplane[2], nearplane[3]);

	if(v_flipped.integer)
	{
		m[0] = -m[0];
		m[4] = -m[4];
		m[8] = -m[8];
		m[12] = -m[12];
	}

	Matrix4x4_FromArrayFloatGL(&v->projectmatrix, m);
}

void R_Viewport_InitPerspectiveInfinite(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float frustumx, float frustumy, float nearclip, const float *nearplane)
{
	matrix4x4_t tempmatrix, basematrix;
	const float nudge = 1.0 - 1.0 / (1<<23);
	float m[16];
	memset(v, 0, sizeof(*v));

	v->type = R_VIEWPORTTYPE_PERSPECTIVE_INFINITEFARCLIP;
	v->cameramatrix = *cameramatrix;
	v->x = x;
	v->y = y;
	v->z = 0;
	v->width = width;
	v->height = height;
	v->depth = 1;
	memset(m, 0, sizeof(m));
	m[ 0] = 1.0 / frustumx;
	m[ 5] = 1.0 / frustumy;
	m[10] = -nudge;
	m[11] = -1;
	m[14] = -2 * nearclip * nudge;
	v->screentodepth[0] = (m[10] + 1) * 0.5 - 1;
	v->screentodepth[1] = m[14] * -0.5;

	Matrix4x4_Invert_Full(&tempmatrix, &v->cameramatrix);
	Matrix4x4_CreateRotate(&basematrix, -90, 1, 0, 0);
	Matrix4x4_ConcatRotate(&basematrix, 90, 0, 0, 1);
	Matrix4x4_Concat(&v->viewmatrix, &basematrix, &tempmatrix);

	if (nearplane)
		R_Viewport_ApplyNearClipPlaneFloatGL(v, m, nearplane[0], nearplane[1], nearplane[2], nearplane[3]);

	if(v_flipped.integer)
	{
		m[0] = -m[0];
		m[4] = -m[4];
		m[8] = -m[8];
		m[12] = -m[12];
	}

	Matrix4x4_FromArrayFloatGL(&v->projectmatrix, m);
}

float cubeviewmatrix[6][16] =
{
    // standard cubemap projections
    { // +X
         0, 0,-1, 0,
         0,-1, 0, 0,
        -1, 0, 0, 0,
         0, 0, 0, 1,
    },
    { // -X
         0, 0, 1, 0,
         0,-1, 0, 0,
         1, 0, 0, 0,
         0, 0, 0, 1,
    },
    { // +Y
         1, 0, 0, 0,
         0, 0,-1, 0,
         0, 1, 0, 0,
         0, 0, 0, 1,
    },
    { // -Y
         1, 0, 0, 0,
         0, 0, 1, 0,
         0,-1, 0, 0,
         0, 0, 0, 1,
    },
    { // +Z
         1, 0, 0, 0,
         0,-1, 0, 0,
         0, 0,-1, 0,
         0, 0, 0, 1,
    },
    { // -Z
        -1, 0, 0, 0,
         0,-1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0, 1,
    },
};
float rectviewmatrix[6][16] =
{
    // sign-preserving cubemap projections
    { // +X
         0, 0,-1, 0,
         0, 1, 0, 0,
         1, 0, 0, 0,
         0, 0, 0, 1,
    },
    { // -X
         0, 0, 1, 0,
         0, 1, 0, 0,
         1, 0, 0, 0,
         0, 0, 0, 1,
    },
    { // +Y
         1, 0, 0, 0,
         0, 0,-1, 0,
         0, 1, 0, 0,
         0, 0, 0, 1,
    },
    { // -Y
         1, 0, 0, 0,
         0, 0, 1, 0,
         0, 1, 0, 0,
         0, 0, 0, 1,
    },
    { // +Z
         1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0,-1, 0,
         0, 0, 0, 1,
    },
    { // -Z
         1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0, 1,
    },
};

void R_Viewport_InitCubeSideView(r_viewport_t *v, const matrix4x4_t *cameramatrix, int side, int size, float nearclip, float farclip, const float *nearplane)
{
	matrix4x4_t tempmatrix, basematrix;
	float m[16];
	memset(v, 0, sizeof(*v));
	v->type = R_VIEWPORTTYPE_PERSPECTIVECUBESIDE;
	v->cameramatrix = *cameramatrix;
	v->width = size;
	v->height = size;
	v->depth = 1;
	memset(m, 0, sizeof(m));
	m[0] = m[5] = 1.0f;
	m[10] = -(farclip + nearclip) / (farclip - nearclip);
	m[11] = -1;
	m[14] = -2 * nearclip * farclip / (farclip - nearclip);

	Matrix4x4_FromArrayFloatGL(&basematrix, cubeviewmatrix[side]);
	Matrix4x4_Invert_Full(&tempmatrix, &v->cameramatrix);
	Matrix4x4_Concat(&v->viewmatrix, &basematrix, &tempmatrix);

	if (nearplane)
		R_Viewport_ApplyNearClipPlaneFloatGL(v, m, nearplane[0], nearplane[1], nearplane[2], nearplane[3]);

	Matrix4x4_FromArrayFloatGL(&v->projectmatrix, m);
}

void R_Viewport_InitRectSideView(r_viewport_t *v, const matrix4x4_t *cameramatrix, int side, int size, int border, float nearclip, float farclip, const float *nearplane, int offsetx, int offsety)
{
	matrix4x4_t tempmatrix, basematrix;
	float m[16];
	memset(v, 0, sizeof(*v));
	v->type = R_VIEWPORTTYPE_PERSPECTIVECUBESIDE;
	v->cameramatrix = *cameramatrix;
	v->x = offsetx + (side & 1) * size;
	v->y = offsety + (side >> 1) * size;
	v->width = size;
	v->height = size;
	v->depth = 1;
	memset(m, 0, sizeof(m));
	m[0] = m[5] = 1.0f * ((float)size - border) / size;
	m[10] = -(farclip + nearclip) / (farclip - nearclip);
	m[11] = -1;
	m[14] = -2 * nearclip * farclip / (farclip - nearclip);

	Matrix4x4_FromArrayFloatGL(&basematrix, rectviewmatrix[side]);
	Matrix4x4_Invert_Full(&tempmatrix, &v->cameramatrix);
	Matrix4x4_Concat(&v->viewmatrix, &basematrix, &tempmatrix);

	if (nearplane)
		R_Viewport_ApplyNearClipPlaneFloatGL(v, m, nearplane[0], nearplane[1], nearplane[2], nearplane[3]);

	Matrix4x4_FromArrayFloatGL(&v->projectmatrix, m);
}

void R_SetViewport(const r_viewport_t *v)
{
	gl_viewport = *v;

	// FIXME: v_flipped_state is evil, this probably breaks somewhere
	GL_SetMirrorState(v_flipped.integer && (v->type == R_VIEWPORTTYPE_PERSPECTIVE || v->type == R_VIEWPORTTYPE_PERSPECTIVE_INFINITEFARCLIP));

	// copy over the matrices to our state
	gl_viewmatrix = v->viewmatrix;
	gl_projectionmatrix = v->projectmatrix;

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		qglViewport(v->x, v->y, v->width, v->height);CHECKGLERROR
		break;
	}

	// force an update of the derived matrices
	gl_modelmatrixchanged = true;
	R_EntityMatrix(&gl_modelmatrix);
}

void R_GetViewport(r_viewport_t *v)
{
	*v = gl_viewport;
}

static void GL_BindVBO(int bufferobject)
{
	if (gl_state.vertexbufferobject != bufferobject)
	{
		gl_state.vertexbufferobject = bufferobject;
		CHECKGLERROR
		qglBindBuffer(GL_ARRAY_BUFFER, bufferobject);CHECKGLERROR
	}
}

static void GL_BindEBO(int bufferobject)
{
	if (gl_state.elementbufferobject != bufferobject)
	{
		gl_state.elementbufferobject = bufferobject;
		CHECKGLERROR
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferobject);CHECKGLERROR
	}
}

static void GL_BindUBO(int bufferobject)
{
	if (gl_state.uniformbufferobject != bufferobject)
	{
		gl_state.uniformbufferobject = bufferobject;
#ifdef GL_UNIFORM_BUFFER
		CHECKGLERROR
		qglBindBuffer(GL_UNIFORM_BUFFER, bufferobject);CHECKGLERROR
#endif
	}
}

static const GLuint drawbuffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
int R_Mesh_CreateFramebufferObject(rtexture_t *depthtexture, rtexture_t *colortexture, rtexture_t *colortexture2, rtexture_t *colortexture3, rtexture_t *colortexture4)
{
	int temp;
	GLuint status;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		qglGenFramebuffers(1, (GLuint*)&temp);CHECKGLERROR
		R_Mesh_SetRenderTargets(temp, NULL, NULL, NULL, NULL, NULL);
		// GL_ARB_framebuffer_object (GL3-class hardware) - depth stencil attachment
#ifdef USE_GLES2
		// FIXME: separate stencil attachment on GLES
		if (depthtexture  && depthtexture->texnum ) { qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , depthtexture->gltexturetypeenum , depthtexture->texnum , 0);CHECKGLERROR }
		if (depthtexture  && depthtexture->renderbuffernum ) { qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , GL_RENDERBUFFER, depthtexture->renderbuffernum );CHECKGLERROR }
#else
		if (depthtexture  && depthtexture->texnum )
		{
			qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , depthtexture->gltexturetypeenum , depthtexture->texnum , 0);CHECKGLERROR
			if (depthtexture->glisdepthstencil) { qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT  , depthtexture->gltexturetypeenum , depthtexture->texnum , 0);CHECKGLERROR }
		}
		if (depthtexture  && depthtexture->renderbuffernum )
		{
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , GL_RENDERBUFFER, depthtexture->renderbuffernum );CHECKGLERROR
			if (depthtexture->glisdepthstencil) { qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT  , GL_RENDERBUFFER, depthtexture->renderbuffernum );CHECKGLERROR }
		}
#endif
		if (colortexture  && colortexture->texnum ) { qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , colortexture->gltexturetypeenum , colortexture->texnum , 0);CHECKGLERROR }
		if (colortexture2 && colortexture2->texnum) { qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , colortexture2->gltexturetypeenum, colortexture2->texnum, 0);CHECKGLERROR }
		if (colortexture3 && colortexture3->texnum) { qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2 , colortexture3->gltexturetypeenum, colortexture3->texnum, 0);CHECKGLERROR }
		if (colortexture4 && colortexture4->texnum) { qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3 , colortexture4->gltexturetypeenum, colortexture4->texnum, 0);CHECKGLERROR }
		if (colortexture  && colortexture->renderbuffernum ) { qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_RENDERBUFFER, colortexture->renderbuffernum );CHECKGLERROR }
		if (colortexture2 && colortexture2->renderbuffernum) { qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_RENDERBUFFER, colortexture2->renderbuffernum);CHECKGLERROR }
		if (colortexture3 && colortexture3->renderbuffernum) { qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2 , GL_RENDERBUFFER, colortexture3->renderbuffernum);CHECKGLERROR }
		if (colortexture4 && colortexture4->renderbuffernum) { qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3 , GL_RENDERBUFFER, colortexture4->renderbuffernum);CHECKGLERROR }

#ifndef USE_GLES2
		if (colortexture4)
		{
			qglDrawBuffers(4, drawbuffers);CHECKGLERROR
			qglReadBuffer(GL_NONE);CHECKGLERROR
		}
		else if (colortexture3)
		{
			qglDrawBuffers(3, drawbuffers);CHECKGLERROR
			qglReadBuffer(GL_NONE);CHECKGLERROR
		}
		else if (colortexture2)
		{
			qglDrawBuffers(2, drawbuffers);CHECKGLERROR
			qglReadBuffer(GL_NONE);CHECKGLERROR
		}
		else if (colortexture)
		{
			qglDrawBuffer(GL_COLOR_ATTACHMENT0);CHECKGLERROR
			qglReadBuffer(GL_COLOR_ATTACHMENT0);CHECKGLERROR
		}
		else
		{
			qglDrawBuffer(GL_NONE);CHECKGLERROR
			qglReadBuffer(GL_NONE);CHECKGLERROR
		}
#endif
		status = qglCheckFramebufferStatus(GL_FRAMEBUFFER);CHECKGLERROR
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			Con_Printf("R_Mesh_CreateFramebufferObject: glCheckFramebufferStatus returned %i\n", status);
			gl_state.framebufferobject = 0; // GL unbinds it for us
			qglDeleteFramebuffers(1, (GLuint*)&temp);CHECKGLERROR
			temp = 0;
		}
		return temp;
	}
	return 0;
}

void R_Mesh_DestroyFramebufferObject(int fbo)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (fbo)
		{
			// GL clears the binding if we delete something bound
			if (gl_state.framebufferobject == fbo)
				gl_state.framebufferobject = 0;
			qglDeleteFramebuffers(1, (GLuint*)&fbo);CHECKGLERROR
		}
		break;
	}
}

void R_Mesh_SetRenderTargets(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture, rtexture_t *colortexture2, rtexture_t *colortexture3, rtexture_t *colortexture4)
{
	unsigned int i;
	unsigned int j;
	rtexture_t *textures[5];
	Vector4Set(textures, colortexture, colortexture2, colortexture3, colortexture4);
	textures[4] = depthtexture;
	// unbind any matching textures immediately, otherwise D3D will complain about a bound texture being used as a render target
	for (j = 0;j < 5;j++)
		if (textures[j])
			for (i = 0;i < MAX_TEXTUREUNITS;i++)
				if (gl_state.units[i].texture == textures[j])
					R_Mesh_TexBind(i, NULL);
	// set up framebuffer object or render targets for the active rendering API
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (gl_state.framebufferobject != fbo)
		{
			gl_state.framebufferobject = fbo;
			qglBindFramebuffer(GL_FRAMEBUFFER, gl_state.framebufferobject ? gl_state.framebufferobject : gl_state.defaultframebufferobject);CHECKGLERROR
		}
		break;
	}
}

static void GL_Backend_ResetState(void)
{
	gl_state.active = true;
	gl_state.depthtest = true;
	gl_state.alphatocoverage = false;
	gl_state.blendfunc1 = GL_ONE;
	gl_state.blendfunc2 = GL_ZERO;
	gl_state.blend = false;
	gl_state.depthmask = GL_TRUE;
	gl_state.colormask = 15;
	gl_state.color4f[0] = gl_state.color4f[1] = gl_state.color4f[2] = gl_state.color4f[3] = 1;
	gl_state.lockrange_first = 0;
	gl_state.lockrange_count = 0;
	gl_state.cullface = GL_FRONT;
	gl_state.cullfaceenable = false;
	gl_state.polygonoffset[0] = 0;
	gl_state.polygonoffset[1] = 0;
	gl_state.framebufferobject = 0;
	gl_state.depthfunc = GL_LEQUAL;

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		// set up debug output early
#ifdef DEBUGGL
		if (vid.support.arb_debug_output)
		{
			GLuint unused = 0;
			CHECKGLERROR
			if (gl_debug.integer >= 1)
				qglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
			if (gl_debug.integer >= 3)
				qglDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, gl_debug.integer >= 3 ? GL_TRUE : GL_FALSE);
			else if (gl_debug.integer >= 1)
			{
				qglDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, &unused, gl_debug.integer >= 3 ? GL_TRUE : GL_FALSE);
				qglDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM_ARB, 0, &unused, gl_debug.integer >= 2 ? GL_TRUE : GL_FALSE);
				qglDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH_ARB, 0, &unused, gl_debug.integer >= 1 ? GL_TRUE : GL_FALSE);
			}
			else
				qglDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, GL_FALSE);
			qglDebugMessageCallbackARB(GL_DebugOutputCallback, NULL);
		}
#endif //DEBUGGL
		CHECKGLERROR
		qglColorMask(1, 1, 1, 1);CHECKGLERROR
		qglBlendFunc(gl_state.blendfunc1, gl_state.blendfunc2);CHECKGLERROR
		qglDisable(GL_BLEND);CHECKGLERROR
		qglCullFace(gl_state.cullface);CHECKGLERROR
		qglDisable(GL_CULL_FACE);CHECKGLERROR
		qglDepthFunc(GL_LEQUAL);CHECKGLERROR
		qglEnable(GL_DEPTH_TEST);CHECKGLERROR
		qglDepthMask(gl_state.depthmask);CHECKGLERROR
		qglPolygonOffset(gl_state.polygonoffset[0], gl_state.polygonoffset[1]);
		qglBindBuffer(GL_ARRAY_BUFFER, 0);
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		qglBindFramebuffer(GL_FRAMEBUFFER, gl_state.defaultframebufferobject);
		qglEnableVertexAttribArray(GLSLATTRIB_POSITION);
		qglDisableVertexAttribArray(GLSLATTRIB_COLOR);
		qglVertexAttrib4f(GLSLATTRIB_COLOR, 1, 1, 1, 1);
		gl_state.unit = MAX_TEXTUREUNITS;
		CHECKGLERROR
		break;
	}
}

void GL_ActiveTexture(unsigned int num)
{
	if (gl_state.unit != num)
	{
		gl_state.unit = num;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			qglActiveTexture(GL_TEXTURE0 + gl_state.unit);CHECKGLERROR
			break;
		}
	}
}

void GL_BlendFunc(int blendfunc1, int blendfunc2)
{
	if (gl_state.blendfunc1 != blendfunc1 || gl_state.blendfunc2 != blendfunc2)
	{
		qbool blendenable;
		gl_state.blendfunc1 = blendfunc1;
		gl_state.blendfunc2 = blendfunc2;
		blendenable = (gl_state.blendfunc1 != GL_ONE || gl_state.blendfunc2 != GL_ZERO);
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			if (qglBlendFuncSeparate)
			{
				qglBlendFuncSeparate(gl_state.blendfunc1, gl_state.blendfunc2, GL_ZERO, GL_ONE);CHECKGLERROR // ELUAN: Adreno 225 (and others) compositing workaround
			}
			else
			{
				qglBlendFunc(gl_state.blendfunc1, gl_state.blendfunc2);CHECKGLERROR
			}
			if (gl_state.blend != blendenable)
			{
				gl_state.blend = blendenable;
				if (!gl_state.blend)
				{
					qglDisable(GL_BLEND);CHECKGLERROR
				}
				else
				{
					qglEnable(GL_BLEND);CHECKGLERROR
				}
			}
			break;
		}
	}
}

void GL_DepthMask(int state)
{
	if (gl_state.depthmask != state)
	{
		gl_state.depthmask = state;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			qglDepthMask(gl_state.depthmask);CHECKGLERROR
			break;
		}
	}
}

void GL_DepthTest(int state)
{
	if (gl_state.depthtest != state)
	{
		gl_state.depthtest = state;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			if (gl_state.depthtest)
			{
				qglEnable(GL_DEPTH_TEST);CHECKGLERROR
			}
			else
			{
				qglDisable(GL_DEPTH_TEST);CHECKGLERROR
			}
			break;
		}
	}
}

void GL_DepthFunc(int state)
{
	if (gl_state.depthfunc != state)
	{
		gl_state.depthfunc = state;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			qglDepthFunc(gl_state.depthfunc);CHECKGLERROR
			break;
		}
	}
}

void GL_DepthRange(float nearfrac, float farfrac)
{
	if (gl_state.depthrange[0] != nearfrac || gl_state.depthrange[1] != farfrac)
	{
		gl_state.depthrange[0] = nearfrac;
		gl_state.depthrange[1] = farfrac;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
#ifdef USE_GLES2
			qglDepthRangef(gl_state.depthrange[0], gl_state.depthrange[1]);CHECKGLERROR
#else
			qglDepthRange(gl_state.depthrange[0], gl_state.depthrange[1]);CHECKGLERROR
#endif
			break;
		}
	}
}

void R_SetStencil(qbool enable, int writemask, int fail, int zfail, int zpass, int compare, int comparereference, int comparemask)
{
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		if (enable)
		{
			qglEnable(GL_STENCIL_TEST);CHECKGLERROR
		}
		else
		{
			qglDisable(GL_STENCIL_TEST);CHECKGLERROR
		}
		qglStencilMask(writemask);CHECKGLERROR
		qglStencilOp(fail, zfail, zpass);CHECKGLERROR
		qglStencilFunc(compare, comparereference, comparemask);CHECKGLERROR
		CHECKGLERROR
		break;
	}
}

void GL_PolygonOffset(float planeoffset, float depthoffset)
{
	if (gl_state.polygonoffset[0] != planeoffset || gl_state.polygonoffset[1] != depthoffset)
	{
		gl_state.polygonoffset[0] = planeoffset;
		gl_state.polygonoffset[1] = depthoffset;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			qglPolygonOffset(gl_state.polygonoffset[0], gl_state.polygonoffset[1]);CHECKGLERROR
			break;
		}
	}
}

void GL_SetMirrorState(qbool state)
{
	if (v_flipped_state != state)
	{
		v_flipped_state = state;
		if (gl_state.cullface == GL_BACK)
			gl_state.cullface = GL_FRONT;
		else if (gl_state.cullface == GL_FRONT)
			gl_state.cullface = GL_BACK;
		else
			return;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			qglCullFace(gl_state.cullface);CHECKGLERROR
			break;
		}
	}
}

void GL_CullFace(int state)
{
	if(v_flipped_state)
	{
		if(state == GL_FRONT)
			state = GL_BACK;
		else if(state == GL_BACK)
			state = GL_FRONT;
	}

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR

		if (state != GL_NONE)
		{
			if (!gl_state.cullfaceenable)
			{
				gl_state.cullfaceenable = true;
				qglEnable(GL_CULL_FACE);CHECKGLERROR
			}
			if (gl_state.cullface != state)
			{
				gl_state.cullface = state;
				qglCullFace(gl_state.cullface);CHECKGLERROR
			}
		}
		else
		{
			if (gl_state.cullfaceenable)
			{
				gl_state.cullfaceenable = false;
				qglDisable(GL_CULL_FACE);CHECKGLERROR
			}
		}
		break;
	}
}

void GL_AlphaToCoverage(qbool state)
{
	if (gl_state.alphatocoverage != state)
	{
		gl_state.alphatocoverage = state;
		switch(vid.renderpath)
		{
		case RENDERPATH_GLES2:
			break;
		case RENDERPATH_GL32:
#ifndef USE_GLES2
			// alpha to coverage turns the alpha value of the pixel into 0%, 25%, 50%, 75% or 100% by masking the multisample fragments accordingly
			CHECKGLERROR
			if (gl_state.alphatocoverage)
			{
				qglEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);CHECKGLERROR
//				qglEnable(GL_MULTISAMPLE);CHECKGLERROR
			}
			else
			{
				qglDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);CHECKGLERROR
//				qglDisable(GL_MULTISAMPLE);CHECKGLERROR
			}
#endif
			break;
		}
	}
}

void GL_ColorMask(int r, int g, int b, int a)
{
	// NOTE: this matches D3DCOLORWRITEENABLE_RED, GREEN, BLUE, ALPHA
	int state = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
	if (gl_state.colormask != state)
	{
		gl_state.colormask = state;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			qglColorMask((GLboolean)r, (GLboolean)g, (GLboolean)b, (GLboolean)a);CHECKGLERROR
			break;
		}
	}
}

void GL_Color(float cr, float cg, float cb, float ca)
{
	if (gl_state.pointer_color_enabled || gl_state.color4f[0] != cr || gl_state.color4f[1] != cg || gl_state.color4f[2] != cb || gl_state.color4f[3] != ca)
	{
		gl_state.color4f[0] = cr;
		gl_state.color4f[1] = cg;
		gl_state.color4f[2] = cb;
		gl_state.color4f[3] = ca;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			qglVertexAttrib4f(GLSLATTRIB_COLOR, cr, cg, cb, ca);CHECKGLERROR
			break;
		}
	}
}

void GL_Scissor (int x, int y, int width, int height)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		qglScissor(x, y,width,height);CHECKGLERROR
		break;
	}
}

void GL_ScissorTest(int state)
{
	if (gl_state.scissortest != state)
	{
		gl_state.scissortest = state;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			if(gl_state.scissortest)
				qglEnable(GL_SCISSOR_TEST);
			else
				qglDisable(GL_SCISSOR_TEST);
			CHECKGLERROR
			break;
		}
	}
}

void GL_Clear(int mask, const float *colorvalue, float depthvalue, int stencilvalue)
{
	// opaque black - if you want transparent black, you'll need to pass in a colorvalue
	static const float blackcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	// prevent warnings when trying to clear a buffer that does not exist
	if (!colorvalue)
		colorvalue = blackcolor;
	if (!vid.stencil)
	{
		mask &= ~GL_STENCIL_BUFFER_BIT;
		stencilvalue = 0;
	}
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		if (mask & GL_COLOR_BUFFER_BIT)
		{
			qglClearColor(colorvalue[0], colorvalue[1], colorvalue[2], colorvalue[3]);CHECKGLERROR
		}
		if (mask & GL_DEPTH_BUFFER_BIT)
		{
#ifdef USE_GLES2
			//qglClearDepthf(depthvalue);CHECKGLERROR
#else
			qglClearDepth(depthvalue);CHECKGLERROR
#endif
		}
		if (mask & GL_STENCIL_BUFFER_BIT)
		{
			qglClearStencil(stencilvalue);CHECKGLERROR
		}
		qglClear(mask);CHECKGLERROR
		break;
	}
}

void GL_ReadPixelsBGRA(int x, int y, int width, int height, unsigned char *outpixels)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
#ifndef GL_BGRA
		{
			int i;
			int r;
		//	int g;
			int b;
		//	int a;
			qglReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);CHECKGLERROR
			for (i = 0;i < width * height * 4;i += 4)
			{
				r = outpixels[i+0];
		//		g = outpixels[i+1];
				b = outpixels[i+2];
		//		a = outpixels[i+3];
				outpixels[i+0] = b;
		//		outpixels[i+1] = g;
				outpixels[i+2] = r;
		//		outpixels[i+3] = a;
			}
		}
#else
		qglReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, outpixels);CHECKGLERROR
#endif
			break;
	}
}

// called at beginning of frame
void R_Mesh_Start(void)
{
	BACKENDACTIVECHECK
	R_Mesh_SetRenderTargets(0, NULL, NULL, NULL, NULL, NULL);
	if (gl_printcheckerror.integer && !gl_paranoid.integer)
	{
		Con_Printf(CON_WARN "WARNING: gl_printcheckerror is on but gl_paranoid is off, turning it on...\n");
		Cvar_SetValueQuick(&gl_paranoid, 1);
	}
}

static qbool GL_Backend_CompileShader(int programobject, GLenum shadertypeenum, const char *shadertype, int numstrings, const char **strings)
{
	int shaderobject;
	int shadercompiled;
	char compilelog[MAX_INPUTLINE];
	shaderobject = qglCreateShader(shadertypeenum);CHECKGLERROR
	if (!shaderobject)
		return false;
	qglShaderSource(shaderobject, numstrings, strings, NULL);CHECKGLERROR
	qglCompileShader(shaderobject);CHECKGLERROR
	qglGetShaderiv(shaderobject, GL_COMPILE_STATUS, &shadercompiled);CHECKGLERROR
	qglGetShaderInfoLog(shaderobject, sizeof(compilelog), NULL, compilelog);CHECKGLERROR
	if (compilelog[0] && ((strstr(compilelog, "error") || strstr(compilelog, "ERROR") || strstr(compilelog, "Error")) || ((strstr(compilelog, "WARNING") || strstr(compilelog, "warning") || strstr(compilelog, "Warning")) && developer.integer) || developer_extra.integer))
	{
		int i, j, pretextlines = 0;
		for (i = 0;i < numstrings - 1;i++)
			for (j = 0;strings[i][j];j++)
				if (strings[i][j] == '\n')
					pretextlines++;
		Con_Printf("%s shader compile log:\n%s\n(line offset for any above warnings/errors: %i)\n", shadertype, compilelog, pretextlines);
	}
	if (!shadercompiled)
	{
		qglDeleteShader(shaderobject);CHECKGLERROR
		return false;
	}
	qglAttachShader(programobject, shaderobject);CHECKGLERROR
	qglDeleteShader(shaderobject);CHECKGLERROR
	return true;
}

unsigned int GL_Backend_CompileProgram(int vertexstrings_count, const char **vertexstrings_list, int geometrystrings_count, const char **geometrystrings_list, int fragmentstrings_count, const char **fragmentstrings_list)
{
	GLint programlinked;
	GLuint programobject = 0;
	char linklog[MAX_INPUTLINE];
	CHECKGLERROR

	programobject = qglCreateProgram();CHECKGLERROR
	if (!programobject)
		return 0;

	qglBindAttribLocation(programobject, GLSLATTRIB_POSITION , "Attrib_Position" );
	qglBindAttribLocation(programobject, GLSLATTRIB_COLOR    , "Attrib_Color"    );
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD0, "Attrib_TexCoord0");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD1, "Attrib_TexCoord1");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD2, "Attrib_TexCoord2");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD3, "Attrib_TexCoord3");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD4, "Attrib_TexCoord4");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD5, "Attrib_TexCoord5");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD6, "Attrib_SkeletalIndex");
	qglBindAttribLocation(programobject, GLSLATTRIB_TEXCOORD7, "Attrib_SkeletalWeight");
#ifndef USE_GLES2
	qglBindFragDataLocation(programobject, 0, "dp_FragColor");
#endif
	CHECKGLERROR

	if (vertexstrings_count && !GL_Backend_CompileShader(programobject, GL_VERTEX_SHADER, "vertex", vertexstrings_count, vertexstrings_list))
		goto cleanup;

#if defined(GL_GEOMETRY_SHADER) && !defined(USE_GLES2)
	if (geometrystrings_count && !GL_Backend_CompileShader(programobject, GL_GEOMETRY_SHADER, "geometry", geometrystrings_count, geometrystrings_list))
		goto cleanup;
#endif

	if (fragmentstrings_count && !GL_Backend_CompileShader(programobject, GL_FRAGMENT_SHADER, "fragment", fragmentstrings_count, fragmentstrings_list))
		goto cleanup;

	qglLinkProgram(programobject);CHECKGLERROR
	qglGetProgramiv(programobject, GL_LINK_STATUS, &programlinked);CHECKGLERROR
	qglGetProgramInfoLog(programobject, sizeof(linklog), NULL, linklog);CHECKGLERROR

	if (linklog[0])
	{

		if (strstr(linklog, "error") || strstr(linklog, "ERROR") || strstr(linklog, "Error") || strstr(linklog, "WARNING") || strstr(linklog, "warning") || strstr(linklog, "Warning") || developer_extra.integer)
			Con_DPrintf("program link log:\n%s\n", linklog);

		// software vertex shader is ok but software fragment shader is WAY
		// too slow, fail program if so.
		// NOTE: this string might be ATI specific, but that's ok because the
		// ATI R300 chip (Radeon 9500-9800/X300) is the most likely to use a
		// software fragment shader due to low instruction and dependent
		// texture limits.
		if (strstr(linklog, "fragment shader will run in software"))
			programlinked = false;
	}

	if (!programlinked)
		goto cleanup;

	return programobject;
cleanup:
	qglDeleteProgram(programobject);CHECKGLERROR
	return 0;
}

void GL_Backend_FreeProgram(unsigned int prog)
{
	CHECKGLERROR
	qglDeleteProgram(prog);
	CHECKGLERROR
}

// renders triangles using vertices from the active arrays
void R_Mesh_Draw(int firstvertex, int numvertices, int firsttriangle, int numtriangles, const int *element3i, const r_meshbuffer_t *element3i_indexbuffer, int element3i_bufferoffset, const unsigned short *element3s, const r_meshbuffer_t *element3s_indexbuffer, int element3s_bufferoffset)
{
	unsigned int numelements = numtriangles * 3;
	int bufferobject3i;
	size_t bufferoffset3i;
	int bufferobject3s;
	size_t bufferoffset3s;
	if (numvertices < 3 || numtriangles < 1)
	{
		if (numvertices < 0 || numtriangles < 0 || developer_extra.integer)
			Con_DPrintf("R_Mesh_Draw(%d, %d, %d, %d, %8p, %8p, %8x, %8p, %8p, %8x);\n", firstvertex, numvertices, firsttriangle, numtriangles, (void *)element3i, (void *)element3i_indexbuffer, (int)element3i_bufferoffset, (void *)element3s, (void *)element3s_indexbuffer, (int)element3s_bufferoffset);
		return;
	}
	// adjust the pointers for firsttriangle
	if (element3i)
		element3i += firsttriangle * 3;
	if (element3i_indexbuffer)
		element3i_bufferoffset += firsttriangle * 3 * sizeof(*element3i);
	if (element3s)
		element3s += firsttriangle * 3;
	if (element3s_indexbuffer)
		element3s_bufferoffset += firsttriangle * 3 * sizeof(*element3s);
	// upload a dynamic index buffer if needed
	if (element3s)
	{
		if (!element3s_indexbuffer)
			element3s_indexbuffer = R_BufferData_Store(numelements * sizeof(*element3s), (void *)element3s, R_BUFFERDATA_INDEX16, &element3s_bufferoffset);
	}
	else if (element3i)
	{
		if (!element3i_indexbuffer)
			element3i_indexbuffer = R_BufferData_Store(numelements * sizeof(*element3i), (void *)element3i, R_BUFFERDATA_INDEX32, &element3i_bufferoffset);
	}
	bufferobject3i = element3i_indexbuffer ? element3i_indexbuffer->bufferobject : 0;
	bufferoffset3i = element3i_bufferoffset;
	bufferobject3s = element3s_indexbuffer ? element3s_indexbuffer->bufferobject : 0;
	bufferoffset3s = element3s_bufferoffset;
	r_refdef.stats[r_stat_draws]++;
	r_refdef.stats[r_stat_draws_vertices] += numvertices;
	r_refdef.stats[r_stat_draws_elements] += numelements;
	if (gl_paranoid.integer)
	{
		unsigned int i;
		if (element3i)
		{
			for (i = 0;i < (unsigned int) numtriangles * 3;i++)
			{
				if (element3i[i] < firstvertex || element3i[i] >= firstvertex + numvertices)
				{
					Con_Printf("R_Mesh_Draw: invalid vertex index %i (outside range %i - %i) in element3i array\n", element3i[i], firstvertex, firstvertex + numvertices);
					return;
				}
			}
		}
		if (element3s)
		{
			for (i = 0;i < (unsigned int) numtriangles * 3;i++)
			{
				if (element3s[i] < firstvertex || element3s[i] >= firstvertex + numvertices)
				{
					Con_Printf("R_Mesh_Draw: invalid vertex index %i (outside range %i - %i) in element3s array\n", element3s[i], firstvertex, firstvertex + numvertices);
					return;
				}
			}
		}
	}
	if (r_render.integer || r_refdef.draw2dstage)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			if (bufferobject3s)
			{
				GL_BindEBO(bufferobject3s);
				qglDrawElements(GL_TRIANGLES, numelements, GL_UNSIGNED_SHORT, (void *)bufferoffset3s);CHECKGLERROR
			}
			else if (bufferobject3i)
			{
				GL_BindEBO(bufferobject3i);
				qglDrawElements(GL_TRIANGLES, numelements, GL_UNSIGNED_INT, (void *)bufferoffset3i);CHECKGLERROR
			}
			else
			{
				qglDrawArrays(GL_TRIANGLES, firstvertex, numvertices);CHECKGLERROR
			}
			break;
		}
	}
}

// restores backend state, used when done with 3D rendering
void R_Mesh_Finish(void)
{
	R_Mesh_SetRenderTargets(0, NULL, NULL, NULL, NULL, NULL);
}

r_meshbuffer_t *R_Mesh_CreateMeshBuffer(const void *data, size_t size, const char *name, qbool isindexbuffer, qbool isuniformbuffer, qbool isdynamic, qbool isindex16)
{
	r_meshbuffer_t *buffer;
	buffer = (r_meshbuffer_t *)Mem_ExpandableArray_AllocRecord(&gl_state.meshbufferarray);
	memset(buffer, 0, sizeof(*buffer));
	buffer->bufferobject = 0;
	buffer->devicebuffer = NULL;
	buffer->size = size;
	buffer->isindexbuffer = isindexbuffer;
	buffer->isuniformbuffer = isuniformbuffer;
	buffer->isdynamic = isdynamic;
	buffer->isindex16 = isindex16;
	dp_strlcpy(buffer->name, name, sizeof(buffer->name));
	R_Mesh_UpdateMeshBuffer(buffer, data, size, false, 0);
	return buffer;
}

void R_Mesh_UpdateMeshBuffer(r_meshbuffer_t *buffer, const void *data, size_t size, qbool subdata, size_t offset)
{
	if (!buffer)
		return;
	if (buffer->isindexbuffer)
	{
		r_refdef.stats[r_stat_indexbufferuploadcount]++;
		r_refdef.stats[r_stat_indexbufferuploadsize] += (int)size;
	}
	else
	{
		r_refdef.stats[r_stat_vertexbufferuploadcount]++;
		r_refdef.stats[r_stat_vertexbufferuploadsize] += (int)size;
	}
	if (!subdata)
		buffer->size = size;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (!buffer->bufferobject)
			qglGenBuffers(1, (GLuint *)&buffer->bufferobject);
		CHECKGLERROR
		if (buffer->isuniformbuffer)
			GL_BindUBO(buffer->bufferobject);
		else if (buffer->isindexbuffer)
			GL_BindEBO(buffer->bufferobject);
		else
			GL_BindVBO(buffer->bufferobject);

		{
			int buffertype;
			buffertype = buffer->isindexbuffer ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
#ifdef GL_UNIFORM_BUFFER
			if (buffer->isuniformbuffer)
				buffertype = GL_UNIFORM_BUFFER;
#endif
			CHECKGLERROR
			if (subdata)
				qglBufferSubData(buffertype, offset, size, data);
			else
				qglBufferData(buffertype, size, data, buffer->isdynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW);
			CHECKGLERROR
		}
		if (buffer->isuniformbuffer)
			GL_BindUBO(0);
		break;
	}
}

void R_Mesh_DestroyMeshBuffer(r_meshbuffer_t *buffer)
{
	if (!buffer)
		return;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		// GL clears the binding if we delete something bound
		if (gl_state.uniformbufferobject == buffer->bufferobject)
			gl_state.uniformbufferobject = 0;
		if (gl_state.vertexbufferobject == buffer->bufferobject)
			gl_state.vertexbufferobject = 0;
		if (gl_state.elementbufferobject == buffer->bufferobject)
			gl_state.elementbufferobject = 0;
		CHECKGLERROR
		qglDeleteBuffers(1, (GLuint *)&buffer->bufferobject);CHECKGLERROR
		break;
	}
	Mem_ExpandableArray_FreeRecord(&gl_state.meshbufferarray, (void *)buffer);
}

static const char *buffertypename[R_BUFFERDATA_COUNT] = {"vertex", "index16", "index32", "uniform"};
void GL_Mesh_ListVBOs(qbool printeach)
{
	int i, endindex;
	int type;
	int isdynamic;
	int index16count, index16mem;
	int index32count, index32mem;
	int vertexcount, vertexmem;
	int uniformcount, uniformmem;
	int totalcount, totalmem;
	size_t bufferstat[R_BUFFERDATA_COUNT][2][2];
	r_meshbuffer_t *buffer;
	memset(bufferstat, 0, sizeof(bufferstat));
	endindex = (int)Mem_ExpandableArray_IndexRange(&gl_state.meshbufferarray);
	for (i = 0;i < endindex;i++)
	{
		buffer = (r_meshbuffer_t *) Mem_ExpandableArray_RecordAtIndex(&gl_state.meshbufferarray, i);
		if (!buffer)
			continue;
		if (buffer->isuniformbuffer)
			type = R_BUFFERDATA_UNIFORM;
		else if (buffer->isindexbuffer && buffer->isindex16)
			type = R_BUFFERDATA_INDEX16;
		else if (buffer->isindexbuffer)
			type = R_BUFFERDATA_INDEX32;
		else
			type = R_BUFFERDATA_VERTEX;
		isdynamic = buffer->isdynamic;
		bufferstat[type][isdynamic][0]++;
		bufferstat[type][isdynamic][1] += buffer->size;
		if (printeach)
			Con_Printf("buffer #%i %s = %i bytes (%s %s)\n", i, buffer->name, (int)buffer->size, isdynamic ? "dynamic" : "static", buffertypename[type]);
	}
	index16count   = (int)(bufferstat[R_BUFFERDATA_INDEX16][0][0] + bufferstat[R_BUFFERDATA_INDEX16][1][0]);
	index16mem     = (int)(bufferstat[R_BUFFERDATA_INDEX16][0][1] + bufferstat[R_BUFFERDATA_INDEX16][1][1]);
	index32count   = (int)(bufferstat[R_BUFFERDATA_INDEX32][0][0] + bufferstat[R_BUFFERDATA_INDEX32][1][0]);
	index32mem     = (int)(bufferstat[R_BUFFERDATA_INDEX32][0][1] + bufferstat[R_BUFFERDATA_INDEX32][1][1]);
	vertexcount  = (int)(bufferstat[R_BUFFERDATA_VERTEX ][0][0] + bufferstat[R_BUFFERDATA_VERTEX ][1][0]);
	vertexmem    = (int)(bufferstat[R_BUFFERDATA_VERTEX ][0][1] + bufferstat[R_BUFFERDATA_VERTEX ][1][1]);
	uniformcount = (int)(bufferstat[R_BUFFERDATA_UNIFORM][0][0] + bufferstat[R_BUFFERDATA_UNIFORM][1][0]);
	uniformmem   = (int)(bufferstat[R_BUFFERDATA_UNIFORM][0][1] + bufferstat[R_BUFFERDATA_UNIFORM][1][1]);
	totalcount = index16count + index32count + vertexcount + uniformcount;
	totalmem = index16mem + index32mem + vertexmem + uniformmem;
	Con_Printf("%i 16bit indexbuffers totalling %i bytes (%.3f MB)\n%i 32bit indexbuffers totalling %i bytes (%.3f MB)\n%i vertexbuffers totalling %i bytes (%.3f MB)\n%i uniformbuffers totalling %i bytes (%.3f MB)\ncombined %i buffers totalling %i bytes (%.3fMB)\n", index16count, index16mem, index16mem / 10248576.0, index32count, index32mem, index32mem / 10248576.0, vertexcount, vertexmem, vertexmem / 10248576.0, uniformcount, uniformmem, uniformmem / 10248576.0, totalcount, totalmem, totalmem / 10248576.0);
}



void R_Mesh_VertexPointer(int components, int gltype, size_t stride, const void *pointer, const r_meshbuffer_t *vertexbuffer, size_t bufferoffset)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (gl_state.pointer_vertex_components != components || gl_state.pointer_vertex_gltype != gltype || gl_state.pointer_vertex_stride != stride || gl_state.pointer_vertex_pointer != pointer || gl_state.pointer_vertex_vertexbuffer != vertexbuffer || gl_state.pointer_vertex_offset != bufferoffset)
		{
			int bufferobject = vertexbuffer ? vertexbuffer->bufferobject : 0;
			if (!bufferobject && gl_paranoid.integer)
				Con_DPrintf("Warning: no bufferobject in R_Mesh_VertexPointer(%i, %i, %i, %p, %p, %08x)", components, gltype, (int)stride, pointer, (void *)vertexbuffer, (unsigned int)bufferoffset);
			gl_state.pointer_vertex_components = components;
			gl_state.pointer_vertex_gltype = gltype;
			gl_state.pointer_vertex_stride = stride;
			gl_state.pointer_vertex_pointer = pointer;
			gl_state.pointer_vertex_vertexbuffer = vertexbuffer;
			gl_state.pointer_vertex_offset = bufferoffset;
			CHECKGLERROR
			GL_BindVBO(bufferobject);
			// LadyHavoc: special flag added to gltype for unnormalized types
			qglVertexAttribPointer(GLSLATTRIB_POSITION, components, gltype & ~0x80000000, (gltype & 0x80000000) == 0, (GLsizei)stride, bufferobject ? (void *)bufferoffset : pointer);CHECKGLERROR
		}
		break;
	}
}

void R_Mesh_ColorPointer(int components, int gltype, size_t stride, const void *pointer, const r_meshbuffer_t *vertexbuffer, size_t bufferoffset)
{
	// note: vertexbuffer may be non-NULL even if pointer is NULL, so check
	// the pointer only.
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		if (pointer)
		{
			// caller wants color array enabled
			int bufferobject = vertexbuffer ? vertexbuffer->bufferobject : 0;
			if (!gl_state.pointer_color_enabled)
			{
				gl_state.pointer_color_enabled = true;
				CHECKGLERROR
				qglEnableVertexAttribArray(GLSLATTRIB_COLOR);CHECKGLERROR
			}
			if (gl_state.pointer_color_components != components || gl_state.pointer_color_gltype != gltype || gl_state.pointer_color_stride != stride || gl_state.pointer_color_pointer != pointer || gl_state.pointer_color_vertexbuffer != vertexbuffer || gl_state.pointer_color_offset != bufferoffset)
			{
				gl_state.pointer_color_components = components;
				gl_state.pointer_color_gltype = gltype;
				gl_state.pointer_color_stride = stride;
				gl_state.pointer_color_pointer = pointer;
				gl_state.pointer_color_vertexbuffer = vertexbuffer;
				gl_state.pointer_color_offset = bufferoffset;
				CHECKGLERROR
				GL_BindVBO(bufferobject);
				// LadyHavoc: special flag added to gltype for unnormalized types
				qglVertexAttribPointer(GLSLATTRIB_COLOR, components, gltype & ~0x80000000, (gltype & 0x80000000) == 0, (GLsizei)stride, bufferobject ? (void *)bufferoffset : pointer);CHECKGLERROR
			}
		}
		else
		{
			// caller wants color array disabled
			if (gl_state.pointer_color_enabled)
			{
				gl_state.pointer_color_enabled = false;
				CHECKGLERROR
				qglDisableVertexAttribArray(GLSLATTRIB_COLOR);CHECKGLERROR
				// when color array is on the current color gets trashed, set it again
				qglVertexAttrib4f(GLSLATTRIB_COLOR, gl_state.color4f[0], gl_state.color4f[1], gl_state.color4f[2], gl_state.color4f[3]);CHECKGLERROR
			}
		}
		break;
	}
}

void R_Mesh_TexCoordPointer(unsigned int unitnum, int components, int gltype, size_t stride, const void *pointer, const r_meshbuffer_t *vertexbuffer, size_t bufferoffset)
{
	gltextureunit_t *unit = gl_state.units + unitnum;
	if (unitnum >= MAX_TEXTUREUNITS)
		Sys_Error("R_Mesh_TexCoordPointer: unitnum %i > max units %i\n", unitnum, MAX_TEXTUREUNITS);
	// update array settings
	// note: there is no need to check bufferobject here because all cases
	// that involve a valid bufferobject also supply a texcoord array
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		if (pointer)
		{
			int bufferobject = vertexbuffer ? vertexbuffer->bufferobject : 0;
			// texture array unit is enabled, enable the array
			if (!unit->arrayenabled)
			{
				unit->arrayenabled = true;
				qglEnableVertexAttribArray(unitnum+GLSLATTRIB_TEXCOORD0);CHECKGLERROR
			}
			// texcoord array
			if (unit->pointer_texcoord_components != components || unit->pointer_texcoord_gltype != gltype || unit->pointer_texcoord_stride != stride || unit->pointer_texcoord_pointer != pointer || unit->pointer_texcoord_vertexbuffer != vertexbuffer || unit->pointer_texcoord_offset != bufferoffset)
			{
				unit->pointer_texcoord_components = components;
				unit->pointer_texcoord_gltype = gltype;
				unit->pointer_texcoord_stride = stride;
				unit->pointer_texcoord_pointer = pointer;
				unit->pointer_texcoord_vertexbuffer = vertexbuffer;
				unit->pointer_texcoord_offset = bufferoffset;
				GL_BindVBO(bufferobject);
				// LadyHavoc: special flag added to gltype for unnormalized types
				qglVertexAttribPointer(unitnum+GLSLATTRIB_TEXCOORD0, components, gltype & ~0x80000000, (gltype & 0x80000000) == 0, (GLsizei)stride, bufferobject ? (void *)bufferoffset : pointer);CHECKGLERROR
			}
		}
		else
		{
			// texture array unit is disabled, disable the array
			if (unit->arrayenabled)
			{
				unit->arrayenabled = false;
				qglDisableVertexAttribArray(unitnum+GLSLATTRIB_TEXCOORD0);CHECKGLERROR
			}
		}
		break;
	}
}

int R_Mesh_TexBound(unsigned int unitnum, int id)
{
	gltextureunit_t *unit = gl_state.units + unitnum;
	if (unitnum >= MAX_TEXTUREUNITS)
		Sys_Error("R_Mesh_TexCoordPointer: unitnum %i > max units %i\n", unitnum, MAX_TEXTUREUNITS);
	if (id == GL_TEXTURE_2D)
		return unit->t2d;
	if (id == GL_TEXTURE_3D)
		return unit->t3d;
	if (id == GL_TEXTURE_CUBE_MAP)
		return unit->tcubemap;
	return 0;
}

void R_Mesh_CopyToTexture(rtexture_t *tex, int tx, int ty, int sx, int sy, int width, int height)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		R_Mesh_TexBind(0, tex);
		GL_ActiveTexture(0);CHECKGLERROR
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, tx, ty, sx, sy, width, height);CHECKGLERROR
		break;
	}
}

void R_Mesh_ClearBindingsForTexture(int texnum)
{
	gltextureunit_t *unit;
	unsigned int unitnum;
	// unbind the texture from any units it is bound on - this prevents accidental reuse of certain textures whose bindings can linger far too long otherwise (e.g. bouncegrid which is a 3D texture) and confuse the driver later.
	for (unitnum = 0; unitnum < MAX_TEXTUREUNITS; unitnum++)
	{
		unit = gl_state.units + unitnum;
		if (unit->texture && unit->texture->texnum == texnum)
			R_Mesh_TexBind(unitnum, NULL);
	}
}

void R_Mesh_TexBind(unsigned int unitnum, rtexture_t *tex)
{
	gltextureunit_t *unit = gl_state.units + unitnum;
	int texnum;
	if (unitnum >= MAX_TEXTUREUNITS)
		Sys_Error("R_Mesh_TexBind: unitnum %i > max units %i\n", unitnum, MAX_TEXTUREUNITS);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (tex)
		{
			texnum = R_GetTexture(tex);
			switch (tex->gltexturetypeenum)
			{
			case GL_TEXTURE_2D:
				if (unit->t2d != texnum) { GL_ActiveTexture(unitnum);qglBindTexture(GL_TEXTURE_2D, texnum); CHECKGLERROR unit->t2d = texnum; }
				if (unit->t3d) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_3D, 0); CHECKGLERROR unit->t3d = 0; }
				if (unit->tcubemap) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_CUBE_MAP, 0); CHECKGLERROR unit->tcubemap = 0; }
				break;
			case GL_TEXTURE_3D:
				if (unit->t2d) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_2D, 0); CHECKGLERROR unit->t2d = 0; }
				if (unit->t3d != texnum) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_3D, texnum); CHECKGLERROR unit->t3d = texnum; }
				if (unit->tcubemap) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_CUBE_MAP, 0); CHECKGLERROR unit->tcubemap = 0; }
				break;
			case GL_TEXTURE_CUBE_MAP:
				if (unit->t2d) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_2D, 0); CHECKGLERROR unit->t2d = 0; }
				if (unit->t3d) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_3D, 0); CHECKGLERROR unit->t3d = 0; }
				if (unit->tcubemap != texnum) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_CUBE_MAP, texnum); CHECKGLERROR unit->tcubemap = texnum; }
				break;
			}
		}
		else
		{
			if (unit->t2d) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_2D, 0); CHECKGLERROR unit->t2d = 0; }
			if (unit->t3d) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_3D, 0); CHECKGLERROR unit->t3d = 0; }
			if (unit->tcubemap) { GL_ActiveTexture(unitnum); qglBindTexture(GL_TEXTURE_CUBE_MAP, 0); CHECKGLERROR unit->tcubemap = 0; }
		}
	}
	unit->texture = tex;
}

void R_Mesh_ResetTextureState(void)
{
#if 0
	unsigned int unitnum;
	
	BACKENDACTIVECHECK

	for (unitnum = 0;unitnum < MAX_TEXTUREUNITS;unitnum++)
		R_Mesh_TexBind(unitnum, NULL);
#endif
}

void R_Mesh_PrepareVertices_Vertex3f(int numvertices, const float *vertex3f, const r_meshbuffer_t *vertexbuffer, int bufferoffset)
{
	// upload temporary vertexbuffer for this rendering
	if (!vertexbuffer)
		vertexbuffer = R_BufferData_Store(numvertices * sizeof(float[3]), (void *)vertex3f, R_BUFFERDATA_VERTEX, &bufferoffset);
	R_Mesh_VertexPointer(     3, GL_FLOAT        , sizeof(float[3])        , vertex3f  , vertexbuffer     , bufferoffset           );
	R_Mesh_ColorPointer(      4, GL_FLOAT        , sizeof(float[4])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(0, 2, GL_FLOAT        , sizeof(float[2])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(1, 3, GL_FLOAT        , sizeof(float[3])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(2, 3, GL_FLOAT        , sizeof(float[3])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(3, 3, GL_FLOAT        , sizeof(float[3])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(4, 2, GL_FLOAT        , sizeof(float[2])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(5, 2, GL_FLOAT        , sizeof(float[2])        , NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(6, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), NULL      , NULL             , 0                      );
	R_Mesh_TexCoordPointer(7, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), NULL      , NULL             , 0                      );
}

void R_Mesh_PrepareVertices_Generic_Arrays(int numvertices, const float *vertex3f, const float *color4f, const float *texcoord2f)
{
	r_meshbuffer_t *buffer_vertex3f = NULL;
	r_meshbuffer_t *buffer_color4f = NULL;
	r_meshbuffer_t *buffer_texcoord2f = NULL;
	int bufferoffset_vertex3f = 0;
	int bufferoffset_color4f = 0;
	int bufferoffset_texcoord2f = 0;
	if (color4f)
		buffer_color4f    = R_BufferData_Store(numvertices * sizeof(float[4]), color4f   , R_BUFFERDATA_VERTEX, &bufferoffset_color4f   );
	if (vertex3f)
		buffer_vertex3f   = R_BufferData_Store(numvertices * sizeof(float[3]), vertex3f  , R_BUFFERDATA_VERTEX, &bufferoffset_vertex3f  );
	if (texcoord2f)
		buffer_texcoord2f = R_BufferData_Store(numvertices * sizeof(float[2]), texcoord2f, R_BUFFERDATA_VERTEX, &bufferoffset_texcoord2f);
	R_Mesh_VertexPointer(     3, GL_FLOAT        , sizeof(float[3])        , vertex3f          , buffer_vertex3f          , bufferoffset_vertex3f          );
	R_Mesh_ColorPointer(      4, GL_FLOAT        , sizeof(float[4])        , color4f           , buffer_color4f           , bufferoffset_color4f           );
	R_Mesh_TexCoordPointer(0, 2, GL_FLOAT        , sizeof(float[2])        , texcoord2f        , buffer_texcoord2f        , bufferoffset_texcoord2f        );
	R_Mesh_TexCoordPointer(1, 3, GL_FLOAT        , sizeof(float[3])        , NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(2, 3, GL_FLOAT        , sizeof(float[3])        , NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(3, 3, GL_FLOAT        , sizeof(float[3])        , NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(4, 2, GL_FLOAT        , sizeof(float[2])        , NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(5, 2, GL_FLOAT        , sizeof(float[2])        , NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(6, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(7, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), NULL              , NULL                     , 0                              );
}

void R_Mesh_PrepareVertices_Mesh_Arrays(int numvertices, const float *vertex3f, const float *svector3f, const float *tvector3f, const float *normal3f, const float *color4f, const float *texcoordtexture2f, const float *texcoordlightmap2f)
{
	r_meshbuffer_t *buffer_vertex3f = NULL;
	r_meshbuffer_t *buffer_color4f = NULL;
	r_meshbuffer_t *buffer_texcoordtexture2f = NULL;
	r_meshbuffer_t *buffer_svector3f = NULL;
	r_meshbuffer_t *buffer_tvector3f = NULL;
	r_meshbuffer_t *buffer_normal3f = NULL;
	r_meshbuffer_t *buffer_texcoordlightmap2f = NULL;
	int bufferoffset_vertex3f = 0;
	int bufferoffset_color4f = 0;
	int bufferoffset_texcoordtexture2f = 0;
	int bufferoffset_svector3f = 0;
	int bufferoffset_tvector3f = 0;
	int bufferoffset_normal3f = 0;
	int bufferoffset_texcoordlightmap2f = 0;
	if (color4f)
		buffer_color4f            = R_BufferData_Store(numvertices * sizeof(float[4]), color4f           , R_BUFFERDATA_VERTEX, &bufferoffset_color4f           );
	if (vertex3f)
		buffer_vertex3f           = R_BufferData_Store(numvertices * sizeof(float[3]), vertex3f          , R_BUFFERDATA_VERTEX, &bufferoffset_vertex3f          );
	if (svector3f)
		buffer_svector3f          = R_BufferData_Store(numvertices * sizeof(float[3]), svector3f         , R_BUFFERDATA_VERTEX, &bufferoffset_svector3f         );
	if (tvector3f)
		buffer_tvector3f          = R_BufferData_Store(numvertices * sizeof(float[3]), tvector3f         , R_BUFFERDATA_VERTEX, &bufferoffset_tvector3f         );
	if (normal3f)
		buffer_normal3f           = R_BufferData_Store(numvertices * sizeof(float[3]), normal3f          , R_BUFFERDATA_VERTEX, &bufferoffset_normal3f          );
	if (texcoordtexture2f)
		buffer_texcoordtexture2f  = R_BufferData_Store(numvertices * sizeof(float[2]), texcoordtexture2f , R_BUFFERDATA_VERTEX, &bufferoffset_texcoordtexture2f );
	if (texcoordlightmap2f)
		buffer_texcoordlightmap2f = R_BufferData_Store(numvertices * sizeof(float[2]), texcoordlightmap2f, R_BUFFERDATA_VERTEX, &bufferoffset_texcoordlightmap2f);
	R_Mesh_VertexPointer(     3, GL_FLOAT        , sizeof(float[3])        , vertex3f          , buffer_vertex3f          , bufferoffset_vertex3f          );
	R_Mesh_ColorPointer(      4, GL_FLOAT        , sizeof(float[4])        , color4f           , buffer_color4f           , bufferoffset_color4f           );
	R_Mesh_TexCoordPointer(0, 2, GL_FLOAT        , sizeof(float[2])        , texcoordtexture2f , buffer_texcoordtexture2f , bufferoffset_texcoordtexture2f );
	R_Mesh_TexCoordPointer(1, 3, GL_FLOAT        , sizeof(float[3])        , svector3f         , buffer_svector3f         , bufferoffset_svector3f         );
	R_Mesh_TexCoordPointer(2, 3, GL_FLOAT        , sizeof(float[3])        , tvector3f         , buffer_tvector3f         , bufferoffset_tvector3f         );
	R_Mesh_TexCoordPointer(3, 3, GL_FLOAT        , sizeof(float[3])        , normal3f          , buffer_normal3f          , bufferoffset_normal3f          );
	R_Mesh_TexCoordPointer(4, 2, GL_FLOAT        , sizeof(float[2])        , texcoordlightmap2f, buffer_texcoordlightmap2f, bufferoffset_texcoordlightmap2f);
	R_Mesh_TexCoordPointer(5, 2, GL_FLOAT        , sizeof(float[2])        , NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(6, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), NULL              , NULL                     , 0                              );
	R_Mesh_TexCoordPointer(7, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), NULL              , NULL                     , 0                              );
}

void GL_BlendEquationSubtract(qbool negated)
{
	CHECKGLERROR
	if(negated)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			qglBlendEquation(GL_FUNC_REVERSE_SUBTRACT);CHECKGLERROR
			break;
		}
	}
	else
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			qglBlendEquation(GL_FUNC_ADD);CHECKGLERROR
			break;
		}
	}
}
