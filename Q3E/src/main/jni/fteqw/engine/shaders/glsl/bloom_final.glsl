!!cvarf r_bloom
!!cvarf r_bloom_retain=1.0
!!samps 4
//add them together
//optionally apply tonemapping

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
uniform float cvar_r_bloom;
uniform float cvar_r_bloom_retain;
void main ()
{
	gl_FragColor = 
		cvar_r_bloom_retain * texture2D(s_t0, tc) +
		cvar_r_bloom*(
			texture2D(s_t1, tc) +
			texture2D(s_t2, tc) +
			texture2D(s_t3, tc)
		) ;
}
#endif
