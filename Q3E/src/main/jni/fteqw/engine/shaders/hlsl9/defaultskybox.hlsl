!!samps reflectcube

	struct a2v
	{
		float4 pos: POSITION;
	};
	struct v2f
	{
#ifndef FRAGMENT_SHADER
		float4 pos: POSITION;
#endif
		float3 texc: TEXCOORD0;
	};

#ifdef VERTEX_SHADER
	float4x4  m_modelviewprojection;
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_modelviewprojection, inp.pos);
		outp.texc = inp.pos.xyz;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	float3 e_eyepos;
	sampler s_reflectcube;
	float4 main (v2f inp) : COLOR0
	{
		float3 tc = inp.texc - e_eyepos.xyz;
		return texCUBE(s_reflectcube, tc);
	}
#endif
