!!permu FOG
!!samps 5

#include "sys/fog.h"

//FIXME: too lazy to implement this right now. rtlights on d3d9 are just bad right now.
#undef RTLIGHT
#undef PCF
#undef CUBE

struct a2v
{
	float3 pos: POSITION0;
	float2 tc: TEXCOORD0;
	float2 lm: TEXCOORD1;
	float4 vc: COLOR;

#ifdef RTLIGHT
	attribute vec3 v_normal;
	attribute vec3 v_svector;
	attribute vec3 v_tvector;
#endif
};
struct v2f
{
	float1 depth: TEXCOORD1;
	float4 tclm: TEXCOORD0;
	float4 vc: COLOR;

#ifdef RTLIGHT
	varying vec3 lightvector;
//	#if defined(SPECULAR) || defined(OFFSETMAPPING)
//		varying vec3 eyevector;
//	#endif
	#if defined(PCF) || defined(CUBE) || defined(SPOT)
		varying vec4 vtexprojcoord;
	#endif
#endif

#ifndef FRAGMENT_SHADER
	float4 pos: POSITION;
#endif
};





#ifdef VERTEX_SHADER

float4x4  m_model;
float4x4  m_view;
float4x4  m_projection;

#ifdef RTLIGHT
	uniform vec3 l_lightposition;
//	#if defined(SPECULAR) || defined(OFFSETMAPPING)
//		uniform vec3 e_eyepos;
//	#endif
	#if defined(PCF) || defined(CUBE) || defined(SPOT)
		uniform mat4 l_cubematrix;
	#endif
#endif

float3 e_lmscale;

v2f main (a2v inp)
{
	v2f outp;
	outp.tclm = float4(inp.tc, inp.lm);
	outp.vc = inp.vc;

	float4 pos = float4(inp.pos, 1);
	pos = mul(m_model, pos);
	pos = mul(m_view, pos);
	outp.depth = pos.z;
	pos = mul(m_projection, pos);
	outp.pos = pos;

	#ifdef RTLIGHT
		//light position is in model space, which is handy.
		vec3 lightminusvertex = l_lightposition - v_position.xyz;

		//no bumpmapping, so we can just use distance without regard for actual surface direction. we still do scalecos stuff. you might notice it on steep slopes.
		lightvector = lightminusvertex;
//		lightvector.x = dot(lightminusvertex, v_svector.xyz);
//		lightvector.y = dot(lightminusvertex, v_tvector.xyz);
//		lightvector.z = dot(lightminusvertex, v_normal.xyz);

//		#if defined(SPECULAR)||defined(OFFSETMAPPING)
//			vec3 eyeminusvertex = e_eyepos - v_position.xyz;
//			eyevector.x = dot(eyeminusvertex, v_svector.xyz);
//			eyevector.y = dot(eyeminusvertex, v_tvector.xyz);
//			eyevector.z = dot(eyeminusvertex, v_normal.xyz);
//		#endif
		#if defined(PCF) || defined(SPOT) || defined(CUBE)
			//for texture projections/shadowmapping on dlights
			vtexprojcoord = (l_cubematrix*vec4(v_position.xyz, 1.0));
		#endif
	#else
		outp.vc.rgb *= e_lmscale.rgb;
	#endif

	return outp;
}
#endif




#ifdef FRAGMENT_SHADER
//four texture passes
sampler s_t0;
sampler s_t1;
sampler s_t2;
sampler s_t3;

//mix values
sampler s_t4;

#ifdef PCF
	uniform sampler2DShadow s_t5;
	#include "sys/pcf.h"
#endif
#ifdef CUBE
	uniform samplerCube s_t6;
#endif

#ifdef RTLIGHT
	uniform float l_lightradius;
	uniform vec3 l_lightcolour;
	uniform vec3 l_lightcolourscale;
#endif














float4 main (v2f inp) : COLOR
{
	float2 lm = inp.tclm.zw;
	float2 tc = inp.tclm.xy;
	float4 vc = inp.vc;

	float4 r;
	float4 m = tex2D(s_t4, lm);

	r  = tex2D(s_t0, tc)*m.r;
	r += tex2D(s_t1, tc)*m.g;
	r += tex2D(s_t2, tc)*m.b;
	r += tex2D(s_t3, tc)*(1.0 - (m.r + m.g + m.b));
	r.a = 1.0;

	//vertex colours provide a scaler that applies even through rtlights.
	r *= vc;

#ifdef RTLIGHT
	vec3 nl = normalize(lightvector);
	float colorscale = max(1.0 - (dot(lightvector, lightvector)/(l_lightradius*l_lightradius)), 0.0);
	vec3 diff;
//	#ifdef BUMP
//		colorscale *= (l_lightcolourscale.x + l_lightcolourscale.y * max(dot(bumps, nl), 0.0));
//	#else
		colorscale *= (l_lightcolourscale.x + l_lightcolourscale.y * max(dot(vec3(0.0, 0.0, 1.0), nl), 0.0));
//	#endif

//	#ifdef SPECULAR
//		vec3 halfdir = normalize(normalize(eyevector) + nl);
//		float spec = pow(max(dot(halfdir, bumps), 0.0), 32.0 * specs.a);
//		diff += l_lightcolourscale.z * spec * specs.rgb;
//	#endif



	#if defined(SPOT)
		if (vtexprojcoord.w < 0.0) discard;
		vec2 spot = ((vtexprojcoord.st)/vtexprojcoord.w);
		colorscale *= 1.0-(dot(spot,spot));
	#endif
	#ifdef PCF
		colorscale *= ShadowmapFilter(s_t5);
	#endif

	r.rgb *= colorscale * l_lightcolour;

	#ifdef CUBE
		r.rgb *= textureCube(s_t6, vtexprojcoord.xyz).rgb;
	#endif

	r = fog4additive(r, inp.pos);
#else
	//lightmap is greyscale in m.a. probably we should just scale the texture mix, but precision errors when editing make me paranoid.
	r.rgb *= m.aaa;
	r = fog4(r, inp.depth);
#endif
	return r;
}
#endif