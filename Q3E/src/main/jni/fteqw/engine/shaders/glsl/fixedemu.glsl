!!ver 100-450
!!samps sourcetex=0

//this shader is present for support for gles/gl3core contexts
//it is single-texture-with-vertex-colours, and doesn't do anything special.
//beware that a few things use this, including apparently fonts and bloom rescaling.
//its really not meant to do anything special.

#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
varying vec2 tc;
#ifndef UC
attribute vec4 v_colour;
varying vec4 vc;
#endif
void main ()
{
	tc = v_texcoord;
#ifndef UC
	vc = v_colour;
#endif
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
varying vec2 tc;
#ifndef UC
varying vec4 vc;
#else
uniform vec4 s_colour;
#define vc s_colour
#endif
float e_time;
void main ()
{
	vec4 fc = texture2D(s_sourcetex, tc) * vc;
#ifdef ALPHATEST
	if (!(fc.a ALPHATEST))
		discard;
#endif
	gl_FragColor = fc;
}
#endif
