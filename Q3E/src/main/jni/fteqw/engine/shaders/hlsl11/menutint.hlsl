!!samps 1
!!cvard3 r_menutint=0.2 0.2 0.2
!!cvardf r_menutint_inverse=0.0

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
		outp.pos = mul(m_projection, inp.pos);
		outp.tc = inp.tc;
		outp.vcol = inp.vcol;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	Texture2D t_t0;
	SamplerState s_t0;
	static const float3 lumfactors = float3 (0.299, 0.587, 0.114);
	float4 main (v2f inp) : SV_TARGET
	{
		float4 texcolor = t_t0.Sample(s_t0, inp.tc);
		float luminance = dot(lumfactors, texcolor.rgb);
		texcolor.rgb = float3(luminance, luminance, luminance);
		texcolor.rgb *= r_menutint;
		texcolor.rgb = (r_menutint_inverse > 0) ? (1.0 - texcolor.rgb) : texcolor.rgb;

		return texcolor * inp.vcol;
	}
#endif

