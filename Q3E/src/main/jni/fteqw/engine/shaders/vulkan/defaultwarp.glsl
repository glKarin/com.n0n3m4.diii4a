!!permu FOG
!!cvarf r_wateralpha=1
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
!!argf alpha=0
!!argb lit=false
!!samps diffuse lightmap

#include "sys/defs.h"

//this is the shader that's responsible for drawing default q1 turbulant water surfaces
//this is expected to be moderately fast.

#include "sys/fog.h"
layout(location=0) varying vec2 tc;
layout(location=1) varying vec2 lmtc;

#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord.st;
	#ifdef FLOW
	tc.s += e_time * -0.5;
	#endif
	lmtc = v_lmcoord.st;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	vec2 ntc = tc + sin(tc.ts+e_time)*0.125;
	vec3 ts = vec3(texture2D(s_diffuse, ntc));

	if (arg_lit)
		ts *= (texture2D(s_lightmap, lmtc) * e_lmscale).rgb;

	if (arg_alpha != 0.0)
		gl_FragColor = fog4(vec4(ts, arg_alpha));
	else
		gl_FragColor = fog4(vec4(ts, cvar_r_wateralpha));
}
#endif
