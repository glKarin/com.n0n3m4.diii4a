//used for generating shadowmaps and stuff that draws nothing.

struct a2v
{
	float4 pos: POSITION;
};
struct v2f
{
	float3 col: TEXCOORD;
	float4 pos: SV_POSITION;
};

#include <ftedefs.h>

#ifdef VERTEX_SHADER
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_model, inp.pos);
		outp.pos = mul(m_view, outp.pos);
		outp.pos = mul(m_projection, outp.pos);
		outp.col = inp.pos.xyz - l_lightposition;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	#if LEVEL < 0x10000
		//pre dx10 requires that we ALWAYS write to a target.
		float4 main (v2f inp) : SV_TARGET
		{
			return float4(0, 0, 0, 1);
		}
	#else
		//but on 10, it'll write depth automatically and we don't care about colour.
		void main (v2f inp)
		{
		}
	#endif
#endif
