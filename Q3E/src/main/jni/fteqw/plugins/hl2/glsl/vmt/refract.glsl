!!ver 110
!!permu BUMP
!!samps diffuse
!!samps =BUMP normalmap
!!samps refraction=0 dudvmap=1

#include "sys/defs.h"

varying vec2 tex_c;
varying mat3 invsurface;
varying vec4 tf_c;
varying vec3 eyeminusvertex;

#ifdef VERTEX_SHADER
	void main ()
	{
		invsurface[0] = v_svector;
		invsurface[1] = v_tvector;
		invsurface[2] = v_normal;
		tf_c = ftetransform();
		tex_c = v_texcoord;
		gl_Position = tf_c;
	}
#endif

#ifdef FRAGMENT_SHADER
	#include "sys/fog.h"
	void main ( void )
	{
		vec2 refl_c;
		vec3 refr_f;
		vec3 dudv_f;
		vec4 out_f = vec4( 1.0, 1.0, 1.0, 1.0 );

		dudv_f = ( texture2D( s_dudvmap, tex_c).xyz);
		dudv_f += ( texture2D( s_dudvmap, tex_c).xyz);
		dudv_f -= 1.0 - ( 4.0 / 256.0 );
		dudv_f = normalize( dudv_f );

		// Reflection/View coordinates
		refl_c = ( 1.0 + ( tf_c.xy / tf_c.w ) ) * 0.5;

		refr_f = texture2D( s_refraction, refl_c + ( dudv_f.st) ).rgb;
		out_f.rgb = refr_f;
#ifdef TINTTEXTURE
		out_f.rgb *= texture2D( s_diffuse, tex_c ).rgb;
#endif

		gl_FragColor = out_f;
	}
#endif
