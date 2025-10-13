!!permu FOG
!!samps 1

//meant to be used for additive stuff. presumably particles and sprites. though actually its only flashblend effects that use this at the time of writing.
//includes fog, apparently.

#include "sys/fog.h"
#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
attribute vec4 v_colour;
varying vec2 tc;
varying vec4 vc;
void main ()
{
	tc = v_texcoord;
	vc = v_colour;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
varying vec2 tc;
varying vec4 vc;
uniform vec4 e_colourident;
void main ()
{
	gl_FragColor = fog4additive(texture2D(s_t0, tc) * vc * e_colourident);
}
#endif
