struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
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
//	Texture2D shaderTexture[5];
//	SamplerState SampleType;

	float4 main (v2f inp) : SV_TARGET
	{
		return float4(1,1,1,1);//shaderTexture[4].Sample(SampleType, inp.lmtc).bgr, 1.0);
	}
#endif