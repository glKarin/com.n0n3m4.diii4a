!!ver 100 130
!!permu BUMP
!!permu SKELETAL
!!permu FRAMEBLEND
!!cvarf r_glsl_offsetmapping_scale
!!samps normalmap specular

//light pre-pass rendering (defered lighting)
//this is the initial pass, that draws the surface normals and depth to the initial colour buffer

#include "sys/defs.h"

#if defined(OFFSETMAPPING)
varying vec3 eyevector;
#endif

varying vec3 norm;
#if defined(BUMP)
varying vec3 tang, bitang;
#endif
#if defined(BUMP) || defined(SPECULAR)
varying vec2 tc;
#endif
#ifdef VERTEX_SHADER
#include "sys/skeletal.h"

void main()
{
#if defined(BUMP)
	gl_Position = skeletaltransform_nst(norm, tang, bitang);
#else
	gl_Position = skeletaltransform_n(norm);
#endif
#if defined(BUMP) || defined(SPECULAR)
	tc = v_texcoord;
#endif

#if defined(OFFSETMAPPING)
	vec3 eyeminusvertex = e_eyepos - v_position.xyz;
	eyevector.x = dot(eyeminusvertex, v_svector.xyz);
	eyevector.y = dot(eyeminusvertex, v_tvector.xyz);
	eyevector.z = dot(eyeminusvertex, v_normal.xyz);
#endif
}
#endif
#ifdef FRAGMENT_SHADER
#ifdef OFFSETMAPPING
#include "sys/offsetmapping.h"
#endif
void main()
{
//adjust texture coords for offsetmapping
#ifdef OFFSETMAPPING
	vec2 tcoffsetmap = offsetmap(s_normalmap, tc, eyevector);
#define tc tcoffsetmap
#endif

	vec3 onorm;
	vec4 ospec;

//need to write surface normals so that light shines on the surfaces properly
#if defined(BUMP)
	vec3 bm = 2.0*texture2D(s_normalmap, tc).xyz - 1.0;
	onorm = normalize(bm.x * tang + bm.y * bitang + bm.z * norm);
#else
	onorm = norm;
#endif

//we need to write specular exponents if we want per-pixel control over that
#if defined(SPECULAR)
	ospec = texture2D(s_specular, tc);
#else
	ospec = vec4(0.0, 0.0, 0.0, 0.0);
#endif

	gl_FragColor = vec4(onorm.xyz, ospec.a * FTE_SPECULAR_EXPONENT);
}
#endif
