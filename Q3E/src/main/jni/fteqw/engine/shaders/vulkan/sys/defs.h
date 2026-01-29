#ifdef VERTEX_SHADER
#define attribute in
#define varying out

out gl_PerVertex
{
  vec4 gl_Position;
};

#else
#define varying in
#endif

layout(std140, binding=0) uniform entityblock
{
	mat4 m_modelviewproj;
	mat4 m_model;
	mat4 m_modelinv;
	vec3 e_eyepos;
	float e_time;
	vec3 e_light_ambient;	float epad1;
	vec3 e_light_dir;	float epad2;
	vec3 e_light_mul;	float epad3;
	vec4 e_lmscales[4];
	vec3 e_uppercolour;	float epad4;
	vec3 e_lowercolour;	float epad5;
	vec3 e_glowmod;		float epad6;
	vec4 e_colourident;
	vec4 w_fogcolours;
	float w_fogdensity;	float w_fogdepthbias;	 vec2 epad7;
};
#define e_lmscale (e_lmscales[0])

layout(std140, binding=1) uniform lightblock
{
	mat4 l_cubematrix;
	vec3 l_lightposition; 	float lpad1;
	vec3 l_lightcolour; 		float lpad2;
	vec3 l_lightcolourscale; 	float l_lightradius;
	vec4 l_shadowmapproj;
	vec2 l_shadowmapscale;	vec2 lpad3;
};


#ifdef VERTEX_SHADER
layout(location=0) attribute vec3 v_position;
layout(location=1) attribute vec2 v_texcoord;
layout(location=2) attribute vec4 v_colour;
layout(location=3) attribute vec2 v_lmcoord;
layout(location=4) attribute vec3 v_normal;
layout(location=5) attribute vec3 v_svector;
layout(location=6) attribute vec3 v_tvector;
#endif
#ifdef FRAGMENT_SHADER
layout(location=0) out vec4 outcolour;
#define gl_FragColor outcolour
#endif

#define texture2D texture
#define textureCube texture

/*defined by front-end, with bindings that suit us
uniform sampler2D s_t0;
uniform sampler2D s_t1;
uniform sampler2D s_t2;
uniform sampler2D s_t3;
uniform sampler2D s_t4;
uniform sampler2D s_t5;
uniform sampler2D s_t6;
uniform sampler2D s_t7;
uniform sampler2D s_diffuse;
uniform sampler2D s_normalmap;
uniform sampler2D s_specular;
uniform sampler2D s_upper;
uniform sampler2D s_lower;
uniform sampler2D s_fullbright;
uniform sampler2D s_paletted;
uniform sampler2D s_shadowmap;
uniform samplerCube s_projectionmap;
uniform samplerCube s_reflectcube;
uniform sampler2D s_reflectmask;
uniform sampler2D s_lightmap;
#define s_lightmap0 s_lightmap
uniform sampler2D s_deluxmap;
#define s_deluxmap0 s_deluxmap
uniform sampler2D s_lightmap1;
uniform sampler2D s_lightmap2;
uniform sampler2D s_lightmap3;
uniform sampler2D s_deluxmap1;
uniform sampler2D s_deluxmap2
uniform sampler2D s_deluxmap3;
*/

#ifdef VERTEX_SHADER
vec4 ftetransform()
{
	vec4 proj = (m_modelviewproj*vec4(v_position,1.0));
	proj.y *= -1;
	proj.z = (proj.z + proj.w) / 2.0;
	return proj;
}
#endif
