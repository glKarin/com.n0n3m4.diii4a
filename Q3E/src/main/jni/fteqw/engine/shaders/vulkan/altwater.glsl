!!permu FOG
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
!!cvarf r_glsl_turbscale_reflect=1
!!cvarf r_glsl_turbscale_refract=1
!!cvarf gl_maxdist=8192
!!cvarf gl_mindist=4
!!samps normalmap diffuse 4
!!argb reflect=0		//s_t1 is a reflection instead of diffusemap
!!argf strength_refl=0.1	//0.1 = fairly gentle, 0.2 = big waves
!!argf strength_refr=0.1	//0.1 = fairly gentle, 0.2 = big waves
!!argf fresnel_exp=5.0	//water should be around 5
!!argf fresnel_range=1.0
!!argf fresnel_min=1.0
!!argf txscale1=0.2		//wave strength
!!argf txscale2=0.2		//wave strength
!!argb ripplemap=0		//s_t2 contains a ripplemap
!!arg3f tint_refr=0.7 0.8 0.7	//some colour value
!!arg3f tint_refl=0.7 0.8 0.7	//some colour value
!!argb depth=0			//s_t3 is a depth image
!!argf alpha=0			//blend in the alpha channel
!!arg3f fogtint=0.2 0.3 0.2 //tints as it gets deeper


#include "sys/defs.h"
#include "sys/fog.h"

layout(location=0) varying vec2 tc;
layout(location=1) varying vec4 tf;
layout(location=2) varying vec3 norm;
layout(location=3) varying vec3 eye;
#ifdef VERTEX_SHADER
void main (void)
{
	tc = v_texcoord.st;
	tf = ftetransform();
	norm = v_normal;
	eye = e_eyepos - v_position.xyz;
	gl_Position = tf;
}
#endif
#ifdef FRAGMENT_SHADER
#define s_refract s_t0
#define s_reflect s_t1
#define s_ripplemap s_t2
#define s_refractdepth s_t3

void main (void)
{
	vec2 stc, ntc;
	vec3 n, refr, refl;
	float fres;
	float depth;
	stc = (1.0 + (tf.xy / tf.w)) * 0.5;
	//hack the texture coords slightly so that there are no obvious gaps
	stc.t -= 1.5*norm.z/1080.0;

	//apply q1-style warp, just for kicks
	ntc.s = tc.s + sin(tc.t+e_time)*0.125;
	ntc.t = tc.t + sin(tc.s+e_time)*0.125;

	//generate the two wave patterns from the normalmap
	n = (texture2D(s_normalmap, arg_txscale1*tc + vec2(e_time*0.1, 0.0)).xyz);
	n += (texture2D(s_normalmap, arg_txscale2*tc - vec2(0, e_time*0.097)).xyz);
	n -= 1.0 - 4.0/256.0;

	if (arg_ripplemap)
		n += texture2D(s_ripplemap, stc).rgb*3.0;
	n = normalize(n);

	//the fresnel term decides how transparent the water should be
	fres = pow(1.0-abs(dot(n, normalize(eye))), arg_fresnel_exp) * arg_fresnel_range + arg_fresnel_min;

	if (arg_depth)
	{
		float far = cvar_gl_maxdist;
		float near = cvar_gl_mindist;
		//get depth value at the surface
		float sdepth = gl_FragCoord.z;
		sdepth = (2.0*near) / (far + near - sdepth * (far - near));
		sdepth = mix(near, far, sdepth);

		//get depth value at the ground beyond the surface.
		float gdepth = texture2D(s_refractdepth, stc).x;
		gdepth = (2.0*near) / (far + near - gdepth * (far - near));
		if (gdepth >= 0.5)
		{
			gdepth = sdepth;
			depth = 0.0;
		}
		else
		{
			gdepth = mix(near, far, gdepth);
			depth = gdepth - sdepth;
		}

		//reduce the normals in shallow water (near walls, reduces the pain of linear sampling)
		if (depth < 100.0)
			n *= depth/100.0;
	}
	else
		depth = 1.0; 


	//refraction image (and water fog, if possible)
	refr = texture2D(s_refract, stc + n.st*arg_strength_refr*cvar_r_glsl_turbscale_refract).rgb * arg_tint_refr;
	if (arg_depth)
		refr = mix(refr, arg_fogtint, min(depth/4096.0, 1.0));

	//reflection/diffuse
	if (arg_reflect)
		refl = texture2D(s_reflect, stc - n.st*arg_strength_refl*cvar_r_glsl_turbscale_reflect).rgb * arg_tint_refl;
	else
		refl = texture2D(s_diffuse, ntc).xyz;

	//FIXME: add specular tints

	//interplate by fresnel
	refr = mix(refr, refl, fres);

	if (arg_alpha != 0)
	{
		vec4 ts = texture2D(s_diffuse, ntc);
		vec4 surf = fog4blend(vec4(ts.rgb, arg_alpha*ts.a));
		refr = mix(refr, surf.rgb, surf.a);
	}

	//done
	gl_FragColor = vec4(refr, 1.0);
}
#endif
