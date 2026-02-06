!!ver 100 450
!!permu FOG
!!samps diffuse lightmap

#include "sys/defs.h"

//this is the shader that's responsible for drawing default q1 turbulant water surfaces
//this is expected to be moderately fast.

#include "sys/fog.h"

varying vec2 tc;
#ifdef LIT
varying vec2 lm0;
#endif
#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord.st;
	#ifdef FLOWV
	tc.st += e_time * vec2(FLOWV);
	#endif
	#ifdef FLOW
	tc.s += e_time * -0.5;
	#endif
	#ifdef LIT
	lm0 = v_lmcoord;
	#endif
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main ()
{
	vec2 ntc = tc + sin(tc.ts+e_time)*0.125;
	vec3 ts = vec3(texture2D(s_diffuse, ntc));

#ifdef LIT
	ts *= (texture2D(s_lightmap, lm0) * e_lmscale).rgb;
#endif

#ifdef ALPHA
	gl_FragColor = fog4blend(vec4(ts, float(ALPHA)) * e_colourident);
#else
	gl_FragColor = fog4(vec4(ts, 1.0) * e_colourident);
#endif
}
#endif
