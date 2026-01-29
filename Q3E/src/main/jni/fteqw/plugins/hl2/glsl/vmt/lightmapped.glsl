!!ver 110
!!permu FOG
!!permu BUMP
!!permu LIGHTSTYLED
!!permu FULLBRIGHT
!!permu REFLECTCUBEMASK
!!permu NOFOG
!!samps diffuse

!!samps lightmap
!!samps =LIGHTSTYLED lightmap1 lightmap2 lightmap3

!!samps =BUMP normalmap
!!samps =FULLBRIGHT fullbright

// envmaps only
!!samps =REFLECTCUBEMASK reflectmask reflectcube

!!permu FAKESHADOWS
!!cvardf r_glsl_pcf
!!samps =FAKESHADOWS shadowmap

#ifndef ENVTINT
#define ENVTINT 1.0,1.0,1.0
#endif

#ifndef ENVSAT
#define ENVSAT 1.0,1.0,1.0
#endif

#include "sys/defs.h"

varying vec2 tex_c;

varying vec2 lm0;

#ifdef LIGHTSTYLED
varying vec2 lm1, lm2, lm3;
#endif

#ifdef FAKESHADOWS
	varying vec4 vtexprojcoord;
#endif

/* CUBEMAPS ONLY */
#ifdef REFLECTCUBEMASK
	varying vec3 eyevector;
	varying mat3 invsurface;
#endif

#ifdef VERTEX_SHADER
	void lightmapped_init(void)
	{
		lm0 = v_lmcoord;
	#ifdef LIGHTSTYLED
		lm1 = v_lmcoord2;
		lm2 = v_lmcoord3;
		lm3 = v_lmcoord4;
	#endif
	}

	void main ()
	{
		lightmapped_init();
		tex_c = v_texcoord;
		gl_Position = ftetransform();

	/* CUBEMAPS ONLY */
	#ifdef REFLECTCUBEMASK
		invsurface = mat3(v_svector, v_tvector, v_normal);

		vec3 eyeminusvertex = e_eyepos - v_position.xyz;
		eyevector.x = dot(eyeminusvertex, v_svector.xyz);
		eyevector.y = dot(eyeminusvertex, v_tvector.xyz);
		eyevector.z = dot(eyeminusvertex, v_normal.xyz);
	#endif

	#ifdef FAKESHADOWS
		vtexprojcoord = (l_cubematrix*vec4(v_position.xyz, 1.0));
	#endif
	}
#endif

#ifdef FRAGMENT_SHADER
	#include "sys/fog.h"
	
#ifdef FAKESHADOWS
	#include "sys/pcf.h"
#endif

	#ifdef LIGHTSTYLED
		#define LIGHTMAP0 texture2D(s_lightmap0, lm0).rgb
		#define LIGHTMAP1 texture2D(s_lightmap1, lm1).rgb
		#define LIGHTMAP2 texture2D(s_lightmap2, lm2).rgb
		#define LIGHTMAP3 texture2D(s_lightmap3, lm3).rgb
	#else
		#define LIGHTMAP texture2D(s_lightmap, lm0).rgb 
	#endif

	vec3 lightmap_fragment()
	{
		vec3 lightmaps;

#ifdef LIGHTSTYLED
		lightmaps  = LIGHTMAP0 * e_lmscale[0].rgb;
		lightmaps += LIGHTMAP1 * e_lmscale[1].rgb;
		lightmaps += LIGHTMAP2 * e_lmscale[2].rgb;
		lightmaps += LIGHTMAP3 * e_lmscale[3].rgb;
#else
		lightmaps  = LIGHTMAP * e_lmscale.rgb;
#endif
		return lightmaps;
	}

	vec3 env_saturation(vec3 rgb, float adjustment) {
	    vec3 intensity = vec3(dot(rgb, vec3(0.2126,0.7152,0.0722)));
	    return mix(intensity, rgb, adjustment);
	}

	void main (void)
	{
		vec4 diffuse_f;

		diffuse_f = texture2D(s_diffuse, tex_c);
		diffuse_f.rgb *= e_colourident.rgb;

#ifdef MASKLT
		if (diffuse_f.a < float(MASK))
			discard;
#endif

#ifdef FAKESHADOWS
		diffuse_f.rgb *= ShadowmapFilter(s_shadowmap, vtexprojcoord);
#endif

		/* deluxemapping isn't working on Source BSP yet */
		diffuse_f.rgb *= lightmap_fragment();

/* CUBEMAPS ONLY */
#ifdef REFLECTCUBEMASK
	/* We currently only use the normal/bumpmap for cubemap warping. move this block out once we do proper radiosity normalmapping */
	#ifdef BUMP
		/* Source's normalmaps are in the DX format where the green channel is flipped */
		vec4 normal_f = texture2D(s_normalmap, tex_c);
		normal_f.g = 1.0 - normal_f.g;
		normal_f.rgb = normalize(normal_f.rgb - 0.5);
	#else
		vec4 normal_f = vec4(0.0,0.0,1.0,0.0);
	#endif


	#if defined(ENVFROMMASK)
		/* We have a dedicated reflectmask */
		#define refl texture2D(s_reflectmask, tex_c).r
	#else
		/* when ENVFROMBASE is set or a normal isn't present, we're getting the reflectivity info from the diffusemap's alpha channel */
		#if defined(ENVFROMBASE) || !defined(BUMP)
			#define refl 1.0 - diffuse_f.a
		#else
			/* when ENVFROMNORM is set, we don't invert the refl */
			#if defined(ENVFROMNORM)
				#define refl texture2D(s_normalmap, tex_c).a
			#else
				#define refl 1.0 - texture2D(s_normalmap, tex_c).a
			#endif
		#endif
	#endif
	

		vec3 cube_c = reflect(-eyevector, normal_f.rgb);
		vec3 cube_tint = vec3(ENVTINT);
		vec3 cube_sat = vec3(ENVSAT);
		cube_c = cube_c.x * invsurface[0] + cube_c.y * invsurface[1] + cube_c.z * invsurface[2];
		cube_c = (m_model * vec4(cube_c.xyz, 0.0)).xyz;
		vec3 cube_t = env_saturation(textureCube(s_reflectcube, cube_c).rgb, cube_sat.r);
		cube_t.r *= cube_tint.r;
		cube_t.g *= cube_tint.g;
		cube_t.b *= cube_tint.b;
		diffuse_f.rgb += (cube_t * vec3(refl,refl,refl));
#endif

	#ifdef FULLBRIGHT
		diffuse_f.rgb += texture2D(s_fullbright, tex_c).rgb * texture2D(s_fullbright, tex_c).a;
	#endif

	#ifdef NOFOG
		gl_FragColor = diffuse_f;
	#else
		gl_FragColor = fog4(diffuse_f);
	#endif
	}
#endif
