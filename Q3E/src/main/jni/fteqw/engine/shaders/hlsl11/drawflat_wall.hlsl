!!samps lightmap

struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
	float2 lmtc: TEXCOORD1;
	float3 norm: NORMAL;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float2 tc: TEXCOORD0;
	float2 lmtc: TEXCOORD1;
	float4 col: TEXCOORD2;
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
		outp.col = ((inp.norm.z<0.73)?float4(0.5, 0.5, 0.5, 1):float4(0.25, 0.25, 0.5, 1));
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	Texture2D t_lightmap : register(t0);
	SamplerState s_lightmap : register(s0);

	float4 main (v2f inp) : SV_TARGET
	{
		return inp.col * t_lightmap.Sample(s_lightmap, inp.lmtc);
	}
#endif