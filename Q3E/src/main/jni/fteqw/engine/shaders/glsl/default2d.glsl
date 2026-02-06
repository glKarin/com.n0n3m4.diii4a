!!ver 100-450
!!samps img=0

//this shader is present for support for gles/gl3core contexts
//it is single-texture-with-vertex-colours, and doesn't do anything special.
//beware that a few things use this, including apparently fonts and bloom rescaling.
//its really not meant to do anything special.

varying vec2 tc;
varying vec4 vc;

#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
attribute vec4 v_colour;
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
#ifdef PREMUL
	f.rgb *= f.a;
#endif
#ifdef DECLAMP
	vec2 ntc = fract(tc);
#define tc ntc
#endif
	f *= texture2D(s_img, tc);
	gl_FragColor = f;
}
#endif
