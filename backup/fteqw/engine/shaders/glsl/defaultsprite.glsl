!!permu FOG
!!samps 1
//used by both particles and sprites.
//note the fog blending mode is all that differs from defaultadditivesprite

#include "sys/defs.h"
#include "sys/fog.h"
varying vec2 tc;
varying vec4 vc;
#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord;
	vc = v_colour;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
uniform vec4 e_vlscale;
void main ()
{
	vec4 col = texture2D(s_t0, tc);
#ifdef MASK
	if (col.a < float(MASK))
		discard;
#endif
	gl_FragColor = fog4blend(col * vc * e_colourident * e_vlscale);
}
#endif
