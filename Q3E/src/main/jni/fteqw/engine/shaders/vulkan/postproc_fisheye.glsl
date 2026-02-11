!!cvarf ffov=360
!!samps reflectcube
#include "sys/defs.h"


//fisheye view rendering, for silly fovs that are still playable.

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
	vec2 d;	
	vec2 ang;	
	d = texcoord;	
	ang.x = sqrt(d.x*d.x+d.y*d.y)*radians(cvar_ffov);	
	ang.y = -atan(d.y, d.x);	
	tc.x = sin(ang.x) * cos(ang.y);	
	tc.y = sin(ang.x) * sin(ang.y);	
	tc.z = cos(ang.x);	
	gl_FragColor = textureCube(s_reflectcube, tc);
}
#endif
