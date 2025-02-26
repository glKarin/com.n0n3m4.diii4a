!!ver 100 150
!!permu FRAMEBLEND
!!permu BUMP
!!permu FOG
!!permu NOFOG
!!permu SKELETAL
!!permu FULLBRIGHT
!!permu AMBIENTCUBE
!!permu REFLECTCUBEMASK
!!samps diffuse
!!samps =BUMP normalmap
!!samps =FULLBRIGHT fullbright
!!permu FAKESHADOWS
!!cvardf r_glsl_pcf
!!samps =FAKESHADOWS shadowmap

// envmaps only
!!samps =REFLECTCUBEMASK reflectmask reflectcube
!!cvardf r_skipDiffuse

#include "sys/defs.h"

varying vec2 tex_c;
varying vec3 norm;
varying vec4 light;

/* CUBEMAPS ONLY */
#ifdef REFLECTCUBEMASK
	varying vec3 eyevector;
	varying mat3 invsurface;

#endif

#ifndef ENVTINT
#define ENVTINT 1.0,1.0,1.0
#endif

#ifndef ENVSAT
#define ENVSAT 1.0
#endif


#ifdef FAKESHADOWS
	varying vec4 vtexprojcoord;
#endif

#ifdef VERTEX_SHADER
	#include "sys/skeletal.h"

	float lambert(vec3 normal, vec3 dir)
	{
		return dot(normal, dir);
	}

	float halflambert(vec3 normal, vec3 dir)
	{
		return (dot(normal, dir) * 0.5) + 0.5;
	}

	void main (void)
	{
		vec3 n, s, t, w;
		tex_c = v_texcoord;
		gl_Position = skeletaltransform_wnst(w,n,s,t);
		norm = n = normalize(n);
		s = normalize(s);
		t = normalize(t);
		light.rgba = vec4(e_light_ambient, 1.0);

	#ifdef AMBIENTCUBE
		//no specular effect here. use rtlights for that.
		vec3 nn = norm*norm; //FIXME: should be worldspace normal.
		light.rgb = nn.x * e_light_ambientcube[(norm.x<0.0)?1:0] +
				nn.y * e_light_ambientcube[(norm.y<0.0)?3:2] +
				nn.z * e_light_ambientcube[(norm.z<0.0)?5:4];
	#else
		#ifdef HALFLAMBERT
			light.rgb += max(0.0,halflambert(n,e_light_dir)) * e_light_mul;
		#else
			light.rgb += max(0.0,dot(n,e_light_dir)) * e_light_mul;
		#endif
	#endif

/* CUBEMAPS ONLY */
#ifdef REFLECTCUBEMASK
		invsurface = mat3(s, t, n);

		vec3 eyeminusvertex = e_eyepos - w.xyz;
		eyevector.x = dot(eyeminusvertex, s.xyz);
		eyevector.y = dot(eyeminusvertex, t.xyz);
		eyevector.z = dot(eyeminusvertex, n.xyz);
#endif
		
		#ifdef FAKESHADOWS
		vtexprojcoord = (l_cubematrix*vec4(w.xyz, 1.0));
		#endif
	}
#endif


#ifdef FRAGMENT_SHADER
	#include "sys/fog.h"
	#include "sys/pcf.h"

	vec3 env_saturation(vec3 rgb, float adjustment) {
	    vec3 intensity = vec3(dot(rgb, vec3(0.2126,0.7152,0.0722)));
	    return mix(intensity, rgb, adjustment);
	}

	void main (void)
	{
		vec4 diffuse_f = texture2D(s_diffuse, tex_c);

#ifdef MASKLT
		if (diffuse_f.a < float(MASK))
			discard;
#endif

/* Normal/Bumpmap Shenanigans */
#ifdef BUMP
		/* Source's normalmaps are in the DX format where the green channel is flipped */
		vec3 normal_f = texture2D(s_normalmap, tex_c).rgb;
		normal_f.g = 1.0 - normal_f.g;
		normal_f = normalize(normal_f.rgb - 0.5);
#else
		vec3 normal_f = vec3(0.0,0.0,1.0);
#endif

/* CUBEMAPS ONLY */
#ifdef REFLECTCUBEMASK

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

		diffuse_f.rgb *= light.rgb * e_colourident.rgb;

	#ifdef FAKESHADOWS
		diffuse_f.rgb *= ShadowmapFilter(s_shadowmap, vtexprojcoord);
	#endif

	#ifdef FULLBRIGHT
		diffuse_f.rgb += texture2D(s_fullbright, tex_c).rgb * texture2D(s_fullbright, tex_c).a;
	#endif


	#if 1
		gl_FragColor = diffuse_f;
	#else
		gl_FragColor = fog4(diffuse_f);
	#endif
	}
#endif
