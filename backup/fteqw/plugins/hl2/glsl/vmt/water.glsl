!!cvardf r_glsl_turbscale_reflect=1	//simpler scaler
!!cvardf r_glsl_turbscale_refract=1	//simpler scaler
!!permu REFLECTCUBEMASK
!!samps diffuse normalmap
!!samps	refract=0	//always present
!!samps reflect=1
!!samps =REFLECTCUBEMASK reflectcube
!!permu FOG

#include "sys/defs.h"

//modifier: REFLECT		(s_t2 is a reflection instead of diffusemap)
//modifier: STRENGTH_REFL	(distortion strength - 0.1 = fairly gentle, 0.2 = big waves)
//modifier: STRENGTH_REFL	(distortion strength - 0.1 = fairly gentle, 0.2 = big waves)
//modifier: FRESNEL_EXP	(5=water)
//modifier: TXSCALE		(wave size - 0.2)
//modifier: RIPPLEMAP		(s_t3 contains a ripplemap
//modifier: TINT_REFR		(some colour value)
//modifier: TINT_REFL		(some colour value)
//modifier: ALPHA		(mix in the normal water texture over the top)
//modifier: USEMODS		(use single-texture scrolling via tcmods - note, also forces the engine to actually use tcmod etc)

//a few notes on DP compat:
//'dpwater' makes numerous assumptions about DP internals
//by default there is a single pass that uses the pass's normal tcmods
//the fresnel has a user-supplied min+max rather than an exponent
//both parts are tinted individually
//if alpha is enabled, the regular water texture is blended over the top, again using the same crappy tcmods...

//legacy crap
#ifndef FRESNEL
#define FRESNEL 5.0
#endif
#ifndef TINT
#define TINT 0.7,0.8,0.7
#endif
#ifndef STRENGTH
#define STRENGTH 0.25
#endif
#ifndef TXSCALE
#define TXSCALE 1
#endif

//current values (referring to legacy defaults where needed)
#ifndef FRESNEL_EXP
#define FRESNEL_EXP 5.0
#endif
#ifndef FRESNEL_MIN
#define FRESNEL_MIN 0.0
#endif
#ifndef FRESNEL_RANGE
#define FRESNEL_RANGE 1.0
#endif
#ifndef STRENGTH_REFL
#define STRENGTH_REFL STRENGTH
#endif
#ifndef STRENGTH_REFR
#define STRENGTH_REFR STRENGTH
#endif
#ifndef TXSCALE1
#define TXSCALE1 TXSCALE
#endif
#ifndef TXSCALE2
#define TXSCALE2 TXSCALE
#endif
#ifndef TINT_REFR
#define TINT_REFR TINT
#endif
#ifndef TINT_REFL
#define TINT_REFL 1.0,1.0,1.0
#endif
#ifndef FOGTINT
#define FOGTINT 0.2,0.3,0.2
#endif

varying vec2 tc;
varying vec4 tf;
varying vec3 norm;
varying vec3 eye;

#ifdef VERTEX_SHADER
void main (void)
{
	tc = v_texcoord.st;
	tf = ftetransform();
	norm = v_normal;
	eye = e_eyepos - v_position.xyz;
	gl_Position = ftetransform();
}
#endif

#ifdef FRAGMENT_SHADER
#include "sys/fog.h"


void main (void)
{
	vec2 stc;	//screen tex coords
	vec2 ntc;	//normalmap/diffuse tex coords
	vec3 n, refr, refl;
	float fres;
	float depth;
	stc = (1.0 + (tf.xy / tf.w)) * 0.5;
	//hack the texture coords slightly so that there are less obvious gaps
	stc.t -= 1.5*norm.z/1080.0;

#if 0//def USEMODS
	ntc = tc;
	n = texture2D(s_normalmap, ntc).xyz - 0.5;
#else
	//apply q1-style warp, just for kicks
	ntc.s = tc.s + sin(tc.t+e_time)*0.125;
	ntc.t = tc.t + sin(tc.s+e_time)*0.125;

	//generate the two wave patterns from the normalmap
	n = (texture2D(s_normalmap, vec2(TXSCALE1)*tc + vec2(e_time*0.1, 0.0)).xyz);
	n += (texture2D(s_normalmap, vec2(TXSCALE2)*tc - vec2(0, e_time*0.097)).xyz);
	n -= 1.0 - 4.0/256.0;
#endif

#ifdef RIPPLEMAP
	n += texture2D(s_ripplemap, stc).rgb*3.0;
#endif
	n = normalize(n);

	//the fresnel term decides how transparent the water should be
	fres = pow(1.0-abs(dot(n, normalize(eye))), float(FRESNEL_EXP)) * float(FRESNEL_RANGE) + float(FRESNEL_MIN);

#ifdef DEPTH
	float far = #include "cvar/gl_maxdist";
	float near = #include "cvar/gl_mindist";
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
#else
	depth = 1.0;
#endif 


	//refraction image (and water fog, if possible)
	refr = texture2D(s_refract, stc + n.st*float(STRENGTH_REFR)*float(r_glsl_turbscale_refract)).rgb * vec3(TINT_REFR);
#ifdef DEPTH
	refr = mix(refr, vec3(FOGTINT), min(depth/4096.0, 1.0));
#endif

#ifdef LQWATER
	refl = textureCube(s_reflectcube, n).rgb;// * vec3(TINT_REFL);
#else
	refl = texture2D(s_reflect, stc - n.st*float(STRENGTH_REFL)*float(r_glsl_turbscale_reflect)).rgb * vec3(TINT_REFL);
#endif

	//interplate by fresnel
	refr = mix(refr, refl, fres);

#ifdef ALPHA
	vec4 ts = texture2D(s_diffuse, ntc);
	vec4 surf = fog4blend(vec4(ts.rgb, float(ALPHA)*ts.a));
	refr = mix(refr, surf.rgb, surf.a);
#else
	refr = fog3(refr);	
#endif

	//done
	gl_FragColor = vec4(refr, 1.0);
}
#endif
