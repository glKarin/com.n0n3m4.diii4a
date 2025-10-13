struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
	float2 lmtc: TEXCOORD0;
	float4 vcol: COLOR0;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float2 tc: TEXCOORD0;
	float2 lmtc: TEXCOORD1;
	float4 vcol: COLOR0;
	float3 vtexprojcoord: TEXCOORD2;
	float3 vtexprojcoord: TEXCOORD2;
};

#include <ftedefs.h>

#ifdef VERTEX_SHADER
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_model, inp.pos);
		outp.pos = mul(m_view, outp.pos);
		outp.pos = mul(m_projection, outp.pos);
		outp.tc = inp.tc;
		outp.lmtc = inp.lmtc;
		outp.vcol = inp.vcol;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	Texture2D shaderTexture[7];
	SamplerState SampleType[7];

	float4 main (v2f inp) : SV_TARGET
	{
		float4 result;

		float4 base = shaderTexture[0].Sample(SampleType[0], inp.tc);
#ifdef BUMP
		float4 bump = shaderTexture[1].Sample(SampleType[1], inp.tc);
#else
		float4 bump = float4(0, 0, 1, 0);
#endif
		float4 spec = shaderTexture[2].Sample(SampleType[2], inp.tc);
#ifdef CUBE
		float4 cubemap = shaderTexture[3].Sample(SampleType[3], inp.vtexprojcoord);
#endif
		//shadowmap 2d
#ifdef LOWER
		float4 lower = shaderTexture[5].Sample(SampleType[5], inp.tc);
		base += lower;
#endif
#ifdef UPPER
		float4 upper = shaderTexture[6].Sample(SampleType[6], inp.tc);
		base += upper;
#endif

		float lightscale = max(1.0 - (dot(inp.lightvector,inp.lightvector)/(l_lightradius*l_lightradius)), 0.0);
		float3 nl = normalize(inp.lightvector);
		float bumpscale = max(dot(bump.xyz, nl), 0.0);
		float3 halfdir = normalize(normalize(eyevector) + nl);
		float specscale = pow(max(dot(halfdir, bumps), 0.0), 32.0 * spec.a);

		result.a = base.a;
		result.rgb = base.rgb * (l_lightcolourscale.x + l_lightcolourscale.y * bumpscale);	//amient light + diffuse
		result.rgb += spec.rgb * l_lightcolourscale.z * specscale;	//specular

		result.rgb *= lightscale;	//fade light by distance

#ifdef CUBE
		result.rgb *= cubemap.rgb;	//fade by cubemap
#endif

		return result;
	}
#endif