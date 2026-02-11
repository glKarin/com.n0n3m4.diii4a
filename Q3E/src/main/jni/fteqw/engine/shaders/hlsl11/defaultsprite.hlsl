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
		outp.tc = inp.tc.xy;
		outp.vcol = inp.vcol;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	Texture2D t_diffuse		: register(t0);
	SamplerState s_diffuse	: register(s0);
	float4 main (v2f inp) : SV_TARGET
	{
		float4 tex = t_diffuse.Sample(s_diffuse, inp.tc);
#ifdef MASK
		if (tex.a < float(MASK))
			discard;
#endif
		//FIXME: no fog, no colourmod
		return tex * inp.vcol;
	}
#endif
