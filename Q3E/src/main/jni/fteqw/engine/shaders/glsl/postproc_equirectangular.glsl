!!cvarf ffov
!!samps screen:samplerCube=0

//equirectangular view rendering, commonly used for sphere->2d map projections.

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

#define PI 3.1415926535897932384626433832795
void main()
{
	vec3 tc;
	float lng = (texcoord.x - 0.5) * PI * 2.0;
	float lat = (texcoord.y) * PI * 1.0;
	
	tc.z = cos(lng) * sin(lat);	
	tc.x = sin(lng) * sin(lat);
	tc.y = cos(lat);
	gl_FragColor = textureCube(s_screen, tc);
}
#endif
