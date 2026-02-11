!!cvarf ffov
!!samps screen:samplerCube=0

//fisheye view rendering, for silly fovs that are still playable.

#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
varying vec2 texcoord;
void main()
{
	texcoord = v_texcoord.xy;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
varying vec2 texcoord;
uniform float cvar_ffov;
void main()
{
	vec3 tc;	
	vec2 d;	
	vec2 ang;	
	d = texcoord;	
	ang.x = sqrt(d.x*d.x+d.y*d.y)*radians(cvar_ffov);	
	ang.y = -atan(d.y, d.x);	
	tc.x = sin(ang.x) * cos(ang.y);	
	tc.y = sin(ang.x) * sin(ang.y);	
	tc.z = cos(ang.x);	
	gl_FragColor = textureCube(s_screen, tc);
}
#endif
