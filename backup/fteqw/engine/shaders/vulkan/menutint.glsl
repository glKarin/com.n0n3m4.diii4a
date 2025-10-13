!!cvari r_menutint_inverse=0
!!cvar3f r_menutint=0.68 0.4 0.13
!!samps 1

#include "sys/defs.h"

const vec4 e_rendertexturescale = vec4(1,1,1,1);

layout(location=0) varying vec2 texcoord;
#ifdef VERTEX_SHADER
		void main(void)
		{
#ifdef VULKAN
			texcoord.xy = v_texcoord.xy*e_rendertexturescale.xy;
#else
			texcoord.x = v_texcoord.x*e_rendertexturescale.x;
			texcoord.y = (1.0-v_texcoord.y)*e_rendertexturescale.y;
#endif
			gl_Position = ftetransform();
		}
#endif
#ifdef FRAGMENT_SHADER
		const vec3 lumfactors = vec3(0.299, 0.587, 0.114);
		const vec3 invertvec = vec3(1.0, 1.0, 1.0);
		void main(void)
		{
			vec3 texcolor = texture2D(s_t0, texcoord).rgb;
			float luminance = dot(lumfactors, texcolor);
			texcolor = vec3(luminance, luminance, luminance);
			texcolor *= cvar_r_menutint;
			texcolor = (cvar_r_menutint_inverse > 0) ? (invertvec - texcolor) : texcolor;
			gl_FragColor = vec4(texcolor, 1.0);
		}
#endif