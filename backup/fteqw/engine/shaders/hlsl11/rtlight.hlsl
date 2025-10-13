!!samps diffuse normalmap specular upper lower shadowmap projectionmap
!!permu BUMP
!!permu FRAMEBLEND
!!permu SKELETAL
!!permu UPPERLOWER
!!permu FOG
!!cvarf r_glsl_offsetmapping_scale
!!cvardf r_glsl_pcf=5


//this is the main shader responsible for realtime dlights.

//texture units:
//s0=diffuse, s1=normal, s2=specular, s3=shadowmap
//custom modifiers:
//PCF(shadowmap)
//CUBEPROJ(projected cubemap)
//SPOT(projected circle
//CUBESHADOW

#undef CUBE	//engine cannot load cubemaps properly with d3d yet.

#ifndef r_glsl_pcf
#error r_glsl_pcf wasn't defined
#endif
#if r_glsl_pcf < 1
	#undef r_glsl_pcf
	#define r_glsl_pcf 9
#endif

#ifdef UPPERLOWER
#define UPPER
#define LOWER
#endif


struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
	float3 n: NORMAL;
	float3 s: TANGENT;
	float3 t: BINORMAL;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float2 tc: TEXCOORD0;
	float3 lightvector: TEXCOORD1;
	float3 eyevector: TEXCOORD2;
	float3 vtexprojcoord: TEXCOORD3;
};

#include <ftedefs.h>

#ifdef VERTEX_SHADER
	v2f main (a2v inp)
	{
		v2f outp;
		float4 wpos;
		wpos = mul(m_model, inp.pos);
		outp.pos = mul(m_view, wpos);
		outp.pos = mul(m_projection, outp.pos);
		outp.tc = inp.tc.xy;
		float3 lightminusvertex = l_lightposition - wpos.xyz;
		outp.lightvector.x = -dot(lightminusvertex, inp.s.xyz);
		outp.lightvector.y = dot(lightminusvertex, inp.t.xyz);
		outp.lightvector.z = dot(lightminusvertex, inp.n.xyz);
		float3 eyeminusvertex = e_eyepos - wpos.xyz;
		outp.eyevector.x = -dot(eyeminusvertex, inp.s.xyz);
		outp.eyevector.y = dot(eyeminusvertex, inp.t.xyz);
		outp.eyevector.z = dot(eyeminusvertex, inp.n.xyz);
		outp.vtexprojcoord = mul(l_cubematrix, wpos).xyz;
		return outp;
	}
#endif


#ifdef FRAGMENT_SHADER

	Texture2D t_shadowmap : register(t0);
	TextureCube t_projectionmap : register(t1);
	Texture2D t_diffuse : register(t2);
	Texture2D t_normalmap : register(t3);
	Texture2D t_specular : register(t4);
	Texture2D t_upper : register(t5);
	Texture2D t_lower : register(t6);

	SamplerComparisonState s_shadowmap : register(s0);
	SamplerState s_projectionmap	 : register(s1);
	SamplerState s_diffuse : register(s2);
	SamplerState s_normalmap : register(s3);
	SamplerState s_specular : register(s4);
	SamplerState s_upper : register(s5);
	SamplerState s_lower : register(s6);


#ifdef PCF
float3 ShadowmapCoord(float3 vtexprojcoord)
{
#ifdef SPOT
	//bias it. don't bother figuring out which side or anything, its not needed
	//l_projmatrix contains the light's projection matrix so no other magic needed
	vtexprojcoord.z -= 0.015;
	return (vtexprojcoord.xyz + float3(1.0, 1.0, 1.0)) * float3(0.5, 0.5, 0.5);
//#elif defined(CUBESHADOW)
//	vec3 shadowcoord = vshadowcoord.xyz / vshadowcoord.w;
//	#define dosamp(x,y) shadowCube(s_t4, shadowcoord + vec2(x,y)*texscale.xy).r
#else
	//figure out which axis to use
	//texture is arranged thusly:
	//forward left  up
	//back    right down
	float3 dir = abs(vtexprojcoord.xyz);
	//assume z is the major axis (ie: forward from the light)
	float3 t = vtexprojcoord.xyz;
	float ma = dir.z;
	float3 axis = float3(0.5/3.0, 0.5/2.0, 0.5);
	if (dir.x > ma)
	{
		ma = dir.x;
		t = vtexprojcoord.zyx;
		axis.x = 0.5;
	}
	if (dir.y > ma)
	{
		ma = dir.y;
		t = vtexprojcoord.xzy;
		axis.x = 2.5/3.0;
	}
	//if the axis is negative, flip it.
	if (t.z > 0.0)
	{
		axis.y = 1.5/2.0;
		t.z = -t.z;
	}

	//we also need to pass the result through the light's projection matrix too
	//the 'matrix' we need only contains 5 actual values. and one of them is a -1. So we might as well just use a vec4.
	//note: the projection matrix also includes scalers to pinch the image inwards to avoid sampling over borders, as well as to cope with non-square source image
	//the resulting z is prescaled to result in a value between -0.5 and 0.5.
	//also make sure we're in the right quadrant type thing
	return axis + ((l_shadowmapproj.xyz*t.xyz + float3(0.0, 0.0, l_shadowmapproj.w)) / -t.z);
#endif
}

float ShadowmapFilter(float3 vtexprojcoord)
{
	float3 shadowcoord = ShadowmapCoord(vtexprojcoord);

//	#define dosamp(x,y) shadow2D(s_t4, shadowcoord.xyz + (vec3(x,y,0.0)*l_shadowmapscale.xyx)).r

//	#define dosamp(x,y) (t_shadowmap.Sample(s_shadowmap, shadowcoord.xy + (float2(x,y)*l_shadowmapscale.xy)).r < shadowcoord.z)
	#define dosamp(x,y) (t_shadowmap.SampleCmpLevelZero(s_shadowmap, shadowcoord.xy+(float2(x,y)*l_shadowmapscale.xy), shadowcoord.z))

	float s = 0.0;
	#if r_glsl_pcf >= 1 && r_glsl_pcf < 5
		s += dosamp(0.0, 0.0);
		return s;
	#elif r_glsl_pcf >= 5 && r_glsl_pcf < 9
		s += dosamp(-1.0, 0.0);
		s += dosamp(0.0, -1.0);
		s += dosamp(0.0, 0.0);
		s += dosamp(0.0, 1.0);
		s += dosamp(1.0, 0.0);
		return s * (1.0/5.0);
	#else
		s += dosamp(-1.0, -1.0);
		s += dosamp(-1.0, 0.0);
		s += dosamp(-1.0, 1.0);
		s += dosamp(0.0, -1.0);
		s += dosamp(0.0, 0.0);
		s += dosamp(0.0, 1.0);
		s += dosamp(1.0, -1.0);
		s += dosamp(1.0, 0.0);
		s += dosamp(1.0, 1.0);
		return s * (1.0/9.0);
	#endif
}
#endif


	float4 main (v2f inp) : SV_TARGET
	{
		float2 tc = inp.tc;	//TODO: offsetmapping.

		float4 base = t_diffuse.Sample(s_diffuse, tc);
#ifdef BUMP
		float4 bump = t_normalmap.Sample(s_normalmap, tc);
		bump.rgb = normalize(bump.rgb - 0.5);
#else
		float4 bump = float4(0, 0, 1, 0);
#endif
		float4 spec = t_specular.Sample(s_specular, tc);
#ifdef CUBE
		float4 cubemap = t_projectionmap.Sample(s_projectionmap, inp.vtexprojcoord);
#endif
#ifdef LOWER
		float4 lower = t_lower.Sample(s_lower, tc);
		base += lower;
#endif
#ifdef UPPER
		float4 upper = t_upper.Sample(s_upper, tc);
		base += upper;
#endif

		float lightscale = max(1.0 - (dot(inp.lightvector,inp.lightvector)/(l_lightradius*l_lightradius)), 0.0);
		float3 nl = normalize(inp.lightvector);
		float bumpscale = max(dot(bump.xyz, nl), 0.0);
		float3 halfdir = normalize(normalize(inp.eyevector) + nl);
		float specscale = pow(max(dot(halfdir, bump.rgb), 0.0), 32.0 * spec.a);

		float4 result;
		result.a    = base.a;
		result.rgb  = base.rgb * (l_lightcolourscale.x + l_lightcolourscale.y * bumpscale);	//amient light + diffuse
		result.rgb += spec.rgb * l_lightcolourscale.z * specscale;	//specular

		result.rgb *= lightscale * l_colour;	//fade light by distance and light colour.

#ifdef CUBE
		result.rgb *= cubemap.rgb;	//fade by cubemap
#endif

#ifdef PCF
		result.rgb *= ShadowmapFilter(inp.vtexprojcoord);
#endif

		//TODO: fog
		return result;
	}
#endif
