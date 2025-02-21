//!!ver 100 150
//!!permu DELUXE
!!permu FULLBRIGHT
!!permu FOG
//!!permu LIGHTSTYLED
//!!permu BUMP
//!!permu SPECULAR
//!!permu REFLECTCUBEMASK
//!!cvarf r_glsl_offsetmapping_scale
//!!cvardf r_tessellation_level=5
//!!samps diffuse lightmap specular normalmap fullbright reflectmask reflectcube paletted lightmap1 lightmap2 lightmap3
!!samps diffuse fullbright lightmap

#undef SPECULAR

//#include "sys/defs.h"
#define vec4 float4
#define vec3 float3
#define vec2 float2
#define texture2D tex2D
#define mat3 float3x3
#define mat4 float4x4
	struct a2v
	{
		vec4 v_position : POSITION;
		vec2 v_texcoord : TEXCOORD0;

#if defined(OFFSETMAPPING) || defined(SPECULAR) || defined(REFLECTCUBEMASK)
		vec3 v_normal	: NORMAL;
		vec3 v_svector	: TANGENT;
		vec3 v_tvector	: BINORMAL;
#endif
#ifdef VERTEXLIT
		vec4 v_colour	: COLOR0;
#else
		vec2 v_lmcoord  : TEXCOORD1;
#endif
	};
	struct v2f
	{
		#ifndef FRAGMENT_SHADER
			vec4 pos: POSITION;
		#endif

		#if defined(OFFSETMAPPING) || defined(SPECULAR) || defined(REFLECTCUBEMASK) || defined(FOG)
			vec3 eyevector : TEXCOORD4;
		#endif

		#if defined(REFLECTCUBEMASK) || defined(BUMPMODELSPACE)
			mat3 invsurface : TEXCOORD5;
		#endif

#ifdef VERTEXLIT
		vec2 tclm : TEXCOORD0;
#else
		vec4 tclm : TEXCOORD0;
#endif
		vec4 vc : COLOR0;
		#ifndef VERTEXLIT
			#ifdef LIGHTSTYLED
				//we could use an offset, but that would still need to be per-surface which would break batches
				//fixme: merge attributes?
				vec2 lm1 : TEXCOORD1, lm2 : TEXCOORD2, lm3 : TEXCOORD3;
			#endif
		#endif
	};

//this is what normally draws all of your walls, even with rtlights disabled
//note that the '286' preset uses drawflat_walls instead.

#include "sys/fog.h"

#ifdef VERTEX_SHADER
float4x4  m_modelviewprojection;
float4x4  m_modelview;
vec3 e_eyepos;
vec4 e_lmscale;
v2f main (a2v inp)
{
	v2f outp;

#if defined(OFFSETMAPPING) || defined(SPECULAR) || defined(REFLECTCUBEMASK)
	vec3 eyeminusvertex = e_eyepos - inp.v_position.xyz;
	outp.eyevector.x = dot(eyeminusvertex, inp.v_svector.xyz);
	outp.eyevector.y = dot(eyeminusvertex, inp.v_tvector.xyz);
	outp.eyevector.z = dot(eyeminusvertex, inp.v_normal.xyz);
#elif defined(FOG)
	outp.eyevector = mul(m_modelview, inp.v_position);
#endif
#if defined(REFLECTCUBEMASK) || defined(BUMPMODELSPACE)
	outp.invsurface[0] = inp.v_svector;
	outp.invsurface[1] = inp.v_tvector;
	outp.invsurface[2] = inp.v_normal;
#endif
	outp.tclm.xy = inp.v_texcoord;
#ifdef FLOW
//	outp.tclm.x += e_time * -0.5;
#endif
#ifdef VERTEXLIT
	#ifdef LIGHTSTYLED
		//FIXME, only one colour.
		outp.vc = inp.v_colour * e_lmscale[0];
	#else
		outp.vc = inp.v_colour * e_lmscale;
	#endif
#else
	outp.vc = e_lmscale;
	outp.tclm.zw = inp.v_lmcoord;
	#ifdef LIGHTSTYLED
		outp.lm1 = inp.v_lmcoord2;
		outp.lm2 = inp.v_lmcoord3;
		outp.lm3 = inp.v_lmcoord4;
	#endif
#endif
	outp.pos = mul(m_modelviewprojection, inp.v_position);

	return outp;
}
#endif










#ifdef FRAGMENT_SHADER
sampler s_diffuse	 : register(s0);
//sampler s_normalmap;
//sampler s_specular;
//sampler s_upper;
//sampler s_lower;
sampler s_fullbright  : register(s1);
//sampler s_paletted;
//sampler s_reflectcube;
//sampler s_reflectmask;
sampler s_lightmap	 : register(s2);
//sampler s_deluxmap;


//samplers
#define s_colourmap	s_t0
//sampler2D	s_colourmap;

//vec4 e_lmscale;
vec4 e_colourident;

#ifdef OFFSETMAPPING
#include "sys/offsetmapping.h"
#endif
vec4 main (v2f inp) : COLOR0
{
	vec4 gl_FragColor;
#define tc (inp.tclm.xy)
#define lm0 (inp.tclm.zw)


//adjust texture coords for offsetmapping
#ifdef OFFSETMAPPING
	vec2 tcoffsetmap = offsetmap(s_normalmap, tc, eyevector);
#define tc tcoffsetmap
#endif

#if defined(EIGHTBIT) && !defined(LIGHTSTYLED)
	//optional: round the lightmap coords to ensure all pixels within a texel have different lighting values either. it just looks wrong otherwise.
	//don't bother if its lightstyled, such cases will have unpredictable correlations anyway.
	//FIXME: this rounding is likely not correct with respect to software rendering. oh well.
#if __VERSION__ >= 130
	vec2 lmsize = vec2(textureSize(s_lightmap0, 0));
#else
	#define lmsize vec2(128.0,2048.0)
#endif
#define texelstolightmap (16.0)
	vec2 lmcoord0 = floor(lm0 * lmsize*texelstolightmap)/(lmsize*texelstolightmap);
#define lm0 lmcoord0
#endif


//yay, regular texture!
	gl_FragColor = texture2D(s_diffuse, tc);

#if defined(BUMP) && (defined(DELUXE) || defined(SPECULAR) || defined(REFLECTCUBEMASK))
	vec3 norm = normalize(texture2D(s_normalmap, tc).rgb - 0.5);
#elif defined(SPECULAR) || defined(DELUXE) || defined(REFLECTCUBEMASK)
	vec3 norm = vec3(0, 0, 1);	//specular lighting expects this to exist.
#endif

//modulate that by the lightmap(s) including deluxemap(s)
#ifdef VERTEXLIT
	#ifdef LIGHTSTYLED
	vec3 lightmaps = inp.vc.rgb;
	#else
	vec3 lightmaps = inp.vc.rgb;
	#endif
	#define delux vec3(0.0,0.0,1.0)
#else
	#ifdef LIGHTSTYLED
#error foobar
		#define delux vec3(0.0,0.0,1.0)
		vec3 lightmaps;
		#ifdef DELUXE
			lightmaps  = texture2D(s_lightmap0, lm0).rgb * e_lmscale[0].rgb * dot(norm, 2.0*texture2D(s_deluxmap0, lm0).rgb-0.5);
			lightmaps += texture2D(s_lightmap1, lm1).rgb * e_lmscale[1].rgb * dot(norm, 2.0*texture2D(s_deluxmap1, lm1).rgb-0.5);
			lightmaps += texture2D(s_lightmap2, lm2).rgb * e_lmscale[2].rgb * dot(norm, 2.0*texture2D(s_deluxmap2, lm2).rgb-0.5);
			lightmaps += texture2D(s_lightmap3, lm3).rgb * e_lmscale[3].rgb * dot(norm, 2.0*texture2D(s_deluxmap3, lm3).rgb-0.5);
		#else
			lightmaps  = texture2D(s_lightmap0, lm0).rgb * e_lmscale[0].rgb;
			lightmaps += texture2D(s_lightmap1, lm1).rgb * e_lmscale[1].rgb;
			lightmaps += texture2D(s_lightmap2, lm2).rgb * e_lmscale[2].rgb;
			lightmaps += texture2D(s_lightmap3, lm3).rgb * e_lmscale[3].rgb;
		#endif
	#else
		vec3 lightmaps = texture2D(s_lightmap, lm0).rgb;
		//modulate by the  bumpmap dot light
		#ifdef DELUXE
#error foobar
			vec3 delux = (texture2D(s_deluxmap, lm0).rgb-0.5);
			#ifdef BUMPMODELSPACE
				delux = normalize(delux*invsurface);
#else
				lightmaps *= 2.0 / max(0.25, delux.z);	//counter the darkening from deluxmaps
			#endif
			lightmaps *= dot(norm, delux);
		#else
			#define delux vec3(0.0,0.0,1.0)
		#endif
	#endif
	lightmaps *= inp.vc.rgb;
#endif

//add in specular, if applicable.
#ifdef SPECULAR
	vec4 specs = texture2D(s_specular, tc);
	vec3 halfdir = normalize(normalize(eyevector) + delux);	//this norm should be the deluxemap info instead
	float spec = pow(max(dot(halfdir, norm), 0.0), FTE_SPECULAR_EXPONENT * specs.a);
	spec *= FTE_SPECULAR_MULTIPLIER;
//NOTE: rtlights tend to have a *4 scaler here to over-emphasise the effect because it looks cool.
//As not all maps will have deluxemapping, and the double-cos from the light util makes everything far too dark anyway,
//we default to something that is not garish when the light value is directly infront of every single pixel.
//we can justify this difference due to the rtlight editor etc showing the *4.
	gl_FragColor.rgb += spec * specs.rgb;
#endif

#ifdef REFLECTCUBEMASK
	vec3 rtc = reflect(normalize(-eyevector), norm);
	rtc = rtc.x*invsurface[0] + rtc.y*invsurface[1] + rtc.z*invsurface[2];
	rtc = (m_model * vec4(rtc.xyz,0.0)).xyz;
	gl_FragColor.rgb += texture2D(s_reflectmask, tc).rgb * textureCube(s_reflectcube, rtc).rgb;
#endif

#ifdef EIGHTBIT //FIXME: with this extra flag, half the permutations are redundant.
	lightmaps *= 0.5;	//counter the fact that the colourmap contains overbright values and logically ranges from 0 to 2 intead of to 1.
	float pal = texture2D(s_paletted, tc).r;	//the palette index. hopefully not interpolated.
	lightmaps -= 1.0 / 128.0;	//software rendering appears to round down, so make sure we favour the lower values instead of rounding to the nearest
	gl_FragColor.r = texture2D(s_colourmap, vec2(pal, 1.0-lightmaps.r)).r;	//do 3 lookups. this is to cope with lit files, would be a waste to not support those.
	gl_FragColor.g = texture2D(s_colourmap, vec2(pal, 1.0-lightmaps.g)).g;	//its not very softwarey, but re-palettizing is ugly.
	gl_FragColor.b = texture2D(s_colourmap, vec2(pal, 1.0-lightmaps.b)).b;	//without lits, it should be identical.
#else
	//now we have our diffuse+specular terms, modulate by lightmap values.
	gl_FragColor.rgb *= lightmaps.rgb;

//add on the fullbright
#ifdef FULLBRIGHT
	vec4 fb = texture2D(s_fullbright, tc);
	gl_FragColor.rgb += fb.rgb*fb.a;
#endif
#endif

//entity modifiers
	gl_FragColor = gl_FragColor * e_colourident;

#if defined(MASK)
#if defined(MASKLT)
	if (gl_FragColor.a < MASK)
		discard;
#else
	if (gl_FragColor.a >= MASK)
		discard;
#endif
	gl_FragColor.a = 1.0;	//alpha blending AND alpha testing usually looks stupid, plus it screws up our fog.
#endif

//and finally hide it all if we're fogged.
#ifdef FOG
	gl_FragColor = fog4(gl_FragColor, length(inp.eyevector));
#endif
	return gl_FragColor;
}
#endif

