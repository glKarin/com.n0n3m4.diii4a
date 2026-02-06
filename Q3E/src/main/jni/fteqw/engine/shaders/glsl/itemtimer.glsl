!!permu FOG

#include "sys/defs.h"
#include "sys/fog.h"

varying vec2 tc;
varying vec4 vc;

#ifdef VERTEX_SHADER
void main ()
{
	tc = v_texcoord;
	vc = v_colour;
	gl_Position = ftetransform();
}
#endif


#ifdef FRAGMENT_SHADER
void main ()
{
	gl_FragColor = vec4(0.5,0.5,0.5,1);//texture2D(s_diffuse, tc.xy);

	vec2 st = (tc-floor(tc)) - 0.5;
	st *= 2.0;
	float dist = sqrt(dot(st,st));

	float ring = 1.0 + smoothstep(0.9, 1.0, dist)
				 - smoothstep(0.8, 0.9, dist);

	//fade out the rim
	if ((atan(st.t, st.s)+3.14)/6.28 > vc.a)
		gl_FragColor.a *= 0.25;
	gl_FragColor.rgb *= mix(vc.rgb, vec3(0.0), ring);
//gl_FragColor.a;

//and finally hide it all if we're fogged.
#ifdef FOG
	gl_FragColor = fog4additive(gl_FragColor);
#endif
}
#endif

