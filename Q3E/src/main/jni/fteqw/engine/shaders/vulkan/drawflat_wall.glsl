!!permu FOG
!!cvar3f r_floorcolor=0.5,0.5,0.5
!!cvar3f r_wallcolor=0.25,0.25,0.5
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
!!samps 1
#include "sys/defs.h"

//this is for the '286' preset walls, and just draws lightmaps coloured based upon surface normals.

#include "sys/fog.h"
layout(location=0) varying vec4 col;
layout(location=1) varying vec2 lm;

#ifdef VERTEX_SHADER
void main ()
{
	col = vec4(e_lmscale.rgb/255.0 * ((v_normal.z < 0.73)?cvar_r_wallcolor:cvar_r_floorcolor), e_lmscale.a);
	lm = v_lmcoord;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	gl_FragColor = fog4(col * texture2D(s_t0, lm));
}
#endif
