!!cvarv r_bloom_filter
!!samps 1
//the bloom filter
//filter out any texels which are not to bloom

varying vec2 tc;

#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
void main ()
{
	tc = v_texcoord;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
uniform vec3 cvar_r_bloom_filter;
void main ()
{
	gl_FragColor.rgb = (texture2D(s_t0, tc).rgb - cvar_r_bloom_filter)/(1.0-cvar_r_bloom_filter);
}
#endif
