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

#ifndef GLQUAKE_H
#define GLQUAKE_H

// disable data conversion warnings
#ifdef MSVCDISABLEWARNINGS
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#endif

void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs);
qboolean BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
void ClearBounds (vec3_t mins, vec3_t maxs);

void ModBrush_LoadGLStuff(void *ctx, void *data, size_t a, size_t b);	//data === builddata_t

#ifdef GLQUAKE
	#if defined(ANDROID) /*FIXME: actually just to use standard GLES headers instead of full GL*/
		#ifndef GLSLONLY
			#include <GLES/gl.h>
			#ifndef GL_CLIP_PLANE0
			#define GL_CLIP_PLANE0 0x3000
			#endif
		#else
			#include <GLES2/gl2.h>
		#endif
		/*gles has no doubles*/
		#define GLclampd GLclampf
		#define GLdouble GLfloat
		#define GL_NONE                           0
	#elif defined(__APPLE__) || defined(__MACOSX__)
		#include <OpenGL/OpenGLAvailability.h>
		#if defined(OPENGL_DEPRECATED) && !defined(GL_SILENCE_DEPRECATION)
			#undef OPENGL_DEPRECATED
			#define OPENGL_DEPRECATED(from, to) API_DEPRECATED("Apple is deprecated. (Define GL_SILENCE_DEPRECATION to silence these warnings)", macos(from, to))
		#endif
		#include <OpenGL/gl.h>	//tuna says use this.
		//apple really do suck.
	#elif defined(FTE_TARGET_WEB)
		#include <GLES2/gl2.h>
		#define GLclampd GLclampf
		#define GLdouble GLfloat
	#else
		#ifdef _WIN32	//windows might use the standard header filename, but it still requires that we manually include windows.h first.
			#if !defined(WIN32_BLOATED) && !defined(WIN32_LEAN_AND_MEAN)
				#define WIN32_LEAN_AND_MEAN
			#endif
			#include <windows.h>
		#endif

		#include <GL/gl.h>
		#ifdef GL_STATIC
			#define GL_GLEXT_PROTOTYPES
			#include <GL/glext.h>
		#endif
	#endif
//	#include <GL/glu.h>

	#ifndef APIENTRY
		#define APIENTRY	//our code decorates function pointers with this for windows, so make sure it exists on systems that don't need it.
	#endif
	#include "glsupp.h"
			


	/*gles2 has no fixed function*/
#ifndef GL_ALPHA_TEST
	#define GL_ALPHA_TEST 0
#endif
#ifndef GL_FILL
	#define GL_FILL (Sys_Error("GL_FILL was used"),0)
#endif
#ifndef GL_CLAMP
	#define GL_CLAMP GL_CLAMP_TO_EDGE
#endif
#ifndef GL_TEXTURE_ENV
	#define GL_TEXTURE_ENV (Con_Printf("GL_TEXTURE_ENV was used"),0)
	#define GL_TEXTURE_ENV_MODE (Con_Printf("GL_TEXTURE_ENV_MODE was used"),0)
	#define GL_VERTEX_ARRAY (Con_Printf("GL_VERTEX_ARRAY was used"),0)
	#define GL_COLOR_ARRAY (Con_Printf("GL_COLOR_ARRAY was used"),0)
	#define GL_TEXTURE_COORD_ARRAY (Con_Printf("GL_TEXTURE_COORD_ARRAY was used"),0)
	#define GL_DECAL (Con_Printf("GL_DECAL was used"),0)
	#define GL_ADD (Con_Printf("GL_ADD was used"),0)
	#define GL_FLAT (Con_Printf("GL_FLAT was used"),0)
	#define GL_SMOOTH (Con_Printf("GL_SMOOTH was used"),0)
	#define GL_MODULATE 0x2100
	#define GL_PROJECTION (Con_Printf("GL_PROJECTION was used"),0)
	#define GL_MODELVIEW (Con_Printf("GL_MODELVIEW was used"),0)
	#define GL_CLIP_PLANE0 (Con_Printf("GL_CLIP_PLANE0 was used"),0)
#endif
#ifndef GL_COLOR_ARRAY_POINTER
	#define GL_COLOR_ARRAY_POINTER 0
	#define GL_NORMAL_ARRAY 0
	#define GL_NORMAL_ARRAY_POINTER 0
	#define GL_TEXTURE_COORD_ARRAY_POINTER 0
	#define GL_VERTEX_ARRAY_POINTER 0
	#define GL_BLEND_SRC 0
	#define GL_BLEND_DST 0
#endif
#ifndef GL_POLYGON
	#define GL_POLYGON (Con_Printf("GL_POLYGON was used"),0)
	#define GL_QUAD_STRIP (Con_Printf("GL_QUAD_STRIP was used"),0)
	#define GL_QUADS (Con_Printf("GL_QUADS was used"),0)
#endif

void GL_InitFogTexture(void);

#ifndef GL_VERSION_2_0
#define GLchar char
#endif

// Function prototypes for the Texture Object Extension routines
typedef GLboolean (APIENTRY *ARETEXRESFUNCPTR)(GLsizei, const GLuint *,
                    const GLboolean *);
typedef void (APIENTRY *BINDTEXFUNCPTR)(GLenum, GLuint);
typedef void (APIENTRY *DELTEXFUNCPTR)(GLsizei, const GLuint *);
typedef void (APIENTRY *GENTEXFUNCPTR)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY *ISTEXFUNCPTR)(GLuint);
typedef void (APIENTRY *PRIORTEXFUNCPTR)(GLsizei, const GLuint *,
                    const GLclampf *);
typedef void (APIENTRY *TEXSUBIMAGEPTR)(int, int, int, int, int, int, int, int, void *);
typedef void (APIENTRY *FTEPFNGLCOMPRESSEDTEXIMAGE2DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *FTEPFNGLCOMPRESSEDTEXIMAGE3DARBPROC)	(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
typedef void (APIENTRY *FTEPFNGLGETCOMPRESSEDTEXIMAGEARBPROC)	(GLenum target, GLint lod, const GLvoid* img);
typedef void (APIENTRY *FTEPFNGLPNTRIANGLESIATIPROC)(GLenum pname, GLint param);
typedef void (APIENTRY *FTEPFNGLPNTRIANGLESFATIPROC)(GLenum pname, GLfloat param);
typedef void (APIENTRY *FTEPFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);

typedef GLuint		(APIENTRYP FTEPFNGLCREATEPROGRAMPROC)		(void);
typedef void		(APIENTRYP FTEPFNGLDELETEPROGRAMPROC)		(GLuint obj);
typedef void		(APIENTRYP FTEPFNGLDELETESHADERPROC)		(GLuint obj);
typedef void		(APIENTRYP FTEPFNGLUSEPROGRAMPROC)			(GLuint programObj);
typedef GLuint		(APIENTRYP FTEPFNGLCREATESHADERPROC)		(GLenum shaderType);
typedef void		(APIENTRYP FTEPFNGLSHADERSOURCEPROC)		(GLuint shaderObj, GLsizei count, const GLchar* const*string, const GLint *length);
typedef void		(APIENTRYP FTEPFNGLCOMPILESHADERPROC)		(GLuint shaderObj);
typedef void        (APIENTRYP FTEPFNGLGETPROGRAMIVPROC)		(GLuint obj, GLenum pname, GLint *params);
typedef void        (APIENTRYP FTEPFNGLGETSHADERIVPROC)			(GLuint obj, GLenum pname, GLint *params);
typedef void		(APIENTRYP FTEPFNGLATTACHSHADERPROC)		(GLuint containerObj, GLuint obj);
typedef void		(APIENTRYP FTEPFNGLGETPROGRAMINFOLOGPROC)	(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void		(APIENTRYP FTEPFNGLGETSHADERINFOLOGPROC)	(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void		(APIENTRYP FTEPFNGLLINKPROGRAMPROC)			(GLuint programObj);
typedef void        (APIENTRYP FTEPFNGLBINDATTRIBLOCATIONPROC)	(GLuint programObj, GLuint index, const GLchar *name);
typedef GLint		(APIENTRYP FTEPFNGLGETATTRIBLOCATIONPROC)	(GLuint programObj, const GLchar *name);
typedef void		(APIENTRYP FTEPFNGLVERTEXATTRIBPOINTERPROC)	(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void		(APIENTRYP FTEPFNGLVERTEXATTRIB4FPROC)		(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void		(APIENTRYP FTEPFNGLENABLEVERTEXATTRIBARRAYPROC)	(GLuint index);
typedef void		(APIENTRYP FTEPFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef GLint		(APIENTRYP FTEPFNGLGETUNIFORMLOCATIONPROC)	(GLuint programObj, const GLchar *name);
typedef void		(APIENTRYP FTEPFNGLGETVERTEXATTRIBIVPROC)	(GLuint index, GLenum pname, GLint *params);
typedef void		(APIENTRYP FTEPFNGLUNIFORM4FPROC)			(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void		(APIENTRYP FTEPFNGLUNIFORMMATRIX4FVPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORMMATRIX3FVPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORMMATRIX4X3FVPROC)		(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORMMATRIX3X4FVPROC)		(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORM4FVPROC)			(GLint location, GLsizei count, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORM3FPROC)			(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void		(APIENTRYP FTEPFNGLUNIFORM3FVPROC)			(GLint location, GLsizei count, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORM2FVPROC)			(GLint location, GLsizei count, const GLfloat *value);
typedef void		(APIENTRYP FTEPFNGLUNIFORM1IPROC)			(GLint location, GLint v0);
typedef void		(APIENTRYP FTEPFNGLUNIFORM1FPROC)			(GLint location, GLfloat v0);
typedef void		(APIENTRYP FTEPFNGLGETSHADERSOURCEPROC)		(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *source);

typedef void (APIENTRY * FTEPFNGLLOCKARRAYSEXTPROC) (GLint first, GLsizei count);
typedef void (APIENTRY * FTEPFNGLUNLOCKARRAYSEXTPROC) (void);

#ifndef GL_STATIC
extern	BINDTEXFUNCPTR qglBindTexture;
extern	DELTEXFUNCPTR delTexFunc;
extern	TEXSUBIMAGEPTR TexSubImage2DFunc;
extern void (APIENTRY *qglStencilOpSeparate) (GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
#endif
extern	FTEPFNGLPNTRIANGLESIATIPROC qglPNTrianglesiATI;
extern	FTEPFNGLPNTRIANGLESFATIPROC qglPNTrianglesfATI;
extern void (APIENTRY *qglPatchParameteriARB)(GLenum pname, GLint value);	//core in gl4

qboolean GL_CheckExtension(char *extname);

struct glfmt_s
{
	int sizedformat;	//texstorage
	int cformat;		//sized format used when gl_compress is set
	int internalformat;	//used instead of internal format when gl_compress is set, or 0
	int format;			//0 for compressed data
	int type;			//0 for compressed data
	int swizzle_r;
	int swizzle_g;
	int swizzle_b;
	int swizzle_a;
};

typedef struct {
	float glversion;
	int maxglslversion;
	int maxattribs;	//max generic attributes. probably only 16 on nvidia.
	qboolean nofixedfunc;
	qboolean gles;
	qboolean webgl_ie;	//workaround ie webgl bugs/omissions.
	qboolean blacklist_invariant; //mesa's invariant keyword is broken (for us when combined with fixed function)
	qboolean tex_env_combine;
	qboolean nv_tex_env_combine4;
	qboolean env_add;

//	qboolean sgis_generate_mipmap;

	qboolean arb_texture_env_combine;
	qboolean arb_texture_env_dot3;

	qboolean arb_texture_compression;	//means we support dynamic compression, rather than any specific compressed texture formats
	qboolean astc_decodeprecision;		//means we can tell the gpu that our astc textures actually are ldr.

	qboolean geometryshaders;
	qboolean arb_tessellation_shader;

	qboolean ext_framebuffer_objects;
	qboolean arb_framebuffer_srgb;
//	qboolean arb_fragment_program;
	qboolean arb_shader_objects;
	qboolean arb_shadow;
	qboolean arb_depth_texture;
	qboolean ext_stencil_wrap;
	qboolean ext_packed_depth_stencil;
	qboolean arb_depth_clamp;
	int ext_texture_filter_anisotropic;

	struct glfmt_s formatinfo[PTI_MAX];
	int unpackalignment;
} gl_config_t;

extern gl_config_t gl_config;

extern	float	gldepthmin, gldepthmax;

void GL_UpdateFiltering(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis);
qboolean GL_LoadTextureMips(texid_t tex, const struct pendingtextureinfo *mips);
void GL_DestroyTexture(texid_t tex);
void GL_SetupFormats(void);

/*
typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;

FTE_DEPRECATED extern glvert_t glv;
*/
#endif

// r_local.h -- private refresh defs

//#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
//#define	MAX_LBM_HEIGHT		480

//#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

//#define SKYSHIFT		7
//#define	SKYSIZE			(1 << SKYSHIFT)
//#define SKYMASK			(SKYSIZE - 1)


void R_TimeRefresh_f (void);

#include "particles.h"

//====================================================

extern	entity_t	r_worldentity;
extern	vec3_t		r_entorigin;
extern	entity_t	*currententity;

extern qboolean		r_loadbumpmapping;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	unsigned int r_viewcontents;
extern	int r_viewarea;
extern	int		r_viewcluster, r_viewcluster2;
extern	texture_t	*r_notexture_mip;

extern	texid_t	netgraphtexture;	// netgraph texture
extern	shader_t *netgraphshader;

extern	const char *gl_vendor;
extern	const char *gl_renderer;
extern	const char *gl_version;

qboolean R_CullBox (vec3_t mins, vec3_t maxs);
qboolean R_CullEntityBox(entity_t *e, vec3_t modmins, vec3_t modmaxs);
qboolean R_CullSphere (vec3_t origin, float radius);
void Sh_PreGenerateLights(void);
void Sh_PurgeShadowMeshes(void);

#ifdef GLQUAKE
void R_TranslatePlayerSkin (int playernum);
void GL_MTBind(int tmu, int target, texid_t texnum); /*use this if you're going to change the texture object (ensures glActiveTexture(tmu))*/
void GL_CullFace(unsigned int sflags);
void GL_TexEnv(GLenum mode);

// Multitexture
#define    GL_TEXTURE0_SGIS				0x835E
#define    GL_TEXTURE1_SGIS				0x835F


extern	int gl_stencilbits;


extern int gl_mtexarbable;	//max texture units
extern qboolean gl_mtexable;

extern int mtexid0;

extern qboolean gl_mtexable;

void GL_SelectTexture (int tmunum);
void GL_SetShaderState2D(qboolean is2d);
void GL_ForceDepthWritable(void);

#endif

//
// gl_draw.c
//
#ifdef GLQUAKE
texid_tf GL_LoadPicTexture (qpic_t *pic);
void GL_Set2D (unsigned int flags);
#endif

//
// gl_rmain.c
//
qboolean R_ShouldDraw(entity_t *e);
#ifdef GLQUAKE
void R_RotateForEntity (float *modelmatrix, float *modelviewmatrix, const entity_t *e, const model_t *mod);

void GL_ShutdownPostProcessing(void);
void GL_InitSceneProcessingShaders (void);
void GL_SetupSceneProcessingTextures (void);
#endif

//
// gl_alias.c
//
void R_DrawGAliasShadowVolume(entity_t *e, vec3_t lightpos, float radius);

#ifdef GLQUAKE
//misc model formats
void R_DrawHLModel(entity_t	*curent);
#endif

//
// gl_rlight.c
//
void R_GenDlightBatches(batch_t *batches[]);
void R_InitFlashblends(void);
#ifdef GLQUAKE
void GLR_MarkQ2Lights (dlight_t *light, int bit, mnode_t *node);
#endif
void GLQ3_LightGrid(model_t *mod, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir);
qboolean R_LoadRTLights(void);
qboolean R_ImportRTLights(const char *entlump, int importmode);

//doom
#ifdef MAP_DOOM
void R_DoomWorld();
#endif
#ifdef MAP_PROC
qboolean QDECL D3_LoadMap_CollisionMap(model_t *mod, void *buf, size_t bufsize);
#endif

//gl_bloom.c
#ifdef GLQUAKE
void R_BloomRegister(void);
qboolean R_CanBloom(void);
void R_BloomBlend (texid_t source, int x, int y, int w, int h);
void R_BloomShutdown(void);
#endif

//
// gl_ngraph.c
//
void R_NetGraph (void);


#if defined(GLQUAKE)

//updates the viewport correctly.
//pxrect.y is relative to the top.
//gl requires viewports specified relative to the bottom.
//so we need to do a little extra maths, which keeps confusing me, so one macro to ensure consistancy.
#define GL_ViewportUpdate() qglViewport(r_refdef.pxrect.x, r_refdef.pxrect.maxheight-(r_refdef.pxrect.y+r_refdef.pxrect.height), r_refdef.pxrect.width, r_refdef.pxrect.height)

#ifdef GL_STATIC
//these are the functions that are valid in gles2.
//other functions should never actually be used.
#define qglActiveTextureARB glActiveTexture
#define qglAttachShader glAttachShader
#define qglBindAttribLocation glBindAttribLocation
#define qglBindBuffer glBindBuffer
#define qglBindFramebuffer glBindFramebuffer
#define qglBindRenderbufferEXT glBindRenderbuffer
#define qglBindTexture glBindTexture
#define qglBlendColor glBlendColor
#define qglBlendEquation glBlendEquation
#define qglBlendEquationSeparate glBlendEquationSeparate
#define qglBlendFunc glBlendFunc
#define qglBlendFuncSeparate glBlendFuncSeparate
#define qglBufferData glBufferData
#define qglBufferSubData glBufferSubData
#define qglCheckFramebufferStatusEXT glCheckFramebufferStatus
#define qglClear glClear
#define qglClearColor glClearColor
#define qglClearDepthf glClearDepthf
#define qglClearStencil glClearStencil
#define qglColorMask glColorMask
#define qglCompileShader glCompileShader
#define qglCompressedTexImage2D glCompressedTexImage2D
#define qglCompressedTexSubImage2D glCompressedTexSubImage2D
#define qglCopyTexImage2D glCopyTexImage2D
#define qglCopyTexSubImage2D glCopyTexSubImage2D
#define qglCreateProgram glCreateProgram
#define qglCreateShader glCreateShader
#define qglCullFace glCullFace
#define qglDeleteBuffers glDeleteBuffers
#define qglDeleteFramebuffers glDeleteFramebuffers
#define qglDeleteProgram glDeleteProgram
#define qglDeleteRenderbuffers glDeleteRenderbuffers
#define qglDeleteShader glDeleteShader
#define qglDeleteTextures glDeleteTextures
#define qglDepthFunc glDepthFunc
#define qglDepthMask glDepthMask
#define qglDepthRangef glDepthRangef
#define qglDetachShader glDetachShader
#define qglDisable glDisable
#define qglDisableVertexAttribArray glDisableVertexAttribArray
#define qglDrawArrays glDrawArrays
#define qglDrawElements glDrawElements
#define qglEnable glEnable
#define qglEnableVertexAttribArray glEnableVertexAttribArray
#define qglFinish glFinish
#define qglFlush glFlush
#define qglFramebufferRenderbufferEXT glFramebufferRenderbuffer
#define qglFramebufferTexture2D glFramebufferTexture2D
#define qglFrontFace glFrontFace
#define qglGenBuffers glGenBuffers
#define qglGenerateMipmap glGenerateMipmap
#define qglGenFramebuffers glGenFramebuffers
#define qglGenRenderbuffersEXT glGenRenderbuffers
#define qglGenTextures glGenTextures
#define qglGetActiveAttrib glGetActiveAttrib
#define qglGetActiveUniform glGetActiveUniform
#define qglGetAttachedShaders glGetAttachedShaders
#define qglGetAttribLocation glGetAttribLocation
#define qglGetBooleanv glGetBooleanv
#define qglGetBufferParameteriv glGetBufferParameteriv
#define qglGetError glGetError
#define qglGetFloatv glGetFloatv
#define qglGetFramebufferAttachmentParameteriv glGetFramebufferAttachmentParameteriv
#define qglGetIntegerv glGetIntegerv
#define qglGetProgramiv glGetProgramiv
#define qglGetProgramInfoLog glGetProgramInfoLog
#define qglGetRenderbufferParameteriv glGetRenderbufferParameteriv
#define qglGetShaderiv glGetShaderiv
#define qglGetShaderInfoLog glGetShaderInfoLog
#define qglGetShaderPrecisionFormat glGetShaderPrecisionFormat
#define qglGetShaderSource glGetShaderSource
#define qglGetString glGetString
#define qglGetTexParameterfv glGetTexParameterfv
#define qglGetTexParameteriv glGetTexParameteriv
#define qglGetUniformfv glGetUniformfv
#define qglGetUniformiv glGetUniformiv
#define qglGetUniformLocation glGetUniformLocation
#define qglGetVertexAttribfv glGetVertexAttribfv
#define qglGetVertexAttribiv glGetVertexAttribiv
#define qglGetVertexAttribPointerv glGetVertexAttribPointerv
#define qglHint glHint
#define qglIsBuffer glIsBuffer
#define qglIsEnabled glIsEnabled
#define qglIsFramebuffer glIsFramebuffer
#define qglIsProgram glIsProgram
#define qglIsRenderbuffer glIsRenderbuffer
#define qglIsShader glIsShader
#define qglIsTexture glIsTexture
#define qglLineWidth glLineWidth
#define qglLinkProgram glLinkProgram
#define qglPixelStorei glPixelStorei
#define qglPolygonOffset glPolygonOffset
#define qglReadPixels glReadPixels
#define qglReleaseShaderCompiler glReleaseShaderCompiler
#define qglRenderbufferStorageEXT glRenderbufferStorage
#define qglSampleCoverage glSampleCoverage
#define qglScissor glScissor
#define qglShaderBinary glShaderBinary
#define qglShaderSource glShaderSource
#define qglStencilFunc glStencilFunc
#define qglStencilFuncSeparate glStencilFuncSeparate
#define qglStencilMask glStencilMask
#define qglStencilMaskSeparate glStencilMaskSeparate
#define qglStencilOp glStencilOp
#define qglStencilOpSeparate glStencilOpSeparate
#define qglTexImage2D glTexImage2D
#define qglTexParameterf glTexParameterf
#define qglTexParameterfv glTexParameterfv
#define qglTexParameteri glTexParameteri
#define qglTexParameteriv glTexParameteriv
#define qglTexSubImage2D glTexSubImage2D
#define qglUniform1f glUniform1f
#define qglUniform1fv glUniform1fv
#define qglUniform1i glUniform1i
#define qglUniform1iv glUniform1iv
#define qglUniform2f glUniform2f
#define qglUniform2fv glUniform2fv
#define qglUniform2i glUniform2i
#define qglUniform2iv glUniform2iv
#define qglUniform3f glUniform3f
#define qglUniform3fv glUniform3fv
#define qglUniform3i glUniform3i
#define qglUniform3iv glUniform3iv
#define qglUniform4f glUniform4f
#define qglUniform4fv glUniform4fv
#define qglUniform4i glUniform4i
#define qglUniform4iv glUniform4iv
#define qglUniformMatrix2fv glUniformMatrix2fv
#define qglUniformMatrix3fv glUniformMatrix3fv
#define qglUniformMatrix4fv glUniformMatrix4fv
#define qglUseProgram glUseProgram
#define qglValidateProgram glValidateProgram
#define qglVertexAttrib1f glVertexAttrib1f
#define qglVertexAttrib1fv glVertexAttrib1fv
#define qglVertexAttrib2f glVertexAttrib2f
#define qglVertexAttrib2fv glVertexAttrib2fv
#define qglVertexAttrib3f glVertexAttrib3f
#define qglVertexAttrib3fv glVertexAttrib3fv
#define qglVertexAttrib4f glVertexAttrib4f
#define qglVertexAttrib4fv glVertexAttrib4fv
#define qglVertexAttribPointer glVertexAttribPointer
#define qglViewport glViewport

#define qglGenFramebuffersEXT qglGenFramebuffers
#define qglDeleteFramebuffersEXT qglDeleteFramebuffers
#define qglBindFramebufferEXT qglBindFramebuffer
#define qglFramebufferTexture2DEXT qglFramebufferTexture2D
#define qglDeleteRenderbuffersEXT qglDeleteRenderbuffers
//#define qglCompressedTexImage2DARB qglCompressedTexImage2D

#define qglCreateProgramObjectARB	glCreateProgram
#define qglDeleteProgramObject_		glDeleteProgram
#define qglDeleteShaderObject_		glDeleteShader
#define qglUseProgramObjectARB		glUseProgram
#define qglCreateShaderObjectARB	glCreateShader
#define qglShaderSourceARB		glShaderSource
#define qglCompileShaderARB		glCompileShader
#define qglGetProgramParameteriv_	glGetProgramiv
#define qglGetShaderParameteriv_	glGetShaderiv
#define qglAttachObjectARB		glAttachShader
#define qglGetProgramInfoLog_		glGetProgramInfoLog
#define qglGetShaderInfoLog_		glGetShaderInfoLog
#define qglLinkProgramARB		glLinkProgram
#define qglBindAttribLocationARB	glBindAttribLocation
#define qglGetAttribLocationARB		glGetAttribLocation
#define qglGetUniformLocationARB	glGetUniformLocation
#define qglUniformMatrix4fvARB		glUniformMatrix4fv
#define qglUniformMatrix3fvARB		glUniformMatrix3fv
#define qglUniform4fARB			glUniform4f
#define qglUniform4fvARB		glUniform4fv
#define qglUniform3fARB			glUniform3f
#define qglUniform3fvARB		glUniform3fv
#define qglUniform2fvARB		glUniform2fv
#define qglUniform1iARB			glUniform1i
#define qglUniform1fARB			glUniform1f

#define qglGenBuffersARB		glGenBuffers
#define qglDeleteBuffersARB		glDeleteBuffers
#define qglBindBufferARB		glBindBuffer
#define qglBufferDataARB		glBufferData
#define qglBufferSubDataARB		glBufferSubData

#else
extern void (APIENTRY *qglBindTexture) (GLenum target, GLuint texture);
extern void (APIENTRY *qglBlendFunc) (GLenum sfactor, GLenum dfactor);
extern void (APIENTRY *qglClear) (GLbitfield mask);
extern void (APIENTRY *qglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void (APIENTRY *qglClearDepthf) (GLclampf depth);
extern void (APIENTRY *qglClearStencil) (GLint s);
extern void (APIENTRY *qglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void (APIENTRY *qglCopyTexImage2D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void (APIENTRY *qglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRY *qglCullFace) (GLenum mode);
extern void (APIENTRY *qglDeleteTextures) (GLsizei n, const GLuint *textures);
extern void (APIENTRY *qglDepthFunc) (GLenum func);
extern void (APIENTRY *qglDepthMask) (GLboolean flag);
extern void (APIENTRY *qglDepthRangef) (GLclampf zNear, GLclampf zFar);
extern void (APIENTRY *qglDisable) (GLenum cap);
extern void (APIENTRY *qglDrawArrays) (GLenum mode, GLint first, GLsizei count);
extern void (APIENTRY *qglDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void (APIENTRY *qglEnable) (GLenum cap);
extern void (APIENTRY *qglFinish) (void);
extern void (APIENTRY *qglFlush) (void);
extern void (APIENTRY *qglFrontFace) (GLenum mode);
extern void (APIENTRY *qglGenTextures) (GLsizei n, GLuint *textures);
extern void (APIENTRY *qglGenerateMipmap)(GLenum target);
extern void (APIENTRY *qglGetBooleanv) (GLenum pname, GLboolean *params);
extern GLenum (APIENTRY *qglGetError) (void);
extern void (APIENTRY *qglGetFloatv) (GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetIntegerv) (GLenum pname, GLint *params);
extern const GLubyte * (APIENTRY *qglGetString) (GLenum name);
extern void (APIENTRY *qglGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetTexParameteriv) (GLenum target, GLenum pname, GLint *params);
extern void (APIENTRY *qglHint) (GLenum target, GLenum mode);
extern GLboolean (APIENTRY *qglIsEnabled) (GLenum cap);
extern GLboolean (APIENTRY *qglIsTexture) (GLuint texture);
extern void (APIENTRY *qglLineWidth) (GLfloat width);
extern void (APIENTRY *qglPixelStorei) (GLenum pname, GLint param);
extern void (APIENTRY *qglPolygonOffset) (GLfloat factor, GLfloat units);
extern void (APIENTRY *qglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void (APIENTRY *qglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRY *qglStencilFunc) (GLenum func, GLint ref, GLuint mask);
extern void (APIENTRY *qglStencilMask) (GLuint mask);
extern void (APIENTRY *qglStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
extern void (APIENTRY *qglTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *qglTexParameterf) (GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRY *qglTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglTexParameteri) (GLenum target, GLenum pname, GLint param);
extern void (APIENTRY *qglTexParameteriv) (GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRY *qglTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *qglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);
extern FTEPFNGLCOMPRESSEDTEXIMAGE2DARBPROC qglCompressedTexImage2D;
extern void (APIENTRY *qglCompressedTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);	//gl1.3

extern void (APIENTRY *qglGenFramebuffersEXT)(GLsizei n, GLuint* ids);
extern void (APIENTRY *qglDeleteFramebuffersEXT)(GLsizei n, const GLuint* ids);
extern void (APIENTRY *qglBindFramebufferEXT)(GLenum target, GLuint id);
extern void (APIENTRY *qglGenRenderbuffersEXT)(GLsizei n, GLuint* ids);
extern void (APIENTRY *qglDeleteRenderbuffersEXT)(GLsizei n, const GLuint* ids);
extern void (APIENTRY *qglBindRenderbufferEXT)(GLenum target, GLuint id);
extern void (APIENTRY *qglRenderbufferStorageEXT)(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);
extern void (APIENTRY *qglFramebufferTexture2DEXT)(GLenum target, GLenum attachmentPoint, GLenum textureTarget, GLuint textureId, GLint  level);
extern void (APIENTRY *qglFramebufferRenderbufferEXT)(GLenum target, GLenum attachmentPoint, GLenum textureTarget, GLuint textureId);
extern GLenum (APIENTRY *qglCheckFramebufferStatusEXT)(GLenum target);
extern void (APIENTRY *qglGetFramebufferAttachmentParameteriv)(GLenum  target,  GLenum  attachment,  GLenum  pname,  GLint * params);

//glslang - arb_shader_objects
extern FTEPFNGLCREATEPROGRAMPROC			qglCreateProgramObjectARB;
extern FTEPFNGLDELETEPROGRAMPROC			qglDeleteProgramObject_;
extern FTEPFNGLDELETESHADERPROC				qglDeleteShaderObject_;
extern FTEPFNGLUSEPROGRAMPROC				qglUseProgramObjectARB;
extern FTEPFNGLCREATESHADERPROC				qglCreateShaderObjectARB;
extern FTEPFNGLSHADERSOURCEPROC				qglShaderSourceARB;
extern FTEPFNGLCOMPILESHADERPROC			qglCompileShaderARB;
extern FTEPFNGLGETPROGRAMIVPROC				qglGetProgramParameteriv_;
extern FTEPFNGLGETSHADERIVPROC				qglGetShaderParameteriv_;
extern FTEPFNGLATTACHSHADERPROC				qglAttachObjectARB;
extern FTEPFNGLGETPROGRAMINFOLOGPROC		qglGetProgramInfoLog_;
extern FTEPFNGLGETSHADERINFOLOGPROC			qglGetShaderInfoLog_;
extern FTEPFNGLLINKPROGRAMPROC				qglLinkProgramARB;
extern FTEPFNGLBINDATTRIBLOCATIONPROC		qglBindAttribLocationARB;
extern FTEPFNGLGETATTRIBLOCATIONPROC		qglGetAttribLocationARB;
extern FTEPFNGLGETUNIFORMLOCATIONPROC		qglGetUniformLocationARB;
extern FTEPFNGLUNIFORMMATRIX4FVPROC			qglUniformMatrix4fvARB;
extern FTEPFNGLUNIFORMMATRIX3FVPROC			qglUniformMatrix3fvARB;
extern FTEPFNGLUNIFORMMATRIX4X3FVPROC		qglUniformMatrix4x3fvARB;	//gl2.1+
extern FTEPFNGLUNIFORMMATRIX3X4FVPROC		qglUniformMatrix3x4fvARB;	//gl2.1+
extern FTEPFNGLUNIFORM4FPROC				qglUniform4fARB;
extern FTEPFNGLUNIFORM4FVPROC				qglUniform4fvARB;
extern FTEPFNGLUNIFORM3FPROC				qglUniform3fARB;
extern FTEPFNGLUNIFORM3FVPROC				qglUniform3fvARB;
extern FTEPFNGLUNIFORM2FVPROC				qglUniform2fvARB;
extern FTEPFNGLUNIFORM1IPROC				qglUniform1iARB;
extern FTEPFNGLUNIFORM1FPROC				qglUniform1fARB;
extern FTEPFNGLVERTEXATTRIB4FPROC			qglVertexAttrib4f;
extern FTEPFNGLVERTEXATTRIBPOINTERPROC		qglVertexAttribPointer;
extern FTEPFNGLGETVERTEXATTRIBIVPROC		qglGetVertexAttribiv;
extern FTEPFNGLENABLEVERTEXATTRIBARRAYPROC	qglEnableVertexAttribArray;
extern FTEPFNGLDISABLEVERTEXATTRIBARRAYPROC	qglDisableVertexAttribArray;

extern void (APIENTRY *qglGenBuffersARB)(GLsizei n, GLuint* ids);
extern void (APIENTRY *qglDeleteBuffersARB)(GLsizei n, GLuint* ids);
extern void (APIENTRY *qglBindBufferARB)(GLenum target, GLuint id);
extern void (APIENTRY *qglBufferDataARB)(GLenum target, GLsizei size, const void* data, GLenum usage);
extern void (APIENTRY *qglBufferSubDataARB)(GLenum target, GLint offset, GLsizei size, void* data);
#endif

#define GLintptr qintptr_t
#define GLsizeiptr quintptr_t
#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT 0x0001
#endif
#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif
#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT 0x0080
#endif
extern void *(APIENTRY *qglMapBufferARB)(GLenum target, GLenum access);
extern GLboolean (APIENTRY *qglUnmapBufferARB)(GLenum target);
extern void *(APIENTRY *qglMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);	//gl3.0
extern void (APIENTRY *qglBufferStorage)(GLenum target, GLsizeiptr size, const GLvoid *data, GLbitfield flags);		//gl4.4

extern void (APIENTRY *qglGenQueriesARB)(GLsizei n, GLuint *ids);
extern void (APIENTRY *qglDeleteQueriesARB)(GLsizei n, const GLuint *ids);
//extern GLboolean (APIENTRY *qglIsQueryARB)(GLuint id);
extern void (APIENTRY *qglBeginQueryARB)(GLenum target, GLuint id);
extern void (APIENTRY *qglEndQueryARB)(GLenum target);
//extern void (APIENTRY *qglGetQueryivARB)(GLenum target, GLenum pname, GLint *params);
//extern void (APIENTRY *qglGetQueryObjectivARB)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRY *qglGetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint *params);

extern void (APIENTRY *qglDrawBuffers)(GLsizei n, GLsizei *ids);	//gl2

extern GLenum (APIENTRY *qglGetGraphicsResetStatus) (void);

//non-gles2 gl functions
extern void (APIENTRY *qglAccum) (GLenum op, GLfloat value);
extern void (APIENTRY *qglAlphaFunc) (GLenum func, GLclampf ref);
extern GLboolean (APIENTRY *qglAreTexturesResident) (GLsizei n, const GLuint *textures, GLboolean *residences);
extern void (APIENTRY *qglArrayElement) (GLint i);
extern void (APIENTRY *qglBitmap) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern void (APIENTRY *qglCallList) (GLuint list);
extern void (APIENTRY *qglCallLists) (GLsizei n, GLenum type, const GLvoid *lists);
extern void (APIENTRY *qglClearAccum) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRY *qglClearDepth) (GLclampd depth);
extern void (APIENTRY *qglClearIndex) (GLfloat c);
extern void (APIENTRY *qglClipPlane) (GLenum plane, const GLdouble *equation);
extern void (APIENTRY *qglColor3b) (GLbyte red, GLbyte green, GLbyte blue);
extern void (APIENTRY *qglColor3bv) (const GLbyte *v);
extern void (APIENTRY *qglColor3d) (GLdouble red, GLdouble green, GLdouble blue);
extern void (APIENTRY *qglColor3dv) (const GLdouble *v);
extern void (APIENTRY *qglColor3f) (GLfloat red, GLfloat green, GLfloat blue);
extern void (APIENTRY *qglColor3fv) (const GLfloat *v);
extern void (APIENTRY *qglColor3i) (GLint red, GLint green, GLint blue);
extern void (APIENTRY *qglColor3iv) (const GLint *v);
extern void (APIENTRY *qglColor3s) (GLshort red, GLshort green, GLshort blue);
extern void (APIENTRY *qglColor3sv) (const GLshort *v);
extern void (APIENTRY *qglColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
extern void (APIENTRY *qglColor3ubv) (const GLubyte *v);
extern void (APIENTRY *qglColor3ui) (GLuint red, GLuint green, GLuint blue);
extern void (APIENTRY *qglColor3uiv) (const GLuint *v);
extern void (APIENTRY *qglColor3us) (GLushort red, GLushort green, GLushort blue);
extern void (APIENTRY *qglColor3usv) (const GLushort *v);
extern void (APIENTRY *qglColor4b) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern void (APIENTRY *qglColor4bv) (const GLbyte *v);
extern void (APIENTRY *qglColor4d) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern void (APIENTRY *qglColor4dv) (const GLdouble *v);
extern void (APIENTRY *qglColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRY *qglColor4fv) (const GLfloat *v);
extern void (APIENTRY *qglColor4i) (GLint red, GLint green, GLint blue, GLint alpha);
extern void (APIENTRY *qglColor4iv) (const GLint *v);
extern void (APIENTRY *qglColor4s) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern void (APIENTRY *qglColor4sv) (const GLshort *v);
extern void (APIENTRY *qglColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void (APIENTRY *qglColor4ubv) (const GLubyte *v);
extern void (APIENTRY *qglColor4ui) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern void (APIENTRY *qglColor4uiv) (const GLuint *v);
extern void (APIENTRY *qglColor4us) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern void (APIENTRY *qglColor4usv) (const GLushort *v);
extern void (APIENTRY *qglColorMaterial) (GLenum face, GLenum mode);
extern void (APIENTRY *qglColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void (APIENTRY *qglCopyPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern void (APIENTRY *qglCopyTexImage1D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern void (APIENTRY *qglCopyTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void (APIENTRY *qglDeleteLists) (GLuint list, GLsizei range);
extern void (APIENTRY *qglDepthRange) (GLclampd zNear, GLclampd zFar);
extern void (APIENTRY *qglDisableClientState) (GLenum array);
extern void (APIENTRY *qglDrawBuffer) (GLenum mode);
extern void (APIENTRY *qglDrawPixels) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *qglEdgeFlag) (GLboolean flag);
extern void (APIENTRY *qglEdgeFlagPointer) (GLsizei stride, const GLvoid *pointer);
extern void (APIENTRY *qglEdgeFlagv) (const GLboolean *flag);
extern void (APIENTRY *qglEndList) (void);
extern void (APIENTRY *qglEvalCoord1d) (GLdouble u);
extern void (APIENTRY *qglEvalCoord1dv) (const GLdouble *u);
extern void (APIENTRY *qglEvalCoord1f) (GLfloat u);
extern void (APIENTRY *qglEvalCoord1fv) (const GLfloat *u);
extern void (APIENTRY *qglEvalCoord2d) (GLdouble u, GLdouble v);
extern void (APIENTRY *qglEvalCoord2dv) (const GLdouble *u);
extern void (APIENTRY *qglEvalCoord2f) (GLfloat u, GLfloat v);
extern void (APIENTRY *qglEvalCoord2fv) (const GLfloat *u);
extern void (APIENTRY *qglEvalMesh1) (GLenum mode, GLint i1, GLint i2);
extern void (APIENTRY *qglEvalMesh2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern void (APIENTRY *qglEvalPoint1) (GLint i);
extern void (APIENTRY *qglEvalPoint2) (GLint i, GLint j);
extern void (APIENTRY *qglFeedbackBuffer) (GLsizei size, GLenum type, GLfloat *buffer);
extern void (APIENTRY *qglFogf) (GLenum pname, GLfloat param);
extern void (APIENTRY *qglFogfv) (GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglFogi) (GLenum pname, GLint param);
extern void (APIENTRY *qglFogiv) (GLenum pname, const GLint *params);
extern void (APIENTRY *qglFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern GLuint (APIENTRY *qglGenLists) (GLsizei range);
extern void (APIENTRY *qglGetClipPlane) (GLenum plane, GLdouble *equation);
extern void (APIENTRY *qglGetDoublev) (GLenum pname, GLdouble *params);
extern void (APIENTRY *qglGetLightfv) (GLenum light, GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetLightiv) (GLenum light, GLenum pname, GLint *params);
extern void (APIENTRY *qglGetMapdv) (GLenum target, GLenum query, GLdouble *v);
extern void (APIENTRY *qglGetMapfv) (GLenum target, GLenum query, GLfloat *v);
extern void (APIENTRY *qglGetMapiv) (GLenum target, GLenum query, GLint *v);
extern void (APIENTRY *qglGetMaterialfv) (GLenum face, GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetMaterialiv) (GLenum face, GLenum pname, GLint *params);
extern void (APIENTRY *qglGetPixelMapfv) (GLenum map, GLfloat *values);
extern void (APIENTRY *qglGetPixelMapuiv) (GLenum map, GLuint *values);
extern void (APIENTRY *qglGetPixelMapusv) (GLenum map, GLushort *values);
extern void (APIENTRY *qglGetPointerv) (GLenum pname, GLvoid* *params);
extern void (APIENTRY *qglGetPolygonStipple) (GLubyte *mask);
extern void (APIENTRY *qglGetTexEnvfv) (GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetTexEnviv) (GLenum target, GLenum pname, GLint *params);
extern void (APIENTRY *qglGetTexGendv) (GLenum coord, GLenum pname, GLdouble *params);
extern void (APIENTRY *qglGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetTexGeniv) (GLenum coord, GLenum pname, GLint *params);
extern void (APIENTRY *qglGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void (APIENTRY *qglGetTexLevelParameterfv) (GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void (APIENTRY *qglGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
extern void (APIENTRY *qglIndexMask) (GLuint mask);
extern void (APIENTRY *qglIndexPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
extern void (APIENTRY *qglIndexd) (GLdouble c);
extern void (APIENTRY *qglIndexdv) (const GLdouble *c);
extern void (APIENTRY *qglIndexf) (GLfloat c);
extern void (APIENTRY *qglIndexfv) (const GLfloat *c);
extern void (APIENTRY *qglIndexi) (GLint c);
extern void (APIENTRY *qglIndexiv) (const GLint *c);
extern void (APIENTRY *qglIndexs) (GLshort c);
extern void (APIENTRY *qglIndexsv) (const GLshort *c);
extern void (APIENTRY *qglIndexub) (GLubyte c);
extern void (APIENTRY *qglIndexubv) (const GLubyte *c);
extern void (APIENTRY *qglInitNames) (void);
extern void (APIENTRY *qglInterleavedArrays) (GLenum format, GLsizei stride, const GLvoid *pointer);
extern GLboolean (APIENTRY *qglIsList) (GLuint list);
extern void (APIENTRY *qglLightModelf) (GLenum pname, GLfloat param);
extern void (APIENTRY *qglLightModelfv) (GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglLightModeli) (GLenum pname, GLint param);
extern void (APIENTRY *qglLightModeliv) (GLenum pname, const GLint *params);
extern void (APIENTRY *qglLightf) (GLenum light, GLenum pname, GLfloat param);
extern void (APIENTRY *qglLightfv) (GLenum light, GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglLighti) (GLenum light, GLenum pname, GLint param);
extern void (APIENTRY *qglLightiv) (GLenum light, GLenum pname, const GLint *params);
extern void (APIENTRY *qglLineStipple) (GLint factor, GLushort pattern);
extern void (APIENTRY *qglListBase) (GLuint base);
extern void (APIENTRY *qglLoadIdentity) (void);
extern void (APIENTRY *qglLoadMatrixd) (const GLdouble *m);
extern void (APIENTRY *qglLoadMatrixf) (const GLfloat *m);
extern void (APIENTRY *qglLoadName) (GLuint name);
extern void (APIENTRY *qglLogicOp) (GLenum opcode);
extern void (APIENTRY *qglMap1d) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void (APIENTRY *qglMap1f) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void (APIENTRY *qglMap2d) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void (APIENTRY *qglMap2f) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern void (APIENTRY *qglMapGrid1d) (GLint un, GLdouble u1, GLdouble u2);
extern void (APIENTRY *qglMapGrid1f) (GLint un, GLfloat u1, GLfloat u2);
extern void (APIENTRY *qglMapGrid2d) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern void (APIENTRY *qglMapGrid2f) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern void (APIENTRY *qglMaterialf) (GLenum face, GLenum pname, GLfloat param);
extern void (APIENTRY *qglMaterialfv) (GLenum face, GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglMateriali) (GLenum face, GLenum pname, GLint param);
extern void (APIENTRY *qglMaterialiv) (GLenum face, GLenum pname, const GLint *params);
extern void (APIENTRY *qglMatrixMode) (GLenum mode);
extern void (APIENTRY *qglMultMatrixd) (const GLdouble *m);
extern void (APIENTRY *qglMultMatrixf) (const GLfloat *m);
extern void (APIENTRY *qglNewList) (GLuint list, GLenum mode);
extern void (APIENTRY *qglNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz);
extern void (APIENTRY *qglNormal3bv) (const GLbyte *v);
extern void (APIENTRY *qglNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz);
extern void (APIENTRY *qglNormal3dv) (const GLdouble *v);
extern void (APIENTRY *qglNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
extern void (APIENTRY *qglNormal3fv) (const GLfloat *v);
extern void (APIENTRY *qglNormal3i) (GLint nx, GLint ny, GLint nz);
extern void (APIENTRY *qglNormal3iv) (const GLint *v);
extern void (APIENTRY *qglNormal3s) (GLshort nx, GLshort ny, GLshort nz);
extern void (APIENTRY *qglNormal3sv) (const GLshort *v);
extern void (APIENTRY *qglNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
//extern void (APIENTRY *qglOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void (APIENTRY *qglPassThrough) (GLfloat token);
extern void (APIENTRY *qglPixelMapfv) (GLenum map, GLsizei mapsize, const GLfloat *values);
extern void (APIENTRY *qglPixelMapuiv) (GLenum map, GLsizei mapsize, const GLuint *values);
extern void (APIENTRY *qglPixelMapusv) (GLenum map, GLsizei mapsize, const GLushort *values);
extern void (APIENTRY *qglPixelStoref) (GLenum pname, GLfloat param);
extern void (APIENTRY *qglPixelTransferf) (GLenum pname, GLfloat param);
extern void (APIENTRY *qglPixelTransferi) (GLenum pname, GLint param);
extern void (APIENTRY *qglPixelZoom) (GLfloat xfactor, GLfloat yfactor);
extern void (APIENTRY *qglPointSize) (GLfloat size);
extern void (APIENTRY *qglPolygonMode) (GLenum face, GLenum mode);
extern void (APIENTRY *qglPolygonStipple) (const GLubyte *mask);
extern void (APIENTRY *qglPopAttrib) (void);
extern void (APIENTRY *qglPopClientAttrib) (void);
extern void (APIENTRY *qglPopMatrix) (void);
extern void (APIENTRY *qglPopName) (void);
extern void (APIENTRY *qglPrioritizeTextures) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern void (APIENTRY *qglPushAttrib) (GLbitfield mask);
extern void (APIENTRY *qglPushClientAttrib) (GLbitfield mask);
extern void (APIENTRY *qglPushMatrix) (void);
extern void (APIENTRY *qglPushName) (GLuint name);
extern void (APIENTRY *qglRasterPos2d) (GLdouble x, GLdouble y);
extern void (APIENTRY *qglRasterPos2dv) (const GLdouble *v);
extern void (APIENTRY *qglRasterPos2f) (GLfloat x, GLfloat y);
extern void (APIENTRY *qglRasterPos2fv) (const GLfloat *v);
extern void (APIENTRY *qglRasterPos2i) (GLint x, GLint y);
extern void (APIENTRY *qglRasterPos2iv) (const GLint *v);
extern void (APIENTRY *qglRasterPos2s) (GLshort x, GLshort y);
extern void (APIENTRY *qglRasterPos2sv) (const GLshort *v);
extern void (APIENTRY *qglRasterPos3d) (GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRY *qglRasterPos3dv) (const GLdouble *v);
extern void (APIENTRY *qglRasterPos3f) (GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *qglRasterPos3fv) (const GLfloat *v);
extern void (APIENTRY *qglRasterPos3i) (GLint x, GLint y, GLint z);
extern void (APIENTRY *qglRasterPos3iv) (const GLint *v);
extern void (APIENTRY *qglRasterPos3s) (GLshort x, GLshort y, GLshort z);
extern void (APIENTRY *qglRasterPos3sv) (const GLshort *v);
extern void (APIENTRY *qglRasterPos4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRY *qglRasterPos4dv) (const GLdouble *v);
extern void (APIENTRY *qglRasterPos4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRY *qglRasterPos4fv) (const GLfloat *v);
extern void (APIENTRY *qglRasterPos4i) (GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRY *qglRasterPos4iv) (const GLint *v);
extern void (APIENTRY *qglRasterPos4s) (GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRY *qglRasterPos4sv) (const GLshort *v);
extern void (APIENTRY *qglRectd) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern void (APIENTRY *qglRectdv) (const GLdouble *v1, const GLdouble *v2);
extern void (APIENTRY *qglRectf) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern void (APIENTRY *qglRectfv) (const GLfloat *v1, const GLfloat *v2);
extern void (APIENTRY *qglRecti) (GLint x1, GLint y1, GLint x2, GLint y2);
extern void (APIENTRY *qglRectiv) (const GLint *v1, const GLint *v2);
extern void (APIENTRY *qglRects) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern void (APIENTRY *qglRectsv) (const GLshort *v1, const GLshort *v2);
extern GLint (APIENTRY *qglRenderMode) (GLenum mode);
extern void (APIENTRY *qglRotated) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRY *qglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *qglScaled) (GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRY *qglScalef) (GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *qglSelectBuffer) (GLsizei size, GLuint *buffer);
extern void (APIENTRY *qglShadeModel) (GLenum mode);
extern void (APIENTRY *qglTexCoord1d) (GLdouble s);
extern void (APIENTRY *qglTexCoord1dv) (const GLdouble *v);
extern void (APIENTRY *qglTexCoord1f) (GLfloat s);
extern void (APIENTRY *qglTexCoord1fv) (const GLfloat *v);
extern void (APIENTRY *qglTexCoord1i) (GLint s);
extern void (APIENTRY *qglTexCoord1iv) (const GLint *v);
extern void (APIENTRY *qglTexCoord1s) (GLshort s);
extern void (APIENTRY *qglTexCoord1sv) (const GLshort *v);
extern void (APIENTRY *qglTexCoord2d) (GLdouble s, GLdouble t);
extern void (APIENTRY *qglTexCoord2dv) (const GLdouble *v);
extern void (APIENTRY *qglTexCoord2f) (GLfloat s, GLfloat t);
extern void (APIENTRY *qglTexCoord2fv) (const GLfloat *v);
extern void (APIENTRY *qglTexCoord2i) (GLint s, GLint t);
extern void (APIENTRY *qglTexCoord2iv) (const GLint *v);
extern void (APIENTRY *qglTexCoord2s) (GLshort s, GLshort t);
extern void (APIENTRY *qglTexCoord2sv) (const GLshort *v);
extern void (APIENTRY *qglTexCoord3d) (GLdouble s, GLdouble t, GLdouble r);
extern void (APIENTRY *qglTexCoord3dv) (const GLdouble *v);
extern void (APIENTRY *qglTexCoord3f) (GLfloat s, GLfloat t, GLfloat r);
extern void (APIENTRY *qglTexCoord3fv) (const GLfloat *v);
extern void (APIENTRY *qglTexCoord3i) (GLint s, GLint t, GLint r);
extern void (APIENTRY *qglTexCoord3iv) (const GLint *v);
extern void (APIENTRY *qglTexCoord3s) (GLshort s, GLshort t, GLshort r);
extern void (APIENTRY *qglTexCoord3sv) (const GLshort *v);
extern void (APIENTRY *qglTexCoord4d) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void (APIENTRY *qglTexCoord4dv) (const GLdouble *v);
extern void (APIENTRY *qglTexCoord4f) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void (APIENTRY *qglTexCoord4fv) (const GLfloat *v);
extern void (APIENTRY *qglTexCoord4i) (GLint s, GLint t, GLint r, GLint q);
extern void (APIENTRY *qglTexCoord4iv) (const GLint *v);
extern void (APIENTRY *qglTexCoord4s) (GLshort s, GLshort t, GLshort r, GLshort q);
extern void (APIENTRY *qglTexCoord4sv) (const GLshort *v);
extern void (APIENTRY *qglTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void (APIENTRY *qglTexEnvf) (GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRY *qglTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglTexEnvi) (GLenum target, GLenum pname, GLint param);
extern void (APIENTRY *qglTexEnviv) (GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRY *qglTexGend) (GLenum coord, GLenum pname, GLdouble param);
extern void (APIENTRY *qglTexGendv) (GLenum coord, GLenum pname, const GLdouble *params);
extern void (APIENTRY *qglTexGenf) (GLenum coord, GLenum pname, GLfloat param);
extern void (APIENTRY *qglTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params);
extern void (APIENTRY *qglTexGeni) (GLenum coord, GLenum pname, GLint param);
extern void (APIENTRY *qglTexGeniv) (GLenum coord, GLenum pname, const GLint *params);
extern void (APIENTRY *qglTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *qglTexImage3D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *qglTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *qglTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRY *qglTranslated) (GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRY *qglTranslatef) (GLfloat x, GLfloat y, GLfloat z);

extern FTEPFNGLUNIFORMMATRIX4X3FVPROC		qglUniformMatrix4x3fv;
extern FTEPFNGLUNIFORMMATRIX3X4FVPROC		qglUniformMatrix3x4fv;

extern FTEPFNGLCOMPRESSEDTEXIMAGE3DARBPROC qglCompressedTexImage3D;
extern void (APIENTRY *qglCompressedTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);	//gl1.3
extern FTEPFNGLGETCOMPRESSEDTEXIMAGEARBPROC qglGetCompressedTexImage;

extern const GLubyte * (APIENTRY * qglGetStringi) (GLenum name, GLuint index);

/*
extern qboolean gl_arb_fragment_program;
extern PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB;
extern PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB;
extern PFNGLBINDPROGRAMARBPROC qglBindProgramARB;
extern PFNGLGENPROGRAMSARBPROC qglGenProgramsARB;
*/

extern FTEPFNGLLOCKARRAYSEXTPROC qglLockArraysEXT;
extern FTEPFNGLUNLOCKARRAYSEXTPROC qglUnlockArraysEXT;

typedef void (APIENTRY *lpSelTexFUNC) (GLenum en);
extern lpSelTexFUNC qglSelectTextureSGIS;

//these functions are not available in gles2, for one reason or another
extern void (APIENTRY *qglBegin) (GLenum mode);
extern void (APIENTRY *qglVertex2d) (GLdouble x, GLdouble y);
extern void (APIENTRY *qglVertex2dv) (const GLdouble *v);
extern void (APIENTRY *qglVertex2f) (GLfloat x, GLfloat y);
extern void (APIENTRY *qglVertex2fv) (const GLfloat *v);
extern void (APIENTRY *qglVertex2i) (GLint x, GLint y);
extern void (APIENTRY *qglVertex2iv) (const GLint *v);
extern void (APIENTRY *qglVertex2s) (GLshort x, GLshort y);
extern void (APIENTRY *qglVertex2sv) (const GLshort *v);
extern void (APIENTRY *qglVertex3d) (GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRY *qglVertex3dv) (const GLdouble *v);
extern void (APIENTRY *qglVertex3f) (GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *qglVertex3fv) (const GLfloat *v);
extern void (APIENTRY *qglVertex3i) (GLint x, GLint y, GLint z);
extern void (APIENTRY *qglVertex3iv) (const GLint *v);
extern void (APIENTRY *qglVertex3s) (GLshort x, GLshort y, GLshort z);
extern void (APIENTRY *qglVertex3sv) (const GLshort *v);
extern void (APIENTRY *qglVertex4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRY *qglVertex4dv) (const GLdouble *v);
extern void (APIENTRY *qglVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRY *qglVertex4fv) (const GLfloat *v);
extern void (APIENTRY *qglVertex4i) (GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRY *qglVertex4iv) (const GLint *v);
extern void (APIENTRY *qglVertex4s) (GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRY *qglVertex4sv) (const GLshort *v);
extern void (APIENTRY *qglEnd) (void);
extern void (APIENTRY *qglReadBuffer) (GLenum mode);

//misc extensions
extern FTEPFNGLACTIVESTENCILFACEEXTPROC qglActiveStencilFaceEXT;
extern void (APIENTRY *qglDepthBoundsEXT) (GLclampd zmin, GLclampd zmax);

extern void (APIENTRY *qglDrawRangeElements) (GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
extern void (APIENTRY *qglMultiDrawElements) (GLenum mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei drawcount);
extern void (APIENTRY *qglEnableClientState) (GLenum array);
extern void (APIENTRY *qglVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

extern void (APIENTRY *qglGenVertexArrays)(GLsizei n, GLuint *arrays);
extern void (APIENTRY *qglBindVertexArray)(GLuint vaoarray);

extern void (APIENTRY *qglTexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);		//gl4.2
extern void (APIENTRY *qglTexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);	//gl4.2


//glslang helper api
struct programshared_s;
union programhandle_u GLSlang_CreateProgram(struct programshared_s *prog, const char *name, int ver, const char **precompilerconstants, const char *vert, const char *cont, const char *eval, const char *geom, const char *frag, qboolean silent, vfsfile_t *blobfile);
GLint GLSlang_GetUniformLocation (GLuint prog, char *name);
void GL_SelectProgram(GLuint program);
#define GLSlang_UseProgram(prog) GL_SelectProgram(prog)
#define GLSlang_SetUniform1i(uni, parm0) qglUniform1iARB(uni, parm0)
#define GLSlang_SetUniform1f(uni, parm0) qglUniform1fARB(uni, parm0)


#ifdef _DEBUG
#if defined(__GNUC__)
#define checkglerror() do {int i=qglGetError(); if (i) Sys_Printf("GL Error %i detected at line %s:%i (caller %p)\n", i, __FILE__, __LINE__, __builtin_return_address(0));}while(0)
#else
#define checkglerror() do {int i=qglGetError(); if (i) Con_Printf("GL Error %i detected at line %s:%i\n", i, __FILE__, __LINE__);}while(0)
#endif
#else
#define checkglerror()
#endif




qboolean GL_Init(rendererstate_t *info, void *(*getglfunction) (char *name));
void GL_ForgetPointers(void);

#endif

qbyte GetPaletteIndex(int red, int green, int blue);
qbyte GetPaletteIndexNoFB(int red, int green, int blue);
int Mod_ReadFlagsFromMD1(char *name, int md3version);

/*
//opengl 3 deprecation

. Application-generated object names - the names of all object types, such as
buffer, query, and texture objects, must be generated using the corresponding
Gen* commands. Trying to bind an object name not returned by a Gen*
command will result in an INVALID OPERATION error. This behavior is
already the case for framebuffer, renderbuffer, and vertex array objects. Object
types which have default objects (objects named zero) , such as vertex
array, framebuffer, and texture objects, may also bind the default object, even
though it is not returned by Gen*.

. OpenGL Shading Language versions 1.10 and 1.20. These versions of the
shading language depend on many API features that have also been deprecated.

. Pixel transfer modes and operations - all pixel transfer modes, including
pixel maps, shift and bias, color table lookup, color matrix, and convolution
commands and state, and all associated state and commands defining
that state.

. Legacy OpenGL 1.0 pixel formats - the values 1, 2, 3, and 4 are no longer
accepted as internal formats by TexImage* or any other command taking
an internal format argument. The initial internal format of a texel array is
RGBA instead of 1.

. Texture borders - the border value to TexImage* must always be zero, or
an INVALID VALUE error is generated (section 3.8.1); all language in section
3.8 referring to nonzero border widths during texture image specification
and texture sampling; and all associated state.

GL_COLOR_INDEX

glBegin
glEnd
glEdgeFlag*; 
glColor*,
glFogCoord*
glIndex*
glNormal3*
glSecondaryColor3*
glTexCoord*
glVertex*

glColorPointer
glEdgeFlagPointer
glFogCoordPointer
glIndexPointer
glNormalPointer
glSecondary-
glColorPointer, 
glTexCoordPointer
glVertexPointer
glEnableClientState
glDisableClientState,

glInterleavedArrays
glClientActiveTexture
glFrustum,
glLoadIdentity
glLoadMatrix
glLoadTransposeMatrix
glMatrixMode,
glMultMatrix
glMultTransposeMatrix
glOrtho
glPopMatrix
glPushMatrix,
glRotate
glScale
glTranslate
GL_RESCALE_NORMAL
GL_NORMALIZE
glTexGen*
GL_TEXTURE_GEN_*,
Material*
glLight*
glLightModel*
glColorMaterial
glShadeModel
GL_LIGHTING
GL_VERTEX_PROGRAM_TWO_SIDE
GL_LIGHTi,
GL_COLOR_MATERIAL
glClipPlane
GL_CLAMP_VERTEX_COLOR
GL_CLAMP_FRAGMENT_COLOR
glRect*

glRasterPos*
glWindowPos*

GL_POINT_SMOOTH
GL_POINT_SPRITE

glLineStipple
GL_LINE_STIPPLE
GL_POLYGON
GL_QUADS
GL_QUAD_STRIP
glPolygonMode
glPolygonStipple
GL_POLYGON_STIPPLE
glDrawPixels
glPixelZoom
glBitmap
GL_BITMAP

GL_TEXTURE_COMPONENTS
GL_ALPHA
GL_LUMINANCE
GL_LUMINANCE_ALPHA
GL_INTENSITY

GL_DEPTH_TEXTURE_MODE

GL_CLAMP

GL_GENERATE_MIPMAP

glAreTexturesResident
glPrioritizeTextures,
GL_TEXTURE_PRIORITY
GL_TEXTURE_ENV
GL_TEXTURE_FILTER_CONTROL
GL_TEXTURE_LOD_BIAS

GL_TEXTURE_1D
GL_TEXTURE_2D,
GL_TEXTURE_3D
GL_TEXTURE_1D_ARRAY
GL_TEXTURE_2D_ARRAY
GL_TEXTURE_CUBE_MAP
GL_COLOR_SUM
GL_FOG
glFog
GL_MAX_TEXTURE_UNITS
GL_MAX_TEXTURE_COORDS
glAlphaFunc
GL_ALPHA_TEST

glClearAccum
GL_ACCUM_BUFFER_BIT

glCopyPixels

GL_AUX0
GL_RED_BITS
GL_GREEN_BITS
GL_BLUE_BITS
GL_ALPHA_BITS
GL_DEPTH_BITS
STENCIL BITS
glMap*
glEvalCoord*
glMapGrid*
glEvalMesh*
glEvalPoint*

glRenderMode
glInitNames
glPopName
glPushName
glLoadName
glSelectBuffer
glFeedbackBuffer
glPassThrough
glNewList
glEndList
glCallList
glCallLists
glListBase
glGenLists,
glIsList
glDeleteLists


GL_PERSPECTIVE_CORRECTION_HINT
GL_POINT_SMOOTH_HINT,
GL_FOG_HINT
GL_GENERATE_MIPMAP_HINT

glPushAttrib
glPushClientAttrib
glPopAttrib
glPopClientAttrib,
GL_MAX_ATTRIB_STACK_DEPTH,
GL_MAX_CLIENT_ATTRIB_STACK_DEPTH
GL_ATTRIB_STACK_DEPTH
GL_CLIENT_ATTRIB_STACK_DEPTH
GL_ALL_ATTRIB_BITS
GL_CLIENT_ALL_ATTRIB_BITS.
GL_EXTENSIONS
*/

#endif
