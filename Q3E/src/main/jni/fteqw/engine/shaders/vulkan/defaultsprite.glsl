!!permu FOG
!!samps 1
!!argf MASK=0
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false

//used by both particles and sprites.
//note the fog blending mode is all that differs from defaultadditivesprite

#include "sys/defs.h"
#include "sys/fog.h"

layout(location=0) varying vec2 tc;
layout(location=1) varying vec4 vc;

#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord;
	vc = v_colour;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	vec4 col = texture2D(s_t0, tc);
	if (arg_MASK!=0.0 && col.a < float(arg_MASK))
		discard;
	gl_FragColor = fog4blend(col * vc * e_colourident);
}
#endif
