!!cvarf ffov=0
!!samps reflectcube
#include "sys/defs.h"

//stereographic view rendering, for high fovs that are still playable.

layout(location=0) varying vec2 texcoord;
#ifdef VERTEX_SHADER
//uniform float cvar_ffov;
void main()
{
	texcoord = v_texcoord.xy;

	//make sure the ffov cvar actually does something meaningful
	texcoord *= cvar_ffov / 90.0;

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

	//compute the 2d->3d projection
	float div = 1.0 + d.x*d.x + d.y*d.y;
	tc.x = 2.0*d.x/div;
	tc.y = -2.0*d.y/div;
	tc.z = -(-1.0 + d.x*d.x + d.y*d.y)/div;

	gl_FragColor = textureCube(s_reflectcube, tc);
}
#endif
