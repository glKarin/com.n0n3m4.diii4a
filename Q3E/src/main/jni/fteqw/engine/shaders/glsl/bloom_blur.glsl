!!samps 1
//apply gaussian filter

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
/*offset should be 1.2 pixels away from the center*/
uniform vec3 e_glowmod;
void main ()
{
	gl_FragColor =
		0.3125 * texture2D(s_t0, tc - e_glowmod.st) +
		0.375 * texture2D(s_t0, tc) +
		0.3125 * texture2D(s_t0, tc + e_glowmod.st);
}
#endif
