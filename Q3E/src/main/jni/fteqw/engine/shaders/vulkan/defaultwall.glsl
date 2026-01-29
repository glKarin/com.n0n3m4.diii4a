!!permu FULLBRIGHT
!!permu BUMP
!!permu REFLECTCUBEMASK
!!cvarf r_glsl_offsetmapping=0.0
!!cvarf r_glsl_offsetmapping_scale=0.04
!!cvarf gl_specular=0.3
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
!!samps diffuse normalmap specular fullbright lightmap
!!samps deluxemap reflectmask reflectcube
!!argb vertexlit=0
//!!samps =EIGHTBIT paletted 1
//!!argb eightbit=0
!!argf mask=1.0
!!argb masklt=false
!!permu FOG
//!!permu DELUXE
//!!permu LIGHTSTYLED //this seems to be breaking nvidia drivers if set from the engine, despite us not using it...

const bool DELUXE = false;
#define SPECULAR (cvar_gl_specular>0)

#include "sys/defs.h"

//this is what normally draws all of your walls, even with rtlights disabled
//note that the '286' preset uses drawflat_walls instead.

#include "sys/fog.h"
layout(location=1) varying vec3 eyevector;
layout(location=2) varying vec2 basetc;
layout(location=3) varying vec4 vc;
layout(location=4) varying mat3 invsurface;
#ifdef LIGHTSTYLED
//we could use an offset, but that would still need to be per-surface which would break batches
//fixme: merge attributes?
varying vec2 lm0, lm1, lm2, lm3;
#else
layout(location=0) varying vec2 lm0;
#endif

#ifdef VERTEX_SHADER
void main ()
{
	if (OFFSETMAPPING || SPECULAR || REFLECTCUBEMASK)
	{
		vec3 eyeminusvertex = e_eyepos - v_position.xyz;
		eyevector.x = dot(eyeminusvertex, v_svector.xyz);
		eyevector.y = dot(eyeminusvertex, v_tvector.xyz);
		eyevector.z = dot(eyeminusvertex, v_normal.xyz);
	}
	if (REFLECTCUBEMASK)
	{
		invsurface[0] = v_svector;
		invsurface[1] = v_tvector;
		invsurface[2] = v_normal;
	}
	basetc = v_texcoord;
	lm0 = v_lmcoord;
#ifdef LIGHTSTYLED
	lm1 = v_lmcoord2;
	lm2 = v_lmcoord3;
	lm3 = v_lmcoord4;
#endif
	if (arg_vertexlit)
		vc = v_colour;
	gl_Position = ftetransform();
}
#endif


#ifdef FRAGMENT_SHADER

//samplers

//for 8bit
#define s_colourmap	s_t0


#include "sys/offsetmapping.h"
void main ()
{
//adjust texture coords for offsetmapping
	vec2 tc = basetc;
	if (OFFSETMAPPING)
		tc = offsetmap(s_normalmap, tc, eyevector);

//yay, regular texture!
	gl_FragColor = texture2D(s_diffuse, tc);

	vec3 norm;
	if (BUMP && (DELUXE || SPECULAR || REFLECTCUBEMASK))
		norm = normalize(texture2D(s_normalmap, tc).rgb - 0.5);
	else if (SPECULAR || DELUXE || REFLECTCUBEMASK)
		norm = vec3(0, 0, 1);	//specular lighting expects this to exist.

	vec3 lightmaps;
	if (arg_vertexlit)
		lightmaps = vc.rgb * e_lmscale.rgb;
	else
	{
		//modulate that by the lightmap(s) including deluxemap(s)
#ifdef LIGHTSTYLED
		if (DELUXE)
		{
			lightmaps  = texture2D(s_lightmap0, lm0).rgb * e_lmscale[0].rgb * dot(norm, 2.0*texture2D(s_deluxemap0, lm0).rgb-0.5);
			lightmaps += texture2D(s_lightmap1, lm1).rgb * e_lmscale[1].rgb * dot(norm, 2.0*texture2D(s_deluxemap1, lm1).rgb-0.5);
			lightmaps += texture2D(s_lightmap2, lm2).rgb * e_lmscale[2].rgb * dot(norm, 2.0*texture2D(s_deluxemap2, lm2).rgb-0.5);
			lightmaps += texture2D(s_lightmap3, lm3).rgb * e_lmscale[3].rgb * dot(norm, 2.0*texture2D(s_deluxemap3, lm3).rgb-0.5);
		}
		else
		{
			lightmaps  = texture2D(s_lightmap0, lm0).rgb * e_lmscale[0].rgb;
			lightmaps += texture2D(s_lightmap1, lm1).rgb * e_lmscale[1].rgb;
			lightmaps += texture2D(s_lightmap2, lm2).rgb * e_lmscale[2].rgb;
			lightmaps += texture2D(s_lightmap3, lm3).rgb * e_lmscale[3].rgb;
		}
#else
		/*if (arg_eightbit)
		{
			//optional: round the lightmap coords to ensure all pixels within a texel have different lighting values either. it just looks wrong otherwise.
			//don't bother if its lightstyled, such cases will have unpredictable correlations anyway.
			//FIXME: this rounding is likely not correct with respect to software rendering. oh well.
			vec2 nearestlm0 = floor(lm0 * 256.0*8.0)/(256.0*8.0);
			lightmaps = (texture2D(s_lightmap, nearestlm0) * e_lmscale).rgb;
		}
		else*/
			lightmaps = (texture2D(s_lightmap, lm0) * e_lmscale).rgb;
		//modulate by the  bumpmap dot light
		if (DELUXE)
		{
			vec3 delux = 2.0*(texture2D(s_deluxemap, lm0).rgb-0.5);
			lightmaps *= 1.0 / max(0.25, delux.z);	//counter the darkening from deluxmaps
			lightmaps *= dot(norm, delux);
		}
#endif
	}

//add in specular, if applicable.
	if (SPECULAR)
	{
		vec4 specs = texture2D(s_specular, tc);
		vec3 halfdir;
		if (DELUXE)
		{
//not lightstyled...
			halfdir = normalize(normalize(eyevector) + 2.0*(texture2D(s_deluxemap0, lm0).rgb-0.5));	//this norm should be the deluxemap info instead
		}
		else
		{
			halfdir = normalize(normalize(eyevector) + vec3(0.0, 0.0, 1.0));	//this norm should be the deluxemap info instead
		}
		float spec = pow(max(dot(halfdir, norm), 0.0), 32.0 * specs.a);
		spec *= cvar_gl_specular;
//NOTE: rtlights tend to have a *4 scaler here to over-emphasise the effect because it looks cool.
//As not all maps will have deluxemapping, and the double-cos from the light util makes everything far too dark anyway,
//we default to something that is not garish when the light value is directly infront of every single pixel.
//we can justify this difference due to the rtlight editor etc showing the *4.
		gl_FragColor.rgb += spec * specs.rgb;
	}

	if (REFLECTCUBEMASK)
	{
		vec3 rtc = reflect(-eyevector, norm);
		rtc = rtc.x*invsurface[0] + rtc.y*invsurface[1] + rtc.z*invsurface[2];
		rtc = (m_model * vec4(rtc.xyz,0.0)).xyz;
		gl_FragColor.rgb += texture2D(s_reflectmask, tc).rgb * textureCube(s_reflectcube, rtc).rgb;
	}

	/*if (arg_eightbit)
	{
		//FIXME: with this extra flag, half the permutations are redundant.
		lightmaps *= 0.5;	//counter the fact that the colourmap contains overbright values and logically ranges from 0 to 2 intead of to 1.
		float pal = texture2D(s_paletted, tc).r;	//the palette index. hopefully not interpolated.
		lightmaps -= 1.0 / 128.0;	//software rendering appears to round down, so make sure we favour the lower values instead of rounding to the nearest
		gl_FragColor.r = texture2D(s_colourmap, vec2(pal, 1.0-lightmaps.r)).r;	//do 3 lookups. this is to cope with lit files, would be a waste to not support those.
		gl_FragColor.g = texture2D(s_colourmap, vec2(pal, 1.0-lightmaps.g)).g;	//its not very softwarey, but re-palettizing is ugly.
		gl_FragColor.b = texture2D(s_colourmap, vec2(pal, 1.0-lightmaps.b)).b;	//without lits, it should be identical.
	}
	else*/
	{
		//now we have our diffuse+specular terms, modulate by lightmap values.
		gl_FragColor.rgb *= lightmaps.rgb;

		//add on the fullbright
		if (FULLBRIGHT)
			gl_FragColor.rgb += texture2D(s_fullbright, tc).rgb;
	}

//entity modifiers
	gl_FragColor = gl_FragColor * e_colourident;

	if (arg_mask != 1.0)
	{
		if (arg_masklt)
		{
			if (gl_FragColor.a < arg_mask)
				discard;
		}
		else
		{
			if (gl_FragColor.a >= arg_mask)
				discard;
		}
		gl_FragColor.a = 1.0;
	}

//and finally hide it all if we're fogged.
#ifdef FOG
	gl_FragColor = fog4(gl_FragColor);
#endif
}
#endif
