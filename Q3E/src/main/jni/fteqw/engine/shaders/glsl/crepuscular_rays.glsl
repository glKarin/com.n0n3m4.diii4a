!!cvarf crep_decay
!!cvarf crep_density
!!cvarf crep_weight
!!samps 1

//this is a post-processing shader, drawn in 2d
//there will be a render target containing sky surfaces drawn with crepuscular_sky, and everything else drawn with crepuscular_opaque (to mask out the sky)
//this shader then just smudges the sky out a bit as though its coming from the sun or whatever through the clouds.
//yoinked from http://fabiensanglard.net/lightScattering/index.php

varying vec2 tc;
#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
void main ()
{
	tc = v_texcoord;
	gl_Position = vec4(v_position, 1.0);
}
#endif
#ifdef FRAGMENT_SHADER
const float crep_decay = 0.94;
const float crep_density = 0.5;
const float crep_weight = 0.2;
uniform vec3 l_lightcolour;
uniform vec3 l_lightscreen;
const int NUM_SAMPLES = 100;
void main()
{
	vec2 deltaTextCoord = vec2(tc.st - l_lightscreen.xy);
	vec2 textCoo = tc.st;
	deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * crep_density;
	float illuminationDecay = 1.0;
	gl_FragColor = vec4(0.0,0.0,0.0,0.0);
	for(int i=0; i < NUM_SAMPLES ; i++)
	{
		textCoo -= deltaTextCoord;
		vec4 sample = texture2D(s_t0, textCoo);
		sample *= illuminationDecay * crep_weight;
		gl_FragColor += sample;
		illuminationDecay *= crep_decay;
	}
	gl_FragColor *= vec4(l_lightcolour, 1.0);
}
#endif
