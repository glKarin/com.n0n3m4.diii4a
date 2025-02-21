!!cvarf ffov
!!samps screen:samplerCube=0

//my attempt at lambert azimuthal equal-area view rendering, because you'll remember that name easily.

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
	float sq = d.x*d.x+d.y*d.y;
	if (sq > 4.0)
		gl_FragColor = vec4(0,0,0,1);
	else
	{
		tc.x = sqrt(1.0-(sq/4.0))*d.x;
		tc.y = sqrt(1.0-(sq/4.0))*d.y;
		tc.z = -1.0 + (sq/2.0);

		tc.y *= -1.0;
		tc.z *= -1.0;

		gl_FragColor = textureCube(s_screen, tc);
	}
}
#endif
