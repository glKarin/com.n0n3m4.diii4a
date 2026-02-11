!!ver 100-450
!!samps 1
//this shader is applies gamma/contrast/brightness to the source image, and dumps it out.

varying vec2 tc;
varying vec4 vc;	//gamma, contrast, brightness, contrastboost

#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
attribute vec4 v_colour;
void main ()
{
	tc = vec2(v_texcoord.s, 1.0-v_texcoord.t);
	vc = v_colour;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	vec3 t = texture2D(s_t0, tc).rgb;
	t = vc.a * t/((vc.a-1.0)*t + 1.0);
	gl_FragColor = vec4(pow(t, vec3(vc.r))*vc.g + vc.b, 1.0);
}
#endif
