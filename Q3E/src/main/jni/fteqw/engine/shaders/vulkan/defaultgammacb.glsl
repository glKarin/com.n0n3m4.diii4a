!!samps 1

//this shader is applies gamma/contrast/brightness to the source image, and dumps it out.

#include "sys/defs.h"

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
	gl_FragColor = pow(texture2D(s_t0, tc) * vc.g, vec4(vc.r)) + vc.b;
}
#endif
