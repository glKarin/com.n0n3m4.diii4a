!!cvarf ffov=360
!!samps reflectcube
#include "sys/defs.h"

//panoramic view rendering, for promo map shots or whatever.

layout(location=0) varying vec2 texcoord;
#ifdef VERTEX_SHADER
void main()
{
	texcoord = v_texcoord.xy;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main()
{
	vec3 tc;	
	float ang;	
	ang = texcoord.x*radians(cvar_ffov);	
	tc.x = sin(ang);	
	tc.y = -texcoord.y;	
	tc.z = cos(ang);	
	gl_FragColor = textureCube(s_reflectcube, tc);
}
#endif
