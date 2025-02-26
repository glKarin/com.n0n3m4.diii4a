!!ver 300-450
!!samps anim:2DArray=0

#include "sys/defs.h"

//this shader is present for support for gles/gl3core contexts
//it is single-texture-with-vertex-colours, and doesn't do anything special.
//beware that a few things use this, including apparently fonts and bloom rescaling.
//its really not meant to do anything special.

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
void main ()
{
	//figure out which frame to use.
    ivec3 sz = textureSize(s_anim, 0);
    float layer = mod(e_time*10, sz.z-1);

	vec4 f = vc;
#ifdef PREMUL
	f.rgb *= f.a;
#endif
    f *= texture(s_anim, vec3(tc, layer));
	gl_FragColor = f;
}
#endif
