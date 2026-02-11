!!permu FULLBRIGHT
!!permu UPPERLOWER
//!!permu FRAMEBLEND
//!!permu SKELETAL
!!permu FOG
!!permu BUMP
!!permu REFLECTCUBEMASK
!!cvarf r_glsl_offsetmapping=0
!!cvarf r_glsl_offsetmapping_scale=0.04
!!cvarf gl_specular=0
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
!!samps diffuse normalmap upper lower specular fullbright reflectcube reflectmask

#include "sys/defs.h"

//standard shader used for models.
//must support skeletal and 2-way vertex blending or Bad Things Will Happen.
//the vertex shader is responsible for calculating lighting values.

layout(location=0) varying vec2 tcbase;
layout(location=1) varying vec3 light;
#if defined(SPECULAR) || defined(OFFSETMAPPING)
layout(location=2) varying vec3 eyevector;
#endif
layout(location=3) varying mat3 invsurface;




#ifdef VERTEX_SHADER
#include "sys/skeletal.h"
void main ()
{
	vec3 n;
	if (SPECULAR||OFFSETMAPPING||REFLECTCUBEMASK)
	{
		vec3 s, t, w;
		gl_Position = skeletaltransform_wnst(w,n,s,t);
		vec3 eyeminusvertex = e_eyepos - w.xyz;
		eyevector.x = dot(eyeminusvertex, s.xyz);
		eyevector.y = dot(eyeminusvertex, t.xyz);
		eyevector.z = dot(eyeminusvertex, n.xyz);

		if (REFLECTCUBEMASK)
		{
			invsurface[0] = s;
			invsurface[0] = t;
			invsurface[0] = n;
		}
	}
	else
	{
		gl_Position = skeletaltransform_n(n);
	}

	float d = dot(n,e_light_dir);
	if (d < 0.0)		//vertex shader. this might get ugly, but I don't really want to make it per vertex.
		d = 0.0;	//this avoids the dark side going below the ambient level.
	light = e_light_ambient + (dot(n,e_light_dir)*e_light_mul);
	tcbase = v_texcoord;
}
#endif
#ifdef FRAGMENT_SHADER
#include "sys/fog.h"

#include "sys/offsetmapping.h"

void main ()
{
	vec4 col, sp;

	vec2 tc;
	if (OFFSETMAPPING)
		tc = offsetmap(s_normalmap, tcbase, eyevector);
	else
		tc = tcbase;

	col = texture2D(s_diffuse, tc);

	if (UPPERLOWER)
	{
		vec4 uc = texture2D(s_upper, tc);
		col.rgb += uc.rgb*e_uppercolour*uc.a;

		vec4 lc = texture2D(s_lower, tc);
		col.rgb += lc.rgb*e_lowercolour*lc.a;
	}

	vec3 bumps = vec3(0,0,1);
	if (BUMP || SPECULAR)
	{
		bumps = normalize(vec3(texture2D(s_normalmap, tc)) - 0.5);
		vec4 specs = texture2D(s_specular, tc);

		vec3 halfdir = normalize(normalize(eyevector) + vec3(0.0, 0.0, 1.0));
		float spec = pow(max(dot(halfdir, bumps), 0.0), 32.0 * specs.a);
		col.rgb += cvar_gl_specular * spec * specs.rgb;
	}

	if (REFLECTCUBEMASK)
	{
		vec3 rtc = reflect(-eyevector, bumps);
		rtc = rtc.x*invsurface[0] + rtc.y*invsurface[1] + rtc.z*invsurface[2];
		rtc = (m_model * vec4(rtc.xyz,0.0)).xyz;
		col.rgb += texture2D(s_reflectmask, tc).rgb * textureCube(s_reflectcube, rtc).rgb;
}

	col.rgb *= light;

	if (FULLBRIGHT)
	{
		vec4 fb = texture2D(s_fullbright, tc);
//		col.rgb = mix(col.rgb, fb.rgb, fb.a);
		col.rgb += fb.rgb * fb.a;
	}

	gl_FragColor = fog4(col * e_colourident);
}
#endif
