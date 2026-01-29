!!samps 4
!!cvarf r_bloom=1
!!cvarf r_bloom_retain=1.0
#include "sys/defs.h"
//add them together
//optionally apply tonemapping

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
	gl_FragColor = 
		cvar_r_bloom_retain * texture2D(s_t0, tc) +
		cvar_r_bloom*(
			texture2D(s_t1, tc) +
			texture2D(s_t2, tc) +
			texture2D(s_t3, tc)
		) ;
}
#endif
