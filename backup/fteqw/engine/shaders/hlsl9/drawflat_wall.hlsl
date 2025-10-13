!!cvard3 r_floorcolour
!!cvard3 r_wallcolour
!!samps 1
//FIXME !!permu FOG

struct a2v {
	float4 pos: POSITION;
	float2 lmtc: TEXCOORD1;
	float3 normal: NORMAL;
};
struct v2f {
	#ifndef FRAGMENT_SHADER
	float4 pos: POSITION;
	#endif
	float2 lmtc: TEXCOORD0;
	float4 col: TEXCOORD1;	//tc not colour to preserve range for oversaturation
};
#ifdef VERTEX_SHADER
	float4x4  m_modelviewprojection;
	float4 e_lmscale;
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_modelviewprojection, inp.pos);
		outp.lmtc = inp.lmtc;
		outp.col = e_lmscale * float4(((inp.normal.z < 0.73)?r_wallcolour:r_floorcolour)/255.0, 1.0);
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	sampler s_t0;
	float4 main (v2f inp) : COLOR0
	{
		return inp.col * tex2D(s_t0, inp.lmtc).xyzw;
	}
#endif
