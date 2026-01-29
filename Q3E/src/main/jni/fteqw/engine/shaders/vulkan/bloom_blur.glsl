!!samps 1
#include "sys/defs.h"
//apply gaussian filter

layout(location=0) varying vec2 tc;

#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
/*offset should be 1.2 pixels away from the center*/
void main ()
{
	gl_FragColor =
		0.3125 * texture2D(s_t0, tc - e_glowmod.st) +
		0.375 * texture2D(s_t0, tc) +
		0.3125 * texture2D(s_t0, tc + e_glowmod.st);
}
#endif
