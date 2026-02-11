!!permu FOG
!!samps base=0, cloud=1
!!cvardf r_skyfog=0.5
#include "sys/fog.h"

//regular sky shader for scrolling q1 skies
//the sky surfaces are thrown through this as-is.

#ifdef VERTEX_SHADER
varying vec3 pos;
void main ()
{
	pos = v_position.xyz;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
uniform float e_time;
uniform vec3 e_eyepos;
varying vec3 pos;
void main ()
{
	vec2 tccoord;
	vec3 dir = pos - e_eyepos;

#ifdef EQUI
#define PI 3.1415926535897932384626433832795
	dir = normalize(dir);
	tccoord.x = atan(dir.y,-dir.x) / (PI*2.0);
	tccoord.y = acos(dir.z) / PI;
	
	vec3 sky = vec3(texture2D(s_base, tccoord));
#else

	dir.z *= 3.0;
	dir.xy /= 0.5*length(dir);
	tccoord = (dir.xy + e_time*0.03125);
	vec3 sky = vec3(texture2D(s_base, tccoord));
	tccoord = (dir.xy + e_time*0.0625);
	vec4 clouds = texture2D(s_cloud, tccoord);
	sky = (sky.rgb*(1.0-clouds.a)) + (clouds.a*clouds.rgb);
#endif

#ifdef FOG
	sky.rgb = mix(sky.rgb, w_fogcolour, float(r_skyfog)*w_fogalpha);	//flat fog ignoring actual geometry
	//sky = fog3(sky);													//fog according to actual geometry
#endif
	gl_FragColor = vec4(sky, 1.0);
}
#endif
