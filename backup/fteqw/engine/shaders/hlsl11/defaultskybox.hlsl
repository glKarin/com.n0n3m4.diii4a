!!samps reflectcube

//regular sky shader for scrolling q1 skies
//the sky surfaces are thrown through this as-is.

struct a2v
{
	float4 pos: POSITION;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float3 texc: TEXCOORD0;
};

#include <ftedefs.h>

#ifdef VERTEX_SHADER
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_model, inp.pos);
		outp.texc= outp.pos.xyz - v_eyepos;
		outp.pos = mul(m_view, outp.pos);
		outp.pos = mul(m_projection, outp.pos);
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	TextureCube t_reflectcube	: register(t0);
	SamplerState s_reflectcube	: register(s0);

	float4 main (v2f inp) : SV_TARGET
	{
		return t_reflectcube.Sample(s_reflectcube, inp.texc);
	}
#endif
