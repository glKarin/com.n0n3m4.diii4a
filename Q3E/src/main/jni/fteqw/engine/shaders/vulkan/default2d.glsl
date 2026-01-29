!!argb PREMUL=false
!!samps 1
#include "sys/defs.h"

//this shader is present for support for gles/gl3core contexts
//it is single-texture-with-vertex-colours, and doesn't do anything special.
//beware that a few things use this, including apparently fonts and bloom rescaling.
//its really not meant to do anything special.

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
	vec4 f = vc;
	if (arg_PREMUL)
		f.rgb *= f.a;
	f *= texture2D(s_t0, tc);
	gl_FragColor = f;
}
#endif
