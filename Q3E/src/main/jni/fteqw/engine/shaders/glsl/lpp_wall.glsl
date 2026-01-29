!!ver 100 150
!!permu BUMP	//for offsetmapping rather than bumpmapping (real bumps are handled elsewhere)
!!cvarf r_glsl_offsetmapping_scale
!!samps diffuse specular fullbright lightmap
!!samps 2

//the final defered lighting pass.
//the lighting values were written to some render target, which is fed into this shader, and now we draw all the wall textures with it.

#include "sys/defs.h"

#if defined(OFFSETMAPPING)
varying vec3 eyevector;
#endif


varying vec2 tc, lm;
varying vec4 tf;
#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord;
	lm = v_lmcoord;
	gl_Position = tf = ftetransform();

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
void main ()
{
//adjust texture coords for offsetmapping
#ifdef OFFSETMAPPING
	vec2 tcoffsetmap = offsetmap(s_normalmap, tc, eyevector);
#define tc tcoffsetmap
#endif

	vec2 nst;
	nst = tf.xy / tf.w;
	nst = (1.0 + nst) / 2.0;
	vec4 dl = texture2D(s_t0, nst);	//diffuse lighting
	vec4 sl = texture2D(s_t1, nst);	//specular lighting
	vec4 c = texture2D(s_diffuse, tc);
	vec4 s = texture2D(s_specular, tc);
	vec4 f = texture2D(s_fullbright, tc);
//fixme: top+bottom should add upper+lower colours to c here
	vec3 lmsamp = texture2D(s_lightmap, lm).rgb*e_lmscale.rgb;
//fixme: fog the legacy lightmap data
	vec3 diff = dl.rgb + lmsamp;
	vec3 spec = sl.rgb * float(SPECMUL);	//should be rgb, but whatever.
	
//fixme: do specular somehow
	gl_FragColor = vec4(diff*c.rgb + spec*s.rgb + f.rgb, 1.0);
//fixme: fullbrights should add to the rgb value
}
#endif
