!!samps 1
!!cvar3f r_bloom_filter=.7,.7,.7
#include "sys/defs.h"
//the bloom filter
//filter out any texels which are not to bloom

layout(location=0) varying vec2 tc;

#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	gl_FragColor.rgb = (texture2D(s_t0, tc).rgb - cvar_r_bloom_filter)/(1.0-cvar_r_bloom_filter);
}
#endif
