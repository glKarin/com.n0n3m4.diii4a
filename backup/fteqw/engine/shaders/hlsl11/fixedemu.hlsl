!!samps 1
struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
	float4 vcol: COLOR0;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float2 tc: TEXCOORD0;
	float4 vcol: COLOR0;
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
		outp.vcol = inp.vcol;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	Texture2D t_t0 : register(t0);
	SamplerState s_t0 : register(s0);
	float4 main (v2f inp) : SV_TARGET
	{
		float4 tc = t_t0.Sample(s_t0, inp.tc).rgba;
		tc.rgba *= inp.vcol.bgra;
#ifdef ALPHATEST
		if (!(tc.a ALPHATEST))
			discard;
#endif
		return tc;
	}
#endif