!!samps diffuse fullbright lightmap

struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
	float2 lmtc: TEXCOORD1;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float2 tc: TEXCOORD0;
	float2 lmtc: TEXCOORD1;
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
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	Texture2D t_lightmap : register(t2);
	SamplerState s_lightmap : register(s2);

	Texture2D t_diffuse : register(t0);
	SamplerState s_diffuse : register(s0);

	Texture2D t_fullbright : register(t1);
	SamplerState s_fullbright : register(s1);

	float4 main (v2f inp) : SV_TARGET
	{
		float4 result;
		result = t_diffuse.Sample(s_diffuse, inp.tc);
		result.rgb *= t_lightmap.Sample(s_lightmap, inp.lmtc).rgb * e_lmscale[0].rgb;
		float4 fb = t_fullbright.Sample(s_fullbright, inp.tc);
		result.rgb += fb.rgb * fb.a;//lerp(result.rgb, fb.rgb, fb.a);
		return result;
	}
#endif