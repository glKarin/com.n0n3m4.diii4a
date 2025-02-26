!!cvarf ffov
!!samps screen:samplerCube=0

//stereographic view rendering, for high fovs that are still playable.

#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
varying vec2 texcoord;
uniform float cvar_ffov;
void main()
{
	texcoord = v_texcoord.xy;

	//make sure the ffov cvar actually does something meaningful
	texcoord *= cvar_ffov / 90.0;

	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
varying vec2 texcoord;
void main()
{
	vec3 tc;
	vec2 d;
	vec2 ang;
	d = texcoord;	

	//compute the 2d->3d projection
	float div = 1.0 + d.x*d.x + d.y*d.y;
	tc.x = 2.0*d.x/div;
	tc.y = -2.0*d.y/div;
	tc.z = -(-1.0 + d.x*d.x + d.y*d.y)/div;

	gl_FragColor = textureCube(s_screen, tc);
}
#endif
