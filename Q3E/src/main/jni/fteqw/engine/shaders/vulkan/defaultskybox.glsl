!!permu FOG
!!samps reflectcube
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
#include "sys/defs.h"
#include "sys/fog.h"

//simple shader for simple skyboxes.

layout(location=0) varying vec3 pos;
#ifdef VERTEX_SHADER
void main ()
{
	pos = v_position.xyz - e_eyepos;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	vec4 skybox = textureCube(s_reflectcube, pos);
	gl_FragColor = vec4(fog3(skybox.rgb), 1.0);
}
#endif
